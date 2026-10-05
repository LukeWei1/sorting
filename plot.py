#plot.py -- reads results.csv and writes runtime_plot.png (log-log)
import csv
from collections import defaultdict
import matplotlib.pyplot as plt
 
data = defaultdict(list)
with open("results.csv") as f:
    for row in csv.DictReader(f):
        data[row["algorithm"]].append((int(row["size"]), float(row["median_seconds"])))
 
styles = {"Insertion": "o-", "Selection": "s-", "Merge": "^-", "Quick": "d-", "std::sort": "x--"}
plt.figure(figsize=(8, 5.5))
for name, pts in data.items():
    pts.sort()
    plt.loglog([p[0] for p in pts], [p[1] for p in pts], styles.get(name, "o-"), label=name)
 
plt.xlabel("List size n")
plt.ylabel("Median runtime (seconds)")
plt.title("Sorting runtime vs. list size (random integers, log-log scale)")
plt.grid(True, which="both", alpha=0.3)
plt.legend()
plt.tight_layout()
plt.savefig("runtime_plot.png", dpi=150)
print("saved runtime_plot.png")
 
