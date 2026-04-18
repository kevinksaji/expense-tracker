#include "query.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <vector>
#include <utility>

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

// Returns current date as "YYYY-MM-DD"
static std::string current_date()
{
    std::time_t t = std::time(nullptr);
    std::tm *tm = std::localtime(&t);
    std::ostringstream oss;
    oss << (1900 + tm->tm_year) << "-"
        << std::setw(2) << std::setfill('0') << (1 + tm->tm_mon) << "-"
        << std::setw(2) << std::setfill('0') << tm->tm_mday;
    return oss.str();
}

static double fetch_total_for_month(sqlite3 *db, const std::string &month)
{
    const std::string sql =
        "SELECT COALESCE(ROUND(SUM(amount) * -1, 2), 0.0) "
        "FROM transactions "
        "WHERE strftime('%Y-%m', started_date) = ? "
        "  AND category NOT IN ('Savings Transfer', 'Transfer', 'Refund') "
        "  AND amount < 0;";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, month.c_str(), -1, SQLITE_TRANSIENT);

    double total = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        total = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return total;
}

static double fetch_total_for_day(sqlite3 *db, const std::string &date)
{
    const std::string sql =
        "SELECT COALESCE(ROUND(SUM(amount) * -1, 2), 0.0) "
        "FROM transactions "
        "WHERE date(started_date) = ? "
        "  AND category NOT IN ('Savings Transfer', 'Transfer', 'Refund') "
        "  AND amount < 0;";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_TRANSIENT);

    double total = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        total = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return total;
}

static std::vector<std::pair<std::string, double>> fetch_categories_for_month(sqlite3 *db, const std::string &month)
{
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
    sqlite3_bind_text(stmt, 1, month.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<std::pair<std::string, double>> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *cat = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        double total = sqlite3_column_double(stmt, 1);
        rows.emplace_back(cat ? cat : "Unknown", total);
    }
    sqlite3_finalize(stmt);
    return rows;
}

static std::vector<std::pair<std::string, double>> fetch_categories_for_day(sqlite3 *db, const std::string &date)
{
    const std::string sql =
        "SELECT category, ROUND(SUM(amount) * -1, 2) AS total "
        "FROM transactions "
        "WHERE date(started_date) = ? "
        "  AND category NOT IN ('Savings Transfer', 'Transfer', 'Refund') "
        "  AND amount < 0 "
        "GROUP BY category "
        "ORDER BY total DESC;";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<std::pair<std::string, double>> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *cat = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        double total = sqlite3_column_double(stmt, 1);
        rows.emplace_back(cat ? cat : "Unknown", total);
    }
    sqlite3_finalize(stmt);
    return rows;
}

static std::string normalise_sql_timestamp_expr(const std::string &column)
{
    return "CASE "
           "WHEN " + column + " IS NULL OR " + column + " = '' THEN '' "
           "WHEN length(" + column + ") = 18 THEN substr(" + column + ", 1, 11) || '0' || substr(" + column + ", 12) "
           "ELSE " + column + " END";
}

static double fetch_latest_balance_for_month(sqlite3 *db, const std::string &month)
{
    const std::string completed_expr = normalise_sql_timestamp_expr("completed_date");
    const std::string started_expr = normalise_sql_timestamp_expr("started_date");

    const std::string sql =
        "SELECT balance "
        "FROM transactions "
        "WHERE substr(started_date, 1, 7) = ? "
        "  AND balance IS NOT NULL "
        "ORDER BY "
        "  CASE WHEN completed_date IS NULL OR completed_date = '' THEN 1 ELSE 0 END ASC, "
        "  COALESCE(NULLIF(" + completed_expr + ", ''), " + started_expr + ") DESC, "
        "  " + started_expr + " DESC "
        "LIMIT 1;";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, month.c_str(), -1, SQLITE_TRANSIENT);

    double balance = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        balance = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return balance;
}

void query_total_month(sqlite3 *db, const std::string &month)
{
    const std::string m = month.empty() ? current_month() : month;
    const double total = fetch_total_for_month(db, m);

    std::cout << "Total expenditure for " << m << ":\n";
    std::cout << "  SGD " << std::fixed << std::setprecision(2) << total << "\n";
}

void query_by_category(sqlite3 *db, const std::string &month)
{
    const std::string m = month.empty() ? current_month() : month;
    const auto rows = fetch_categories_for_month(db, m);

    std::cout << "Expenditure by category for " << m << ":\n";
    for (const auto &[cat, total] : rows)
    {
        std::cout << "  " << std::left << std::setw(20) << cat
                  << "SGD " << std::fixed << std::setprecision(2) << total << "\n";
    }
}

void query_range(sqlite3 *db, const std::string &from, const std::string &to)
{
    const std::string sql =
        "SELECT COALESCE(ROUND(SUM(amount) * -1, 2), 0.0) "
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

void query_total_day(sqlite3 *db, const std::string &date)
{
    const std::string d = date.empty() ? current_date() : date;
    const double total = fetch_total_for_day(db, d);

    std::cout << "Total expenditure for " << d << ":\n";
    std::cout << "  SGD " << std::fixed << std::setprecision(2) << total << "\n";
}

void query_by_category_day(sqlite3 *db, const std::string &date)
{
    const std::string d = date.empty() ? current_date() : date;
    const auto rows = fetch_categories_for_day(db, d);

    std::cout << "Expenditure by category for " << d << ":\n";
    for (const auto &[cat, total] : rows)
    {
        std::cout << "  " << std::left << std::setw(20) << cat
                  << "SGD " << std::fixed << std::setprecision(2) << total << "\n";
    }
}

void query_summary_month(sqlite3 *db, const std::string &month)
{
    const std::string m = month.empty() ? current_month() : month;
    const double total = fetch_total_for_month(db, m);
    const auto rows = fetch_categories_for_month(db, m);

    std::cout << m << " — Expenditure\n";
    std::cout << "Total: SGD " << std::fixed << std::setprecision(2) << total << "\n";
    std::cout << "Breakdown:\n";
    if (rows.empty())
    {
        std::cout << "- No spending recorded\n";
        return;
    }
    for (const auto &[cat, value] : rows)
    {
        std::cout << "- " << cat << ": SGD " << std::fixed << std::setprecision(2) << value << "\n";
    }
}

void query_summary_day(sqlite3 *db, const std::string &date)
{
    const std::string d = date.empty() ? current_date() : date;
    const double total = fetch_total_for_day(db, d);
    const auto rows = fetch_categories_for_day(db, d);

    std::cout << d << " — Daily Expenditure\n";
    std::cout << "Total: SGD " << std::fixed << std::setprecision(2) << total << "\n";
    std::cout << "Breakdown:\n";
    if (rows.empty())
    {
        std::cout << "- No spending recorded\n";
        return;
    }
    for (const auto &[cat, value] : rows)
    {
        std::cout << "- " << cat << ": SGD " << std::fixed << std::setprecision(2) << value << "\n";
    }
}

void query_balance(sqlite3 *db, const std::string &month)
{
    const std::string m = month.empty() ? current_month() : month;
    const double balance = fetch_latest_balance_for_month(db, m);

    std::cout << "Latest reliable balance for " << m << ":\n";
    std::cout << "  SGD " << std::fixed << std::setprecision(2) << balance << "\n";
}
