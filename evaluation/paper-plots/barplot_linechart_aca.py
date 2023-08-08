import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_aca_demo.csv')
print(df)

chunksizes=list(df.loc[:10,'log_2(chunksize)'])
low_latency_times=list(df.loc[:10,'low latency input'])
high_latency_times=list(df.loc[:10,'high latency input'])

print(chunksizes)
print(low_latency_times)
print(high_latency_times)

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#4575b4']

fig = go.Figure()
fig.add_trace(go.Scatter(x=chunksizes, y=high_latency_times,
                    mode='lines',
                    name='High Latency Input'))
fig.add_trace(go.Scatter(x=chunksizes, y=low_latency_times,
                    mode='lines',
                    name='Low Latency Input'))

# Move legend
fig.update_layout(
    legend=dict(
        font_size=18,
        yanchor="top",
        y=0.99,
        xanchor="left",
        x=0.01,
        bgcolor="#e8e8e8",
        bordercolor="Black",
        borderwidth=1),
    xaxis_title="log2(chunksize)",
    yaxis_title="Program Runtime (s)",
    plot_bgcolor="white",
    height=350,
    width=800,
    margin=dict(l=0,r=0,b=20,t=0)
)

fig.update_yaxes(dtick=5, showgrid=True, gridwidth=1, gridcolor='grey')
fig.update_xaxes(dtick=2, showgrid=True, gridwidth=1, gridcolor='grey')
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black', titlefont_size=20, tickfont_size=14)
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black', titlefont_size=20, tickfont_size=14)
fig.write_image('plots/plot_aca_demo.pdf', format='pdf')

df2 = pd.read_csv('data_aca_vs_static.csv')
print(df2)

chunksizes2=[str(x) for x in df2.loc[:,'chunksize']]
chunksizes2=[(['',] * (len(chunksizes2) - 1)) + ['       '], chunksizes2]
speedups=list(df2.loc[:,'speedup'])

bar_colors = [colors[1],] * len(speedups)
bar_colors[-1] = colors[2]
print(bar_colors)

fig2 = go.Figure(data=[go.Bar(
    y=chunksizes2,
    x=speedups,
    text=speedups,
    textposition='outside',
    textfont_size=11,
    marker_color=bar_colors,
    orientation='h'
)])

# Move legend
fig2.update_layout(
    legend=dict(
        yanchor="top",
        y=0.99,
        xanchor="left",
        x=0.01),
    uniformtext_minsize=14,
    uniformtext_mode='show',
    xaxis_title="Program Speedup",
    xaxis_tickvals=[1, 5, 10, 15, 20, 25, 30],
    plot_bgcolor="white",
    height=350,
    width=800,
    margin=dict(l=65,r=0,b=0,t=0)
)

fig2.add_annotation(text='Static Chunksizes', font_size=20, textangle=-90, showarrow=False, xref='paper', x=-0.09, yref='paper', y=0.55)
fig2.add_annotation(text='w/ AC', font_size=20, showarrow=False, xref='paper', x=-0.088, yref='paper', y=0.005)
fig2['layout']['yaxis']['autorange'] = "reversed"
fig2.update_xaxes(dtick=10, showgrid=True, gridwidth=1, gridcolor='grey', titlefont_size=20, tickfont_size=14)
fig2.update_yaxes(showline=True, mirror=True, linewidth=1, tickfont_size=14, linecolor='black')
fig2.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black')

fig2.write_image('plots/plot_aca_vs_static.pdf', format='pdf')