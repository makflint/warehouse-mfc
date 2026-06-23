# tests/ui — assertion-based UI suite

Machine-verifiable end-to-end tests that drive the **running** app and assert on **control
state** (not screenshots, so no human/AI in the loop). Built on the same driver as the
exploratory harness (`../manual/uia.ps1`), in Pester (the version built into Windows).

```powershell
# build the Release app first (the tests launch it if it isn't running), then:
powershell -File tests\ui\Run.ps1      # exit code = number of failed tests (0 = green)
```

## How state is read
- **UIA** (`uia.ps1`) for the dialog (combo selections, the quantity field) and top-level
  windows — these the MFC framework exposes.
- **Cross-process Win32** (`gridreader.ps1`) for the stock grid, which the Feature-Pack list
  does **not** expose to UIA. `LVM_GETITEMCOUNT`/`GETNEXTITEM` return ints directly; cell text
  needs the `VirtualAllocEx` reader (a buffer inside `app.exe` — see the file header). This is
  external, black-box: the C++ app ships unchanged, with no test hooks.

Async UI changes are awaited with `Wait-Until` (poll the state), not fixed sleeps, so the
assertions don't race the UI thread.

## What it asserts
- **`RecordDialog.Tests`** — pre-select follows the selected grid row (open *Przyjmij* on two
  rows → two different combo selections; a fixed default would read identical); out-of-range
  quantity keeps the dialog open.
- **`Grid.Tests`** — grid populated; Stan sorts numerically (asc/desc); text columns sort
  ordinally (Magazyn/Symbol/Produkt); the filter narrows to low rows and restores; selection
  tracked; a recorded movement changes on-hand and **undo/redo** restores it; **double-click**
  a row opens the record dialog; **right-click** shows a context menu (`#32768`).
- **`Window.Tests`** — the OUT dialog opens titled *Wydanie*; **F1 opens the About dialog**;
  hiding/showing a dockable pane reflows the grid (width changes and returns); an extreme
  resize doesn't crash.

The movement case records then undoes (net-zero, with a `finally` that restores even on
failure); every dialog case cancels. So **the demo DB is left unchanged** and the suite repeats.

## Known limits (stay visual, in the sweep)
- The **sort arrow** is owner-drawn by `CMFCHeaderCtrl` and not exposed as a header flag, so we
  assert the row *ordering* (the substantive part) but not the arrow glyph.
- Theme colours and the owner-drawn dashboard have no readable state — pixel/eye only.

## Why it isn't in CI
It needs an **interactive desktop** (UIA + synthetic input require a logged-in, unlocked
session), plus the full Windows + MFC + LocalDB stack. Hosted CI runners have neither. It's a
local gate; `core_tests` is the one that runs anywhere. See [../manual/README.md](../manual/README.md)
for the DPI/timing quirks the driver works around.

## What stays manual (genuinely visual)
- Theme recolouring, the owner-drawn dashboard (KPI tiles + bar chart), no-clipping-on-resize.
- **Main-grid row contents** — the MFC Feature-Pack list exposes only bare panes to UIA (no
  readable rows), so grid cell text can't be asserted this way; it's checked by eye via the
  exploratory harness.
