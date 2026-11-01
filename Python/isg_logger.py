import csv
import sys
import os

SENTINELS = {"32768", "36864", "32896"}

def convert_for_date(date_str):
    input_file = f"ISGAPILogger_{date_str}.csv.txt"
    output_file_am = f"ISGAPILogger_{date_str}_kv_AM.csv.txt"
    output_file_pm = f"ISGAPILogger_{date_str}_kv_PM.csv.txt"

    if not os.path.exists(input_file):
        print(f"Inputbestand niet gevonden: {input_file}")
        sys.exit(1)

    # Eerste pass: bepaal kolommen die ooit een echte waarde hebben
    used_cols = set()
    col_values = {}

    with open(input_file, "r", encoding="utf-8") as fin:
        reader = csv.DictReader(fin)
        for row in reader:
            for col, val in row.items():
                if col == "time":
                    continue
                if col not in col_values:
                    col_values[col] = set()
                col_values[col].add(val)

                if val and val not in SENTINELS:
                    used_cols.add(col)

    # Verwijder kolommen die nooit variëren
    for col, vals in col_values.items():
        if len(vals) <= 1:
            used_cols.discard(col)

    # Tweede pass: schrijven naar AM en PM bestanden
    with open(input_file, "r", encoding="utf-8") as fin, \
         open(output_file_am, "w", newline="", encoding="utf-8") as fout_am, \
         open(output_file_pm, "w", newline="", encoding="utf-8") as fout_pm:

        reader = csv.DictReader(fin)

        writer_am = csv.writer(fout_am)
        writer_pm = csv.writer(fout_pm)

        writer_am.writerow(["time", "key", "value"])
        writer_pm.writerow(["time", "key", "value"])

        prev_am = {}
        prev_pm = {}

        for row in reader:
            timestamp = row["time"]
            hour = int(timestamp.split(" ")[1].split(":")[0])

            # AM = < 12:00, PM = >= 12:00
            if hour < 12:
                writer = writer_am
                prev = prev_am
            else:
                writer = writer_pm
                prev = prev_pm

            for col, val in row.items():
                if col == "time":
                    continue
                if col not in used_cols:
                    continue
                if val in SENTINELS:
                    continue

                prev_val = prev.get(col)
                if prev_val != val:
                    writer.writerow([timestamp, col, val])
                    prev[col] = val

    print("Conversie voltooid (AM/PM splitsing).")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Gebruik: python convert.py yyyy-MM-dd")
        sys.exit(1)

    convert_for_date(sys.argv[1])