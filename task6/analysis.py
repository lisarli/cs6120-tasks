import pandas as pd


def main():

    results_static = pd.read_csv("results_static.csv")

    for run in ["baseline", "ssa", "roundtrip"]:
        print(f"{run}: {results_static[results_static.run == run].result.sum()}")


if __name__ == "__main__":
    main()
