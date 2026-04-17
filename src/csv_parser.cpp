#include "csv_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::vector<Transaction> parse_tsv(const std::string &filepath)
{
    std::vector<Transaction> transactions;

    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "Error: Cannot open file " << filepath << "\n";
        return transactions;
    }

    std::string line;

    // this function reads characters from file until it hits newline, stores everything read to line variable, advances cursor to next line
    std::getline(file, line); // skip header row

    while (std::getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::stringstream ss(line);
        std::string token;
        Transaction t;

        auto next = [&]()
        {
            std::getline(ss, token, '\t');
            return token;
        };

        t.type = next();
        next(); // skip Product column
        t.started_date = next();
        t.completed_date = next();
        t.description = next();
        try
        {
            t.amount = std::stod(next());
        }
        catch (...)
        {
            t.amount = 0.0;
        }
        try
        {
            t.fee = std::stod(next());
        }
        catch (...)
        {
            t.fee = 0.0;
        }
        t.currency = next();
        t.state = next();
        try
        {
            t.balance = std::stod(next());
        }
        catch (...)
        {
            t.balance = 0.0;
        }

        transactions.push_back(t);
    }

    return transactions;
}