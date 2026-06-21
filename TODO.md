# TODO — warehouse-mfc

Single source of truth for open work. Milestones follow `docs/PLAN.md`.

## Done
- [x] **M0 — Database.** Schema + idempotent seed on LocalDB; `vCurrentStock` and
  `sp_RecordMovement` (IN / OUT / insufficient-stock THROW) verified.
- [x] **M1 — `core/` (pure C++, TDD).** `StockMath` (onHand, isLow); `Movement` +
  `compensating`; Command pattern (`ICommand`, `MovementCommand`, `CommandStack`);
  `VoiceCommandParser`. 48 assertions green (Debug + Release).
- [x] **M2 — `data/` (ODBC).** `StockRepository` (pImpl): `loadCurrentStock()`,
  `recordMovement()` (calls `sp_RecordMovement`, surfaces THROW as exception),
  implements `IMovementRecorder`. Verified by `tools/data_smoke`.
- [x] **M3 — MFC UI.** SDI doc/view; `CListView` bound to `vCurrentStock` (low-stock rows
  owner-drawn red); Record-movement dialog (DDX/DDV, product/warehouse/qty); menu Refresh
  (F5) / Przyjmij (IN) / Wydaj (OUT); **Undo/Redo (Ctrl+Z/Ctrl+Y)** via `core/` CommandStack;
  low-stock filter toggle. Verified through the GUI against LocalDB (record IN 35->42,
  undo ->35; filter; red rows). Note: the Debug build needs the debug MFC DLLs on PATH to
  launch outside VS — Release runs standalone.

- [~] **M4 — Voice.** **TTS done** (SAPI, Microsoft Paulina): app speaks Polish
  confirmations on record/undo/redo. **STT (recognition) deferred** — Windows has no
  on-device pl-PL recognizer (only en-US), and real Polish recognition needs cloud STT,
  which conflicts with the ODBC-only / no-networking rule. `VoiceCommandParser` (core/)
  stays as the tested design for that path.

- [x] **M5 — Installer.** `installer/warehouse-mfc.iss` (Inno Setup) bundles app + SQL scripts
  + VC++ runtime + LocalDB MSI, installs runtimes silently. App self-seeds the DB on first run
  (verified: dropped DB, launched app, DB recreated). Builds `warehouse-mfc-setup.exe`.
- [x] **M6 — Polish.** README screenshots (`docs/screenshots/`), demo-installer + SmartScreen
  notes, secret scan clean. (Optional: a short screen recording is still a nice-to-have.)

## Done — all milestones (M0–M6) complete
Voice STT is the only deferred item (no on-device pl-PL recognizer; see M4 above).

## Build / test (Windows)
```bash
# DB (once): sqlcmd -S "(localdb)\MSSQLLocalDB" -i db\01_schema.sql ; ... -i db\02_seed.sql
msbuild warehouse-mfc.sln /p:Configuration=Debug /p:Platform=x64
./x64/Debug/core_tests.exe     # unit tests (Catch2), exit 0 = green
./x64/Debug/data_smoke.exe     # data/ smoke check against LocalDB
```

## Notes for the next session
- Toolchain installed: VS 2022 Community + C++/MFC v143, SQL Server LocalDB 2022, sqlcmd
  (go-sqlcmd), ODBC Driver 17. Machine setup history (gitignored): `docs/WINDOWS_SETUP_LOG.md`.
- go-sqlcmd does **not** resolve `(localdb)\MSSQLLocalDB` — connect via the named pipe
  (`SqlLocalDB.exe info MSSQLLocalDB`). The C++ ODBC layer resolves `(localdb)\` natively.
- DB product/warehouse IDs are **not** 1..N (IDENTITY climbed across re-seeds) — resolve
  ids from data (Sku/Code), never hardcode them.
