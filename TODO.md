# TODO — warehouse-mfc

Single source of truth for open work. Milestones follow `docs/PLAN.md`.

## ⏸ Voice (STT + TTS) — built, evaluated, ARCHIVED off `main` (2026-06-22)
M4 (SAPI TTS) + M7 (offline Polish STT via whisper.cpp) were fully built and verified, then
**removed from `main`** — too many moving parts (mic/WASAPI, whisper, 465 MB model, OS
audio-engine quirks) for a portfolio piece, and recognition stayed flaky on borderline mic
input. **Nothing is lost** — the complete working voice app is preserved on branch
**`archive/voice-stt-tts`** (tag **`voice-m4-m7-complete`**, commit `07a85aa`).
Restore: `git checkout archive/voice-stt-tts`. That branch holds all the detail (WASAPI capture,
the fuzzy command parser, the model research base/small/medium, the e2e harness, and the
"voice commands don't name a warehouse" limitation).

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
  Builds `warehouse-mfc-setup.exe`.
- [x] **M6 — Polish.** README screenshots (`docs/screenshots/`), demo-installer + SmartScreen
  notes, secret scan clean.

*(M4 TTS + M7 STT were done too — now archived, see the banner above.)*

## M8 — modern MFC Feature Pack UI + dashboard  (core DONE)
The strongest *Senior C++/MFC* signal: the plain SDI shell is now a modern Feature-Pack UI.
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
