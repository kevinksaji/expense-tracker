#include "db.h"
#include "classifier.h"
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
            UNIQUE(started_date, description, amount)
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

    sqlite3_finalize(stmt);
}

std::vector<std::pair<std::string, double>> DB::unknown_merchants(const std::string &month) const
{
    const bool use_month = !month.empty();
    const std::string sql = use_month
                                ? "SELECT description, ROUND(SUM(amount) * -1, 2) AS total "
                                  "FROM transactions "
                                  "WHERE category = 'Other' "
                                  "  AND amount < 0 "
                                  "  AND strftime('%Y-%m', started_date) = ? "
                                  "GROUP BY description "
                                  "ORDER BY total DESC, description ASC;"
                                : "SELECT description, ROUND(SUM(amount) * -1, 2) AS total "
                                  "FROM transactions "
                                  "WHERE category = 'Other' "
                                  "  AND amount < 0 "
                                  "GROUP BY description "
                                  "ORDER BY total DESC, description ASC;";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (use_month)
    {
        sqlite3_bind_text(stmt, 1, month.c_str(), -1, SQLITE_TRANSIENT);
    }

    std::vector<std::pair<std::string, double>> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *merchant = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        const double total = sqlite3_column_double(stmt, 1);
        rows.emplace_back(merchant ? merchant : "", total);
    }

    sqlite3_finalize(stmt);
    return rows;
}

int DB::update_category_for_merchant(const std::string &merchant, const std::string &category)
{
    const char *sql = R"(
        UPDATE transactions
        SET category = ?
        WHERE description = ?;
    )";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, merchant.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "Update error: " << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return sqlite3_changes(db_);
}

int DB::reclassify_all(Classifier &classifier)
{
    const char *select_sql = R"(
        SELECT id, type, description
        FROM transactions;
    )";
    const char *update_sql = R"(
        UPDATE transactions
        SET category = ?
        WHERE id = ?;
    )";

    sqlite3_stmt *select_stmt = nullptr;
    sqlite3_stmt *update_stmt = nullptr;
    sqlite3_prepare_v2(db_, select_sql, -1, &select_stmt, nullptr);
    sqlite3_prepare_v2(db_, update_sql, -1, &update_stmt, nullptr);

    int updated = 0;
    while (sqlite3_step(select_stmt) == SQLITE_ROW)
    {
        Transaction t;
        t.type = reinterpret_cast<const char *>(sqlite3_column_text(select_stmt, 1));
        t.description = reinterpret_cast<const char *>(sqlite3_column_text(select_stmt, 2));
        classifier.classify(t);

        sqlite3_reset(update_stmt);
        sqlite3_clear_bindings(update_stmt);
        sqlite3_bind_text(update_stmt, 1, t.category.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(update_stmt, 2, sqlite3_column_int64(select_stmt, 0));

        if (sqlite3_step(update_stmt) != SQLITE_DONE)
        {
            std::cerr << "Reclassify update error: " << sqlite3_errmsg(db_) << "\n";
            continue;
        }
        updated += sqlite3_changes(db_);
    }

    sqlite3_finalize(select_stmt);
    sqlite3_finalize(update_stmt);
    return updated;
}
