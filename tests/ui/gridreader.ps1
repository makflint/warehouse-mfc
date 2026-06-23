# Cross-process reader for the MFC stock grid (a SysListView32 the app never exposes to UIA).
# Lives entirely in the test harness — the C++ app ships unchanged, no test hooks.
#
# LVM_GETITEMCOUNT / LVM_GETNEXTITEM return an int (no pointer) -> trivial cross-process.
# LVM_GETITEMTEXT / HDM_GETITEM take a struct-with-pointer, which the ListView dereferences
# in ITS address space, so the buffer must live inside app.exe: OpenProcess -> VirtualAllocEx
# -> WriteProcessMemory -> SendMessage -> ReadProcessMemory -> VirtualFreeEx. All of that is
# encapsulated in the Grid class below; callers just see Grid-Cell / Grid-SortArrow.

if (-not ([System.Management.Automation.PSTypeName]'Grid').Type) {
Add-Type @"
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public class Grid {
    [DllImport("user32.dll")] static extern IntPtr SendMessage(IntPtr h, int msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr p, EnumProc cb, IntPtr l);
    [DllImport("user32.dll")] static extern int GetClassName(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
    [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(uint access, bool inherit, uint pid);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("kernel32.dll")] static extern IntPtr VirtualAllocEx(IntPtr h, IntPtr addr, IntPtr size, uint type, uint protect);
    [DllImport("kernel32.dll")] static extern bool VirtualFreeEx(IntPtr h, IntPtr addr, IntPtr size, uint type);
    [DllImport("kernel32.dll")] static extern bool WriteProcessMemory(IntPtr h, IntPtr addr, byte[] buf, int size, out IntPtr written);
    [DllImport("kernel32.dll")] static extern bool ReadProcessMemory(IntPtr h, IntPtr addr, byte[] buf, int size, out IntPtr read);

    public delegate bool EnumProc(IntPtr h, IntPtr l);
    public struct RECT { public int Left, Top, Right, Bottom; }

    [StructLayout(LayoutKind.Sequential)]
    struct LVITEM {
        public uint mask; public int iItem; public int iSubItem; public uint state; public uint stateMask;
        public IntPtr pszText; public int cchTextMax; public int iImage; public IntPtr lParam;
        public int iIndent; public int iGroupId; public uint cColumns; public IntPtr puColumns;
        public IntPtr piColFmt; public int iGroup;
    }
    [StructLayout(LayoutKind.Sequential)]
    struct HDITEM {
        public uint mask; public int cxy; public IntPtr pszText; public IntPtr hbm; public int cchTextMax;
        public int fmt; public IntPtr lParam; public int iImage; public int iOrder; public uint type;
        public IntPtr pvFilter; public uint state;
    }

    const int LVM_GETITEMCOUNT = 0x1004, LVM_GETNEXTITEM = 0x100C, LVM_GETHEADER = 0x101F, LVM_GETITEMTEXTW = 0x1073;
    const int HDM_GETITEMW = 0x120B, HDI_FORMAT = 0x0004, HDF_SORTDOWN = 0x0200, HDF_SORTUP = 0x0400;
    const int LVNI_SELECTED = 0x0002;
    const uint PROC_VM = 0x0038;                 // VM_OPERATION|VM_READ|VM_WRITE
    const uint MEM_COMMIT_RESERVE = 0x3000, PAGE_RW = 0x04, MEM_RELEASE = 0x8000;

    static List<IntPtr> _lv;
    static EnumProc _cb = Collect;
    static bool Collect(IntPtr h, IntPtr l) {
        var sb = new StringBuilder(64); GetClassName(h, sb, 64);
        if (sb.ToString() == "SysListView32") _lv.Add(h);
        return true;
    }

    static byte[] ToBytes(object s) {
        int size = Marshal.SizeOf(s);
        byte[] arr = new byte[size];
        IntPtr p = Marshal.AllocHGlobal(size);
        Marshal.StructureToPtr(s, p, false); Marshal.Copy(p, arr, 0, size); Marshal.FreeHGlobal(p);
        return arr;
    }

    // The widest SysListView32 under the window = the central stock grid (the Dziennik list is narrower).
    public static IntPtr MainGrid(IntPtr top) {
        _lv = new List<IntPtr>();
        EnumChildWindows(top, _cb, IntPtr.Zero);
        IntPtr best = IntPtr.Zero; int bestW = -1;
        foreach (IntPtr h in _lv) { RECT r; GetWindowRect(h, out r); int w = r.Right - r.Left; if (w > bestW) { bestW = w; best = h; } }
        return best;
    }

    public static int Count(IntPtr h) { return SendMessage(h, LVM_GETITEMCOUNT, IntPtr.Zero, IntPtr.Zero).ToInt32(); }
    public static int Selected(IntPtr h) { return SendMessage(h, LVM_GETNEXTITEM, (IntPtr)(-1), (IntPtr)LVNI_SELECTED).ToInt32(); }

    public static string CellText(IntPtr hwnd, int row, int col) {
        uint pid; GetWindowThreadProcessId(hwnd, out pid);
        IntPtr proc = OpenProcess(PROC_VM, false, pid);
        if (proc == IntPtr.Zero) return null;
        try {
            int lvSize = Marshal.SizeOf(typeof(LVITEM));
            int chars = 512, textBytes = chars * 2;
            IntPtr remote = VirtualAllocEx(proc, IntPtr.Zero, (IntPtr)(lvSize + textBytes), MEM_COMMIT_RESERVE, PAGE_RW);
            IntPtr remoteText = (IntPtr)(remote.ToInt64() + lvSize);
            LVITEM item = new LVITEM(); item.iSubItem = col; item.pszText = remoteText; item.cchTextMax = chars;
            IntPtr w; WriteProcessMemory(proc, remote, ToBytes(item), lvSize, out w);
            SendMessage(hwnd, LVM_GETITEMTEXTW, (IntPtr)row, remote);
            byte[] buf = new byte[textBytes]; IntPtr r; ReadProcessMemory(proc, remoteText, buf, textBytes, out r);
            VirtualFreeEx(proc, remote, IntPtr.Zero, MEM_RELEASE);
            string s = Encoding.Unicode.GetString(buf); int z = s.IndexOf('\0'); if (z >= 0) s = s.Substring(0, z);
            return s;
        } finally { CloseHandle(proc); }
    }

    public static string SortArrow(IntPtr hwnd, int col) {
        IntPtr header = SendMessage(hwnd, LVM_GETHEADER, IntPtr.Zero, IntPtr.Zero);
        if (header == IntPtr.Zero) return "none";
        uint pid; GetWindowThreadProcessId(header, out pid);
        IntPtr proc = OpenProcess(PROC_VM, false, pid);
        if (proc == IntPtr.Zero) return "none";
        try {
            int size = Marshal.SizeOf(typeof(HDITEM));
            IntPtr remote = VirtualAllocEx(proc, IntPtr.Zero, (IntPtr)size, MEM_COMMIT_RESERVE, PAGE_RW);
            HDITEM hdi = new HDITEM(); hdi.mask = HDI_FORMAT;
            IntPtr w; WriteProcessMemory(proc, remote, ToBytes(hdi), size, out w);
            SendMessage(header, HDM_GETITEMW, (IntPtr)col, remote);
            byte[] rb = new byte[size]; IntPtr r; ReadProcessMemory(proc, remote, rb, size, out r);
            VirtualFreeEx(proc, remote, IntPtr.Zero, MEM_RELEASE);
            IntPtr hp = Marshal.AllocHGlobal(size); Marshal.Copy(rb, 0, hp, size);
            HDITEM outItem = (HDITEM)Marshal.PtrToStructure(hp, typeof(HDITEM)); Marshal.FreeHGlobal(hp);
            if ((outItem.fmt & HDF_SORTUP) != 0) return "up";
            if ((outItem.fmt & HDF_SORTDOWN) != 0) return "down";
            return "none";
        } finally { CloseHandle(proc); }
    }

    public static int[] Rect(IntPtr h) { RECT r; GetWindowRect(h, out r); return new int[] { r.Left, r.Top, r.Right - r.Left, r.Bottom - r.Top }; }
    public static bool Visible(IntPtr h) { return IsWindowVisible(h); }
}
"@
}

# --- PowerShell wrappers ----------------------------------------------------
function Get-MainGrid     { [Grid]::MainGrid((Get-AppHwnd)) }
function Grid-Count       { param($g) [Grid]::Count($g) }
function Grid-Selected    { param($g) [Grid]::Selected($g) }
function Grid-Cell        { param($g, [int]$Row, [int]$Col) [Grid]::CellText($g, $Row, $Col) }
function Grid-SortArrow   { param($g, [int]$Col) [Grid]::SortArrow($g, $Col) }
function Grid-Width       { param($g) ([Grid]::Rect($g))[2] }

function Grid-Column {
    param($g, [int]$Col)
    $n = [Grid]::Count($g); $out = @()
    for ($i = 0; $i -lt $n; $i++) { $out += [Grid]::CellText($g, $i, $Col) }
    return ,$out
}

# Find the row whose two columns match (used to relocate a row after a re-sort/refresh).
function Grid-FindRow {
    param($g, [int]$Col1, [string]$Val1, [int]$Col2, [string]$Val2)
    $n = [Grid]::Count($g)
    for ($i = 0; $i -lt $n; $i++) {
        if ([Grid]::CellText($g, $i, $Col1) -eq $Val1 -and [Grid]::CellText($g, $i, $Col2) -eq $Val2) { return $i }
    }
    return -1
}
