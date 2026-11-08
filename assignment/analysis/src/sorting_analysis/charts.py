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


def time_chart(summary: pl.DataFrame) -> alt.Chart:
    base = alt.Chart(summary).encode(x=X_NP)
    line = base.mark_line(point=True).encode(
        y=alt.Y("median_s:Q", title="Median time (s)"), color=COLOR
    )
    band = base.mark_errorbar(ticks=True).encode(
        y=alt.Y("min_s:Q", title="Median time (s)"), y2="max_s:Q", color=COLOR
    )
    return (line + band).properties(width=WIDTH, height=HEIGHT)


def speedup_chart(summary: pl.DataFrame) -> alt.Chart:
    line = (
        alt.Chart(summary)
        .mark_line(point=True)
        .encode(x=X_NP, y=alt.Y("speedup:Q", title="Speedup (T1 / TNP)"), color=COLOR)
    )
    return (ideal_speedup() + line).properties(width=WIDTH, height=HEIGHT)


def breakdown_chart(summary: pl.DataFrame) -> alt.Chart:
    long = summary.unpivot(
        on=["comm_s", "comp_s"], index=["label", "np"], variable_name="part"
    ).with_columns(
        pl.col("part").replace_strict(
            {"comm_s": "Communication", "comp_s": "Computation"}
        )
    )
    return (
        alt.Chart(long)
        .mark_bar()
        .encode(
            x=alt.X("np:O", title="Processes (NP)", axis=alt.Axis(labelAngle=0)),
            y=alt.Y("value:Q", title="Median time (s)"),
            color=alt.Color("part:N", title=None),
            order=alt.Order("part:N", sort="descending"),
        )
        .properties(width=120, height=160)
        .facet(
            facet=alt.Facet("label:N", title=None,
                            sort=[LABELS[a] for a in ALGORITHMS]),
            columns=len(ALGORITHMS),
        )
        .configure_legend(orient="bottom")
    )


def legend_below(chart: alt.Chart) -> alt.Chart:
    return chart.configure_legend(orient="bottom", columns=2, title=None)


def all_charts(summary: pl.DataFrame) -> dict[str, alt.Chart]:
    return {
        "comparison-time": legend_below(time_chart(summary)),
        "comparison-speedup": legend_below(speedup_chart(summary)),
        "time-breakdown": breakdown_chart(summary),
    }
