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

del tpal_stdev[-1]
del hbc_stdev[-1]

# Some colourblind-aware colour palette that I found online
colors = ['#377eb8', '#ff7f00', '#4daf4a',
           '#f781bf', '#a65628', '#984ea3',
           '#999999', '#e41a1c', '#dede00']

# Create the bar chart
fig = go.Figure(data=[
        go.Bar(name='TPAL', y=benchmarks, x=tpal_speedups, error_x=dict(type='data', array=tpal_stdev), marker_color=colors[0], text=tpal_text, textposition="outside", textfont_size=9, orientation='h'),
        go.Bar(name='Heartbeat Compiler', y=benchmarks, x=hbc_speedups, error_x=dict(type='data', array=hbc_stdev), marker_color=colors[1], text=hbc_text, textposition="outside", textfont_size=9, orientation='h')
    ])

# Move legend
fig.update_layout(
    legend=dict(
        yanchor="bottom",
        y=0.01,
        xanchor="auto",
        x=0.99),
    xaxis_title="Program Speedup"
)

# Add lines
fig.add_hline(y=6.5)
fig.add_vline(x=1, line_dash="5", annotation_text="baseline", annotation_font_size=10, annotation_textangle=315, annotation_x=0, annotation_y=0.99, annotation_yanchor="bottom")
fig.add_vline(x=64, line_dash="5", line_color="red", annotation_text="cores", annotation_font_color="red", annotation_font_size=10, annotation_textangle=315, annotation_x=63, annotation_y=0.99, annotation_yanchor="bottom")

fig['layout']['yaxis']['autorange'] = "reversed"
fig.update_layout(barmode='group')
fig.update_xaxes(dtick=10, showgrid=True, gridwidth=1, gridcolor='grey')
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black')
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black')
fig.write_image('plot_tpal_vs_hbc.pdf', format='pdf')