#pragma once
#include <string>
#include <vector>

struct Transaction
{
    std::string type;
    std::string started_date;
    std::string completed_date;
    std::string description;
    double amount;
    double fee;
    std::string currency;
    std::string state;
    double balance;
    std::string category;
};

std::vector<Transaction> parse_tsv(const std::string &filepath);