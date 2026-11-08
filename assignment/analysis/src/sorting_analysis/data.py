from pathlib import Path

import polars as pl

ROOT = Path(__file__).resolve().parents[2]
RESULTS = ROOT / "data" / "results.csv"
SUMMARY = ROOT / "data" / "summary.csv"
FIGURES = ROOT.parent / "report" / "figures"
TABLES = ROOT.parent / "report" / "tables"

ALGORITHMS = ["direct", "bucket", "all-to-all", "odd-even", "shell"]
LABELS = {
    "direct": "Direct",
    "bucket": "Bucket",
    "all-to-all": "All-to-All Bucket",
    "odd-even": "Odd-Even",
    "shell": "Shell",
}


def load_runs(path: Path = RESULTS) -> pl.DataFrame:
    return pl.read_csv(path)


def medians(runs: pl.DataFrame) -> pl.DataFrame:
    return runs.group_by("algorithm", "np").agg(
        median_s=pl.col("time_s").median(),
        min_s=pl.col("time_s").min(),
        max_s=pl.col("time_s").max(),
        comm_s=pl.col("comm_s").median(),
        comp_s=pl.col("comp_s").median(),
    )


def summarize(runs: pl.DataFrame) -> pl.DataFrame:
    stats = medians(runs)
    serial = stats.filter(pl.col("np") == 1).select(
        "algorithm", serial_s="median_s"
    )
    return (
        stats.join(serial, on="algorithm")
        .with_columns(speedup=pl.col("serial_s") / pl.col("median_s"))
        .with_columns(label=pl.col("algorithm").replace_strict(LABELS))
        .sort(pl.col("algorithm").cast(pl.Enum(ALGORITHMS)), "np")
    )
