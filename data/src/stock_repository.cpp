#include "warehouse/stock_repository.hpp"

#include <windows.h>
#include <sql.h>
#include <sqlext.h>

#include <algorithm>
#include <stdexcept>
#include <string>

#pragma comment(lib, "odbc32.lib")

namespace warehouse {
namespace {

bool failed(SQLRETURN rc) {
    return rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO;
}

std::string utf16ToUtf8(const SQLWCHAR* text, int lengthChars) {
    if (lengthChars <= 0) {
        return {};
    }
    const int bytes = ::WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t*>(text),
                                            lengthChars, nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<size_t>(bytes), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t*>(text), lengthChars,
                          out.data(), bytes, nullptr, nullptr);
    return out;
}

// Collect the ODBC diagnostic records into a readable message and throw.
[[noreturn]] void throwOdbc(SQLSMALLINT handleType, SQLHANDLE handle, const std::string& context) {
    std::string message = context;
    SQLWCHAR state[6] = {};
    SQLWCHAR text[1024] = {};
    SQLINTEGER native = 0;
    SQLSMALLINT length = 0;
    for (SQLSMALLINT row = 1;
         SQLGetDiagRecW(handleType, handle, row, state, &native, text,
                        static_cast<SQLSMALLINT>(std::size(text)), &length) == SQL_SUCCESS;
         ++row) {
        message += "\n  [" + utf16ToUtf8(state, 5) + "] " + utf16ToUtf8(text, length);
    }
    throw std::runtime_error(message);
}

void check(SQLRETURN rc, SQLSMALLINT handleType, SQLHANDLE handle, const std::string& context) {
    if (failed(rc)) {
        throwOdbc(handleType, handle, context);
    }
}

int getInt(SQLHSTMT stmt, SQLUSMALLINT column) {
    SQLINTEGER value = 0;
    SQLLEN indicator = 0;
    check(SQLGetData(stmt, column, SQL_C_SLONG, &value, 0, &indicator), SQL_HANDLE_STMT, stmt,
          "SQLGetData (int)");
    return value;
}

std::string getString(SQLHSTMT stmt, SQLUSMALLINT column) {
    SQLWCHAR buffer[256] = {};
    SQLLEN indicator = 0;
    check(SQLGetData(stmt, column, SQL_C_WCHAR, buffer, sizeof(buffer), &indicator),
          SQL_HANDLE_STMT, stmt, "SQLGetData (string)");
    if (indicator == SQL_NULL_DATA) {
        return {};
    }
    return utf16ToUtf8(buffer, static_cast<int>(indicator / static_cast<SQLLEN>(sizeof(SQLWCHAR))));
}

}  // namespace

struct StockRepository::Impl {
    SQLHENV env = SQL_NULL_HENV;
    SQLHDBC dbc = SQL_NULL_HDBC;
};

StockRepository::StockRepository(const std::string& connectionString)
    : impl_(std::make_unique<Impl>()) {
    check(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &impl_->env), SQL_HANDLE_ENV,
          impl_->env, "alloc env");
    check(SQLSetEnvAttr(impl_->env, SQL_ATTR_ODBC_VERSION,
                        reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0),
          SQL_HANDLE_ENV, impl_->env, "set ODBC version");
    check(SQLAllocHandle(SQL_HANDLE_DBC, impl_->env, &impl_->dbc), SQL_HANDLE_DBC, impl_->dbc,
          "alloc dbc");

    const std::wstring wide(connectionString.begin(), connectionString.end());
    check(SQLDriverConnectW(impl_->dbc, nullptr,
                            const_cast<SQLWCHAR*>(reinterpret_cast<const SQLWCHAR*>(wide.c_str())),
                            SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT),
          SQL_HANDLE_DBC, impl_->dbc, "connect");
}

StockRepository::~StockRepository() {
    if (impl_->dbc != SQL_NULL_HDBC) {
        SQLDisconnect(impl_->dbc);
        SQLFreeHandle(SQL_HANDLE_DBC, impl_->dbc);
    }
    if (impl_->env != SQL_NULL_HENV) {
        SQLFreeHandle(SQL_HANDLE_ENV, impl_->env);
    }
}

