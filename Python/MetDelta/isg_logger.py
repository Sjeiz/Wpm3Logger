import requests
import time
import json
import sys
import os
import csv
from datetime import datetime, timezone

# ---------------------------------------------------------
# CONFIGUREERBARE PARAMETERS
# ---------------------------------------------------------
INTERVAL_SECONDS = 30
URL = "http://servicewelt.iot.cheizoo.lan/isg_api.php"
MANIFEST_FILE = "manifest-small.json"

# ---------------------------------------------------------
# Manifest laden
# ---------------------------------------------------------
with open(MANIFEST_FILE, "r", encoding="utf-8") as f:
    manifest = json.load(f)

ALL_WEBIDS = sorted([int(k) for k in manifest.keys()])

# Key–value CSV header
CSV_HEADER = ["time", "key", "value"]

current_csv_filename = None
current_csv_file = None
current_csv_writer = None


def get_csv_filename(now_utc):
    return f"ISGAPILogger_{now_utc.strftime('%Y-%m-%d')}.csv.txt"


def ensure_csv(now_utc):
    global current_csv_filename, current_csv_file, current_csv_writer

    filename = get_csv_filename(now_utc)

    if filename != current_csv_filename:
        if current_csv_file:
            current_csv_file.close()

        current_csv_filename = filename
        current_csv_file = open(filename, "a", newline="", encoding="utf-8")
        current_csv_writer = csv.writer(current_csv_file)

        if os.stat(filename).st_size == 0:
            current_csv_writer.writerow(CSV_HEADER)


def clean_value(v):
    if v == 32768 or v == "32768":
        return ""
    return v


while True:
    try:
        now_utc = datetime.now(timezone.utc)
        time_str = now_utc.strftime("%H:%M:%S")
        full_ts = now_utc.strftime("%Y-%m-%dT%H:%M:%SZ")

        ensure_csv(now_utc)

        data = requests.get(URL, timeout=5).json()

        # Voor elke web-ID één regel
        for wid in ALL_WEBIDS:
            raw = data.get(str(wid), "")
            val = clean_value(raw)
            current_csv_writer.writerow([time_str, f"w{wid}", val])

        current_csv_file.flush()

        print(f"Logged snapshot {full_ts}")

    except Exception as e:
        print("Error:", e)

    time.sleep(INTERVAL_SECONDS)