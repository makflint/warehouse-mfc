# TODO — warehouse-mfc

Single source of truth for open work. Milestones follow `docs/PLAN.md`.

## Done
- [x] **M0 — Database.** Schema + idempotent seed on LocalDB; `vCurrentStock` and
  `sp_RecordMovement` (IN / OUT / insufficient-stock THROW) verified.
- [x] **M1 — `core/` (pure C++, TDD).** `StockMath` (onHand, isLow); `Movement` +
  `compensating`; Command pattern (`ICommand`, `MovementCommand`, `CommandStack`). Catch2,
  **31 assertions green** (Debug + Release).
- [x] **M2 — `data/` (ODBC).** `StockRepository` (pImpl): `loadCurrentStock()`,
  `recordMovement()` (calls `sp_RecordMovement`, surfaces THROW as exception),
  implements `IMovementRecorder`. Verified by `tools/data_smoke`.
- [x] **M3 — MFC UI.** SDI doc/view; `CListView` bound to `vCurrentStock` (low-stock rows
  owner-drawn red); Record-movement dialog (DDX/DDV, product/warehouse/qty); menu Refresh
  (F5) / Przyjmij (IN) / Wydaj (OUT); **Undo/Redo (Ctrl+Z/Ctrl+Y)** via `core/` CommandStack;
  low-stock filter toggle. Note: the Debug build needs the debug MFC DLLs on PATH to launch
  outside VS — Release runs standalone.
- [x] **M5 — Installer.** `installer/warehouse-mfc.iss` (Inno Setup) bundles app + SQL scripts +
  VC++ runtime + LocalDB MSI, installs runtimes silently. App self-seeds the DB on first run.
  Builds `WarehouseMFC-Setup.exe`.
- [x] **M6 — Polish.** README screenshots (`docs/screenshots/`), demo-installer + SmartScreen
  notes, secret scan clean.

## M8 — modern MFC Feature Pack UI + dashboard  (core DONE)
The modern-MFC showcase: the plain SDI shell is now a modern Feature-Pack UI.
- [x] **Frame upgrade** — `CWinAppEx` + `CFrameWndEx`, `CMFCVisualManager` with a **theme picker**
  (Widok → Motyw drop-down: Office 2007 Blue/Black/Silver/Aqua, Office 2003, VS 2008, Windows 7,
  radio-checked); `<afxcontrolbars.h>`.
- [x] **Ribbon** (`CMFCRibbonBar`) replaces the menu — *Magazyn* tab (Stany / Ruchy / Edycja
  panels on the existing command ids) + *Widok* tab (theme + pane toggles). The IDR_MAINFRAME
  menu is kept as the SDI shared menu but hidden via `SetMenu(nullptr)`.
- [x] **Dockable panes** (`CDockablePane`, `DockPanes.*`): **Pulpit/Dashboard**, **Dziennik
  ruchów** (Movement log), **Szczegóły** (Details), toggled from the Widok tab.
- [x] **Dashboard** — `CDashboardPane` owner-draws (double-buffered) 3 KPI tiles (SKUs /
  low-stock / total units) + a bar chart of on-hand per SKU from `vCurrentStock`; repaints on
  every grid update (KPIs verified 5 / 7 / 154).
- [x] **README + screenshots** refreshed for the Feature Pack UI
  (`docs/screenshots/01-dashboard`, `02-dark-theme`, `03-record-dialog`).
- [x] **Panes populated**: Details (selection-driven product info) and Movement log
  (`StockRepository::loadRecentMovements` → recent IN/OUT with timestamps).
- [x] **Ribbon icons** — 32-bit ARGB (alpha-blended) glyph strips per category
  (`app/res/ribbon_*.bmp`, regenerate with `tools/gen_ribbon_icons.ps1`).
- [x] **Tabbed dock layout** — Szczegóły + Dziennik share one tab group
  (`AttachToTabWnd`) instead of two same-side panes, so both stay reachable at any
  width (fixes the earlier narrow-window clipping).
- [x] **Status bar** (`CMFCRibbonStatusBar`) — *Pozycje* (stock rows) + selected SKU on
  the left, connection profile (`DEMO · LocalDB`) right-aligned.
- [x] **UX polish round:** Dziennik gained a **Magazyn** column and now shows **local
  Warsaw time** (`AT TIME ZONE`, DST-aware) instead of UTC; all three list grids share
  the system UI font (Segoe UI) for a consistent look; the main grid is **sortable** by
  clicking any header (toggles asc/desc); the dashboard repaints fully on resize (no
  stale rectangles when dragging the divider).
