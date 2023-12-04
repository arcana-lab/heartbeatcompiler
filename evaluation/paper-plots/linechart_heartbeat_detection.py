import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd
import math
from plotly.subplots import make_subplots

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_heartbeat_detection.csv')
print(df)

polling_counts=[int(x) for x in df.columns[1:]]
mandelbrot=list(df.loc[0][1:])
spmv_arrowhead=list(df.loc[1][1:])
spmv_powerlaw=list(df.loc[2][1:])
spmv_random=list(df.loc[3][1:])
plus_reduce_array=list(df.loc[4][1:])
srad=list(df.loc[5][1:])
kmeans=list(df.loc[6][1:])
print(polling_counts)
print(mandelbrot)
print(spmv_arrowhead)
print(spmv_powerlaw)
print(spmv_random)
print(plus_reduce_array)
print(srad)
print(kmeans)

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
colors = ['#fee090','#fc8d59','#d73027','#af8dc3','#91bfdb','#4575b4', '#e0f3f8']
symbols = ['circle', 'square', 'diamond', 'cross', 'triangle-up', 'triangle-down', 'star']

fig = make_subplots(rows=2, cols=1, vertical_spacing = 0.05,
                    row_width=[0.1, 0.9])
for i in [1, 2]:
    show = True if i == 1 else False
    fig.add_trace(go.Scatter(x=polling_counts, y=mandelbrot,
                        mode='lines+markers',
                        name='mandelbrot',
                        line_color=colors[0],
                        marker_symbol=symbols[0],
                        legendgroup=1,
                        showlegend=show),
                        row=i, col=1)
    fig.add_trace(go.Scatter(x=polling_counts, y=spmv_arrowhead,
                        mode='lines+markers',
                        name='spmv-arrowhead',
                        line_color=colors[3],
                        marker_symbol=symbols[1],
                        legendgroup=1,
                        showlegend=show),
                        row=i, col=1)
    fig.add_trace(go.Scatter(x=polling_counts, y=spmv_powerlaw,
                        mode='lines+markers',
                        name='spmv-powerlaw',
                        line_color=colors[6],
                        marker_symbol=symbols[2],
                        legendgroup=1,
                        showlegend=show),
                        row=i, col=1)
    fig.add_trace(go.Scatter(x=polling_counts, y=spmv_random,
                        mode='lines+markers',
                        name='spmv-random',
                        line_color=colors[2],
                        marker_symbol=symbols[6],
                        legendgroup=1,
                        showlegend=show),
                        row=i, col=1)
    fig.add_trace(go.Scatter(x=polling_counts, y=kmeans,
                        mode='lines+markers',
                        name='kmeans',
                        line_color=colors[5],
                        marker_symbol=symbols[3],
                        legendgroup=1,
                        showlegend=show),
                        row=i, col=1)
    fig.add_trace(go.Scatter(x=polling_counts, y=plus_reduce_array,
                        mode='lines+markers',
                        name='plus-reduce-array',
                        line_color=colors[4],
                        marker_symbol=symbols[4],
                        legendgroup=1,
                        showlegend=show),
                        row=i, col=1)
    fig.add_trace(go.Scatter(x=polling_counts, y=srad,
                        mode='lines+markers',
                        name='srad',
                        line_color=colors[1],
                        marker_symbol=symbols[5],
                        legendgroup=1,
                        showlegend=show),
                        row=i, col=1)

fig.update_traces(line_width=3,
                  marker=dict(size=12,
                              line=dict(width=1,
                                        color='DarkSlateGrey')))
# Move legend
fig.update_layout(
    legend=dict(
        font_size=16,
        yanchor="bottom",
        y=0.01,
        xanchor="right",
        x=0.99,
        bgcolor="#e8e8e8",
        bordercolor="Black",
        borderwidth=1),
    plot_bgcolor="white",
    height=300,
    width=800,
    margin=dict(l=65,r=0,b=0,t=0)
)

fig.add_vline(x=8, layer='below', line_dash="5", line_color="red", row=1)
fig.add_vline(x=8, line_dash="5", annotation_text="8", annotation_font_size=14, line_color="red", annotation_font_color="red", annotation_x=8.01, annotation_xanchor="center", annotation_y=-0.04, annotation_yanchor="top", row=2)

cut_interval = [10, 45]
fig.add_annotation(text="Heartbeat detection rate (%)", xref='paper', x=-0.05, yref='paper', y=0.38, textangle=-90, font_size=18)
fig.update_xaxes(row=1, col=1, range=[0, 35], dtick=5, showgrid=True, gridwidth=1, gridcolor='grey', showline=True, showticklabels=False, mirror=True, linewidth=1, linecolor='black', titlefont_size=20, tickfont_size=14)
fig.update_xaxes(row=2, col=1, range=[0, 35], dtick=5, showgrid=True, gridwidth=1, gridcolor='grey', showline=True, mirror=True, linewidth=1, linecolor='black', title="Target polling count", titlefont_size=20, tickfont_size=14)
fig.update_yaxes(range=[0, cut_interval[0]], dtick=10, row=2, col=1, showgrid=True, gridwidth=1, gridcolor='grey', mirror=True, linewidth=1, linecolor='black', titlefont_size=18, tickfont_size=14)
fig.update_yaxes(range=[cut_interval[1], 105], dtick=10, row=1, col=1, showline=True, gridwidth=1, gridcolor='grey', mirror=True, linewidth=1, linecolor='black', tickfont_size=14)
fig.write_image('plots/plot_heartbeat_detection.pdf', format='pdf')
