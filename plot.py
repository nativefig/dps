#!/usr/bin/env python3

import argparse
import csv
import matplotlib.pyplot as plt

def main():
    parser = argparse.ArgumentParser(description="plot script")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("files", nargs="+")

    args = parser.parse_args()

    for filename in args.files:
        rows = []
        with open(filename, "r", newline="") as f:
            data = csv.reader(f, delimiter=",")
            for row in data:
                rows.append(row)

        x = list(map(int, rows[0][1:]))
        for row in rows[1:]:
            y = list(map(float, row[1:]))
            assert len(y) == len(x)
            plt.plot(x, y, label=row[0])

        plt.ylabel("dps")
        plt.title(filename)
        plt.legend()
        plt.show()

main()
