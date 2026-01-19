import requests
import time
import json
import sys
import select
import msvcrt
from datetime import datetime, timezone
from prettytable import PrettyTable
import os
import csv

URL = "http://servicewelt.iot.cheizoo.lan/isg_api.php"
MANIFEST_FILE = "manifest.json"

# ---------------------------------------------------------
# AANGEPASTE PARAMETERS
# ---------------------------------------------------------
ENABLE_FILTER_NA = ("filter-na" in sys.argv)
ENABLE_FILTER_DONEONLY = ("filter-doneonly" in sys.argv)

GREEN = "\033[92m"
ORANGE = "\033[33m"
RESET = "\033[0m"

with open(MANIFEST_FILE, "r", encoding="utf-8") as f:
    manifest = json.load(f)

current_csv_filename = None
current_csv_file = None
current_csv_writer = None


def key_pressed():
    if sys.platform.startswith("win"):
        return msvcrt.kbhit()
    dr, _, _ = select.select([sys.stdin], [], [], 0)
    return bool(dr)


def get_csv_filename(now_utc):
    minute_block = "00" if now_utc.minute < 30 else "30"
    return f"ISGAPILogger_{now_utc.strftime('%Y-%m-%d_%H')}_{minute_block}.csv"


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
            current_csv_writer.writerow([
                "webid", "modbus", "name", "value", "raw",
                "datatype", "scaling", "empirisch"
            ])


def is_16bit(datatype):
    if not isinstance(datatype, str):
        return False
    dt = datatype.lower()
    return dt in ("uint16", "int16")


# ---------------------------------------------------------
# NIEUWE SCHAALFUNCTIE
# ---------------------------------------------------------
def format_scaled_value(raw_numeric, scaling, unit):
    scaled = raw_numeric * scaling

    if not unit:
        return str(int(round(scaled)))

    return f"{round(scaled, 1)} {unit}".strip()


while True:
    try:
        now_utc = datetime.now(timezone.utc)
        now_utc_str = now_utc.strftime("%Y-%m-%dT%H:%M:%SZ")

        ensure_csv(now_utc)

        current_csv_writer.writerow([now_utc_str])
        current_csv_file.flush()

        data = requests.get(URL, timeout=5).json()

        table = PrettyTable()
        table.field_names = [
            "WebID", "Modbus", "Name", "Value",
            "Raw", "Datatype", "Scaling", "Empirisch"
        ]
        table.align["Value"] = "r"
        table.align["Raw"] = "r"
        table.align["Scaling"] = "r"

        for webid, raw_value in sorted(data.items(), key=lambda x: int(x[0])):
            wid = int(webid)
            meta = manifest.get(webid) or {}

            name = meta.get("name") or ""
            unit = meta.get("unit") or ""
            datatype = meta.get("datatype") or ""
            scaling = meta.get("scaling")

            try:
                scaling = float(scaling) if scaling is not None else 1.0
            except:
                scaling = 1.0

            raw_numeric = raw_value

            # ---------------------------------------------------------
            # BITMASK LOGICA (alleen 16-bit)
            # ---------------------------------------------------------
            if isinstance(raw_numeric, int) and is_16bit(datatype):
                bit15 = (raw_numeric & 0x8000) != 0
                bit12 = (raw_numeric & 0x1000) != 0

                if ENABLE_FILTER_NA:
                    if bit15 and not bit12:
                        continue

                    if bit15 and bit12:
                        value = f"--- {unit}".strip()
                    else:
                        value = format_scaled_value(raw_numeric, scaling, unit)

                else:
                    if bit15 and not bit12:
                        value = f"N/A {unit}".strip()
                    elif bit15 and bit12:
                        value = f"--- {unit}".strip()
                    else:
                        value = format_scaled_value(raw_numeric, scaling, unit)

            else:
                if isinstance(raw_numeric, (int, float)):
                    value = format_scaled_value(raw_numeric, scaling, unit)
                else:
                    value = f"{raw_numeric} {unit}".strip()

            # ---------------------------------------------------------
            # EMPIRISCH (kleur + truncate)
            # ---------------------------------------------------------
            desc = meta.get("description")
            if not isinstance(desc, str):
                desc = ""

            desc_lower = desc.lower()

            if "done" in desc_lower:
                empir_plain = desc
                color = GREEN
            elif "empirisch" in desc_lower:
                empir_plain = desc
                color = ORANGE
            else:
                empir_plain = ""
                color = ""

            if len(empir_plain) > 40:
                empir_plain = empir_plain[:37] + "..."

            empir_display = f"{color}{empir_plain}{RESET}" if empir_plain and color else empir_plain
            empir_csv = empir_plain

            # ---------------------------------------------------------
            # MODBUS ADDRESS LOOKUP
            # ---------------------------------------------------------
            mod = meta.get("modbus") or {}
            addr = mod.get("address")
            span = mod.get("span", 1)

            if isinstance(addr, int):
                if span == 1:
                    modbus_display = str(addr)
                elif span == 2:
                    modbus_display = f"{addr}-{addr+1}"
                else:
                    modbus_display = str(addr)
            else:
                modbus_display = ""

            # ---------------------------------------------------------
            # FILTER-DONEONLY
            # ---------------------------------------------------------
            if ENABLE_FILTER_DONEONLY:
                if empir_plain == "":
                    continue

            # ---------------------------------------------------------
            # TABEL OUTPUT
            # ---------------------------------------------------------
            table.add_row([
                wid,
                modbus_display,
                name,
                value,
                raw_numeric,
                datatype,
                scaling,
                empir_display
            ])

            # ---------------------------------------------------------
            # CSV OUTPUT
            # ---------------------------------------------------------
            current_csv_writer.writerow([
                wid,
                modbus_display,
                name,
                value,
                raw_numeric,
                datatype,
                scaling,
                empir_csv
            ])
            current_csv_file.flush()

        print("\033c", end="")
        print(f"ISG Live Monitor — {now_utc_str}   (filter-na={'ON' if ENABLE_FILTER_NA else 'OFF'}, doneonly={'ON' if ENABLE_FILTER_DONEONLY else 'OFF'})")
        print(table)

    except Exception as e:
        print("Error:", e)

    for _ in range(300):
        if key_pressed():
            print("\nStop requested by key press.")
            if current_csv_file:
                current_csv_file.close()
            sys.exit(0)
        time.sleep(0.1)
