#ifndef WAREHOUSE_CONNECTION_PROFILES_HPP
#define WAREHOUSE_CONNECTION_PROFILES_HPP

namespace warehouse {

// The connection string is chosen at build/config time. Only the server differs
// between profiles; all T-SQL is identical on LocalDB and full SQL Server.
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
