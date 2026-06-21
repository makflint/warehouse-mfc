# PLAN — warehouse-mfc (ordered milestones)

Each milestone is small and verifiable. Do them in order. TDD the `core/` lib.

## M0 — Database (no C++ yet)
- Run `db/01_schema.sql` then `db/02_seed.sql` against LocalDB.
- In SSMS verify: `SELECT * FROM vCurrentStock;` and exercise `sp_RecordMovement` (an IN, an OUT,
  and an OUT that must fail with insufficient stock).
- ✅ Done when the view and proc behave per SPEC.

## M1 — core/ domain lib (pure C++, TDD, no MFC/ODBC)
- `StockMath` (sum/derive on-hand, low-stock check) — tests first.
- `MovementCommand` + `CommandStack` (execute/undo/redo via compensating movements) — tests.
- `VoiceCommandParser` (phrase → command/intent) — tests for each grammar rule + garbage input.
- ✅ Done when `core_tests` (Catch2) is green.

## M2 — data/ ODBC layer
- `StockRepository`: connect (profile-driven string), `loadCurrentStock()`, `recordMovement()`
  calling `sp_RecordMovement` (capture the OUTPUT new-on-hand and the `THROW` error).
- Manual check against LocalDB.
- ✅ Done when a tiny console driver can read stock and record a movement.

## M3 — MFC UI
- Scaffold SDI doc/view (VS MFC App wizard). Add `CMFCListCtrl` bound to `vCurrentStock`; Refresh.
- Owner-draw low-stock rows red.
- Dialogs: Product edit (DDX/DDV), Record movement. Wire to `data/` via `core/` commands.
- Undo/Redo on toolbar + Ctrl+Z/Ctrl+Y. Low-stock filter toggle.
- ✅ Done when full CRUD-of-movements works from the GUI with working undo/redo.

## M4 — Voice (SAPI) + TTS
- Init SAPI in command-and-control mode with the SPEC grammar (PL).
- Route recognized commands through the **same** `MovementCommand` path as the dialogs.
- TTS confirmation of results.
- ✅ Done when "przyjmij/wydaj/cofnij/odśwież" drive the app hands-free.

## M5 — Demo packaging (MSI)
- Build the DEMO profile (LocalDB). Generate a seeded LocalDB on first run (run schema+seed,
  or ship an attached `.mdf`).
- **Inno Setup** script → one-click MSI; installs app + ensures LocalDB DB exists.
- Note "unsigned demo build" (SmartScreen).
- ✅ Done when the MSI installs and runs on a clean Windows VM.

## M6 — Polish / portfolio
- README screenshots + a short screen-recording (the real "demo" for a desktop app).
- Make the repo public-ready (no secrets; sensible commit history).
