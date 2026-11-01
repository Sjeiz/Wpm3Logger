import csv
import sys
import os

def convert_for_date(date_str):
    input_file = f"ISGAPILogger_{date_str}.csv.txt"

    outputs = {
        "S1": f"ISGAPILogger_{date_str}_kv_S1.csv.txt",  # 00–03
        "S2": f"ISGAPILogger_{date_str}_kv_S2.csv.txt",  # 03–06
        "S3": f"ISGAPILogger_{date_str}_kv_S3.csv.txt",  # 06–09
        "S4": f"ISGAPILogger_{date_str}_kv_S4.csv.txt",  # 09–12
        "S5": f"ISGAPILogger_{date_str}_kv_S5.csv.txt",  # 12–15
        "S6": f"ISGAPILogger_{date_str}_kv_S6.csv.txt",  # 15–18
        "S7": f"ISGAPILogger_{date_str}_kv_S7.csv.txt",  # 18–21
        "S8": f"ISGAPILogger_{date_str}_kv_S8.csv.txt",  # 21–24
    }

    if not os.path.exists(input_file):
        print(f"Inputbestand niet gevonden: {input_file}")
        sys.exit(1)

    fout = {}
    writer = {}
    prev = {}
    first = {}

    for key, fname in outputs.items():
        f = open(fname, "w", newline="", encoding="utf-8")
        fout[key] = f
        w = csv.writer(f)
        writer[key] = w
        w.writerow(["time", "key", "value"])
        prev[key] = {}
        first[key] = True

    with open(input_file, "r", encoding="utf-8") as fin:
        reader = csv.DictReader(fin)

        for row in reader:
            ts = row["time"]
            hour = int(ts.split(":")[0])

            if hour < 3:
                bucket = "S1"
            elif hour < 6:
                bucket = "S2"
            elif hour < 9:
                bucket = "S3"
            elif hour < 12:
                bucket = "S4"
            elif hour < 15:
                bucket = "S5"
            elif hour < 18:
                bucket = "S6"
            elif hour < 21:
                bucket = "S7"
            else:
                bucket = "S8"

            w = writer[bucket]
            p = prev[bucket]

            if first[bucket]:
                for col, val in row.items():
                    if col == "time":
                        continue
                    w.writerow([ts, col, val])
                    p[col] = val
                first[bucket] = False
                continue

            for col, val in row.items():
                if col == "time":
                    continue
                if p.get(col) != val:
                    w.writerow([ts, col, val])
                    p[col] = val

    for f in fout.values():
        f.close()

    print("Conversie voltooid (8 delen + snapshots + delta's).")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Gebruik: python convert.py yyyy-MM-dd")
        sys.exit(1)

    convert_for_date(sys.argv[1])