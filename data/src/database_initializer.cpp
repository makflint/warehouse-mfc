#include "warehouse/database_initializer.hpp"

#include <windows.h>
#include <sql.h>
#include <sqlext.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "odbc32.lib")

namespace warehouse {
namespace {

bool failed(SQLRETURN rc) {
    return rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO;
}

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    std::ostringstream stream;
    stream << file.rdbuf();
    std::string content = stream.str();
    // Strip a leading UTF-8 BOM so it doesn't end up as U+FEFF at the start of the batch.
    if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }
    return content;
}

// The .sql scripts are UTF-8; widen so SQLExecDirectW preserves Polish in N'...' literals
// (SQLExecDirectA would reinterpret the bytes in the client ANSI code page and mangle them).
std::wstring toWide(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int length =
        ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(length, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(),
                          length);
    return wide;
}

std::string trim(const std::string& line) {
    const size_t first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const size_t last = line.find_last_not_of(" \t\r\n");
    return line.substr(first, last - first + 1);
}

// Split a sqlcmd-style script into batches on lines that are just "GO".
std::vector<std::string> splitBatches(const std::string& script) {
    std::vector<std::string> batches;
    std::istringstream input(script);
    std::string line;
    std::string batch;
    while (std::getline(input, line)) {
        if (_stricmp(trim(line).c_str(), "GO") == 0) {
            if (!trim(batch).empty()) {
                batches.push_back(batch);
            }
            batch.clear();
        } else {
            batch += line;
            batch += '\n';
        }
    }
    if (!trim(batch).empty()) {
        batches.push_back(batch);
    }
    return batches;
}

bool warehouseExists(SQLHDBC dbc) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    if (failed(SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt))) {
        return true;  // assume it exists -> don't attempt a risky re-create
    }
    bool exists = true;
    auto query = reinterpret_cast<SQLCHAR*>(const_cast<char*>("SELECT DB_ID(N'Warehouse')"));
    if (!failed(SQLExecDirectA(stmt, query, SQL_NTS)) && SQLFetch(stmt) == SQL_SUCCESS) {
        SQLINTEGER value = 0;
        SQLLEN indicator = 0;
        SQLGetData(stmt, 1, SQL_C_SLONG, &value, 0, &indicator);
        exists = (indicator != SQL_NULL_DATA);
    }
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return exists;
}

void runScript(SQLHDBC dbc, const std::string& script) {
    for (const std::string& batch : splitBatches(script)) {
        SQLHSTMT stmt = SQL_NULL_HSTMT;
        if (failed(SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt))) {
            continue;
        }
        std::wstring wide = toWide(batch);
        SQLExecDirectW(stmt, reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(wide.c_str())),
                       SQL_NTS);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }
}

}  // namespace

void ensureDatabase(const std::string& masterConnectionString,
                    const std::string& schemaSqlPath,
                    const std::string& seedSqlPath) {
    const std::string schema = readFile(schemaSqlPath);
    const std::string seed = readFile(seedSqlPath);
    if (schema.empty() || seed.empty()) {
        return;  // scripts not shipped next to the exe (dev build) -> nothing to do
    }

    SQLHENV env = SQL_NULL_HENV;
    SQLHDBC dbc = SQL_NULL_HDBC;
    if (failed(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env))) {
        return;
    }
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

    std::string connection = masterConnectionString;
    if (!failed(SQLDriverConnectA(dbc, nullptr,
                                  reinterpret_cast<SQLCHAR*>(connection.data()), SQL_NTS,
                                  nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT))) {
        if (!warehouseExists(dbc)) {
            runScript(dbc, schema);  // CREATE DATABASE + USE Warehouse + objects
            runScript(dbc, seed);    // opening stock via sp_RecordMovement
        }
        SQLDisconnect(dbc);
    }
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
}

}  // namespace warehouse
