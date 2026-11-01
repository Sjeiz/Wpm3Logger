# isg_logger_delta_engine.py
import csv

SENTINEL_VALUES = {"32896", "-200", "-400", "65535"}

# ---------------------------------------------------------
# Type-detectie
# ---------------------------------------------------------

def detect_column_type(values):
    """
    Bepaal het type van een kolom op basis van alle waarden.
    Types: 'sentinel', 'timer', 'value'
    """
    # 1. Altijd sentinel?
    if values and all(v in SENTINEL_VALUES for v in values):
        return "sentinel"

    # 2. Timer-achtig gedrag?
    timer_like = 0
    numeric_pairs = []

    for i in range(1, len(values)):
        old, new = values[i - 1], values[i]
        if old in SENTINEL_VALUES or new in SENTINEL_VALUES:
            continue
        try:
            old_i = int(old)
            new_i = int(new)
            numeric_pairs.append((old_i, new_i))
        except:
            continue

    for old_i, new_i in numeric_pairs:
        if new_i == old_i + 1:
            timer_like += 1
        elif 0 < (new_i - old_i) <= 5:
            timer_like += 1
        elif old_i > 200 and new_i == 0:
            timer_like += 1

    if numeric_pairs and timer_like >= max(3, len(numeric_pairs) // 2):
        return "timer"

    return "value"


def build_column_types(all_rows, analyse_wids, log):
    log("Bepalen van kolomtypes...")
    col_values = {wid: [] for wid in analyse_wids}

    for row in all_rows:
        for wid in analyse_wids:
            col_values[wid].append(row.get(wid, ""))

    col_types = {}
    for wid in analyse_wids:
        t = detect_column_type(col_values[wid])
        col_types[wid] = t
        log(f"  kolom {wid}: type = {t}")

    return col_types


# ---------------------------------------------------------
# Timer-event classifier
# ---------------------------------------------------------

def classify_timer_event(old, new):
    try:
        old_i = int(old)
        new_i = int(new)
    except:
        return None

    if old_i > new_i and new_i == 0:
        return f"reset({old}→{new})"
    if old_i == 0 and new_i > 0:
        return f"start({old}→{new})"
    if abs(new_i - old_i) > 10:
        return f"jump({old}→{new})"
    if old_i > 200 and new_i == 0:
        return f"cycle({old}→{new})"
    if old_i == new_i:
        return f"stop({old}→{new})"

    return None


# ---------------------------------------------------------
# Vergelijkingslogica
# ---------------------------------------------------------

def compare_values(wid, old, new, col_type):
    """
    Return:
    - None → geen delta
    - eventstring → timer/sentinel event
    - new → normale waarde
    """

    # Sentinel-type kolom → nooit events
    if col_type == "sentinel":
        return None

    # Sentinel-sprongen
    if old not in SENTINEL_VALUES and new in SENTINEL_VALUES:
        return f"sentinel({new})"
    if old in SENTINEL_VALUES and new not in SENTINEL_VALUES:
        return f"restore({old}→{new})"

    # Timer-type kolom
    if col_type == "timer":
        event = classify_timer_event(old, new)
        return event

    # Normale kolom
    if old != new:
        return new

    return None


# ---------------------------------------------------------
# Delta-engine
# ---------------------------------------------------------

def generate_delta(all_rows, analyse_wids, log):
    """
    Retourneert:
    - delta_rows: lijst dicts
    - col_types: dict {wid: type}
    """
    if not all_rows:
        log("Geen rijen ontvangen in engine.")
        return [], {}

    # 1) Kolomtypes bepalen
    col_types = build_column_types(all_rows, analyse_wids, log)

    delta_rows = []

    # 2) Baseline
    baseline = all_rows[0]
    baseline_out = {"time": baseline["time"], "event": "baseline"}
    for wid in analyse_wids:
        baseline_out[wid] = baseline.get(wid, "")
    delta_rows.append(baseline_out)

    prev = baseline

    # 3) Delta-vergelijking
    log("Start delta-berekening...")
    for i in range(1, len(all_rows)):
        row = all_rows[i]
        changes = {}

        for wid in analyse_wids:
            old = prev.get(wid, "")
            new = row.get(wid, "")
            col_type = col_types[wid]

            delta_value = compare_values(wid, old, new, col_type)
            if delta_value is not None:
                changes[wid] = delta_value

        if changes:
            out_row = {"time": row["time"], "event": "change"}
            out_row.update(changes)
            delta_rows.append(out_row)
            log(f"  rij {i}: {len(changes)} wijzigingen")

        prev = row

    return delta_rows, col_types