import polars as pl

from .data import ALGORITHMS

ALGORITHM_COLUMNS = {
    "np": ("NP", "{:d}"),
    "median_s": ("Median (s)", "{:.3f}"),
    "speedup": ("Speedup", "{:.2f}"),
    "efficiency": ("Efficiency", "{:.2f}"),
}


def tabular(header: list[str], rows: list[list[str]]) -> str:
    lines = [
        "\\begin{tabular}{" + "r" * len(header) + "}",
        "  \\toprule",
        "  " + " & ".join(header) + " \\\\",
        "  \\midrule",
        *("  " + " & ".join(row) + " \\\\" for row in rows),
        "  \\bottomrule",
        "\\end{tabular}",
    ]
    return "\n".join(lines) + "\n"


def algorithm_table(summary: pl.DataFrame, algorithm: str) -> str:
    subset = summary.filter(pl.col("algorithm") == algorithm)
    header = [title for title, _ in ALGORITHM_COLUMNS.values()]
    rows = [
        [fmt.format(row[col]) for col, (_, fmt) in ALGORITHM_COLUMNS.items()]
        for row in subset.iter_rows(named=True)
    ]
    return tabular(header, rows)


def all_tables(summary: pl.DataFrame) -> dict[str, str]:
    return {f"{a}-scaling": algorithm_table(summary, a) for a in ALGORITHMS}
