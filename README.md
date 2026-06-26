# Warehouse MFC

[![CI](https://github.com/makflint/warehouse-mfc/actions/workflows/ci.yml/badge.svg)](https://github.com/makflint/warehouse-mfc/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A small but real **MFC + SQL Server (LocalDB)** desktop app: warehouse stock & movements with an Office-style
**MFC Feature Pack** UI (ribbon, dockable panes, themed dashboard) and **undo/redo** (Command
pattern). A demo of **MFC / Windows desktop** application development.

![Warehouse MFC — record a movement, undo/redo it, and switch to the dark theme; the dashboard repaints live](docs/screenshots/demo-en.gif)

<sub>UI shown in English; the app also ships in Polish — [same demo, Polish UI](docs/screenshots/demo.gif). Product/warehouse names stay Polish by design (they're DB data).</sub>

## What it demonstrates
- **MFC**: SDI doc/view, list-view grid, dialogs with DDX/DDV.
- **MS SQL Server**: real schema, a **view** and a **stored procedure with a transaction**.
- **Design patterns**: **Command** pattern for undo/redo.
- **Testable core**: domain logic (stock math, the Command/undo stack) lives in a pure C++
  `core/` static lib with **TDD** (Catch2), verified without a GUI.
- **Feature Pack UI**: a `CMFCRibbonBar`, dockable panes, visual-manager themes (dark mode),
  and an owner-drawn **dashboard** (KPI tiles + bar chart).
- **Bilingual & Unicode-correct**: the UI ships in **Polish *and* English**. A small string
  catalog (`I18n`) picks the language from the **Windows UI language** on first run, with a
  **Język / Language** toggle in the ribbon (applied on restart). Data (product/warehouse names)
  and the SQL Server error text stay Polish by design. Full diacritics end-to-end: UTF-8
  `.cpp`/`.rc` resources and wide ODBC (`SQLExecDirectW`, `N'…'` literals, `NVARCHAR`); the seed
  names (*Młotek*, *Wkrętarka*, *Śruba*…) round-trip cleanly to the UI.

## Screenshots
![Ribbon UI + dashboard + docked panes](docs/screenshots/04-feature-pack.png)

The MFC **Feature Pack** UI: a `CMFCRibbonBar` with icon glyphs (*Magazyn* / *Widok* tabs), an
owner-drawn **Pulpit** (dashboard) pane with KPI tiles + an on-hand bar chart, the stock grid (with
low-stock rows in red and proper Polish names), the **Szczegóły** / **Dziennik ruchów** panes sharing
one **tab group**, and a `CMFCRibbonStatusBar` (row count · selected symbol · connection profile).

| Dark theme (Widok → Motyw → Ciemny) | Record movement (DDX/DDV) |
|---|---|
| ![Dark theme](docs/screenshots/02-dark-theme.png) | ![Record dialog](docs/screenshots/03-record-dialog.png) |

On-hand is summed from the movement log; rows at/below the reorder level are drawn red. Recording
a movement runs through the **Command** stack (undo/redo, Ctrl+Z/Ctrl+Y) and the dashboard
repaints live.

## Architecture
Three layers with hard boundaries — the testable logic is isolated from the GUI so it can be
TDD'd without one. Dependencies point downward; no business rules live in `app/`.

```
app/    MFC Feature Pack — doc/view, ribbon, dockable panes, owner-drawn
        dashboard, DDX/DDV dialogs, i18n catalog, visual-manager themes
  │     (wires the layers below; holds no business logic)
  ▼
data/   thin ODBC — StockRepository (pImpl) over vCurrentStock + sp_RecordMovement
  │
  ▼
core/   pure C++17 (no MFC / ODBC / Windows) — stock math, the Command/undo
        stack, grid-sort & error-cleaning logic · unit-tested (Catch2, 100% lines)
```

- **[`core/`](core/include/warehouse/)** — domain only: `MovementCommand` + `CommandStack`
  (undo/redo via *compensating* movements), `StockMath`, grid-sort / dialog-preselect
  (`view_logic.hpp`), ODBC-error cleaning (`db_error.hpp`). No framework headers → tested without a GUI.
- **[`data/`](data/include/warehouse/)** — `StockRepository` (pImpl) is a thin ODBC wrapper over the
  view + stored proc; connection string from `connection_profiles.hpp`.
- **[`app/`](app/)** — MFC UI; wires `core` + `data`. Pushing the logic into a GUI-free `core/` is
  what makes the TDD + 100% coverage possible.

## C++ / Windows techniques on show
| Area | What |
|---|---|
| **Patterns** | **Command** (undo/redo, compensating ops) · **pImpl** (`StockRepository`) · RAII + clear ownership (`std::unique_ptr`, MFC `CDC`/`CFont`) · enforced layer boundaries |
| **MFC Feature Pack** | `CMFCRibbonBar`, `CDockablePane`, `CMFCListCtrl`, `CMFCPropertyGridCtrl`, `CMFCVisualManager` (incl. a custom **dark** manager) · owner-drawn, double-buffered dashboard · DWM dark title bar |
| **Data / SQL** | ODBC **wide** API (`SQLExecDirectW`, `NVARCHAR`, `N'…'`) Unicode round-trip · a **view** + a **stored proc with a transaction** (`UPDLOCK/HOLDLOCK`, `THROW` on overdraw) |
| **i18n** | PL/EN string catalog with **compile-time** consistency (`static_assert` on a deduced table size) |
| **Testing** | **TDD** (Catch2; **100%** `core/` line coverage via OpenCppCoverage) · cross-process **UI Automation + Win32** assertion suite · scripted visual sweep |
| **Tooling** | `/W4 /WX` (MFC headers external-silenced) · **clang-tidy** clean · one-command local CI ([`run-tests.ps1`](run-tests.ps1)) |

## SQL Server connection
The app talks to **SQL Server over ODBC** only (no web/HTTP in C++) and ships pointed at
**LocalDB** — zero-config, self-seeds on first run. LocalDB *is* the SQL Server engine, so
targeting a full instance (LAN box, VPS over Tailscale, Azure SQL) is a one-line `Server=` change
in [`connection_profiles.hpp`](data/include/warehouse/connection_profiles.hpp).

## Getting started

### 1. Prerequisites (all free)
1. **Visual Studio Community 2022** → workload *"Desktop development with C++"* + optional
   component **"C++ MFC for latest v143 build tools"**.
2. **SQL Server 2022 Express** (includes **LocalDB**) + **SSMS**.

That's all you need to build, run and test the app. *Optional:* the visual sweep's **AI review**
(`run-tests.ps1 -AiReview`) shells out to the [`claude`](https://claude.com/claude-code) CLI to
eyeball the screenshots — it is **not** a prerequisite; without it the sweep is just reviewed by
eye and every test gate still runs.

### 2. One command (build → test → installer)
From zero — build (Debug + Release) → run the test gates → produce the installer:
```powershell
git clone https://github.com/makflint/warehouse-mfc.git
cd warehouse-mfc
powershell -File release.ps1            # add -NoSweep to skip the slow visual sweeps
```
It stops **before** the installer if any test gate fails. What it emits:

| Artifact | Location |
|---|---|
| **Test results** | console — per-layer pass/fail; the run **exits non-zero on any failed gate** |
| **App binary** (Release · x64) | `app\x64\Release\app.exe` |
| **Installer** | `installer\Output\WarehouseMFC-Setup.exe` |

No publishing — push a GitHub release by hand with `gh` when you want one.

## Testing
Three layers — **unit tests** (`core/`, TDD, Catch2, **100% line coverage**), an **assertion UI
suite** ([`tests/ui/`](tests/ui/), Pester + UI Automation on the running app) and a **visual sweep**
([`tests/manual/`](tests/manual/)) for the genuinely visual cases. One script runs them all against
LocalDB; the exit code is the number of failed gates:
```powershell
powershell -File run-tests.ps1          # -Build to build first · -Coverage · -AiReview
```
Full methodology, layers and case list: **[docs/TESTING.md](docs/TESTING.md)**.

## Download
Grab [release v1.1](https://github.com/makflint/warehouse-mfc/releases/tag/v1.1) →
`WarehouseMFC-Setup.exe` and run it on a clean Windows machine — the [Inno Setup](installer/warehouse-mfc.iss)
installer bundles the app, SQL scripts and the VC++ runtime + LocalDB, and the app self-seeds its DB
on first run, so nothing needs preinstalling. (Unsigned — SmartScreen warns once: *More info* →
*Run anyway*.) To build it yourself, use `release.ps1` above.

See [docs/SPEC.md](docs/SPEC.md) for the design and [TODO.md](TODO.md) for the roadmap & open work.

## Author
Built by **Maciej Krzemiński**, using **Claude** generative-AI tooling — [Claude Code](https://claude.com/claude-code)
and Claude models (Anthropic) — throughout for design, code generation and tests.

## License
[MIT](LICENSE) © Maciej Krzemiński
