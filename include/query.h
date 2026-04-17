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
