import altair as alt

from .charts import all_charts
from .data import FIGURES, SUMMARY, TABLES, load_runs, summarize
from .tables import all_tables


def save_chart(chart: alt.Chart, name: str) -> None:
    chart.save(FIGURES / f"{name}.pdf")
    print(f"wrote figures/{name}.pdf")


def save_table(table: str, name: str) -> None:
    (TABLES / f"{name}.tex").write_text(table)
    print(f"wrote tables/{name}.tex")


def main() -> None:
    summary = summarize(load_runs())
    summary.write_csv(SUMMARY, float_precision=4)
    FIGURES.mkdir(parents=True, exist_ok=True)
    TABLES.mkdir(parents=True, exist_ok=True)
    for name, chart in all_charts(summary).items():
        save_chart(chart, name)
    for name, table in all_tables(summary).items():
        save_table(table, name)
