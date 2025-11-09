#!/bin/bash
# Run database tests with test database

cd "$(dirname "$0")/../build" || exit 1

# Check if .env exists
if [ ! -f "../.env" ]; then
    echo "ERROR: .env file not found in project root"
    echo "Please create .env with database credentials"
    exit 1
fi

# Source .env and export DB variables
source ../.env

# Use test database by default
export DB_HOST=${DB_HOST:-85.214.240.143}
export DB_PORT=${DB_PORT:-5432}
export DB_NAME=${DB_NAME:-vergabefix_test}
export DB_USER=${DB_USER:-vergabefix_test}
export DB_PASS=${DB_PASS:-ctPoPXqdsCXW2A2l}

echo "=== Running Flucture Database Tests ==="
echo "DB: $DB_USER@$DB_HOST:$DB_PORT/$DB_NAME"
echo "Excludes: [slow]"
echo ""

./flucture_tests "[db]~[slow]"
