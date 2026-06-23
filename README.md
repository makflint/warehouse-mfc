# warehouse-mfc

A small but real **MFC + SQL Server** desktop app: warehouse stock & movements with a modern
**MFC Feature Pack** UI (ribbon, dockable panes, themed dashboard) and **undo/redo** (Command
pattern). Built as a portfolio piece for a *Senior C++ Developer (MFC)* role.

> Status: **M0–M6 + M8 done** — DB, `core/` (TDD), `data/` (ODBC), MFC UI (undo/redo, low-stock),
> one-click installer, and a **Feature Pack UI**: `CMFCRibbonBar`, dockable panes, dark-mode
> themes, and an owner-drawn dashboard (KPI tiles + bar chart). Open work lives in [TODO.md](TODO.md).
>
> *An earlier offline-voice experiment (Polish STT via whisper.cpp + SAPI TTS) was built,
> evaluated, and archived — preserved on branch `archive/voice-stt-tts` (tag
> `voice-m4-m7-complete`); `git checkout archive/voice-stt-tts` brings it back.*

## Why this app
- **MFC**: SDI doc/view, list-view grid, dialogs with DDX/DDV — core of the offer.
- **MS SQL Server**: real schema, a **view** and a **stored procedure with a transaction**.
- **Design patterns**: **Command** pattern for undo/redo (explicitly requested in the offer).
- **Testable core**: domain logic (stock math, the Command/undo stack) lives in a pure C++
  `core/` static lib with **TDD** (Catch2), verified without a GUI.
- **Modern MFC (M8)**: an MFC **Feature Pack** UI — `CMFCRibbonBar`, dockable panes,
  visual-manager themes (dark mode), and an owner-drawn **dashboard** (KPI tiles + bar chart).
- **Localized & Unicode-correct**: Polish UI *and* data with full diacritics end-to-end —
  UTF-8 `.cpp`/`.rc` resources (incl. the localized Feature Pack context menus) and wide ODBC
  (`SQLExecDirectW`, `N'…'` literals, `NVARCHAR`). The seed names (*Młotek*, *Wkrętarka*,
  *Śruba*…) and SQL Server error messages round-trip cleanly to the UI.

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

## Two build profiles (same code, different connection string)
| | DEMO (for guests) | DEV (for you) |
|---|---|---|
| DB | **SQL Server LocalDB** (zero-config, seeded) | **SQL Server on VPS** over Tailscale |
| Connection | `Server=(localdb)\MSSQLLocalDB` | `Server=100.84.173.113` |
| Install | one-click **MSI** (Inno Setup) | manual |

No web/HTTP in C++ — the app only talks to SQL Server via **ODBC**.

## Windows prerequisites (all free)
1. **Visual Studio Community 2022** → workload *"Desktop development with C++"* + optional
   component **"C++ MFC for latest v143 build tools"**.
2. **SQL Server 2022 Express** (includes **LocalDB**) + **SSMS**.
3. **Claude Code** for Windows (to continue this project where the toolchain lives).

## Quickstart (Windows)
```powershell
git clone <this repo>
cd warehouse-mfc
# 1) create the demo DB in LocalDB and seed it (optional — the app self-seeds on first run)
sqlcmd -S "(localdb)\MSSQLLocalDB" -i db\01_schema.sql
sqlcmd -S "(localdb)\MSSQLLocalDB" -i db\02_seed.sql
# 2) open warehouse-mfc.sln in Claude Code / VS and follow docs/PLAN.md
```
> The SQL scripts are **UTF-8** (Polish names with diacritics). The app's first-run self-seed
> (`SQLExecDirectW`) and the modern `sqlcmd` handle this natively; the classic SQL-tools
> `sqlcmd` needs `-f 65001`.

## Testing
- **Unit tests (`core/`)** — stock math and the Command/undo stack are TDD'd with **Catch2**;
  build `core_tests` and run `x64\Debug\core_tests.exe` (exit `0` = green).
- **Manual UI harness** — [`tests/manual/`](tests/manual/) drives the *running* app with real
  mouse/keyboard + **UI Automation** (PowerShell) and screenshots each step, for exploratory
  end-to-end checks beyond the happy path: right-click context menus, invalid/boundary quantity,
  stock overdraw (rejected by the stored proc), undo/redo, theme switching, tiny-window resize.
  See [its README](tests/manual/README.md) for the Windows DPI/timing quirks it works around.

```powershell
# unit tests
msbuild warehouse-mfc.sln /p:Configuration=Debug /p:Platform=x64 /t:core_tests
x64\Debug\core_tests.exe
# manual UI pass (app must be built)
. tests\manual\uia.ps1   # then: Pin-App; Click-Point ...; Shot-App 01
```

## Demo installer (one-click)
An [Inno Setup](installer/warehouse-mfc.iss) script bundles the app, the SQL scripts and the
two runtime prerequisites (Visual C++ runtime + SQL Server LocalDB) and installs them silently.
The app **seeds its LocalDB database on first run**, so it works on a fresh machine.

```powershell
# build the Release app, then:
"%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" installer\warehouse-mfc.iss
# -> installer\Output\warehouse-mfc-setup.exe
```
> The binary assets (`vc_redist.x64.exe`, `SqlLocalDB.msi`) live under `installer/assets/`
> (gitignored) and must be present before compiling. The build is **unsigned**, so Windows
> SmartScreen will warn on first run ("More info" → "Run anyway").

See [docs/SPEC.md](docs/SPEC.md) for the design and [docs/PLAN.md](docs/PLAN.md) for the
ordered implementation milestones.
