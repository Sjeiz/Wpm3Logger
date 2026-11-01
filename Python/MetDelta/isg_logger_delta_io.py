# isg_logger_delta_io.py
import csv
import os

def log(msg):
    print(f"[delta] {msg}")

def read_raw_files(daterange, analyse_wids):
    all_rows = []

    for d in daterange:
        filename = f"ISGAPILogger_{d}.csv.txt"
        if not os.path.exists(filename):
            log(f"[SKIP] {filename} bestaat niet")
            continue

        log(f"[OK]   Verwerk {filename}")
        with open(filename, "r", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            for row in reader:
                row["__date"] = str(d)
                all_rows.append(row.copy())

    if not all_rows:
        log("[WARN] Geen data gevonden")
        return []

    # sorteren op datum + tijd
    all_rows.sort(key=lambda r: (r["__date"], r["time"]))
    log(f"Totaal {len(all_rows)} rijen ingelezen.")
    return all_rows


def write_delta_file(out_file, delta_rows, col_types, analyse_wids):
    log(f"Schrijven delta naar {out_file}")

    # Alle kolommen verzamelen
    all_keys = set()
    for d in delta_rows:
        all_keys.update(d.keys())

    all_keys = ["time", "event"] + sorted(k for k in all_keys if k not in ("time", "event"))

    with open(out_file, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=all_keys)
        writer.writeheader()

        # type-rij
        type_row = {k: "" for k in all_keys}
        type_row["time"] = "type"
        type_row["event"] = "type"
        for wid in analyse_wids:
            type_row[wid] = col_types.get(wid, "")
        writer.writerow(type_row)

        # delta-rijen
        for d in delta_rows:
            writer.writerow(d)

    log("Delta-bestand klaar.")