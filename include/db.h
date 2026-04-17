#pragma once
#include "csv_parser.h"
#include <string>
#include <sqlite3.h>
#include <vector>
#include <utility>

class DB
{
public:
    // Opens (or creates) the SQLite .db file at db_path
    explicit DB(const std::string &db_path);
    ~DB();

    // Creates transactions table if it doesn't already exist
    void create_table();

    // Inserts a single classified transaction
    void insert(const Transaction &t);

    // Find merchants currently classified as Other, optionally filtered by month (YYYY-MM)
    std::vector<std::pair<std::string, double>> unknown_merchants(const std::string &month = "") const;

    // Update category for all rows whose description matches the merchant string exactly
    int update_category_for_merchant(const std::string &merchant, const std::string &category);

    // Reclassify all rows using the provided classifier
    int reclassify_all(class Classifier &classifier);

private:
    sqlite3 *db_ = nullptr; // raw SQLite handle
};
