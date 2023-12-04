import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_tpal_vs_hbc.csv')
print(df)

benchmarks=list(df.loc[:,'benchmarks'])
tpal_speedups=list(df.loc[:,'tpal (speedup)'])
hbc_speedups=list(df.loc[:,'hbc (speedup)'])
tpal_stdev=list(df.loc[:,'tpal (stdev)'])
hbc_stdev=list(df.loc[:,'hbc (stdev)'])

# Manually offset the number annotations spacing
tpal_text = []
for i, s in enumerate(tpal_speedups):
    if math.isnan(tpal_stdev[i]):
        tpal_text.append(tpal_speedups[i])
        continue
    spaces = round(tpal_stdev[i] / 0.4) * ' '
    tpal_text.append(spaces + str(tpal_speedups[i]))
print(tpal_text)

hbc_text = []
for i, s in enumerate(hbc_speedups):
    if math.isnan(hbc_stdev[i]):
        hbc_text.append(hbc_speedups[i])
        continue
    spaces = round(hbc_stdev[i] / 0.4) * ' '
    hbc_text.append(spaces + str(hbc_speedups[i]))
print(hbc_text)

benchmarks[-1] = '<b>geomean</b>'
hbc_text[-1] = '<b>' + str(hbc_text[-1]) + '</b>'
tpal_text[-1] = "<b>" + str(tpal_text[-1]) + "</b>"
del tpal_stdev[-1]
del hbc_stdev[-1]

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#4575b4']

# Create the bar chart
fig = go.Figure(data=[
        go.Bar(name='TPAL', y=benchmarks, x=tpal_speedups, error_x=dict(type='data', array=tpal_stdev), marker_color=colors[4], text=tpal_text, textposition="outside", textfont_size=14, orientation='h'),
        go.Bar(name='HBC', y=benchmarks, x=hbc_speedups, error_x=dict(type='data', array=hbc_stdev), marker_color=colors[2], text=hbc_text, textposition="outside", textfont_size=14, orientation='h')
    ])

# Move legend
fig.update_layout(
    legend=dict(
        font_size=18,
        yanchor="middle",
        y=0.5,
        xanchor="right",
        x=0.9,
        bgcolor="#e8e8e8",
        bordercolor="Black",
        borderwidth=1),
    xaxis_title="Program speedup",
    xaxis_tickvals=[1, 10, 20, 30, 40, 50, 60],
    plot_bgcolor="white",
    height=350,
    width=800,
    margin=dict(l=0,r=0,b=0,t=20)
)

# Add lines
fig.add_hline(y=7.5)
fig.add_vline(x=1, y1=1.1, line_dash="5", annotation_text="baseline", annotation_font_size=16, annotation_x=1.5, annotation_y=1, annotation_yanchor="bottom")
fig.add_vline(x=64, y1=1.1, line_dash="5", line_color="red", annotation_text="cores", annotation_font_color="red", annotation_font_size=16, annotation_x=63.5, annotation_xanchor='right', annotation_y=1, annotation_yanchor="bottom")

fig['layout']['yaxis']['autorange'] = "reversed"
fig.update_layout(barmode='group')
fig.update_xaxes(dtick=10, range=[0, 69], showgrid=True, gridwidth=1, gridcolor='grey')
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black', titlefont_size=20, tickfont_size=14)
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickfont_size=20)
fig.write_image('plots/plot_tpal_vs_hbc.pdf', format='pdf')