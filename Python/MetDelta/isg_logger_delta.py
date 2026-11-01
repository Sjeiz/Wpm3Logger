# isg_logger_delta.py
import sys
import json
from datetime import datetime, timedelta

from isg_logger_delta_io import read_raw_files, write_delta_file, log
from isg_logger_delta_engine import generate_delta

MANIFEST = "manifest-small-with-analyse.json"

def parse_date(s):
    return datetime.strptime(s, "%Y-%m-%d").date()

def load_manifest():
    with open(MANIFEST, "r", encoding="utf-8") as f:
        manifest = json.load(f)
    return [f"w{wid}" for wid, meta in manifest.items()
            if isinstance(meta, dict) and meta.get("analyse", True)]

def daterange(start, end):
    for n in range((end - start).days + 1):
        yield start + timedelta(days=n)

def main():
    # CLI
    if len(sys.argv) == 1:
        today = datetime.now().date()
        start = today
        end = today
        log(f"Geen datum opgegeven → gebruik {today}")

    elif len(sys.argv) == 2:
        start = parse_date(sys.argv[1])
        end = start

    elif len(sys.argv) == 3:
        start = parse_date(sys.argv[1])
        end = parse_date(sys.argv[2])

    else:
        print("Gebruik: python isg_logger_delta.py [begindatum] [einddatum]")
        print("Formaat: YYYY-MM-DD")
        return

    analyse_wids = load_manifest()
    log(f"{len(analyse_wids)} kolommen uit manifest geladen.")

    # Inlezen
    rows = read_raw_files(daterange(start, end), analyse_wids)
    if not rows:
        return

    # Engine
    delta_rows, col_types = generate_delta(rows, analyse_wids, log)

    # Output-bestandsnaam
    if start == end:
        out_file = f"delta-{start}.csv.txt"
    else:
        out_file = f"delta-{start}-{end}.csv.txt"

    # Schrijven
    write_delta_file(out_file, delta_rows, col_types, analyse_wids)

    log("Klaar.")

if __name__ == "__main__":
    main()