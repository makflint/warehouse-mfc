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

- [x] **M4 — Voice (TTS).** SAPI, Microsoft Paulina: app speaks Polish confirmations on
  record/undo/redo. STT (recognition) is M7 below.

- [x] **M5 — Installer.** `installer/warehouse-mfc.iss` (Inno Setup) bundles app + SQL scripts
  + VC++ runtime + LocalDB MSI, installs runtimes silently. App self-seeds the DB on first run
  (verified: dropped DB, launched app, DB recreated). Builds `warehouse-mfc-setup.exe`.
- [x] **M6 — Polish.** README screenshots (`docs/screenshots/`), demo-installer + SmartScreen
  notes, secret scan clean. (Optional: a short screen recording is still a nice-to-have.)

- [x] **M7 — Offline Polish STT (whisper.cpp).** Real hands-free voice, fully offline (model is
  a local file, inference local, **no runtime networking** — does not break the ODBC-only rule).
  - whisper.cpp **v1.9.1** as a git **submodule** (`third_party/whisper.cpp`), built to **static
    libs** (Release+Debug, OpenMP off) with the VS-bundled cmake+ninja.
  - `app/MicCapture.*` (waveIn, 4 s @ 16 kHz mono) + `app/Stt.*` (whisper wrapper, `language="pl"`).
    Push-to-talk: **Słuchaj (F2)** menu/hotkey → capture+recognise on a worker thread →
    `PostMessage(WM_STT_RESULT)` → UI thread.
  - Recognised text → `warehouse::parseVoiceCommand` (core/) → same doc commands as the menus.
    Parser gained TDD'd **normalisation** (lower-case incl. Polish letters + strip punctuation)
    so whisper's "Pokaż niskie stany." matches. 60 assertions green.
  - Model **`ggml-base.bin`** (multilingual, 141 MB) loaded from next to the exe; shipped in the
    installer; gitignored locally. Verified: jfk.wav + SAPI-spoken "Pokaż niskie stany" both
    recognised correctly through our exact libs. (Live mic path is interactive — test with F2.)

## Done — all milestones (M0–M7) complete
Working tree: new submodule + app/core/installer/docs changes (commit pending). The **live mic
→ command** path is the only thing needing a human with a microphone to exercise (press **F2**,
speak, e.g. "odśwież", "pokaż niskie stany", "cofnij", "przyjmij 10 4521").

### Possible later polish (not blocking)
- Larger model (`ggml-small.bin`) if base mis-hears; a brief on-screen "Słucham…" status while
  capturing; configurable capture seconds; recognised-text echo in the status bar.

## Build / test (Windows)
```bash
# DB (once): sqlcmd -S "(localdb)\MSSQLLocalDB" -i db\01_schema.sql ; ... -i db\02_seed.sql

# whisper.cpp (once): fetch the submodule, build static libs, download the model
git submodule update --init --recursive
cmake -S third_party/whisper.cpp -B third_party/whisper.cpp/build -G "Visual Studio 17 2022" -A x64 \
      -DBUILD_SHARED_LIBS=OFF -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF -DGGML_OPENMP=OFF
cmake --build third_party/whisper.cpp/build --config Release --target whisper   # and --config Debug
# model -> models\ggml-base.bin (from https://huggingface.co/ggerganov/whisper.cpp)

msbuild warehouse-mfc.sln /p:Configuration=Debug /p:Platform=x64
./x64/Debug/core_tests.exe     # unit tests (Catch2), exit 0 = green
./x64/Debug/data_smoke.exe     # data/ smoke check against LocalDB
# app -> app\x64\<Config>\app.exe (model auto-copied next to it by a post-build step)
```

## Notes for the next session
- Toolchain installed: VS 2022 Community + C++/MFC v143, SQL Server LocalDB 2022, sqlcmd
  (go-sqlcmd), ODBC Driver 17. Machine setup history (gitignored): `docs/WINDOWS_SETUP_LOG.md`.
- go-sqlcmd does **not** resolve `(localdb)\MSSQLLocalDB` — connect via the named pipe
  (`SqlLocalDB.exe info MSSQLLocalDB`). The C++ ODBC layer resolves `(localdb)\` natively.
- DB product/warehouse IDs are **not** 1..N (IDENTITY climbed across re-seeds) — resolve
  ids from data (Sku/Code), never hardcode them.
