import plotly.graph_objects as go
from plotly.subplots import make_subplots
import plotly.io as pio
import pandas as pd
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_nonzeros_vs_chunksize.csv')
print(df)
step=40
# iters_arrowhead=list(df.loc[:,'i (arrowhead)'])
# iters_arrowhead=iters_arrowhead[:len(iters_arrowhead)//step]
# offset = len(iters_arrowhead)
# iters_arrowhead = [x + (offset/10) for x in iters_arrowhead]
# nonzeros_arrowhead=list(df.loc[:,'nonzeros (arrowhead)'])[::step]
# chunksizes_arrowhead=list(df.loc[:,'chunksize (arrowhead)'])[::step]

# iters_powerlaw=list(df.loc[:,'i (powerlaw)'])
# iters_powerlaw=iters_powerlaw[:len(iters_powerlaw)//step]
# iters_powerlaw = [x + offset + (offset/5) for x in iters_powerlaw]
# nonzeros_powerlaw=list(df.loc[:,'nonzeros (powerlaw)'])[::step]
# chunksizes_powerlaw=list(df.loc[:,'chunksize (powerlaw)'])[::step]

# iters_powerlaw_reverse=list(df.loc[:,'i (powerlaw_reverse)'])
# iters_powerlaw_reverse=iters_powerlaw_reverse[:len(iters_powerlaw_reverse)//step]
# iters_powerlaw_reverse = [x + 2*offset + (offset/5) for x in iters_powerlaw_reverse]
# # nonzeros_powerlaw_reverse=list(df.loc[:,'nonzeros (powerlaw_reverse)'])[::step]
# nonzeros_powerlaw_reverse=[x for x in nonzeros_powerlaw if str(x) != 'nan']
# nonzeros_powerlaw_reverse.reverse()
# chunksizes_powerlaw_reverse=list(df.loc[:,'chunksize (powerlaw_reverse)'])[::step]

# iters_random=list(df.loc[:,'i (random)'])
# iters_random=iters_random[:len(iters_random)//step]
# iters_random = [x + 3*offset + (offset/5) for x in iters_random]
# nonzeros_random=list(df.loc[:,'nonzeros (random)'])[::step]
# chunksizes_random=list(df.loc[:,'chunksize (random)'])[::step]

