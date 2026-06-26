# Testing

How `warehouse-mfc` is tested, by layer. The architecture (`core/` pure logic → `data/`
ODBC → `app/` MFC UI) drives the strategy: push the real logic into a GUI-free `core/`
that can be unit-tested with TDD, and cover the UI boundary with a scripted manual harness.

| Layer | What | How tested |
|---|---|---|
| `core/` | Stock math, Command/undo stack — pure C++, no MFC/ODBC/Windows | **Unit tests, TDD** (Catch2) |
| `data/` | ODBC access (`vCurrentStock`, `sp_RecordMovement`) | **Integration** against LocalDB; rules enforced in the SP |
| `app/`  | MFC Feature Pack UI (ribbon, panes, grids, dialogs) | **Assertion UI suite** (Pester + UIA) + **exploratory harness** |

## Running the suite (local CI)

[`run-tests.ps1`](../run-tests.ps1) runs every layer against LocalDB, resetting the DB to a clean
baseline and clearing the saved language first so the run is deterministic and self-contained. The
**exit code is the number of failed gating layers** (0 = all green).

| Layer | What | Gates? |
|---|---|---|
| `core_tests` | Catch2 unit tests (`core/`) | ✓ exit code |
| `data_smoke` | `data/` ODBC smoke vs LocalDB | ✓ exit code |
| `tests/ui` | Pester + UI-Automation state assertions | ✓ exit code |
| `sweep pl/en` | visual exploratory screenshots | informational (review by eye / `-AiReview`) |

