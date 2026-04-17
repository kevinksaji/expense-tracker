#pragma once
#include "csv_parser.h"
#include <string>
#include <sqlite3.h>

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

private:
    sqlite3 *db_ = nullptr; // raw SQLite handle
};
