#include "classifier.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

Classifier::Classifier(const std::string &map_filepath) : map_filepath(map_filepath)
{
    std::ifstream f(map_filepath);
    if (f.is_open())
    {
        json j;
        f >> j;
        for (auto &[key, value] : j.items())
        {
            merchant_map[key] = value.get<std::string>();
        }
    }
}

std::string Classifier::normalise(const std::string &name)
{
    std::string result = name;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c)
                   { return static_cast<char>(std::tolower(c)); });

    auto start = result.find_first_not_of(" \t");
    auto end = result.find_last_not_of(" \t");
    if (start == std::string::npos)
        return "";

    result = result.substr(start, end - start + 1);

    std::string compact;
    compact.reserve(result.size());
    bool last_was_space = false;
    for (unsigned char c : result)
    {
        if (std::isspace(c))
        {
            if (!last_was_space)
            {
                compact.push_back(' ');
                last_was_space = true;
            }
        }
        else
        {
            compact.push_back(static_cast<char>(c));
            last_was_space = false;
        }
    }

    return compact;
}

std::string Classifier::heuristic_classify(const std::string &merchant) const
{
    const std::string key = normalise(merchant);

    const std::vector<std::pair<std::string, std::string>> rules = {
        {"smrt", "Transport"},
        {"mrt", "Transport"},
        {"bus", "Transport"},
        {"grab", "Food"},
        {"shopee", "Shopping"},
        {"lazada", "Shopping"},
        {"amazon", "Shopping"},
        {"fairprice", "Groceries"},
        {"ntuc", "Groceries"},
        {"cold storage", "Groceries"},
        {"giant", "Groceries"},
        {"sheng siong", "Groceries"},
        {"toast box", "Food"},
        {"ya kun", "Food"},
        {"stuff'd", "Food"},
        {"wok hey", "Food"},
        {"burger king", "Food"},
        {"mcdonald", "Food"},
        {"starbucks", "Food"},
        {"hawker", "Food"},
        {"warburg vending", "Food"},
        {"circles.life", "Bills"},
        {"singtel", "Bills"},
        {"starhub", "Bills"},
        {"netflix", "Subscriptions"},
        {"spotify", "Subscriptions"},
        {"github", "Subscriptions"},
        {"apple.com/bill", "Subscriptions"},
        {"decathlon", "Sports"},
        {"airlines", "Travel"},
        {"airport", "Travel"}};

    for (const auto &[needle, category] : rules)
    {
        if (key.find(needle) != std::string::npos)
        {
            return category;
        }
    }

    return "";
}

std::string Classifier::llm_classify(const std::string &merchant) const
{
    std::cerr << "[LLM] Unknown merchant: " << merchant << " → tagged as Other\n";
    return "Other";
}

std::string Classifier::classify_merchant(const std::string &merchant) const
{
    const std::string key = normalise(merchant);

    if (merchant_map.count(key))
    {
        return merchant_map.at(key);
    }

    std::vector<std::pair<std::string, std::string>> sorted(merchant_map.begin(), merchant_map.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b)
              { return a.first.size() > b.first.size(); });
    for (const auto &[k, v] : sorted)
    {
        if (key.find(k) != std::string::npos)
        {
            return v;
        }
    }

    const std::string heuristic = heuristic_classify(merchant);
    if (!heuristic.empty())
    {
        return heuristic;
    }

    return llm_classify(merchant);
}

void Classifier::classify(Transaction &t)
{
    if (t.type == "Transfer" && normalise(t.description) == "kevin k saji")
    {
        t.category = "Savings Transfer";
        return;
    }

    if (t.type == "Transfer")
    {
        t.category = "Transfer";
        return;
    }

    if (t.type == "Card Refund")
    {
        t.category = "Refund";
        return;
    }

    t.category = classify_merchant(t.description);
}

void Classifier::set_mapping(const std::string &merchant, const std::string &category)
{
    const std::string key = normalise(merchant);
    if (!key.empty())
    {
        merchant_map[key] = category;
    }
}

void Classifier::save_map()
{
    json j = json::object();
    std::vector<std::pair<std::string, std::string>> entries(merchant_map.begin(), merchant_map.end());
    std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b)
              { return a.first < b.first; });

    for (const auto &[key, value] : entries)
    {
        j[key] = value;
    }

    std::ofstream f(map_filepath);
    f << j.dump(2) << "\n";
}
