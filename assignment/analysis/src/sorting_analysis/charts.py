import altair as alt
import polars as pl

from .data import ALGORITHMS, LABELS

NP_VALUES = [1, 2, 4, 8]
WIDTH, HEIGHT = 300, 180

X_NP = alt.X(
    "np:Q",
    title="Processes (NP)",
    scale=alt.Scale(domain=[1, 8]),
    axis=alt.Axis(values=NP_VALUES, format="d"),
)
COLOR = alt.Color(
    "label:N", title="Algorithm", sort=[LABELS[a] for a in ALGORITHMS]
)


def ideal_speedup() -> alt.Chart:
    ideal = pl.DataFrame({"np": NP_VALUES, "speedup": NP_VALUES})
    return (
        alt.Chart(ideal)
        .mark_line(strokeDash=[6, 4], color="gray")
        .encode(x=X_NP, y="speedup:Q")
    )


def time_layer(summary: pl.DataFrame, color: alt.Color | dict) -> alt.Chart:
    base = alt.Chart(summary).encode(x=X_NP)
    line = base.mark_line(point=True).encode(
        y=alt.Y("median_s:Q", title="Median time (s)"), color=color
    )
    band = base.mark_errorbar(ticks=True).encode(
        y=alt.Y("min_s:Q", title="Median time (s)"), y2="max_s:Q", color=color
    )
    return (line + band).properties(width=WIDTH, height=HEIGHT)


def speedup_layer(summary: pl.DataFrame, color: alt.Color | dict) -> alt.Chart:
    line = (
        alt.Chart(summary)
        .mark_line(point=True)
        .encode(x=X_NP, y=alt.Y("speedup:Q", title="Speedup (T1 / Tp)"), color=color)
    )
    return (ideal_speedup() + line).properties(width=WIDTH, height=HEIGHT)


def efficiency_layer(summary: pl.DataFrame) -> alt.Chart:
    return (
        alt.Chart(summary)
        .mark_line(point=True)
        .encode(
            x=X_NP,
            y=alt.Y("efficiency:Q", title="Efficiency (speedup / NP)",
                    scale=alt.Scale(domain=[0, 1.05])),
            color=COLOR,
        )
        .properties(width=WIDTH, height=HEIGHT)
    )


def algorithm_chart(summary: pl.DataFrame, algorithm: str) -> alt.Chart:
    subset = summary.filter(pl.col("algorithm") == algorithm)
    color = alt.value("#4c78a8")
    return alt.hconcat(
        time_layer(subset, color).properties(title="Median time"),
        speedup_layer(subset, color).properties(title="Speedup"),
    ).properties(title=LABELS[algorithm])


def legend_below(chart: alt.Chart) -> alt.Chart:
    return chart.configure_legend(orient="bottom", columns=2, title=None)


def comparison_charts(summary: pl.DataFrame) -> dict[str, alt.Chart]:
    charts = {
        "comparison-time": time_layer(summary, COLOR),
        "comparison-speedup": speedup_layer(summary, COLOR),
        "comparison-efficiency": efficiency_layer(summary),
    }
    return {name: legend_below(chart) for name, chart in charts.items()}


def all_charts(summary: pl.DataFrame) -> dict[str, alt.Chart]:
    charts = {f"{a}-scaling": algorithm_chart(summary, a) for a in ALGORITHMS}
    return charts | comparison_charts(summary)
