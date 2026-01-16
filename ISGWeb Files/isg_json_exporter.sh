#!/bin/sh

RAW="/var/volatile/isg_raw.txt"
OUT="/var/volatile/isg.json"

# Run isg_tester with -p (full table)
/var/volatile/firmware/bin/isg_tester -p > "$RAW"

echo "{" > "$OUT"

awk -F';' '
NR<=2 { next }   # skip first TWO header lines

{
    # trim whitespace
    for(i=1;i<=NF;i++){
        gsub(/^[ \t]+|[ \t]+$/, "", $i)
    }

    webid=$1
    value=$2
    wtype=$3
    min=$4
    max=$5
    infonum=$6
    rw=$7
    unit=$8

    printf "  \"%s\": {\n", webid
    printf "    \"value\": %s,\n", value
    printf "    \"wtype\": %s,\n", wtype
    printf "    \"min\": %s,\n", min
    printf "    \"max\": %s,\n", max
    printf "    \"info\": \"%s\",\n", infonum
    printf "    \"access\": \"%s\",\n", rw
    printf "    \"unit\": \"%s\"\n", unit
    printf "  },\n"
}
' "$RAW" >> "$OUT"

# Remove trailing comma
sed -i '$ s/,$//' "$OUT"

echo "}" >> "$OUT"