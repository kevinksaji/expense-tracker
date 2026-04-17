#pragma once
#include <string>
#include <sqlite3.h>

// All queries exclude "Savings Transfer" from expenditure totals.
// Amounts from Revolut are negative for spending — results are returned as positive values.

// Total spending for a given month. month format: "YYYY-MM" (defaults to current month if empty)
void query_total_month(sqlite3 *db, const std::string &month = "");

// Spending breakdown by category for a given month (defaults to current month if empty)
void query_by_category(sqlite3 *db, const std::string &month = "");

// Total spending between two dates inclusive. date format: "YYYY-MM-DD"
void query_range(sqlite3 *db, const std::string &from, const std::string &to);

// Total spending for a given date. date format: "YYYY-MM-DD" (defaults to today if empty)
void query_total_day(sqlite3 *db, const std::string &date = "");

// Spending breakdown by category for a given date (defaults to today if empty)
void query_by_category_day(sqlite3 *db, const std::string &date = "");

// Compact summary output for a month (YYYY-MM) or current month if empty
void query_summary_month(sqlite3 *db, const std::string &month = "");

// Compact summary output for a date (YYYY-MM-DD) or today if empty
void query_summary_day(sqlite3 *db, const std::string &date = "");