std::vector<StockRow> StockRepository::loadCurrentStock() {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    check(SQLAllocHandle(SQL_HANDLE_STMT, impl_->dbc, &stmt), SQL_HANDLE_DBC, impl_->dbc,
          "alloc stmt");

    const SQLWCHAR* query = reinterpret_cast<const SQLWCHAR*>(
        L"SELECT ProductId, Sku, ProductName, Unit, ReorderLevel, "
        L"WarehouseId, WarehouseCode, WarehouseName, OnHand, IsLow "
        L"FROM vCurrentStock ORDER BY WarehouseCode, Sku");

    std::vector<StockRow> rows;
    try {
        check(SQLExecDirectW(stmt, const_cast<SQLWCHAR*>(query), SQL_NTS), SQL_HANDLE_STMT, stmt,
              "select vCurrentStock");
        SQLRETURN rc;
        while ((rc = SQLFetch(stmt)) != SQL_NO_DATA) {
            check(rc, SQL_HANDLE_STMT, stmt, "fetch");
            StockRow row;
            row.productId = getInt(stmt, 1);
            row.sku = getString(stmt, 2);
            row.productName = getString(stmt, 3);
            row.unit = getString(stmt, 4);
            row.reorderLevel = getInt(stmt, 5);
            row.warehouseId = getInt(stmt, 6);
            row.warehouseCode = getString(stmt, 7);
            row.warehouseName = getString(stmt, 8);
            row.onHand = getInt(stmt, 9);
            row.isLow = getInt(stmt, 10) != 0;
            rows.push_back(std::move(row));
        }
    } catch (...) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        throw;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return rows;
}

std::vector<MovementRow> StockRepository::loadRecentMovements(int limit) {
    if (limit < 1) limit = 1;
    if (limit > 1000) limit = 1000;

    SQLHSTMT stmt = SQL_NULL_HSTMT;
    check(SQLAllocHandle(SQL_HANDLE_STMT, impl_->dbc, &stmt), SQL_HANDLE_DBC, impl_->dbc,
          "alloc stmt");

    // CreatedAt is stored UTC (SYSUTCDATETIME); convert to Warsaw local time for
    // display. AT TIME ZONE handles DST (CEST/CET) and needs SQL Server 2016+.
    const std::wstring sql =
        L"SELECT TOP (" + std::to_wstring(limit) +
        L") CONVERT(NVARCHAR(19), m.CreatedAt AT TIME ZONE 'UTC' "
        L"AT TIME ZONE 'Central European Standard Time', 120), "
        L"m.MovementType, p.Sku, w.Code, m.Qty "
        L"FROM StockMovements m "
        L"JOIN Products p   ON p.ProductId  = m.ProductId "
        L"JOIN Warehouses w ON w.WarehouseId = m.WarehouseId "
        L"ORDER BY m.MovementId DESC";

    std::vector<MovementRow> rows;
    try {
        check(SQLExecDirectW(stmt, const_cast<SQLWCHAR*>(reinterpret_cast<const SQLWCHAR*>(sql.c_str())),
                             SQL_NTS),
              SQL_HANDLE_STMT, stmt, "select recent movements");
        SQLRETURN rc;
        while ((rc = SQLFetch(stmt)) != SQL_NO_DATA) {
            check(rc, SQL_HANDLE_STMT, stmt, "fetch");
            MovementRow row;
            row.createdAt = getString(stmt, 1);
            row.type = getString(stmt, 2);
            row.sku = getString(stmt, 3);
            row.warehouseCode = getString(stmt, 4);
            row.qty = getInt(stmt, 5);
            rows.push_back(std::move(row));
        }
    } catch (...) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        throw;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return rows;
}

int StockRepository::recordMovement(const Movement& movement) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    check(SQLAllocHandle(SQL_HANDLE_STMT, impl_->dbc, &stmt), SQL_HANDLE_DBC, impl_->dbc,
          "alloc stmt");

    SQLINTEGER productId = movement.productId;
    SQLINTEGER warehouseId = movement.warehouseId;
    SQLINTEGER qty = movement.qty;
    SQLWCHAR type[8] = {};
    std::wstring typeText = (movement.type == MovementType::In) ? L"IN" : L"OUT";
    std::copy(typeText.begin(), typeText.end(), reinterpret_cast<wchar_t*>(type));
    SQLLEN cbType = SQL_NTS;
    SQLLEN cbNote = SQL_NULL_DATA;
    SQLINTEGER newOnHand = 0;
    SQLLEN cbNewOnHand = 0;

    try {
        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &productId, 0, nullptr);
        SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &warehouseId, 0, nullptr);
        SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &qty, 0, nullptr);
        SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 8, 0, type, sizeof(type), &cbType);
        SQLBindParameter(stmt, 5, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 200, 0, nullptr, 0, &cbNote);
        SQLBindParameter(stmt, 6, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &newOnHand, 0, &cbNewOnHand);

        const SQLWCHAR* call =
            reinterpret_cast<const SQLWCHAR*>(L"{ CALL sp_RecordMovement(?, ?, ?, ?, ?, ?) }");
        check(SQLExecDirectW(stmt, const_cast<SQLWCHAR*>(call), SQL_NTS), SQL_HANDLE_STMT, stmt,
              "sp_RecordMovement");
        while (SQLMoreResults(stmt) == SQL_SUCCESS) {
            // drain any results so the OUTPUT parameter is populated
        }
    } catch (...) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        throw;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return newOnHand;
}

}  // namespace warehouse
