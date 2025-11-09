#!/bin/bash
# Run all fast tests (excludes slow integration tests)

cd "$(dirname "$0")/../build" || exit 1

echo "=== Running All Fast Tests ==="
echo "Excludes: [slow], [disabled]"
echo ""

./flucture_tests "~[slow]~[disabled]"
