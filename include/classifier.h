#pragma once
#include "csv_parser.h"
#include <string>
#include <unordered_map>

class Classifier
{
public:
    // Loads merchant_map.json from disk into memory
    explicit Classifier(const std::string &map_filepath);

    // Looks up merchant, assigns t.category using map + heuristics (no persistence side effects)
    void classify(Transaction &t);

    // Deterministically classify a merchant string without mutating state
    std::string classify_merchant(const std::string &merchant) const;

    // Best-effort resolution for unknown merchants; ignores stale `Other` map entries
    std::string resolve_unknown_merchant(const std::string &merchant) const;

    // Add/update a merchant mapping in memory using normalised key
    void set_mapping(const std::string &merchant, const std::string &category);

    // Persists merchant_map back to disk as JSON
    void save_map();

    // Expose known mappings for tooling/debugging
    const std::unordered_map<std::string, std::string> &mappings() const { return merchant_map; }

private:
    std::string map_filepath; // path to merchant_map.json

    // In-memory hash map: "toast box" -> "Food"
    std::unordered_map<std::string, std::string> merchant_map;

    // Lowercases + trims merchant name for consistent map lookups
    static std::string normalise(const std::string &name);

    // Deterministic heuristic fallback for unknown merchants
    std::string heuristic_classify(const std::string &merchant) const;

    // Best-effort fallback hook for unresolved merchants; always returns a concrete category
    std::string llm_classify(const std::string &merchant) const;
};
