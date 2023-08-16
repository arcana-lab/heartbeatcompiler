import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_overhead.csv')
print(df)

benchmarks=list(df.loc[:,'benchmarks'])
no_chunking_overheads=list(df.loc[:,'no_chunking_overhead'])
static_chunking_overheads=list(df.loc[:,'static_chunking_overhead'])
aca_overheads=list(df.loc[:,'ac_polling_overhead'])
# rf_overheads=list(df.loc[:,'rf_overhead'])
# rf_kmod_overheads=list(df.loc[:,'rf_kmod_overhead'])
# benchmarks[-1] = '<b>geomean</b>'
no_chunking_overheads_text=no_chunking_overheads.copy()
static_chunking_overheads_text=static_chunking_overheads.copy()
aca_overheads_text=aca_overheads.copy()

print(no_chunking_overheads_text)
print(static_chunking_overheads_text)
print(aca_overheads_text)
# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#4575b4']

# Create the bar chart
fig = go.Figure(data=[
    go.Bar(name='No chunking', x=benchmarks, y=no_chunking_overheads, marker_color=colors[0], text=no_chunking_overheads_text),
    go.Bar(name='Static chunking', x=benchmarks, y=static_chunking_overheads, marker_color=colors[1], text=static_chunking_overheads_text),
    go.Bar(name='Adaptive Chunking', x=benchmarks, y=aca_overheads, marker_color=colors[2], text=aca_overheads_text)
])
fig.update_traces(textposition='outside', texttemplate='%{text:.d}')


# Move legend
fig.update_layout(
    legend=dict(
        font_size=16,
        yanchor="top",
        y=0.88,
        xanchor="left",
        x=0.01,
        bgcolor="#e8e8e8",
        bordercolor="Black",
        borderwidth=1),
    uniformtext_minsize=13,
    uniformtext_mode='show',
    yaxis_title="Overhead (%)",
    plot_bgcolor="white",
    margin=dict(l=0,r=0,b=0,t=0),
    height=350,
    width=800
)

no_chunking_overheads_text=[round(x, 1) for x in no_chunking_overheads_text]

# Add cut-off annotations
for i, o in enumerate(no_chunking_overheads):
    if o > 10:
        fig.add_annotation(xref='paper', x=(0.03 + i/7), axref='x domain', ax=45, yref='paper', y=1, ayref='y', ay=9, text=str(no_chunking_overheads_text[i]), font_size=13)

# fig.add_vline(x=6.5)

# Change the bar mode
fig.update_layout(barmode='group')
fig.update_yaxes(dtick=2, showgrid=True, gridwidth=1, gridcolor='grey', range=[0, 12])
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickangle=-20, ticks="outside", tickfont_size=16)
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickfont_size=14, titlefont_size=20)
fig.write_image('plots/plot_chunking_overhead.pdf', format='pdf')