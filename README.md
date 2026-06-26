# Warehouse MFC

[![CI](https://github.com/makflint/warehouse-mfc/actions/workflows/ci.yml/badge.svg)](https://github.com/makflint/warehouse-mfc/actions/workflows/ci.yml)
[![coverage: OpenCppCoverage](https://img.shields.io/badge/coverage-OpenCppCoverage-blue.svg)](docs/TESTING.md)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A working **MFC + SQL Server (LocalDB)** desktop app for tracking warehouse stock and movements.
The UI is the Office-style MFC Feature Pack (ribbon, dockable panes, themed dashboard), and every
edit is undoable through the Command pattern. It's meant as a demonstration of MFC and Windows
desktop development.

![Warehouse MFC: record a movement, undo and redo it, then switch to the dark theme while the dashboard repaints live](docs/screenshots/demo-en.gif)

<sub>UI shown in English. The app also ships in Polish ([same demo, Polish UI](docs/screenshots/demo.gif)); product and warehouse names stay Polish because they're database data.</sub>

## What it demonstrates
- **MFC**: SDI doc/view, list-view grid, dialogs with DDX/DDV.
- **MS SQL Server**: a real schema with a view and a stored procedure that runs in a transaction.
- **Design patterns**: the Command pattern for undo/redo.
- **Testable core**: the domain logic (stock math, the Command/undo stack) lives in a pure C++
  `core/` static lib, tested with Catch2 and no GUI in sight.
- **Feature Pack UI**: a `CMFCRibbonBar`, dockable panes, visual-manager themes (dark mode),
  and an owner-drawn dashboard (KPI tiles plus a bar chart).
- **Bilingual & Unicode-correct**: the UI comes in Polish and English. A small string catalog
  (`I18n`) picks the language from the Windows UI language on first run, and a Język / Language
  toggle in the ribbon switches it on restart. Product names and the SQL Server error text stay
  Polish, since they're data. Diacritics work end to end: the `.cpp`/`.rc` resources are UTF-8 and
  the ODBC calls use the wide API (`SQLExecDirectW`, `N'…'` literals, `NVARCHAR`), so seed names
  like *Młotek*, *Wkrętarka* and *Śruba* show up correctly in the UI.

## Screenshots
![Ribbon UI + dashboard + docked panes](docs/screenshots/04-feature-pack.png)

The MFC Feature Pack UI. The ribbon carries icon glyphs on its *Magazyn* and *Widok* tabs (Stock
and View). The owner-drawn *Pulpit* (dashboard) pane shows KPI tiles and an on-hand bar chart, the
stock grid draws low-stock rows in red with the real Polish names, and the *Szczegóły* and *Dziennik
ruchów* panes (Details and Movement-log) share a tab group. Along the bottom, a `CMFCRibbonStatusBar`
shows the row count, the selected symbol, and the connection profile.

| Dark theme (Widok → Motyw → Ciemny, i.e. View → Theme → Dark) | Record movement (DDX/DDV) |
|---|---|
| ![Dark theme](docs/screenshots/02-dark-theme.png) | ![Record dialog](docs/screenshots/03-record-dialog.png) |

On-hand is summed from the movement log, and rows at or below the reorder level are drawn red.
Recording a movement runs through the Command stack (undo/redo, Ctrl+Z/Ctrl+Y), and the dashboard
repaints live.

## Architecture
The code splits into three layers with strict boundaries. The testable logic sits apart from the
GUI, so it can be driven test-first without one. Dependencies point downward, and no business rules
live in `app/`.

```
app/    MFC Feature Pack — doc/view, ribbon, dockable panes, owner-drawn
        dashboard, DDX/DDV dialogs, i18n catalog, visual-manager themes
  │     (wires the layers below; holds no business logic)
  ▼
data/   thin ODBC — StockRepository (pImpl) over vCurrentStock + sp_RecordMovement
  │
  ▼
core/   pure C++17 (no MFC / ODBC / Windows) — stock math, the Command/undo
        stack, grid-sort & error-cleaning logic, unit-tested (Catch2)
```

- [`core/`](core/include/warehouse/) is the domain layer: `MovementCommand` and `CommandStack`
  (undo/redo via compensating movements), `StockMath`, the grid-sort and dialog-preselect helpers
  (`view_logic.hpp`), and ODBC-error cleaning (`db_error.hpp`). It pulls in no framework headers, so
  it tests without a GUI.
- [`data/`](data/include/warehouse/) is a thin ODBC wrapper: `StockRepository` (pImpl) over the view
  and stored proc, with the connection string coming from `connection_profiles.hpp`.
- [`app/`](app/) is the MFC UI and wires `core` and `data` together. Keeping the logic in a GUI-free
  `core/` is what makes the test-first work and the coverage measurement possible.

## C++ / Windows techniques on show
| Area | What |
|---|---|
| **Patterns** | Command (undo/redo, compensating ops); pImpl (`StockRepository`); RAII with clear ownership (`std::unique_ptr`, MFC `CDC`/`CFont`); enforced layer boundaries |
| **MFC Feature Pack** | `CMFCRibbonBar`, `CDockablePane`, `CMFCListCtrl`, `CMFCPropertyGridCtrl`, `CMFCVisualManager` (including a custom dark manager); an owner-drawn, double-buffered dashboard; a DWM dark title bar |
| **Data / SQL** | the ODBC wide API (`SQLExecDirectW`, `NVARCHAR`, `N'…'`) for a Unicode round-trip; a view plus a stored proc that runs in a transaction (`UPDLOCK/HOLDLOCK`, `THROW` on overdraw) |
| **i18n** | a PL/EN string catalog checked for consistency at compile time (`static_assert` on a deduced table size) |
| **Testing** | TDD (Catch2; `core/` line coverage via OpenCppCoverage); a cross-process UI Automation + Win32 assertion suite; a scripted visual sweep |
| **Tooling** | `/W4 /WX` (MFC headers external-silenced); clang-tidy clean; one-command local CI ([`run-tests.ps1`](run-tests.ps1)) |

## SQL Server connection
The app talks to SQL Server over ODBC only (no web or HTTP in the C++). It ships pointed at LocalDB,
which needs no setup and seeds itself on first run. LocalDB is the same engine as full SQL Server,
so pointing it at a real instance (a LAN box, a VPS over Tailscale, or Azure SQL) is a one-line
`Server=` change in [`connection_profiles.hpp`](data/include/warehouse/connection_profiles.hpp).

## Getting started

### 1. Prerequisites (all free)
1. **Visual Studio Community 2022**, with the *Desktop development with C++* workload and the
   optional *C++ MFC for latest v143 build tools* component.
2. **SQL Server 2022 Express** (it includes LocalDB), plus SSMS.

That's everything you need to build, run and test the app. One optional extra: the visual sweep can
hand its screenshots to the [`claude`](https://claude.com/claude-code) CLI for an AI review
(`run-tests.ps1 -AiReview`). It isn't required. Without it you just review the sweep by eye, and
every test gate still runs.

### 2. One command (build → test → installer)
One script builds both Debug and Release, runs the test gates, and produces the installer:
```powershell
git clone https://github.com/makflint/warehouse-mfc.git
cd warehouse-mfc
powershell -File release.ps1            # add -NoSweep to skip the slow visual sweeps
```
It stops before building the installer if any test gate fails. Here's what it produces:

| Artifact | Location |
|---|---|
| Test results | console: per-layer pass/fail, and the run exits non-zero if any gate fails |
| App binary (Release, x64) | `app\x64\Release\app.exe` |
| Installer | `installer\Output\WarehouseMFC-Setup.exe` |

It doesn't publish anything; push a GitHub release by hand with `gh` when you want one.

## Testing
Testing comes in three layers: unit tests (`core/`, TDD with Catch2), an assertion UI suite
([`tests/ui/`](tests/ui/), Pester plus UI Automation driving the running app), and a visual sweep
([`tests/manual/`](tests/manual/)) for the cases that really need a human eye. One script runs all
three against LocalDB, and its exit code is the number of failed gates:
```powershell
powershell -File run-tests.ps1          # -Build to build first · -Coverage · -AiReview
```
Line coverage for `core/` is measured with OpenCppCoverage over the Debug `core_tests` (run
`run-tests.ps1 -Coverage`; the HTML and Cobertura reports land in `coverage/`). The full methodology
and case list is in [docs/TESTING.md](docs/TESTING.md).

## Download
Grab `WarehouseMFC-Setup.exe` from [release v1.1](https://github.com/makflint/warehouse-mfc/releases/tag/v1.1)
and run it on a clean Windows machine. The [Inno Setup](installer/warehouse-mfc.iss) installer bundles
the app, the SQL scripts, and the VC++ runtime plus LocalDB, and the app seeds its database on first
run, so nothing needs to be installed beforehand. The build is unsigned, so SmartScreen will warn the
first time: choose *More info* → *Run anyway*. To build the installer yourself, use `release.ps1` above.

See [docs/SPEC.md](docs/SPEC.md) for the design and [TODO.md](TODO.md) for the roadmap and open work.

## Author
Built by Maciej Krzemiński. I used Claude's tooling throughout — [Claude Code](https://claude.com/claude-code)
and the Claude models from Anthropic — for the design, the code, and the tests.

## License
[MIT](LICENSE) © Maciej Krzemiński
