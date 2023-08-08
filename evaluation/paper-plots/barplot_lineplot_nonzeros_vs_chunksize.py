import plotly.graph_objects as go
from plotly.subplots import make_subplots
import plotly.io as pio
import pandas as pd
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_nonzeros_vs_chunksize.csv')
print(df)

iters_arrowhead=list(df.loc[:,'i (arrowhead)'])
iters_arrowhead = [x + 1000 for x in iters_arrowhead]
nonzeros_arrowhead=list(df.loc[:,'nonzeros (arrowhead)'])
chunksizes_arrowhead=list(df.loc[:,'chunksize (arrowhead)'])

iters_powerlaw=list(df.loc[:,'i (powerlaw)'])
iters_powerlaw = [x + len(iters_arrowhead) + 2000 for x in iters_powerlaw]
nonzeros_powerlaw=list(df.loc[:,'nonzeros (powerlaw)'])
chunksizes_powerlaw=list(df.loc[:,'chunksize (powerlaw)'])

iters_powerlaw_reverse=list(df.loc[:,'i (powerlaw_reverse)'])
iters_powerlaw_reverse = [x + len(iters_arrowhead) + len(iters_powerlaw) + 2000 for x in iters_powerlaw_reverse]
nonzeros_powerlaw_reverse=list(df.loc[:,'nonzeros (powerlaw_reverse)'])
chunksizes_powerlaw_reverse=list(df.loc[:,'chunksize (powerlaw_reverse)'])

iters_random=list(df.loc[:,'i (random)'])
iters_random = [x + len(iters_arrowhead) + 2*len(iters_powerlaw) + 2000 for x in iters_random]
nonzeros_random=list(df.loc[:,'nonzeros (random)'])
chunksizes_random=list(df.loc[:,'chunksize (random)'])

print(len(iters_arrowhead), len(iters_powerlaw), len(iters_powerlaw_reverse), len(iters_random))

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#0f233d']

fig = make_subplots(specs=[[{"secondary_y": True}]])

fig.add_trace(go.Bar(
    x=iters_arrowhead,
    y=nonzeros_arrowhead,
    marker_color=colors[5],
    name='Nonzeros'),
    secondary_y=False
)
fig.add_trace(go.Scatter(
    x=iters_arrowhead,
    y=chunksizes_arrowhead,
    mode='lines',
    marker_color=colors[2],
    name='Chunksize'),
    secondary_y=True
)

fig.add_trace(go.Bar(
    x=iters_powerlaw,
    y=nonzeros_powerlaw,
    marker_color=colors[5],
    name='Nonzeros'),
    secondary_y=False
)
fig.add_trace(go.Scatter(
    x=iters_powerlaw,
    y=chunksizes_powerlaw,
    mode='lines',
    marker_color=colors[2],
    name='Chunksize'),
    secondary_y=True
)

fig.add_trace(go.Bar(
    x=iters_powerlaw_reverse,
    y=nonzeros_powerlaw_reverse,
    marker_color=colors[5],
    name='Nonzeros'),
    secondary_y=False
)
fig.add_trace(go.Scatter(
    x=iters_powerlaw_reverse,
    y=chunksizes_powerlaw_reverse,
    mode='lines',
    marker_color=colors[2],
    name='Chunksize'),
    secondary_y=True
)

fig.add_trace(go.Bar(
    x=iters_random,
    y=nonzeros_random,
    marker_color=colors[5],
    name='Nonzeros'),
    secondary_y=False
)
fig.add_trace(go.Scatter(
    x=iters_random,
    y=chunksizes_random,
    mode='lines',
    marker_color=colors[2],
    name='Chunksize'),
    secondary_y=True
)

# Move legend
fig.update_layout(
    # legend=dict(
    #     yanchor="top",
    #     y=0.99,
    #     xanchor="center",
    #     x=0.5,
    #     bgcolor="#e8e8e8",
    #     bordercolor="Black",
    #     borderwidth=1),
    bargap=0,
    showlegend=False,
    yaxis_title="Nonzeros",
    plot_bgcolor="white",
    height=200,
    width=1000,
    margin=dict(l=0,r=0,b=0,t=0)
)

fig.add_annotation(text='arrowhead', font_size=16, xref='paper', x=0.085, yref='paper', y=0.95, showarrow=False)
fig.add_annotation(text='powerlaw', font_size=16, xref='paper', x=0.31, yref='paper', y=0.95, showarrow=False)
fig.add_annotation(text='powerlaw_reverse', font_size=16, xref='paper', x=0.57, yref='paper', y=0.95, showarrow=False)
fig.add_annotation(text='random', font_size=16, xref='paper', x=0.84, yref='paper', y=0.95, showarrow=False)

# fig.add_hline(layer='below', y=detection_rates[-1], x1=0.94, line_dash="5", line_color='red', annotation_text="ACA detection rate", annotation_font_size=12, secondary_y=True)
# fig.add_hline(layer='below', y=wasted_polls[-1], x1=0.94, line_dash="5", annotation_text="ACA wasted polls", annotation_font_size=12, secondary_y=False)

fig.update_xaxes(showticklabels=False, range=[0, 43000], showline=True, mirror=True, linewidth=1, linecolor='black')
fig.update_yaxes(title="Chunksize", range=[0,400], dtick=100, showgrid=False, titlefont_size=20, titlefont_color=colors[2], tickfont_size=14, tickfont_color=colors[2], secondary_y=True)
fig.update_yaxes(type='log', range=[0, 4], dtick=1, titlefont_color=colors[5], titlefont_size=20, tickfont_size=14, tickfont_color=colors[5], showgrid=True, gridwidth=1, gridcolor='grey', showline=True, mirror=True, linewidth=1, linecolor='black', secondary_y=False)
fig.write_image('plots/plot_nonzeros_vs_chunksize.pdf', format='pdf')