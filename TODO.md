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

## Next — M8: modern MFC Feature Pack UI + dashboard
The strongest *Senior C++/MFC* signal: replace the plain SDI shell with a modern Feature-Pack UI.
Decisions locked: **full Ribbon + dockable panes + themes**, and a **bar chart of current stock**
(no time-series).
- [ ] **Frame upgrade** to the MFC Feature Pack: `CWinAppEx` + `CFrameWndEx`, `CMFCVisualManager`
  theming with a dark-mode toggle (`#include <afxcontrolbars.h>`).
- [ ] **Ribbon** (`CMFCRibbonBar`) replacing the menu — tabs *Magazyn* (Refresh / Przyjmij /
  Wydaj / Filtr) and *Widok* (panes, theme), with a quick-access toolbar.
- [ ] **Dockable panes** (`CDockablePane`): **Dashboard**, **Movement log** (recent movements),
  **Details** (selected product).
- [ ] **Dashboard**: GDI+ owner-drawn **bar chart** of on-hand per product (and/or per
  warehouse) from `vCurrentStock`, plus KPI tiles (total SKUs, # low-stock, total units).
  No schema change.
- [ ] **README** refresh with new screenshots; update `docs/SPEC.md` / `docs/PLAN.md` for M8.

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
