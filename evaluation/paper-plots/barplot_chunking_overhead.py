import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_chunking_overhead.csv')
print(df)

benchmarks=list(df.loc[:,'benchmarks'])
no_chunking_overheads=list(df.loc[:,'no_chunking_overhead'])
static_chunking_overheads=list(df.loc[:,'static_chunking_overhead'])
aca_overheads=list(df.loc[:,'aca_overhead'])
# rf_overheads=list(df.loc[:,'rf_overhead'])
# rf_kmod_overheads=list(df.loc[:,'rf_kmod_overhead'])

# Some colourblind-aware colour palette that I found online
colors = ['#377eb8', '#ff7f00', '#4daf4a',
           '#f781bf', '#a65628', '#984ea3',
           '#999999', '#e41a1c', '#dede00']

# Create the bar chart
fig = go.Figure(data=[
    go.Bar(name='No Chunking', x=benchmarks, y=no_chunking_overheads, marker_color=colors[0], text=no_chunking_overheads),
    go.Bar(name='Static Chunking', x=benchmarks, y=static_chunking_overheads, marker_color=colors[1], text=static_chunking_overheads),
    go.Bar(name='Adaptive Chunking', x=benchmarks, y=aca_overheads, marker_color=colors[2], text=aca_overheads)
])
fig.update_traces(textposition='outside', textfont_size=9)


# Move legend
fig.update_layout(
    legend=dict(
        yanchor="top",
        y=0.99,
        xanchor="right",
        x=0.99),
    yaxis_title="Overhead (%)"
)

# Add cut-off annotations
for i, o in enumerate(no_chunking_overheads):
    if o > 10:
        fig.add_annotation(xref='paper', x=(0.03 + i/8), yref='paper', y=1.01, text=str(o), font_size=9)


# Change the bar mode
fig.update_layout(barmode='group')
fig.update_yaxes(dtick=2, showgrid=True, gridwidth=1, gridcolor='grey', range=[0, 10])
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickangle=-45, ticks="outside")
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black')
fig.write_image('plots/plot_chunking_overhead.pdf', format='pdf')