#ifndef WAREHOUSE_DATABASE_INITIALIZER_HPP
#define WAREHOUSE_DATABASE_INITIALIZER_HPP

#include <string>

namespace warehouse {

// First-run convenience for the packaged demo: if the Warehouse database does not
// exist on the LocalDB instance, create and seed it by running the given schema and
// seed scripts (split on GO batches). No-op if the database already exists or the
// script files are missing (e.g. a dev build run from the build tree). Best-effort:
// never throws, so app startup is unaffected if anything goes wrong.
void ensureDatabase(const std::string& masterConnectionString,
                    const std::string& schemaSqlPath,
                    const std::string& seedSqlPath);

}  // namespace warehouse

#endif  // WAREHOUSE_DATABASE_INITIALIZER_HPP
