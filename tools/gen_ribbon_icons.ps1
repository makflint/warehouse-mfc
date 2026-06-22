# Generates the ribbon icon strips (32-bit ARGB BMP, alpha-blended) for the
# MFC Feature Pack ribbon. Re-run only when the glyph set changes; the BMPs are
# committed under app/res so a normal build needs no PowerShell.
#
#   Magazyn tab : refresh filter in out undo redo   -> ribbon_magazyn_{16,32}.bmp
#   Widok   tab : theme dashboard movements details  -> ribbon_widok_{16,32}.bmp

Add-Type -AssemblyName System.Drawing
$ErrorActionPreference = 'Stop'

$outDir = Join-Path $PSScriptRoot '..\app\res'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

function New-Pen([int]$argb, [single]$w) {
    $p = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb($argb)), $w
    $p.StartCap = 'Round'; $p.EndCap = 'Round'; $p.LineJoin = 'Round'
    return $p
}
function New-Brush([int]$argb) {
    New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb($argb))
}
# Filled triangle arrowhead at tip (tx,ty) pointing by unit vector (dx,dy), size h.
function Arrowhead($g, $brush, [single]$tx, [single]$ty, [single]$dx, [single]$dy, [single]$h) {
    $px = -$dy; $py = $dx                       # perpendicular
    $bx = $tx - $dx * $h; $by = $ty - $dy * $h  # base centre
    $pts = @(
        (New-Object System.Drawing.PointF $tx, $ty),
        (New-Object System.Drawing.PointF ($bx + $px * $h * 0.6), ($by + $py * $h * 0.6)),
        (New-Object System.Drawing.PointF ($bx - $px * $h * 0.6), ($by - $py * $h * 0.6))
    )
    $g.FillPolygon($brush, $pts)
}

# Draw one glyph into the unit square scaled to s, with origin already translated.
function Draw-Glyph($g, [string]$name, [single]$s) {
    $blue   = 0xFF1E78D2; $green = 0xFF28A044; $red = 0xFFC8402C
    $orange = 0xFFE68C14; $slate = 0xFF55556E; $indigo = 0xFF3C4678; $gold = 0xFFE8C034
    $stroke = [single]([math]::Max(1.4, $s * 0.10))

    switch ($name) {
        'refresh' {
            $pen = New-Pen $blue $stroke
            $m = $s * 0.16; $rect = New-Object System.Drawing.RectangleF $m, $m, ($s-2*$m), ($s-2*$m)
            $g.DrawArc($pen, $rect, 40, 280)
            $cx = $s*0.5; $cy = $s*0.5; $r = ($s-2*$m)/2
            $a = [math]::PI/180*40
            $tx = $cx + $r*[math]::Cos($a); $ty = $cy + $r*[math]::Sin($a)
            Arrowhead $g (New-Brush $blue) $tx $ty 0.7 -0.7 ($s*0.20)
            $pen.Dispose()
        }
        'filter' {
            $b = New-Brush $orange
            $pts = @(
                (New-Object System.Drawing.PointF ($s*0.16), ($s*0.20)),
                (New-Object System.Drawing.PointF ($s*0.84), ($s*0.20)),
                (New-Object System.Drawing.PointF ($s*0.58), ($s*0.52)),
                (New-Object System.Drawing.PointF ($s*0.58), ($s*0.82)),
                (New-Object System.Drawing.PointF ($s*0.42), ($s*0.82)),
                (New-Object System.Drawing.PointF ($s*0.42), ($s*0.52))
            )
            $g.FillPolygon($b, $pts); $b.Dispose()
        }
        'in' {
            $pen = New-Pen $green $stroke
            $g.DrawLine($pen, $s*0.22, $s*0.62, $s*0.22, $s*0.82)
            $g.DrawLine($pen, $s*0.22, $s*0.82, $s*0.78, $s*0.82)
            $g.DrawLine($pen, $s*0.78, $s*0.82, $s*0.78, $s*0.62)
            $g.DrawLine($pen, $s*0.5, $s*0.14, $s*0.5, $s*0.52)
            Arrowhead $g (New-Brush $green) ($s*0.5) ($s*0.60) 0 1 ($s*0.22)
            $pen.Dispose()
        }
        'out' {
            $pen = New-Pen $red $stroke
            $g.DrawLine($pen, $s*0.22, $s*0.62, $s*0.22, $s*0.82)
            $g.DrawLine($pen, $s*0.22, $s*0.82, $s*0.78, $s*0.82)
            $g.DrawLine($pen, $s*0.78, $s*0.82, $s*0.78, $s*0.62)
            $g.DrawLine($pen, $s*0.5, $s*0.56, $s*0.5, $s*0.22)
            Arrowhead $g (New-Brush $red) ($s*0.5) ($s*0.14) 0 -1 ($s*0.22)
            $pen.Dispose()
        }
        'undo' {
            $pen = New-Pen $slate $stroke
            $m = $s*0.20; $rect = New-Object System.Drawing.RectangleF $m, $m, ($s-2*$m), ($s-2*$m)
            $g.DrawArc($pen, $rect, 200, 160)
            $cx = $s*0.5; $r = ($s-2*$m)/2; $a=[math]::PI/180*200
            $tx=$cx+$r*[math]::Cos($a); $ty=$s*0.5+$r*[math]::Sin($a)
            Arrowhead $g (New-Brush $slate) $tx $ty -0.4 -0.92 ($s*0.20)
            $pen.Dispose()
        }
        'redo' {
            $pen = New-Pen $slate $stroke
            $m = $s*0.20; $rect = New-Object System.Drawing.RectangleF $m, $m, ($s-2*$m), ($s-2*$m)
            $g.DrawArc($pen, $rect, -20, -160)
            $cx = $s*0.5; $r = ($s-2*$m)/2; $a=[math]::PI/180*(-20)
            $tx=$cx+$r*[math]::Cos($a); $ty=$s*0.5+$r*[math]::Sin($a)
            Arrowhead $g (New-Brush $slate) $tx $ty 0.4 -0.92 ($s*0.20)
            $pen.Dispose()
        }
        'theme' {
            $b = New-Brush $indigo
            $g.FillEllipse($b, $s*0.18, $s*0.18, $s*0.64, $s*0.64)
            $g.CompositingMode = 'SourceCopy'
            $clear = New-Brush 0x00000000
            $g.FillEllipse($clear, $s*0.34, $s*0.10, $s*0.62, $s*0.62)
            $g.CompositingMode = 'SourceOver'
            $gb = New-Brush $gold
            $g.FillEllipse($gb, $s*0.60, $s*0.20, $s*0.08, $s*0.08)
            $b.Dispose(); $clear.Dispose(); $gb.Dispose()
        }
        'dashboard' {
            $base = $s*0.80
            $cols = @(@($blue,0.40),@($green,0.62),@($orange,0.28))
            $w = $s*0.16; $x = $s*0.18
            foreach ($c in $cols) {
                $b = New-Brush $c[0]; $h = $s*$c[1]
                $g.FillRectangle($b, $x, $base-$h, $w, $h); $b.Dispose()
                $x += $w + $s*0.07
            }
            $pen = New-Pen $slate ([single]([math]::Max(1.0,$s*0.06)))
            $g.DrawLine($pen, $s*0.12, $base, $s*0.88, $base); $pen.Dispose()
        }
        'movements' {
            $pen = New-Pen $slate $stroke
            $dot = New-Brush $blue
            $ys = @(0.26,0.5,0.74)
            foreach ($yy in $ys) {
                $g.FillEllipse($dot, $s*0.16, $s*$yy-$s*0.05, $s*0.10, $s*0.10)
                $g.DrawLine($pen, $s*0.34, $s*$yy, $s*0.84, $s*$yy)
            }
            $pen.Dispose(); $dot.Dispose()
        }
        'details' {
            $pen = New-Pen $slate $stroke
            $rect = New-Object System.Drawing.RectangleF ($s*0.22), ($s*0.14), ($s*0.56), ($s*0.72)
            $g.DrawRectangle($pen, $rect.X, $rect.Y, $rect.Width, $rect.Height)
            $lp = New-Pen $slate ([single]([math]::Max(1.0,$s*0.06)))
            $g.DrawLine($lp, $s*0.32, $s*0.34, $s*0.68, $s*0.34)
            $g.DrawLine($lp, $s*0.32, $s*0.50, $s*0.68, $s*0.50)
            $g.DrawLine($lp, $s*0.32, $s*0.66, $s*0.56, $s*0.66)
            $bb = New-Brush $blue
            $g.FillEllipse($bb, $s*0.60, $s*0.58, $s*0.26, $s*0.26)
            $pen.Dispose(); $lp.Dispose(); $bb.Dispose()
        }
    }
}

