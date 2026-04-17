# Expense Tracker

A C++ expense tracking tool that parses Revolut TSV exports, classifies transactions by category, and stores them in a local SQLite database. Designed to be queried by an AI agent (OpenClaw).

---

## For the Agent — How to Use This Tool

### Binary Location

```
/Users/kevinsaji/Desktop/expense-tracker/expense-tracker
```

Always run commands from the project root:

```
/Users/kevinsaji/Desktop/expense-tracker/
```

---

### Loading New Transactions

When a new TSV file is added to the `revolut/` folder, load it into the database:

```bash
./expense-tracker load <path-to-tsv>
```

Example:

```bash
./expense-tracker load revolut/april_2026/april_2026_01_17.tsv
```

- Safe to run multiple times — duplicates are automatically ignored
- Always load every TSV in a folder when processing a new month to ensure no gaps

---

### Querying the Database

**Total spending for the current month:**

```bash
./expense-tracker query total-month
```

**Total spending for a specific month:**

```bash
./expense-tracker query total-month YYYY-MM
```

Example: `./expense-tracker query total-month 2026-04`

**Spending breakdown by category (current month):**

```bash
./expense-tracker query by-category
```

**Spending breakdown by category (specific month):**

```bash
./expense-tracker query by-category YYYY-MM
```

**Total spending for a specific day:**

```bash
./expense-tracker query total-day YYYY-MM-DD
```

**Daily spending breakdown by category:**

```bash
./expense-tracker query by-category-day YYYY-MM-DD
```

**Compact monthly summary:**

```bash
./expense-tracker query summary-month YYYY-MM
```

**Compact daily summary:**

```bash
./expense-tracker query summary-day YYYY-MM-DD
```

**Total spending over a custom date range:**

```bash
./expense-tracker query range YYYY-MM-DD YYYY-MM-DD
```

Example: `./expense-tracker query range 2026-04-01 2026-04-17`

> All query results automatically exclude Savings Transfers, Transfers, and Refunds from totals.

---

### Checking the Raw Database

The SQLite database lives at:

```
data/transactions.db
```

Schema:

```sql
CREATE TABLE transactions (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    type           TEXT,
    started_date   TEXT,   -- format: YYYY-MM-DD HH:MM:SS
    completed_date TEXT,
    description    TEXT,   -- merchant name
    amount         REAL,   -- negative = spending, positive = refund/credit
    fee            REAL,
    currency       TEXT,
    state          TEXT,   -- COMPLETED or PENDING
    balance        REAL,
    category       TEXT
);
```

---

### When a New TSV is Added — Agent Checklist

1. Confirm the file follows the naming convention (see below)
2. Run `./expense-tracker load <path-to-new-file>`
3. Confirm output ends with `Done. N records written to data/transactions.db`
4. Optionally run `./expense-tracker query by-category` to confirm new data is visible

---

## TSV File Naming Convention

### Format

```
month_year_startday_endday.tsv
```

### Folder Structure

Each calendar month gets its own subfolder inside `revolut/`:

```
revolut/
├── april_2026/
│   ├── april_2026_01_17.tsv     ← April 1–17
│   └── april_2026_18_30.tsv     ← April 18–30
├── may_2026/
│   └── may_2026_01_31.tsv       ← Full month
└── ...
```

### Rules

- Folder name: `month_year` (e.g. `april_2026`)
- File name: `month_year_startday_endday.tsv` (e.g. `april_2026_01_17.tsv`)
- Day values are zero-padded (`01`, `09`, `17`, `30`)
- Overlapping date ranges are safe — duplicates are ignored on load

---

## Spending Categories

| Category         | Examples                                                |
| ---------------- | ------------------------------------------------------- |
| Food             | Toast Box, Burger King, Hawkers' Street                 |
| Groceries        | FairPrice, NTUC                                         |
| Transport        | SMRT, Grab                                              |
| Shopping         | Shopee                                                  |
| Bills            | circles.life                                            |
| Subscriptions    | GitHub, Netflix, Spotify                                |
| Travel           | Singapore Airlines, Changi Airport                      |
| Sports           | Decathlon                                               |
| Leisure          | Singapore Zoo                                           |
| Refund           | Card refunds (excluded from totals)                     |
| Transfer         | Third-party transfers (excluded from totals)            |
| Savings Transfer | Transfers to KEVIN K SAJI / POSB (excluded from totals) |
| Other            | Unknown merchants (LLM fallback)                        |

To add a new merchant mapping, edit `merchant_map.json` at the project root.

---

## Project Structure

```
expense-tracker/
├── include/              # Header files (class/function declarations)
│   ├── csv_parser.h
│   ├── classifier.h
│   ├── db.h
│   ├── query.h
│   └── json.hpp          # nlohmann/json single-header library
├── src/                  # Implementation files
│   ├── main.cpp          # Entry point + CLI subcommands
│   ├── csv_parser.cpp    # Parses Revolut TSV into Transaction structs
│   ├── classifier.cpp    # Rule-based classifier + LLM fallback
│   ├── db.cpp            # SQLite insert/table wrapper
│   └── query.cpp         # Templated query operations
├── revolut/              # Raw TSV exports from Revolut (organised by month)
├── data/
│   └── transactions.db   # SQLite database (persistent, local only)
├── merchant_map.json     # Merchant → category lookup table (grows over time)
└── Makefile
```

---

## Building from Source

```bash
# Install dependencies (macOS)
brew install sqlite3

# Download nlohmann/json single header
curl -o include/json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp

# Build
make
```
