# SPEC — warehouse-mfc

## Goal
A small, polished MFC + SQL Server desktop app demonstrating, for a Senior C++/MFC role:
MFC UI (doc/view, grid, dialogs+DDX/DDV), SQL Server (view + stored proc + transaction),
the **Command** design pattern (undo/redo), and a creative, domain-justified **voice control**.

## Domain
Warehouse inventory. Stock is **derived from an append-only movement log** (no mutable
"quantity" column) — every receive/ship is a row; current stock = sum of movements. This is
clean, auditable, and makes the transaction/concurrency story real.

### Entities
- **Warehouse** (Code, Name)
- **Product** (Sku, Name, Unit, ReorderLevel)
- **StockMovement** (Product, Warehouse, Qty signed +IN/−OUT, MovementType, Note, CreatedAt)

### SQL objects (see `db/01_schema.sql`)
- View **`vCurrentStock`** — per product×warehouse on-hand = `SUM(Qty)`, plus `IsLow` flag
  (`OnHand <= ReorderLevel`). This is the main grid source (a JOIN + GROUP BY).
- Stored proc **`sp_RecordMovement`** — wraps validation + insert in a **transaction** with
  `UPDLOCK/HOLDLOCK` (concurrency), rejects an OUT that would drive stock negative (`THROW`),
  returns the new on-hand. This is the interview centrepiece.

## Application architecture
Three layers (see `CLAUDE.md` for the rule "keep these boundaries"):

1. **core/** (pure C++, unit-tested, no MFC/ODBC)
   - `MovementCommand` (IExecutable/IUndoable): execute = record a movement; undo = record the
     **compensating** movement. `CommandStack` holds undo/redo history.
   - `VoiceCommandParser`: maps a recognized phrase → a `MovementCommand` or a query intent.
2. **data/** (ODBC, thin)
   - `StockRepository`: `loadCurrentStock()`, `recordMovement(...)` (calls `sp_RecordMovement`),
     `products()`, `warehouses()`. Connection string from the active profile.
3. **app/** (MFC + SAPI)
   - SDI **doc/view**; `CMFCListCtrl` showing `vCurrentStock`, low-stock rows owner-drawn red.
   - Dialogs: **Product edit** (DDX/DDV), **Record movement** (product/warehouse/qty/type).
   - Toolbar/menu: Refresh, Record IN, Record OUT, **Undo/Redo (Ctrl+Z/Ctrl+Y)**, Low-stock filter.
   - **Voice (SAPI, command-and-control grammar)** + **TTS** confirmation.

## Voice control (SAPI, scoped)
Use **command-and-control with a small fixed grammar** (NOT free dictation) for reliability.
Polish grammar (requires Windows PL speech pack):
- `przyjmij <number> <product>`  → IN movement
- `wydaj <number> <product>`     → OUT movement
- `pokaż niskie stany`           → toggle low-stock filter
- `odśwież`                      → refresh grid
- `cofnij` / `ponów`            → undo / redo

`<product>` is referenced by SKU spoken as digits (most reliable) or a short alias.
Recognized phrase → `VoiceCommandParser` → same `MovementCommand` used by the dialogs (one code
path). TTS speaks the result: *"zarejestrowano, nowy stan 35"*.

**Implementation note (reality on Windows):** TTS is implemented (SAPI, Microsoft Paulina —
the app speaks Polish confirmations). Speech *recognition* is **not** shipped: Windows has no
on-device pl-PL recognizer (only en-US), and real Polish recognition would require cloud STT,
which conflicts with the **ODBC-only / no-networking** rule. `VoiceCommandParser` is built and
unit-tested in `core/`, so the recognition path is designed and ready to wire if that
constraint is ever relaxed.

## Two build profiles
- **DEMO**: `Server=(localdb)\MSSQLLocalDB`, seeded by `db/02_seed.sql`, shipped as one-click MSI.
- **DEV**: `Server=100.84.173.113` (SQL Server on VPS over Tailscale).
Only the connection string differs; all T-SQL is identical on LocalDB and full SQL Server.

## Out of scope (YAGNI)
- No web/HTTP in C++ (ODBC only). No live data ingestion in the app.
- No multi-user auth, no reporting beyond the low-stock filter.
- Warehouse-map visualization = possible later stretch, not in v1.

## Definition of done (demo)
LocalDB seeded; app shows current stock; record IN/OUT via dialog **and** voice; undo/redo works;
low-stock rows highlighted; packaged as an MSI that installs and runs on a clean Windows box.
