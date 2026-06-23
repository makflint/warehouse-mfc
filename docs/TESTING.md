# Testing

How `warehouse-mfc` is tested, by layer. The architecture (`core/` pure logic → `data/`
ODBC → `app/` MFC UI) drives the strategy: push the real logic into a GUI-free `core/`
that can be unit-tested with TDD, and cover the UI boundary with a scripted manual harness.

| Layer | What | How tested |
|---|---|---|
| `core/` | Stock math, Command/undo stack — pure C++, no MFC/ODBC/Windows | **Unit tests, TDD** (Catch2) |
| `data/` | ODBC access (`vCurrentStock`, `sp_RecordMovement`) | **Integration** against LocalDB; rules enforced in the SP |
| `app/`  | MFC Feature Pack UI (ribbon, panes, grids, dialogs) | **Manual exploratory harness** (UI Automation) |

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

## 3. UI — `app/` (manual exploratory harness)

MFC UI behaviour (message routing, owner-draw, docking, theming) is not practical to
unit-test without heavy mocking, and its real value is at the **interaction boundary**. It
is covered by a scripted manual harness in [`tests/manual/`](../tests/manual/) that drives
the *running* app with real mouse/keyboard + **UI Automation** (PowerShell) and screenshots
each step for a visual check.

### Cases covered (happy path + corners)
- **Selection** — left-click, right-click and keyboard arrows update the Details pane and
  the status bar.
- **Sorting** — every column sorts in both grids; the sort arrow shows from startup (main
  grid by Magazyn ↑, Dziennik by Czas ↓); numeric columns sort numerically; the selection
  is preserved across a re-sort.
- **Filter** — "Tylko niskie" narrows the grid to low-stock rows.
- **Record dialog (DDX/DDV)** — quantity is numeric-only and range-validated (1–1 000 000);
  empty / zero / over-max are rejected with a titled Polish message; Cancel is a no-op.
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
  for a portfolio demo. `core_tests` is the gate that matters and runs locally.
- **UI checks are read by eye** — the harness captures screenshots rather than asserting
  pixels. This is deliberate: the goal is fast exploratory coverage of corner cases, not a
  brittle pixel-diff suite.
