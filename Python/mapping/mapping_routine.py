import time
import json
import csv
import requests
from datetime import datetime, timezone

ISG_URL = "http://servicewelt.iot.cheizoo.lan/isg_api.php"
OUTPUT_FILE = "analyse.csv"
SAMPLE_INTERVAL = 30   # seconden

# ---------------------------------------------------------
# VELDEN NODIG VOOR ANALYSE (STRICTE MINIMALE SET)
# ---------------------------------------------------------
# Compressorfrequentie:
#   1080, 1079, 1081
#
# Flow / pompen:
#   412 (HK-pomp %)
#   371 (Durchfluss l/min)
#   438 (Wasservolumenstrom WP1)
#
# Delta-T / hydrauliek:
#   445, 455
#
# Vermogen:
#   1120 (elektrisch)
#   60703 (heizleistung)
#
# Interne status:
#   5, 6, 14
#
# Warmwater detectie:
#   60312 (WW boven)
#   70, 68, 53, 60024
#
# WP temperaturen:
#   1029, 1030, 1006, 1004, 1013, 1092, 1119, 60705
#
# Setpoints:
#   31, 16, 26, 33, 336
#
# Veiligheid / beperkingen:
#   463, 1100, 453, 357, 60018
#
# ---------------------------------------------------------
# FILTERLIJST (ALLEEN DEZE VELDEN WORDEN OPGESLAGEN)
# ---------------------------------------------------------

FILTER_FIELDS = {
    "1080","1079","1081",
    "412","371","438",
    "445","455",
    "1120","60703",
    "5","6","14",
    "60312","70","68","53","60024",
    "1029","1030","1006","1004","1013","1092","1119","60705",
    "31","16","26","33","336",
    "463","1100","453","357","60018"
}


def load_isg_data():
    """Haalt JSON op van de ISG API."""
    try:
        r = requests.get(ISG_URL, timeout=5)
        r.raise_for_status()
        return r.json()
    except Exception as e:
        print(f"[ERROR] ISG API niet bereikbaar: {e}")
        return None


def filter_isg_data(isg_data):
    """Filtert ISG JSON zodat alleen relevante velden overblijven."""
    filtered = {}
    for key, value in isg_data.items():
        if str(key) in FILTER_FIELDS:
            filtered[key] = value
    return filtered


def append_to_output(filtered_data):
    """Schrijft gefilterde data naar analyse.csv."""
    file_exists = False
    try:
        with open(OUTPUT_FILE, "r"):
            file_exists = True
    except FileNotFoundError:
        pass

    with open(OUTPUT_FILE, "a", newline="") as f:
        writer = csv.writer(f)

        if not file_exists:
            writer.writerow(["timestamp_utc", "isg_filtered"])

        writer.writerow([
            datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            json.dumps(filtered_data)
        ])


def main():
    print("Eenvoudige ISG logging gestart (30s interval)...")

    while True:
        isg_data = load_isg_data()
        if isg_data:
            filtered = filter_isg_data(isg_data)
            append_to_output(filtered)
            print("[INFO] Regel toegevoegd aan analyse.csv")

        time.sleep(SAMPLE_INTERVAL)


if __name__ == "__main__":
    main()