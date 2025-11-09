#!/bin/bash
# Run complete test suite (requires DB + OpenAI API)

cd "$(dirname "$0")/../build" || exit 1

# Check for .env
if [ ! -f "../.env" ]; then
    echo "ERROR: .env file not found"
    echo "Need OPENAI_API_KEY and DB credentials"
    exit 1
fi

source ../.env

# Use test DB if not specified
export DB_HOST=${DB_HOST:-85.214.240.143}
export DB_PORT=${DB_PORT:-5432}
export DB_NAME=${DB_NAME:-vergabefix_test}
export DB_USER=${DB_USER:-vergabefix_test}
export DB_PASS=${DB_PASS:-ctPoPXqdsCXW2A2l}

# Check for API key
if [ -z "$OPENAI_API_KEY" ]; then
    echo "ERROR: OPENAI_API_KEY not set in .env"
    exit 1
fi

echo "=== Running Complete Flucture Test Suite ==="
echo "DB: $DB_USER@$DB_HOST:$DB_PORT/$DB_NAME"
echo "API: OpenAI (key present)"
echo "Excludes: [disabled]"
echo ""

./flucture_tests "~[disabled]"
