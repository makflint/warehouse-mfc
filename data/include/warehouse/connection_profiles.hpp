#ifndef WAREHOUSE_CONNECTION_PROFILES_HPP
#define WAREHOUSE_CONNECTION_PROFILES_HPP

namespace warehouse {

// The app ships with the DEMO (LocalDB) profile below. LocalDB *is* the SQL Server engine, so
// targeting a full instance (LAN box, VPS over Tailscale, Azure SQL) is a one-line change here:
// swap the `Server=` host (and add `UID=`/`PWD=` instead of Trusted_Connection). The schema,
// view, stored proc and all T-SQL are identical — nothing else moves.
//
// DEMO: zero-config SQL Server LocalDB, seeded by db/02_seed.sql.
inline const char* demoConnectionString() {
    return "Driver={ODBC Driver 17 for SQL Server};"
           "Server=(localdb)\\MSSQLLocalDB;Database=Warehouse;Trusted_Connection=yes;";
}

// Same LocalDB instance but the master database — used to create/seed Warehouse on
// first run, before the Warehouse database necessarily exists.
inline const char* demoMasterConnectionString() {
    return "Driver={ODBC Driver 17 for SQL Server};"
           "Server=(localdb)\\MSSQLLocalDB;Database=master;Trusted_Connection=yes;";
}

}  // namespace warehouse

#endif  // WAREHOUSE_CONNECTION_PROFILES_HPP
