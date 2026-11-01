import json

INPUT = "manifest.json"
OUTPUT = "manifest-small.json"

with open(INPUT, "r", encoding="utf-8") as f:
    manifest = json.load(f)

small = {}

for webid, entry in manifest.items():
    small[webid] = {
        "name": entry.get("name"),
        "scaling": entry.get("scaling"),
        "unit": entry.get("unit")
    }

with open(OUTPUT, "w", encoding="utf-8") as f:
    json.dump(small, f, indent=4, ensure_ascii=False)

print(f"Done. Wrote {OUTPUT} with {len(small)} entries.")