```powershell
powershell -File run-tests.ps1                 # all layers (assumes already built)
powershell -File run-tests.ps1 -Build          # build Debug + Release first
powershell -File run-tests.ps1 -NoSweep        # gates only (skip the visual sweeps)
powershell -File run-tests.ps1 -Coverage       # also report core/ line coverage
powershell -File run-tests.ps1 -AiReview       # also AI-review the sweep shots (needs the `claude` CLI)
```
Only the three gating layers decide the exit code; the sweep is informational.

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
- **Coverage:** `core/` is at **100% line coverage** ([`tools/coverage.ps1`](../tools/coverage.ps1),
  or `run-tests.ps1 -Coverage`) — [OpenCppCoverage](https://github.com/OpenCppCoverage/OpenCppCoverage)
  runs the **Debug** `core_tests` (optimised Release folds lines away) and exports HTML + Cobertura
  to `coverage/`. `data/` and `app/` aren't coverage targets (integration / UI, not unit-tested).

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
that read **control state** (not pixels), so they pass/fail on their own with no human in the
loop. State comes from **UIA** for what the framework exposes (dialog combos, the quantity
field, top-level windows) and from **cross-process Win32 messages** for the stock grid, which
the Feature-Pack list does *not* expose to UIA (`gridreader.ps1` — a `VirtualAllocEx` reader,
entirely in the harness, no hooks in the app). Async changes are awaited with `Wait-Until`
(poll), not fixed sleeps, so the checks don't race the UI thread. Covered:

- **Dialog** — pre-select *follows the selected grid row* (the end-to-end proof of the wiring;
  the decision `selectedIndexForId` is unit-tested in `core/`); out-of-range quantity rejected.
- **Grid** — Stan sorts numerically, text columns ordinally; the filter narrows to low rows and
  restores; selection tracked; a recorded movement changes on-hand and **undo/redo** restores it.
- **Window** — the OUT dialog opens titled *Wydanie*; an **over-stock OUT** is rejected by the
  stored proc and surfaces a clean error message box with on-hand unchanged (rolled back); a
  dockable pane hide/show reflows the grid; an extreme resize doesn't crash.

The movement case records then undoes (net-zero, restored in a `finally`); dialog cases cancel —
so the demo DB is unchanged and the suite repeats.

```powershell
powershell -File tests\ui\Run.ps1      # exit code = failed-test count (0 = green)
```

Two things still resist assertion: the **sort arrow** (owner-drawn by `CMFCHeaderCtrl`, not a
header flag — we assert the ordering instead) and theme colours / the owner-drawn dashboard
(no readable state). Those stay with the exploratory harness below.

**3b. Exploratory harness — [`tests/manual/`](../tests/manual/).** A scripted manual harness
that drives the app and screenshots each step for a visual check — for corner-case sweeps and
the genuinely visual cases (theming, owner-draw, no-clipping-on-resize).

### Cases covered (happy path + corners)
- **Selection** — left-click, right-click and keyboard arrows update the Details pane and
  the status bar. *(selection tracking asserted in `tests/ui`.)*
- **Sorting** — every column sorts in both grids; the sort arrow shows from startup (main
  grid by Magazyn ↑, Dziennik by Czas ↓); numeric columns sort numerically; the selection
  is preserved across a re-sort. *(ordering asserted in `tests/ui`; the arrow stays visual.)*
- **Filter** — "Tylko niskie" narrows the grid to low-stock rows. *(asserted in `tests/ui`.)*
- **Record dialog (DDX/DDV)** — quantity is numeric-only and range-validated (1–1 000 000);
  empty / zero / over-max are rejected with a titled Polish message; Cancel is a no-op.
  *(zero-rejection and pre-select-follows-row are also asserted in `tests/ui`.)*
- **Pre-select** — opening the dialog defaults the combos to the selected grid row.
  *(asserted in `tests/ui`.)*
- **Overdraw** — issuing more than on-hand is rejected by the stored proc; stock unchanged.
  *(asserted in `tests/ui`: the cleaned error reaches a message box and on-hand is unchanged.)*
- **Undo / redo** — data and dashboard restore; correct enable/disable; persists to the DB.
  *(on-hand restore asserted in `tests/ui`.)*
- **Themes** — switch across all visual managers incl. the custom dark theme; chrome, grids,
  dashboard and status bar all recolour. *(visual only.)*
- **Panes** — hide/show via the ribbon; the central grid reflows. *(reflow asserted in `tests/ui`.)*
- **Right-click menus** — the ribbon QAT and dockable-pane context menus (localized Polish).
- **Robustness** — extreme window resize (no clipping or crash). *(no-crash asserted in `tests/ui`.)*

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

## 4. Static analysis

Two static gates back the test layers:

- **Warnings-as-errors** — `core/`, `data/` and `app/` compile at **`/W4 /WX`** (Debug + Release).
  The MFC app would otherwise drown in Feature-Pack-header warnings, so angle-bracket includes are
  treated as **external** (`TreatAngleIncludesAsExternal` + `ExternalWarningLevel=TurnOffAllWarnings`)
  — `/W4` gates *our* code while MFC/SDK headers stay quiet. The build is clean; a new warning now
  breaks it.
- **clang-tidy** — a curated check set (`.clang-tidy` at the repo root: `bugprone`, `performance`,
  `modernize`, `readability`, `cppcoreguidelines`, minus the noisy/opinionated ones). Scoped to the
  portable C++ in `core/` (and `data/`, which also parses); `app/` is not a clang-tidy target (it
  leans on MSVC + the Feature Pack — the `/W4 /WX` gate covers it). Run:
  ```powershell
  $ct = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-tidy.exe"
  & $ct core\src\*.cpp --quiet -- -std=c++17 -Icore\include      # 0 diagnostics = clean
  & $ct data\src\*.cpp --quiet -- -std=c++17 -Idata\include -Icore\include -DUNICODE -D_UNICODE
  ```
  (needs the *C++ Clang tools for Windows* VS component.)

## Not automated (and why)
- **No CI** — the build needs Windows + Visual Studio + MFC + LocalDB, which is out of scope
  for a desktop demo. `core_tests` is the gate that matters and runs anywhere; the `tests/ui`
  suite needs an interactive desktop, so it's a local gate (see [`tests/ui/README.md`](../tests/ui/README.md)).
- **Visual UI checks are read by eye** — the assertion suite (`tests/ui`) covers what UIA can
  read as control *state* (dialog wiring, validation). What's left is genuinely visual — theme
  recolouring, the owner-drawn dashboard, no-clipping-on-resize — plus grid *row contents* (the
  Feature-Pack list exposes no readable rows to UIA). Those are checked via screenshots in the
  exploratory harness, not pixel-diffed: fast corner-case coverage over a brittle suite.

  **Optional AI review (`run-tests.ps1 -AiReview`).** Eyeballing those sweep screenshots can be
  delegated to an LLM: with `-AiReview`, after the sweep the runner pipes a per-shot rubric to the
  [`claude`](https://claude.com/claude-code) CLI (every shot's expected state, plus "is the EN run
  fully translated?"), which replies `VERDICT: CLEAN` or `VERDICT: ISSUES` + a bullet list. This is
  **purely optional and not a prerequisite** — it's gated behind the `-AiReview` flag *and* a
  `Get-Command claude` check, so without the CLI on PATH it's silently skipped and the three test
  **gates still run and still decide the exit code**. Nothing in the build/test path requires it.