iters_arrowhead=list(df.loc[:,'i (arrowhead)'])
iters_arrowhead=iters_arrowhead[:len(iters_arrowhead)//step]
offset = len(iters_arrowhead)
iters_arrowhead = [x + (offset/10) - (offset/100) for x in iters_arrowhead]
nonzeros_arrowhead=list(df.loc[:,'nonzeros (arrowhead)'])[::step]
chunksizes_arrowhead=list(df.loc[:,'chunksize (arrowhead)'])[::step]

iters_powerlaw=list(df.loc[:,'i (powerlaw)'])
iters_powerlaw=iters_powerlaw[:len(iters_powerlaw)//step]
iters_powerlaw = [x + (offset/10) + (offset/15) for x in iters_powerlaw]
nonzeros_powerlaw=list(df.loc[:,'nonzeros (powerlaw)'])[::step]
chunksizes_powerlaw=list(df.loc[:,'chunksize (powerlaw)'])[::step]

iters_powerlaw_reverse=list(df.loc[:,'i (powerlaw_reverse)'])
iters_powerlaw_reverse=iters_powerlaw_reverse[:len(iters_powerlaw_reverse)//step]
iters_powerlaw_reverse = [x + offset + (offset/5) + (1809//step) for x in iters_powerlaw_reverse]
# nonzeros_powerlaw_reverse=list(df.loc[:,'nonzeros (powerlaw_reverse)'])[::step]
nonzeros_powerlaw_reverse=[x for x in nonzeros_powerlaw if str(x) != 'nan']
nonzeros_powerlaw_reverse.reverse()
chunksizes_powerlaw_reverse=list(df.loc[:,'chunksize (powerlaw_reverse)'])[::step]

iters_random=list(df.loc[:,'i (random)'])
iters_random=iters_random[:len(iters_random)//step]
iters_random = [x + offset + (offset/5) + (offset/10) for x in iters_random]
nonzeros_random=list(df.loc[:,'nonzeros (random)'])[::step]
chunksizes_random=list(df.loc[:,'chunksize (random)'])[::step]

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#0f233d']
fig = make_subplots(rows=2, cols=1, vertical_spacing = 0.1,
                    specs=[[{"secondary_y": True}], [{"secondary_y": True}]])

fig.add_trace(go.Bar(
    x=iters_arrowhead,
    y=nonzeros_arrowhead,
    marker_color=colors[5],
    name='Nonzeros'),
    secondary_y=False,
    row=1, col=1
)
fig.add_trace(go.Scatter(
    x=iters_arrowhead,
    y=chunksizes_arrowhead,
    mode='lines',
    marker_color=colors[2],
    name='Chunk size'),
    secondary_y=True,
    row=1, col=1
)

fig.add_trace(go.Bar(
    x=iters_powerlaw,
    y=nonzeros_powerlaw,
    marker_color=colors[5],
    name='Nonzeros'),
    secondary_y=False,
    row=2, col=1
)
fig.add_trace(go.Scatter(
    x=iters_powerlaw,
    y=chunksizes_powerlaw,
    mode='lines',
    marker_color=colors[2],
    name='Chunk size'),
    secondary_y=True,
    row=2, col=1
)

fig.add_trace(go.Bar(
    x=iters_powerlaw_reverse,
    y=nonzeros_powerlaw_reverse,
    marker_color=colors[5],
    name='Nonzeros'),
    secondary_y=False,
    row=2, col=1
)
fig.add_trace(go.Scatter(
    x=iters_powerlaw_reverse,
    y=chunksizes_powerlaw_reverse,
    mode='lines',
    marker_color=colors[2],
    name='Chunk size'),
    secondary_y=True,
    row=2, col=1
)

fig.add_trace(go.Bar(
    x=iters_random,
    y=nonzeros_random,
    marker_color=colors[5],
    name='Nonzeros'),
    secondary_y=False,
    row=1, col=1
)
fig.add_trace(go.Scatter(
    x=iters_random,
    y=chunksizes_random,
    mode='lines',
    marker_color=colors[2],
    name='Chunk size'),
    secondary_y=True,
    row=1, col=1
)

# Move legend
fig.update_layout(
    bargap=0,
    showlegend=False,
    plot_bgcolor="white",
    height=300,
    width=800,
    margin=dict(l=0,r=0,b=0,t=0)
)

fig.add_annotation(text='arrowhead', font_size=18, xref='paper', x=0.16, yref='paper', y=1.01, showarrow=False)
fig.add_annotation(text='powerlaw', font_size=18, xref='paper', x=0.165, yref='paper', y=0.417, showarrow=False)
fig.add_annotation(text='powerlaw-reverse', font_size=18, xref='paper', x=0.82, yref='paper', y=0.417, showarrow=False)
fig.add_annotation(text='random', font_size=18, xref='paper', x=0.76, yref='paper', y=1.01, showarrow=False)

# fig.add_hline(layer='below', y=detection_rates[-1], x1=0.94, line_dash="5", line_color='red', annotation_text="ACA detection rate", annotation_font_size=12, secondary_y=True)
# fig.add_hline(layer='below', y=wasted_polls[-1], x1=0.94, line_dash="5", annotation_text="ACA wasted polls", annotation_font_size=12, secondary_y=False)

fig.update_xaxes(showticklabels=False, range=[0, iters_random[-1] + (offset/10)], dtick=(iters_random[-1] + (offset/10))//2 + 1, showgrid=True, gridwidth=1, gridcolor='grey', showline=True, mirror=True, linewidth=1, linecolor='black')
fig.update_yaxes(title="Chunk size", range=[0,401], dtick=100, showgrid=False, titlefont_size=20, titlefont_color=colors[2], tickfont_size=14, tickfont_color=colors[2], secondary_y=True)
fig.update_yaxes(title="Nonzeros", type='log', range=[0, 4.01], dtick=1, titlefont_color='#4575b4', titlefont_size=20, tickfont_size=14, tickfont_color='#4575b4', showgrid=True, gridwidth=1, gridcolor='grey', showline=True, mirror=True, linewidth=1, linecolor='black', secondary_y=False)
fig.write_image('plots/plot_nonzeros_vs_chunksize.pdf', format='pdf')