- [x] **Polish terminology:** *Asortyment* (distinct items), *Symbol* (item-code column —
  was "SKU"), *Niskie stany magazynowe*, *Suma sztuk*, *Stan magazynowy* (was the "na
  rękę" calque), *Pozycje* (status bar).
- [x] **Feature-Pack list controls (full sweep):** the bare `SysListView32` grids are
  gone. The **main grid** is a `CView` hosting a `CMFCListCtrl` (themed header, native
  click-sort via `OnCompareItems` with the orange marked column + arrow, low-stock red
  via `OnGetCellTextColor`); the **Dziennik** is a `CMFCListCtrl`; **Szczegóły** is a
  `CMFCPropertyGridCtrl` (themed key/value grid). All are drawn by the visual manager,
  so they look consistent with the ribbon/panes instead of "z innej bajki".
- [x] **True dark theme** ("Ciemny (pełny)" in the Motyw picker): a custom
  `CDarkVisualManager : CMFCVisualManagerOffice2007` (ObsidianBlack chrome) overrides the
  header draw (`OnFillHeaderCtrlBackground` / `OnDrawHeaderCtrlBorder` /
  `OnDrawHeaderCtrlSortArrow`) so the list/grid headers go dark; header **text** uses the
  global `clrBtnText`, flipped light for this theme. Content surfaces recolour via
  `ApplyContentTheme`: grid + Dziennik (`OnGetCell*Color`), dashboard (owner-draw), and the
  Szczegóły property grid (`SetCustomColors`). The other 7 themes are the framework's clean
  light content. (No turnkey OSS dark manager exists for the Feature Pack — this is the
  documented derive-and-override approach.)
- [x] **Dziennik sorts by the Czas column only** (`CMovementLogList` overrides `Sort` to
  ignore the other columns and `OnCompareItems` to order by the timestamp); the main grid
  stays sortable on every column.
- [ ] *Optional later:* per-column **filters** on the main grid; update `docs/SPEC.md` /
  `docs/PLAN.md`. Note: the docked list panes + ribbon status bar only paint when the
  window is foreground-active, so the headless screenshot tooling can't render them —
  capture hero shots live.

## M9 — localisation, polish & testability  (DONE this round)
- [x] **Bilingual UI (PL + EN).** String catalog `app/I18n.*` (`T(Tx)`), language chosen at
  startup (saved choice → else Windows UI language), **Język/Language** toggle on the Widok tab
  (applied on restart). Whole chrome localised; EN layout fix (Warehouse column). *Scope A:* DB
  content + stored-proc error stay Polish.
- [x] **App icon.** Multi-res `app/res/app.ico` (warehouse shelving) → window / taskbar /
  Alt+Tab / Explorer; plus a round **ribbon application button** (`IDB_APPBTN`) showing it in the title bar.
- [x] **Status bar filtered count** — `Pozycje: <shown> z <all>` when the low-stock filter is on.
- [x] **„Przywróć układ okien"** — clears saved docking/window state and restarts to the default layout.
- [x] **Assertion UI suite** `tests/ui/` (Pester + UIA + cross-process `LVcnt`/`LVtext`): grid
  sort/filter/selection/undo, dialog pre-select + validation, panes/resize. Codified visual
  **`sweep.ps1`** (PL/EN). Pure logic extracted to `core/` (`cleanDbError`, grid-sort,
  combo-preselect) + unit-tested. Record dialog is theme-aware (dark) with Segoe UI.

## Backlog — open work
### UX & features
- [x] **App button:** menu dropped — the orb is **just the icon** (`CIconOnlyAppButton` swallows the click).
- [x] **Main grid double-click** → opens *Przyjmij* pre-selected to that row.
- [x] **Main grid right-click context menu** (Przyjmij / Wydaj / Odśwież / Szczegóły).
- [x] **Help** — a *Pomoc/Help* button (top-right of the tabs) + **F1** open an **About** dialog
  (icon, name, description, version, shortcut hints; localised + dark-aware).
- [ ] *(later)* per-column filters on the main grid.

### i18n — beyond scope A
- [ ] Framework context menus (dock-pane Float/Dock/Hide) + ribbon **tooltips / status prompts**
  are still Polish (compiled `.rc` STRINGTABLE) → **satellite resource DLLs** for full localisation.
- [ ] DB content (product/warehouse names) + the `sp_RecordMovement` error stay Polish → English
  columns + language-aware error (code → client message) if wanted.
- [ ] *(nice)* live language switch without a restart.

### Dialog polish
- [ ] Owner-draw **theme-following combos** in the record dialog (flat, dark-aware) — the
  "BCG-style combo" deferred earlier; MFC ships no themed combo for dialogs.

### Testing
- [x] **Coverage** — `core/` at **100% line coverage** (OpenCppCoverage over Debug `core_tests`),
  `tools/coverage.ps1` / `run-tests.ps1 -Coverage`. Edge-case tests added for the trim / unbalanced-
  bracket / spaced-prefix branches and the text-column movement compares. See `docs/TESTING.md`.
- [ ] **Trim `sweep.ps1`** to the genuinely-visual + un-assertable cases (don't duplicate what
  `tests/ui` already asserts).
- [ ] Document **`LVcnt` / `LVtext`** (cross-process grid reading) in `docs/TESTING.md`.
- [ ] Record the automation limits: ribbon menus / confirm modals aren't drivable here
  (foreground/focus); the sort arrow is owner-drawn (not machine-readable).

### Docs / README
- [x] **Test methodology** summary in the README (link `docs/TESTING.md`).
- [x] **Build / "local CI"** section: the local build+test script (`run-tests.ps1` — layers,
  switches, exit-code = failed gates; `-AiReview` for the AI eyeball pass).
- [x] README **SQL Server** section made honest: ships on LocalDB (same engine), full server is a
  one-line `Server=` switch in `connection_profiles.hpp`; everything in the repo runs against
  LocalDB — the VPS/Tailscale profile is the documented switch, not an exercised second profile.
- [x] **Installer + GitHub** — published [release **v1.1**](https://github.com/macius702/warehouse-mfc/releases/tag/v1.1)
  with `WarehouseMFC-Setup.exe` (installs as `WarehouseMFC.exe`, app "Warehouse MFC"); README links
  the latest release. (v1.0 was the older name + pre-pane-fix build.)
- [x] **Architecture** section + **C++/Windows techniques** table in the README (layer diagram
  with enforced boundaries; Command/pImpl/RAII, Feature Pack, ODBC wide API + proc/transaction,
  compile-time i18n, TDD/coverage, /WX + clang-tidy).
- [ ] *(nice)* a guided **project tour**.

### Engineering hygiene
- [x] **Warnings-as-errors** — `core/` + `data/` + `app/` at **`/W4 /WX`** (MFC headers external-
  silenced); **clang-tidy** curated set (`.clang-tidy`) clean on `core/` + `data/`. See
  `docs/TESTING.md` → *Static analysis*.
- [x] **Clean-code sweep.** Dashboard `OnPaint` (~100-line monolith) split into named steps
  (`computeKpis` / `drawKpiTiles` / `drawBarChart`) with the palette + layout magic numbers named;
  `BuildRibbon` split into `BuildStockTab` / `BuildViewTab`; Dziennik column indices replaced by a
  `MovementLogColumn` enum. Pixels + behaviour unchanged (sweep + all gates green).

## Build / test (Windows)
```bash
# DB (once): sqlcmd -S "(localdb)\MSSQLLocalDB" -i db\01_schema.sql ; ... -i db\02_seed.sql
msbuild warehouse-mfc.sln /p:Configuration=Debug /p:Platform=x64
./x64/Debug/core_tests.exe     # unit tests (Catch2), exit 0 = green
./x64/Debug/data_smoke.exe     # data/ smoke check against LocalDB
# app -> app\x64\<Config>\app.exe (Release runs standalone)
```

## Notes for the next session
- Toolchain: VS 2022 Community + C++/MFC v143, SQL Server LocalDB 2022, sqlcmd (go-sqlcmd),
  ODBC Driver 17. Machine setup history (gitignored): `docs/WINDOWS_SETUP_LOG.md`.
- go-sqlcmd does **not** resolve `(localdb)\MSSQLLocalDB` — connect via the named pipe
  (`SqlLocalDB.exe info MSSQLLocalDB`). The C++ ODBC layer resolves `(localdb)\` natively.
- DB product/warehouse IDs are **not** 1..N (IDENTITY climbed across re-seeds) — resolve
  ids from data (Sku/Code), never hardcode them.
- M8 Feature Pack needs `afxcontrolbars.h` in `framework.h` and the app class on `CWinAppEx`.
