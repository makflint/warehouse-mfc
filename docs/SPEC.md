# SPEC — warehouse-mfc

## Goal
A small, polished **MFC / Windows desktop** demo app, showing: MFC UI (doc/view, grid,
dialogs+DDX/DDV), SQL Server (LocalDB) (view + stored proc + transaction), and the **Command**
design pattern (undo/redo).

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
2. **data/** (ODBC, thin)
   - `StockRepository`: `loadCurrentStock()`, `recordMovement(...)` (calls `sp_RecordMovement`),
     `products()`, `warehouses()`. Connection string from the active profile.
3. **app/** (MFC)
   - SDI **doc/view**; `CMFCListCtrl` showing `vCurrentStock`, low-stock rows owner-drawn red.
   - Dialogs: **Product edit** (DDX/DDV), **Record movement** (product/warehouse/qty/type).
   - Toolbar/menu: Refresh, Record IN, Record OUT, **Undo/Redo (Ctrl+Z/Ctrl+Y)**, Low-stock filter.

## SQL Server connection
Ships on **LocalDB** (`Server=(localdb)\MSSQLLocalDB`, self-seeded by `db/02_seed.sql`, one-click
MSI). LocalDB *is* the SQL Server engine, so a full instance (LAN box, VPS over Tailscale, Azure
SQL) is a one-line `Server=` change in `connection_profiles.hpp` — identical T-SQL/view/proc.
Everything in the repo runs against LocalDB; the full-server target is the documented switch, not
an exercised profile.

## Out of scope (YAGNI)
- No web/HTTP in C++ (ODBC only). No live data ingestion in the app.
- No multi-user auth, no reporting beyond the low-stock filter.
- Warehouse-map visualization = possible later stretch, not in v1.

## Definition of done (demo)
LocalDB seeded; app shows current stock; record IN/OUT via dialog; undo/redo works;
low-stock rows highlighted; packaged as an MSI that installs and runs on a clean Windows box.
