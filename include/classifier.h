#pragma once
#include "csv_parser.h"
#include <string>
#include <unordered_map>

class Classifier
{
public:
    // Loads merchant_map.json from disk into memory
    Classifier(const std::string &map_filepath);

    // Looks up merchant, assigns t.category — LLM fallback if unknown
    void classify(Transaction &t);

    // Persists merchant_map back to disk as JSON
    void save_map();

private:
    std::string map_filepath; // path to merchant_map.json

    // In-memory hash map: "toast box" -> "Food"
    std::unordered_map<std::string, std::string> merchant_map;

    // Lowercases + trims merchant name for consistent map lookups
    std::string normalise(const std::string &name);

    // HTTP call to LLM for unknown merchants — returns category string
    std::string llm_classify(const std::string &merchant);
};