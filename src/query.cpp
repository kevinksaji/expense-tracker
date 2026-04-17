#include "query.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

// Returns current month as "YYYY-MM"
static std::string current_month()
{
    std::time_t t = std::time(nullptr);
    std::tm *tm = std::localtime(&t);
    std::ostringstream oss;
    oss << (1900 + tm->tm_year) << "-"
        << std::setw(2) << std::setfill('0') << (1 + tm->tm_mon);
    return oss.str();
}

void query_total_month(sqlite3 *db, const std::string &month)
{
    const std::string m = month.empty() ? current_month() : month;

    const std::string sql =
        "SELECT ROUND(SUM(amount) * -1, 2) "
        "FROM transactions "
        "WHERE strftime('%Y-%m', started_date) = ? "
        "  AND category NOT IN ('Savings Transfer', 'Transfer', 'Refund') "
        "  AND amount < 0;";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, m.c_str(), -1, SQLITE_TRANSIENT);

    std::cout << "Total expenditure for " << m << ":\n";
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        double total = sqlite3_column_double(stmt, 0);
        std::cout << "  SGD " << std::fixed << std::setprecision(2) << total << "\n";
    }
    sqlite3_finalize(stmt);
}

void query_by_category(sqlite3 *db, const std::string &month)
{
    const std::string m = month.empty() ? current_month() : month;

    const std::string sql =
        "SELECT category, ROUND(SUM(amount) * -1, 2) AS total "
        "FROM transactions "
        "WHERE strftime('%Y-%m', started_date) = ? "
        "  AND category NOT IN ('Savings Transfer', 'Transfer', 'Refund') "
        "  AND amount < 0 "
        "GROUP BY category "
        "ORDER BY total DESC;";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, m.c_str(), -1, SQLITE_TRANSIENT);

    std::cout << "Expenditure by category for " << m << ":\n";
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *cat = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        double total = sqlite3_column_double(stmt, 1);
        std::cout << "  " << std::left << std::setw(20) << cat
                  << "SGD " << std::fixed << std::setprecision(2) << total << "\n";
    }
    sqlite3_finalize(stmt);
}

void query_range(sqlite3 *db, const std::string &from, const std::string &to)
{
    const std::string sql =
        "SELECT ROUND(SUM(amount) * -1, 2) "
        "FROM transactions "
        "WHERE date(started_date) BETWEEN ? AND ? "
        "  AND category NOT IN ('Savings Transfer', 'Transfer', 'Refund') "
        "  AND amount < 0;";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, from.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, to.c_str(), -1, SQLITE_TRANSIENT);

    std::cout << "Total expenditure from " << from << " to " << to << ":\n";
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        double total = sqlite3_column_double(stmt, 0);
        std::cout << "  SGD " << std::fixed << std::setprecision(2) << total << "\n";
    }
    sqlite3_finalize(stmt);
}