function Save-Strip([string[]]$names, [int]$s, [string]$path) {
    $w = $names.Count * $s
    $bmp = New-Object System.Drawing.Bitmap $w, $s, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = 'AntiAlias'
    $g.Clear([System.Drawing.Color]::Transparent)
    for ($i = 0; $i -lt $names.Count; $i++) {
        $st = $g.Save()
        $g.TranslateTransform([single]($i * $s), 0)
        Draw-Glyph $g $names[$i] ([single]$s)
        $g.Restore($st)
    }
    $g.Dispose()

    # Write a 32bpp BI_RGB DIB by hand: GDI+ drops alpha when saving BMP, and
    # CMFCToolBarImages needs the alpha channel for clean ribbon icons.
    $rect = New-Object System.Drawing.Rectangle 0, 0, $w, $s
    $data = $bmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
                          [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $stride = [math]::Abs($data.Stride)
    $buf = New-Object byte[] ($stride * $s)
    [System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $buf, 0, $buf.Length)
    $bmp.UnlockBits($data); $bmp.Dispose()

    $fs = [System.IO.File]::Create($path)
    $bw = New-Object System.IO.BinaryWriter $fs
    $imgSize = $stride * $s
    $bw.Write([byte][char]'B'); $bw.Write([byte][char]'M')
    $bw.Write([int](54 + $imgSize))      # file size
    $bw.Write([int]0)                    # reserved
    $bw.Write([int]54)                   # pixel offset
    $bw.Write([int]40)                   # info header size
    $bw.Write([int]$w); $bw.Write([int]$s)
    $bw.Write([int16]1); $bw.Write([int16]32)
    $bw.Write([int]0)                    # BI_RGB
    $bw.Write([int]$imgSize)
    $bw.Write([int]2835); $bw.Write([int]2835)
    $bw.Write([int]0); $bw.Write([int]0)
    for ($y = $s - 1; $y -ge 0; $y--) { $bw.Write($buf, $y * $stride, $stride) }  # bottom-up
    $bw.Flush(); $bw.Close(); $fs.Close()
    Write-Host "wrote $path ($w x $s)"
}

$magazyn = @('refresh','filter','in','out','undo','redo')
$widok   = @('theme','dashboard','movements','details')
Save-Strip $magazyn 16 (Join-Path $outDir 'ribbon_magazyn_16.bmp')
Save-Strip $magazyn 32 (Join-Path $outDir 'ribbon_magazyn_32.bmp')
Save-Strip $widok   16 (Join-Path $outDir 'ribbon_widok_16.bmp')
Save-Strip $widok   32 (Join-Path $outDir 'ribbon_widok_32.bmp')
