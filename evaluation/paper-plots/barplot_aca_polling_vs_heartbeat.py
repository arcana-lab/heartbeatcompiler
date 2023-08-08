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
chunksizes=[(['',] * (len(chunksizes) - 1)) + [' '], chunksizes]
wasted_polls=list(df.loc[:,'wasted polls'])
detection_rates=list(df.loc[:,'detection rate'])

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#4575b4']
bar_colors=[colors[1],] * (len(chunksizes[1]) - 1)
bar_colors.append(colors[2])
marker_colors=[colors[5],] * (len(chunksizes[1]) - 1)
marker_colors.append(colors[5])

fig = make_subplots(specs=[[{"secondary_y": True}]])

fig.add_trace(go.Bar(
    x=chunksizes,
    y=wasted_polls,
    # text=wasted_polls,
    # textposition='inside',
    # textfont_size=11,
    # texttemplate="%{y:.3s}",
    marker_color=bar_colors,
    name='Wasted Polls'),
    secondary_y=False
)
fig.add_trace(go.Scatter(
    x=chunksizes,
    y=detection_rates[:-1],
    mode='lines',
    # text=detection_rates,
    # textposition='bottom center',
    # textfont_size=9,
    # texttemplate="%{y:.3s}",
    marker_color=colors[5],
    name='Detection Rate'),
    secondary_y=True
)

fig.add_trace(go.Scatter(
    x=chunksizes,
    y=detection_rates,
    mode='markers',
    marker_color=marker_colors,
    marker_size=10,
    # text=detection_rates,
    # textposition='bottom center',
    # textfont_size=11,
    # texttemplate="%{y:.3s}",
    name='Detection Rate'),
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
    showlegend=False,
    yaxis_title="Wasted Polling Count",
    plot_bgcolor="white",
    height=300,
    width=800,
    margin=dict(l=0,r=0,b=50,t=0)
)

fig.add_annotation(text=str(detection_rates[-1]), font_size=15, font_color=colors[5], arrowcolor=colors[5], xref='paper', x=0.885, yref='paper', y=0.94, ayref='y domain', ay=25)
fig.add_annotation(text=str(wasted_polls[-1]), font_size=15, font_color=colors[2], arrowcolor=colors[2], xref='paper', x=0.885, yref='paper', y=0.46, ayref='y domain', ay=-25)
fig.add_annotation(text="Static Chunksizes", font_size=20, showarrow=False, xref='paper', x=0.3, yref='paper', y=-0.2)
fig.add_annotation(text="w/ AC", font_size=18, showarrow=False, xref='paper', x=0.923, yref='paper', y=-0.2)

# fig.add_hline(layer='below', y=detection_rates[-1], x1=0.94, line_dash="5", line_color='red', annotation_text="ACA detection rate", annotation_font_size=12, secondary_y=True)
# fig.add_hline(layer='below', y=wasted_polls[-1], x1=0.94, line_dash="5", annotation_text="ACA wasted polls", annotation_font_size=12, secondary_y=False)

fig.update_xaxes(dtick=1, tickfont_size=15, showline=True, mirror=True, linewidth=1, linecolor='black')
fig.update_yaxes(title="Detection Rate (%)", range=[0,100], showgrid=False, titlefont_size=20, titlefont_color=colors[5], dtick=20, tickfont_size=15, tickfont_color=colors[5], secondary_y=True)
fig.update_yaxes(type='log', range=[2,7], titlefont_size=20, titlefont_color=colors[1], dtick=1, tickfont_size=15, tickfont_color=colors[1], showgrid=True, gridwidth=1, gridcolor='grey', showline=True, mirror=True, linewidth=1, linecolor='black', secondary_y=False)
fig.write_image('plots/plot_aca_polling_vs_heartbeat.pdf', format='pdf')