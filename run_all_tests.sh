#!/bin/bash
# run_all_tests.sh
# Runs all basic algorithm test cases and puts results in basic/output/

BINARY="./myProg"
RANGES_DIR="basic/ranges"
REGISTERS_DIR="basic/registers"
OUTPUT_DIR="basic/output"

# Check binary exists
if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary '$BINARY' not found. Compile first with:"
    echo "  g++ main.cpp -o myProg"
    exit 1
fi

# Create output dir if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Map: ranges file → registers file (from the README table)
declare -A REG_MAP
REG_MAP["ranges1.txt"]="registers2.txt"
REG_MAP["ranges2.txt"]="registers2.txt"
REG_MAP["ranges3.txt"]="registers2.txt"
REG_MAP["ranges4.txt"]="registers1.txt"
REG_MAP["ranges5.txt"]="registers1.txt"
REG_MAP["ranges6.txt"]="registers3.txt"

echo "======================================"
echo " Running basic algorithm test cases"
echo "======================================"

PASS=0
FAIL=0

for i in 1 2 3 4 5 6; do
    RANGES="$RANGES_DIR/ranges${i}.txt"
    REGS_FILE="${REG_MAP["ranges${i}.txt"]}"
    REGISTERS="$REGISTERS_DIR/$REGS_FILE"
    OUTPUT="$OUTPUT_DIR/allocation${i}.txt"

    # Check input files exist
    if [ ! -f "$RANGES" ]; then
        echo "  [SKIP] ranges${i}.txt not found"
        continue
    fi
    if [ ! -f "$REGISTERS" ]; then
        echo "  [SKIP] $REGS_FILE not found"
        continue
    fi

    # Run
    "$BINARY" -b "$RANGES" "$REGISTERS" "$OUTPUT" 2>/tmp/stderr_${i}.txt
    EXIT_CODE=$?

    if [ $EXIT_CODE -eq 0 ]; then
        # Count registers used from output
        REGS_USED=$(grep "^registers:" "$OUTPUT" | awk '{print $2}')
        echo "  [OK]   ranges${i}.txt + $REGS_FILE → allocation${i}.txt  (registers used: $REGS_USED)"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] ranges${i}.txt + $REGS_FILE → exit code $EXIT_CODE"
        cat /tmp/stderr_${i}.txt
        FAIL=$((FAIL + 1))
    fi

    # Print any warnings from stderr
    if [ -s /tmp/stderr_${i}.txt ]; then
        echo "         WARNING: $(cat /tmp/stderr_${i}.txt)"
    fi
done

echo "======================================"
echo " Results: $PASS passed, $FAIL failed"
echo " Output files in: $OUTPUT_DIR/"
echo "======================================"
