# CLAUDE.md — warehouse-mfc

## What this is
MFC + SQL Server desktop demo (warehouse stock & movements) with voice control and undo/redo.
Portfolio piece for a Senior C++/MFC role. Read `docs/SPEC.md` (design) and `docs/PLAN.md`
(ordered milestones) before coding.

## Platform
- Built on **Windows** with **Visual Studio 2022 Community** (MFC component installed).
- DB: **SQL Server LocalDB** for the demo build, full **SQL Server** (VPS, Tailscale) for dev.

## Architecture (keep these boundaries)
- **`core/`** — pure C++ static lib, **no MFC, no ODBC, no Windows headers**. Domain logic:
  stock math, the Command/undo stack, the voice-command parser (text → Command). Unit-tested.
- **`data/`** — ODBC data access (query `vCurrentStock`, call `sp_RecordMovement`). Thin.
- **`app/`** — MFC UI (doc/view, dialogs) + SAPI TTS + **offline STT** (`Stt.*` wraps
  whisper.cpp, `MicCapture.*` is waveIn). Wires `core` + `data`. Recognised speech goes through
  `core/`'s parser, so the platform/IO stays here and the logic stays testable.
- **`third_party/whisper.cpp`** — git submodule (offline ASR), built to static libs, linked into
  `app/`. The ggml model is a local file under `models/` (gitignored) — see `TODO.md`.
- Rationale: the testable logic lives in `core/` so we can TDD it without a GUI.

## Conventions
- **TDD for `core/`**: red → green → refactor. Test framework: **Catch2** (amalgamated header).
- Clean Code: small functions, meaningful names, no magic numbers, DRY. C++17/20.
- Patterns the offer asks for: **Command** (undo/redo), RAII, clear ownership.
- **No networking in C++.** The app uses **ODBC only**. Voice STT (whisper.cpp) is fully
  **offline** — local model file, local inference — so it keeps this rule; the model download is
  a one-time build/install step (like `SqlLocalDB.msi`). Data ingestion is out of scope here.

## Connection profiles (single switch)
Connection string chosen at build/config time:
- DEMO: `Driver={ODBC Driver 17 for SQL Server};Server=(localdb)\MSSQLLocalDB;Database=Warehouse;Trusted_Connection=yes;`
- DEV:  `Driver={ODBC Driver 17 for SQL Server};Server=100.84.173.113;Database=Warehouse;UID=...;PWD=...;`

## Build / test commands
- DB setup: `sqlcmd -S "(localdb)\MSSQLLocalDB" -i db\01_schema.sql` then `... -i db\02_seed.sql`
- Build: open `warehouse-mfc.sln` in VS, or
  `msbuild warehouse-mfc.sln /p:Configuration=Debug /p:Platform=x64`
- Core tests: build & run `core_tests` (Catch2 amalgamated under `tests/third_party/catch2`):
  the exe lands at `x64\Debug\core_tests.exe` (exit 0 = green). In VS: set `core_tests` as
  startup, Ctrl+F5.
- Solution layout: `core/` (static lib, pure C++) + `tests/core_tests` (console, Catch2),
  both `Debug|x64` / `Release|x64`, toolset v143, C++17.
