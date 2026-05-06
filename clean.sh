#!/bin/bash
# clean.sh
# Deletes all generated allocation files from basic/output/

OUTPUT_DIR="basic/output"

if [ ! -d "$OUTPUT_DIR" ]; then
    echo "Nothing to clean — '$OUTPUT_DIR' does not exist."
    exit 0
fi

COUNT=$(ls "$OUTPUT_DIR"/allocation*.txt 2>/dev/null | wc -l)

if [ "$COUNT" -eq 0 ]; then
    echo "Nothing to clean — no allocation files in '$OUTPUT_DIR'."
    exit 0
fi

echo "Deleting $COUNT allocation file(s) from $OUTPUT_DIR/ ..."
rm -f "$OUTPUT_DIR"/allocation*.txt
echo "Done."
