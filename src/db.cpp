#include "db.h"
#include <iostream>

DB::DB(const std::string &db_path)
{
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK)
        std::cerr << "Error opening DB: " << sqlite3_errmsg(db_) << "\n";
}

DB::~DB()
{
    sqlite3_close(db_);
}

void DB::create_table()
{
    const char *sql = R"(
        CREATE TABLE IF NOT EXISTS transactions (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            type           TEXT,
            started_date   TEXT,
            completed_date TEXT,
            description    TEXT,
            amount         REAL,
            fee            REAL,
            currency       TEXT,
            state          TEXT,
            balance        REAL,
            category       TEXT,
            UNIQUE(started_date, description, amount)  -- dedup key: prevents double-loading same TSV
        );
    )";

    char *err = nullptr;
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &err) != SQLITE_OK)
    {
        std::cerr << "Create table error: " << err << "\n";
        sqlite3_free(err);
    }
}

void DB::insert(const Transaction &t)
{
    const char *sql = R"(
        INSERT OR IGNORE INTO transactions
            (type, started_date, completed_date, description,
             amount, fee, currency, state, balance, category)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    // Bind each field to its ? placeholder (1-indexed)
    sqlite3_bind_text(stmt, 1, t.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, t.started_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, t.completed_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, t.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, t.amount);
    sqlite3_bind_double(stmt, 6, t.fee);
    sqlite3_bind_text(stmt, 7, t.currency.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, t.state.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 9, t.balance);
    sqlite3_bind_text(stmt, 10, t.category.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
        std::cerr << "Insert error: " << sqlite3_errmsg(db_) << "\n";

    sqlite3_finalize(stmt); // always free the statement
}
