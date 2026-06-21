# warehouse-mfc

A small but real **MFC + SQL Server** desktop app: warehouse stock & movements, with
**voice control** (Windows SAPI) and **undo/redo** (Command pattern). Built as a portfolio
piece for a *Senior C++ Developer (MFC)* role.

> Status: M0 (DB) + M1 (`core/`, TDD) + M2 (`data/`, ODBC) + M3 (MFC UI) done.
> Voice control (M4) is next. Open work and milestone state live in [TODO.md](TODO.md).

## Why this app
- **MFC**: SDI doc/view, `CMFCListCtrl` grid, dialogs with DDX/DDV — core of the offer.
- **MS SQL Server**: real schema, a **view** and a **stored procedure with a transaction**.
- **Design patterns**: **Command** pattern for undo/redo (explicitly requested in the offer).
- **Creative angle**: hands-free **voice commands** ("przyjmij 10 sztuk artykułu 4521"),
  justified by the domain — warehouse workers have their hands full.

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
3. For voice: Windows **Polish speech** pack (Settings → Time & Language → Speech).
4. **Claude Code** for Windows (to continue this project where the toolchain lives).

## Quickstart (Windows)
```powershell
git clone <this repo>
cd warehouse-mfc
# 1) create the demo DB in LocalDB and seed it
sqlcmd -S "(localdb)\MSSQLLocalDB" -i db\01_schema.sql
sqlcmd -S "(localdb)\MSSQLLocalDB" -i db\02_seed.sql
# 2) open in Claude Code and follow docs/PLAN.md
```

See [docs/SPEC.md](docs/SPEC.md) for the design and [docs/PLAN.md](docs/PLAN.md) for the
ordered implementation milestones.
