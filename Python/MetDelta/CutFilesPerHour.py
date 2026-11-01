import sys
import os
import csv
from datetime import datetime

def main():
    if len(sys.argv) != 2:
        print("Gebruik: python split_csv.py YYYY-MM-DD")
        sys.exit(1)

    date = sys.argv[1]  # datum van cmdline
    filename = f"ISGAPILogger_{date}.csv.txt"

    if not os.path.exists(filename):
        print(f"Bestand niet gevonden: {filename}")
        sys.exit(1)

    outdir = "Logging"
    os.makedirs(outdir, exist_ok=True)

    print(f"Bestand laden: {filename}")

    # Detecteer delimiter automatisch
    with open(filename, "r", encoding="utf-8") as f:
        sample = f.readline()
        delimiter = ";" if sample.count(";") > sample.count(",") else ","

    print(f"Delimiter gedetecteerd: '{delimiter}'")

    with open(filename, "r", encoding="utf-8") as f:
        reader = csv.reader(f, delimiter=delimiter)

        header = next(reader)
        print(f"Kolommen: {header}")

        ts_count = 0
        file_count = 0

        outfile = None
        writer = None
        line_count = 0
        part = 1

        def open_new_file():
            nonlocal outfile, writer, part, line_count, file_count
            if outfile:
                outfile.close()
            outname = os.path.join(outdir, f"{date}_part{part:03d}.csv")
            outfile = open(outname, "w", encoding="utf-8", newline="")
            writer = csv.writer(outfile, delimiter=delimiter)
            writer.writerow(header)
            print(f"Nieuw bestand → {outname}")
            part += 1
            file_count += 1
            line_count = 0

        # Start eerste bestand
        open_new_file()

        for row in reader:
            if not row:
                continue

            time_str = row[0].strip().replace("\ufeff", "")

            # Parse alleen HH:mm:ss
            try:
                datetime.strptime(time_str, "%H:%M:%S")
            except ValueError:
                continue  # geen geldige tijd → overslaan

            ts_count += 1

            # Nieuwe file na 100 regels
            if line_count >= 50:
                open_new_file()

            writer.writerow(row)
            line_count += 1

        if outfile:
            outfile.close()

    print(f"Totaal regels met geldige tijd: {ts_count}")
    print(f"Aantal outputbestanden: {file_count}")
    print("Klaar.")

if __name__ == "__main__":
    main()