import plotly.graph_objects as go
from plotly.subplots import make_subplots
import plotly.io as pio
import pandas as pd
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_aca_polling_vs_heartbeat.csv')
print(df)

chunksizes=list(df.loc[:,'log_2(chunksize)'])
chunksizes[-1]=''
print(chunksizes)
chunksizes=[(['Static Chunksizes (log_2)',] * (len(chunksizes) - 1)) + ['w/ ACA'], chunksizes]
wasted_polls=list(df.loc[:,'wasted polls'])
detection_rates=list(df.loc[:,'detection rate'])

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#4575b4']

fig = make_subplots(specs=[[{"secondary_y": True}]])

fig.add_trace(go.Bar(
    x=chunksizes,
    y=wasted_polls,
    text=wasted_polls,
    textposition='outside',
    textfont_size=10,
    texttemplate="%{y:.3s}",
    marker_color=colors[0],
    name='Wasted Polls'),
    secondary_y=False
)
fig.add_trace(go.Scatter(
    x=chunksizes,
    y=detection_rates,
    mode='lines+markers+text',
    text=detection_rates,
    textposition='bottom center',
    textfont_size=9,
    texttemplate="%{y:.3s}",
    name='Detection Rate'),
    secondary_y=True
)

# Move legend
fig.update_layout(
    legend=dict(
        yanchor="top",
        y=0.99,
        xanchor="center",
        x=0.5,
        bgcolor="#e8e8e8",
        bordercolor="Black",
        borderwidth=1),
    yaxis_title="Wasted Polling Count",
    plot_bgcolor="white"
)

fig.update_xaxes(dtick=1)
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black')
fig.update_yaxes(title="Detection Rate (%)", showgrid=True, gridwidth=1, gridcolor='pink', secondary_y=True)
fig.update_yaxes(type='log', dtick=1, showgrid=True, gridwidth=1, gridcolor='grey', showline=True, mirror=True, linewidth=1, linecolor='black', secondary_y=False)
fig.write_image('plots/plot_aca_polling_vs_heartbeat.pdf', format='pdf')