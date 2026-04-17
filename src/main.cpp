#include <iostream>
#include <filesystem>
#include <string>
#include <sqlite3.h>
#include "csv_parser.h"
#include "classifier.h"
#include "db.h"
#include "query.h"

static void print_usage()
{
    std::cerr << "Usage:\n"
              << "  expense-tracker load <path-to-tsv>                  # parse + classify + store\n"
              << "  expense-tracker classify-unknowns [YYYY-MM]         # map + backfill merchants currently in Other\n"
              << "  expense-tracker reclassify                          # reclassify all DB rows from current rules/map\n"
              << "  expense-tracker query total-month [YYYY-MM]         # total spending for month\n"
              << "  expense-tracker query by-category [YYYY-MM]         # month breakdown by category\n"
              << "  expense-tracker query total-day [YYYY-MM-DD]        # total spending for day\n"
              << "  expense-tracker query by-category-day [YYYY-MM-DD]  # daily breakdown by category\n"
              << "  expense-tracker query summary-month [YYYY-MM]       # compact month summary\n"
              << "  expense-tracker query summary-day [YYYY-MM-DD]      # compact daily summary\n"
              << "  expense-tracker query unknown-merchants [YYYY-MM]   # unresolved merchants currently tagged Other\n"
              << "  expense-tracker query range YYYY-MM-DD YYYY-MM-DD   # spending over date range\n";
}

static sqlite3 *open_db(const std::string &db_path)
{
    sqlite3 *db = nullptr;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK)
    {
        std::cerr << "Error opening DB: " << sqlite3_errmsg(db) << "\n";
        return nullptr;
    }
    return db;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_usage();
        return 1;
    }

    const std::string cmd = argv[1];
    const std::string db_path = "data/transactions.db";

    // ── load subcommand ───────────────────────────────────────────────────────
    if (cmd == "load")
    {
        if (argc < 3)
        {
            std::cerr << "Usage: expense-tracker load <path-to-tsv>\n";
            return 1;
        }

        const std::string tsv_path = argv[2];
        std::filesystem::create_directories("data");

        auto transactions = parse_tsv(tsv_path);
        std::cout << "Parsed " << transactions.size() << " transactions.\n";

        Classifier classifier("merchant_map.json");
        for (auto &t : transactions)
            classifier.classify(t);

        DB database(db_path);
        database.create_table();
        for (const auto &t : transactions)
        {
            database.insert(t);
            std::cout << t.started_date << " | " << t.description
                      << " | " << t.amount << " " << t.currency
                      << " | " << t.category << "\n";
        }
        std::cout << "\nDone. " << transactions.size()
                  << " records written to " << db_path << "\n";
        return 0;
    }

    if (cmd == "classify-unknowns")
    {
        const std::string month = (argc >= 3) ? argv[2] : "";
        std::filesystem::create_directories("data");

        DB database(db_path);
        database.create_table();
        Classifier classifier("merchant_map.json");

        const auto unknowns = database.unknown_merchants(month);
        if (unknowns.empty())
        {
            std::cout << "No unknown merchants found.\n";
            return 0;
        }

        int mapped = 0;
        int backfilled = 0;
        std::cout << "Change Summary\n";
        for (const auto &[merchant, total] : unknowns)
        {
            const std::string category = classifier.resolve_unknown_merchant(merchant);
            classifier.set_mapping(merchant, category);
            const int updated = database.update_category_for_merchant(merchant, category);
            std::cout << "- " << merchant << " → " << category
                      << " (SGD " << total << ", backfilled " << updated << " row(s))\n";
            ++mapped;
            backfilled += updated;
        }

        classifier.save_map();
        std::cout << "Total merchants updated: " << mapped << "\n";
        std::cout << "Total rows backfilled: " << backfilled << "\n";
        return 0;
    }

    if (cmd == "reclassify")
    {
        std::filesystem::create_directories("data");
        DB database(db_path);
        database.create_table();
        Classifier classifier("merchant_map.json");
        const int updated = database.reclassify_all(classifier);
        std::cout << "Reclassified " << updated << " row(s).\n";
        return 0;
    }

    // ── query subcommand ──────────────────────────────────────────────────────
    if (cmd == "query")
    {
        if (argc < 3)
        {
            print_usage();
            return 1;
        }

        sqlite3 *db = open_db(db_path);
        if (!db)
            return 1;

        const std::string op = argv[2];

        if (op == "total-month")
        {
            const std::string month = (argc >= 4) ? argv[3] : "";
            query_total_month(db, month);
        }
        else if (op == "by-category")
        {
            const std::string month = (argc >= 4) ? argv[3] : "";
            query_by_category(db, month);
        }
        else if (op == "total-day")
        {
            const std::string date = (argc >= 4) ? argv[3] : "";
            query_total_day(db, date);
        }
        else if (op == "by-category-day")
        {
            const std::string date = (argc >= 4) ? argv[3] : "";
            query_by_category_day(db, date);
        }
        else if (op == "summary-month")
        {
            const std::string month = (argc >= 4) ? argv[3] : "";
            query_summary_month(db, month);
        }
        else if (op == "summary-day")
        {
            const std::string date = (argc >= 4) ? argv[3] : "";
            query_summary_day(db, date);
        }
        else if (op == "unknown-merchants")
        {
            DB database(db_path);
            const std::string month = (argc >= 4) ? argv[3] : "";
            const auto rows = database.unknown_merchants(month);
            if (rows.empty())
            {
                std::cout << "No unknown merchants found.\n";
            }
            else
            {
                std::cout << "Unknown merchants";
                if (!month.empty())
                    std::cout << " for " << month;
                std::cout << ":\n";
                for (const auto &[merchant, total] : rows)
                {
                    std::cout << "  " << merchant << " | SGD " << total << "\n";
                }
            }
        }
        else if (op == "range")
        {
            if (argc < 5)
            {
                std::cerr << "Usage: expense-tracker query range YYYY-MM-DD YYYY-MM-DD\n";
                return 1;
            }
            query_range(db, argv[3], argv[4]);
        }
        else
        {
            std::cerr << "Unknown query operation: " << op << "\n";
            print_usage();
            sqlite3_close(db);
            return 1;
        }

        sqlite3_close(db);
        return 0;
    }

    print_usage();
    return 1;
}
