#include "classifier.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

// Constructor — reads merchant_map.json from disk into merchant_map hash map
Classifier::Classifier(const std::string &map_filepath) : map_filepath(map_filepath)
{
    std::ifstream f(map_filepath);
    if (f.is_open())
    {
        json j;
        f >> j; // parses entire JSON file into j in one shot
        for (auto &[key, value] : j.items())
        { // structured binding — unpacks key/value pairs
            merchant_map[key] = value.get<std::string>();
        }
    }
}

// Lowercases + strips leading/trailing whitespace for consistent map lookups
std::string Classifier::normalise(const std::string &name)
{
    std::string result = name;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    auto start = result.find_first_not_of(" \t");
    auto end = result.find_last_not_of(" \t");
    if (start == std::string::npos)
        return "";
    return result.substr(start, end - start + 1);
}

void Classifier::classify(Transaction &t)
{
    // Self-transfer to your own bank (POSB) — savings, excluded from expenditure totals
    if (t.type == "Transfer" && normalise(t.description) == "kevin k saji")
    {
        t.category = "Savings Transfer";
        return;
    }

    // Other transfers and refunds — no merchant lookup needed
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

    std::string key = normalise(t.description);

    // Exact match — O(1) hash map lookup
    if (merchant_map.count(key))
    {
        t.category = merchant_map[key];
        return;
    }

    // Partial match — sort by key length descending so longer (more specific) keys win
    // e.g. "fairprice pasir ris west plaza" beats "pasir ris west plaza"
    std::vector<std::pair<std::string, std::string>> sorted(merchant_map.begin(), merchant_map.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b)
              { return a.first.size() > b.first.size(); });
    for (const auto &[k, v] : sorted)
    {
        if (key.find(k) != std::string::npos)
        {
            t.category = v;
            return;
        }
    }

    // LLM fallback — only for truly unknown merchants
    // Result is persisted to merchant_map.json so we never ask again
    std::string category = llm_classify(t.description);
    t.category = category;
    merchant_map[key] = category;
    save_map();
}

// Placeholder — will be replaced with real HTTP call to Claude/OpenAI later
std::string Classifier::llm_classify(const std::string &merchant)
{
    std::cerr << "[LLM] Unknown merchant: " << merchant << " → tagged as Other\n";
    return "Other";
}

// Serialises merchant_map back to disk as formatted JSON
void Classifier::save_map()
{
    json j = merchant_map;
    std::ofstream f(map_filepath);
    f << j.dump(2); // dump(2) = pretty print with 2-space indentation
}