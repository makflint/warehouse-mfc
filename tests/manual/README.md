# Manual UI test harness

Exploratory end-to-end driver for the MFC app: launches `app.exe`, drives it with
real mouse / keyboard + UI Automation, and captures screenshots for visual checks.
Used to exercise the happy path **and** the corners (right-click menus, invalid input,
boundary quantities, overdraw, undo, theme switch, tiny-window resize).

This is a developer aid, not an automated assertion suite — the screenshots are read
back by eye. The captures land in `shots/` (git-ignored).

## Files
- `uia.ps1` — driver library (dot-source it). Helpers: `Pin-App`, `Fix-Size`,
  `Find-El`, `Click-Point [-Right]`, `Send-Key`, `Shot-App`, `Shot-Screen`, `Dump-Tree`.
- `shot.ps1` — standalone foreground window-capture (`-Name app -Out file.png`).

## Usage
```powershell
. .\tests\manual\uia.ps1
Pin-App                       # launch must be running; pins to 1280x1015 on-screen
Click-Point 158 110           # e.g. the Przyjmij ribbon button
Shot-App 01-dialog            # -> tests/manual/shots/01-dialog.png
```

## Gotchas (Windows specifics this harness works around)
- `SetProcessDPIAware()` is called on load so window rect / mouse / screenshot / UIA
  all share one physical-pixel space (the app is DPI-unaware; the box runs at 125%).
- Size the window with `HWND_TOP`, **not** `HWND_TOPMOST` — topmost makes the
  DPI-unaware app balloon to ~1102 px tall (off-screen on a 1080 display).
- The app restores a ~1102 px height on activation; `Fix-Size` re-asserts 1280x1015
  with `SWP_NOACTIVATE` after any click.
- Synthetic `mouse_event` clicks land on ribbon buttons, grid rows, headers and dialog
  buttons, but **not** on ribbon popup menus or dock-pane tab strips — drive those with
  the keyboard (`{END}{ENTER}`) instead.
- `PrintWindow` flag 2 (`PW_RENDERFULLCONTENT`) renders list controls too tall and is
  misleading — use flag 0 or `CopyFromScreen`.
