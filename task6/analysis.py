import pandas as pd


def main():
    results_dynamic = pd.read_csv("results_dynamic.csv")
    results_static = pd.read_csv("results_static.csv")

    for run in ["baseline", "ssa", "roundtrip"]:
        print(f"dynamic {run}: {results_dynamic[results_dynamic.run == run].result.sum()}")

    for run in ["baseline", "ssa", "roundtrip"]:
        print(f"static {run}: {results_static[results_static.run == run].result.sum()}")


if __name__ == "__main__":
    main()
