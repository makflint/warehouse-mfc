# Testing

How `warehouse-mfc` is tested, by layer. The architecture (`core/` pure logic → `data/`
ODBC → `app/` MFC UI) drives the strategy: push the real logic into a GUI-free `core/`
that can be unit-tested with TDD, and cover the UI boundary with a scripted manual harness.

| Layer | What | How tested |
|---|---|---|
| `core/` | Stock math, Command/undo stack — pure C++, no MFC/ODBC/Windows | **Unit tests, TDD** (Catch2) |
| `data/` | ODBC access (`vCurrentStock`, `sp_RecordMovement`) | **Integration** against LocalDB; rules enforced in the SP |
| `app/`  | MFC Feature Pack UI (ribbon, panes, grids, dialogs) | **Assertion UI suite** (Pester + UIA) + **exploratory harness** |

## 1. Unit tests — `core/` (TDD, Catch2)

The domain logic lives in a pure C++ static lib with no GUI/DB dependencies, so it is
tested in isolation. Development follows **red → green → refactor** (see CLAUDE.md).

- Framework: **Catch2** (amalgamated header under `tests/third_party/catch2`).
- Covered: stock on-hand math (IN/OUT, low-stock threshold), the **Command** stack
  (execute / undo / redo invariants, stack depth), and **ODBC error cleaning**
  (`cleanDbError` strips the `[SQLSTATE][driver][server]` prefixes; UTF-8 message text,
  incl. Polish diacritics, passes through — the logic behind the *Overdraw* message below).
- Run:
  ```powershell
  msbuild warehouse-mfc.sln /p:Configuration=Debug /p:Platform=x64 /t:core_tests
  x64\Debug\core_tests.exe        # exit 0 = all green
  ```
  In Visual Studio: set `core_tests` as the startup project, Ctrl+F5.

## 2. Data layer — `data/` (integration)

`data/` is intentionally thin (query a view, call a stored procedure); it is verified by
running the app against **LocalDB**:

- The **business rule** — an `OUT` that would drive stock negative — is enforced
  **server-side** in `sp_RecordMovement` (`THROW`) inside a transaction, and surfaced to
  the user as a clean Polish message (ODBC diagnostic prefixes stripped). Exercised by the
  *Overdraw* case below.
- **Unicode round-trip** (Polish names with diacritics) is verified end-to-end:
  seed → `SQLExecDirectW` → `NVARCHAR` → wide fetch (`SQL_C_WCHAR`) → UI.

## 3. UI — `app/` (assertion suite + exploratory harness)

MFC UI behaviour (message routing, owner-draw, docking, theming) is not practical to
unit-test without heavy mocking, and its real value is at the **interaction boundary**. Two
complementary layers cover it, both driving the *running* app with real mouse/keyboard +
**UI Automation** (PowerShell):

**3a. Assertion suite — [`tests/ui/`](../tests/ui/) (Pester).** Machine-verifiable checks
that read **control state via UIA** (not pixels), so they pass/fail on their own with no human
in the loop. Currently the record-movement dialog: that the combos *follow the selected grid
row* (the end-to-end proof of the pre-select wiring — the decision itself, `selectedIndexForId`,
is unit-tested in `core/`) and that an out-of-range quantity is rejected. Every case cancels
out, so nothing is recorded and the suite is repeatable.

```powershell
powershell -File tests\ui\Run.ps1      # exit code = failed-test count (0 = green)
```

This layer can't reach everything: the MFC Feature-Pack **grids expose only bare panes to
UIA** (no readable rows), and theme colours / the owner-drawn dashboard are inherently visual.
Those stay with the exploratory harness below.

**3b. Exploratory harness — [`tests/manual/`](../tests/manual/).** A scripted manual harness
that drives the app and screenshots each step for a visual check — for corner-case sweeps and
the genuinely visual cases (theming, owner-draw, no-clipping-on-resize).

### Cases covered (happy path + corners)
- **Selection** — left-click, right-click and keyboard arrows update the Details pane and
  the status bar.
- **Sorting** — every column sorts in both grids; the sort arrow shows from startup (main
  grid by Magazyn ↑, Dziennik by Czas ↓); numeric columns sort numerically; the selection
  is preserved across a re-sort.
- **Filter** — "Tylko niskie" narrows the grid to low-stock rows.
- **Record dialog (DDX/DDV)** — quantity is numeric-only and range-validated (1–1 000 000);
  empty / zero / over-max are rejected with a titled Polish message; Cancel is a no-op.
  *(zero-rejection and pre-select-follows-row are also asserted in `tests/ui`.)*
- **Pre-select** — opening the dialog defaults the combos to the selected grid row.
  *(asserted in `tests/ui`.)*
- **Overdraw** — issuing more than on-hand is rejected by the stored proc; stock unchanged.
- **Undo / redo** — data and dashboard restore; correct enable/disable; persists to the DB.
- **Themes** — switch across all visual managers incl. the custom dark theme; chrome, grids,
  dashboard and status bar all recolour.
- **Panes** — hide/show via the ribbon; the central grid reflows.
- **Right-click menus** — the ribbon QAT and dockable-pane context menus (localized Polish).
- **Robustness** — extreme window resize (no clipping or crash).

### Run
```powershell
# build the Release app first, then:
. tests\manual\uia.ps1
Pin-App                 # launch app.exe first; pins it on-screen at a known size
Click-Point 158 110     # e.g. open the Przyjmij dialog
Shot-App 01-dialog      # -> tests/manual/shots/01-dialog.png  (git-ignored)
```
See [`tests/manual/README.md`](../tests/manual/README.md) for the Windows DPI/timing quirks
the harness works around (process DPI-awareness, `HWND_TOP` vs `HWND_TOPMOST`, re-asserting
the window size after activation, driving ribbon popups via the keyboard, `PrintWindow`
caveats).

## Not automated (and why)
- **No CI** — the build needs Windows + Visual Studio + MFC + LocalDB, which is out of scope
  for a portfolio demo. `core_tests` is the gate that matters and runs anywhere; the `tests/ui`
  suite needs an interactive desktop, so it's a local gate (see [`tests/ui/README.md`](../tests/ui/README.md)).
- **Visual UI checks are read by eye** — the assertion suite (`tests/ui`) covers what UIA can
  read as control *state* (dialog wiring, validation). What's left is genuinely visual — theme
  recolouring, the owner-drawn dashboard, no-clipping-on-resize — plus grid *row contents* (the
  Feature-Pack list exposes no readable rows to UIA). Those are checked via screenshots in the
  exploratory harness, not pixel-diffed: fast corner-case coverage over a brittle suite.
