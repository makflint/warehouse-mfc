# tests/ui — assertion-based UI suite

Machine-verifiable end-to-end tests that drive the **running** app and assert on **control
state read via UI Automation** (not screenshots, so no human/AI in the loop). Built on the
same driver as the exploratory harness (`../manual/uia.ps1`); the assertions live in
[`RecordDialog.Tests.ps1`](RecordDialog.Tests.ps1) (Pester — the version built into Windows).

```powershell
# build the Release app first (the tests launch it if it isn't running), then:
powershell -File tests\ui\Run.ps1      # exit code = number of failed tests (0 = green)
```

## What it asserts
- **Pre-select follows the grid** — opening *Przyjmij* on two different rows yields two
  different combo selections (and real `MAG-*` codes). If the dialog still hard-coded the
  first product/warehouse, the two reads would be identical. This is the end-to-end proof of
  the wiring; the *decision* behind it (`selectedIndexForId`) is unit-tested in `core/`.
- **Quantity validation** — `0` + OK keeps the dialog open (nothing recorded).

Every case cancels out of the dialog, so **no movement is recorded** — the demo DB is left
unchanged and the suite is repeatable.

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
