#!/bin/bash
# Basic smoke tests for the idea tracker CLI.
# Run with: make test  (or: bash tests/test.sh)

set -e
BIN="./ideas"
PASS=0
FAIL=0

check() {
    local description="$1"
    local expected_substring="$2"
    local actual_output="$3"

    if echo "$actual_output" | grep -qF "$expected_substring"; then
        echo "PASS: $description"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $description"
        echo "  expected to find: $expected_substring"
        echo "  got: $actual_output"
        FAIL=$((FAIL + 1))
    fi
}

# Use a temporary HOME so tests never touch the real ~/.ideas database.
export HOME=$(mktemp -d)

OUT=$($BIN add "test idea one" --priority=high --tags=testing)
check "add creates an idea with id 1" "id 1" "$OUT"

OUT=$($BIN add test idea two without quotes)
check "add works without quotes" "test idea two without quotes" "$OUT"

OUT=$($BIN list --all)
check "list shows added ideas" "test idea one" "$OUT"

OUT=$($BIN status 1 in_progress)
check "status updates correctly" "updated to 'in_progress'" "$OUT"

OUT=$($BIN status 1 not_a_real_status 2>&1 || true)
check "status rejects invalid value" "invalid status" "$OUT"

OUT=$($BIN edit 2 "edited text")
check "edit updates text" "Idea 2 updated" "$OUT"

OUT=$($BIN search testing)
check "search finds by tag" "test idea one" "$OUT"

OUT=$($BIN delete 2 --force)
check "delete removes an idea" "Idea 2 deleted" "$OUT"

OUT=$($BIN stats)
check "stats shows total count" "Total ideas: 1" "$OUT"

$BIN export csv /tmp/ideas_test_export.csv > /dev/null
check "export creates a csv file" "id,created_at,text,status,priority,tags" "$(cat /tmp/ideas_test_export.csv)"

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
