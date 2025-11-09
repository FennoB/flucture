#!/bin/bash
# Run fast unit tests only (no DB, no external APIs)

cd "$(dirname "$0")/../build" || exit 1

echo "=== Running Flucture Unit Tests ==="
echo "Excludes: [db], [slow], [integration], [ai], [llm], [disabled]"
echo ""

./flucture_tests "~[db]~[slow]~[integration]~[ai]~[llm]~[disabled]"
