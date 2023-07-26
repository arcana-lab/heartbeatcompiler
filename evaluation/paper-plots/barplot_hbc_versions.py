import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_hbc_versions.csv')
print(df)

benchmarks=list(df.loc[:,'benchmarks'])
hbc_rf_speedups=list(df.loc[:,'hbc_rf (speedup)'])
hbc_rf_kmod_speedups=list(df.loc[:,'hbc_rf_kmod (speedup)'])
hbc_static_speedups=list(df.loc[:,'hbc_static (speedup)'])
hbc_speedups=list(df.loc[:,'hbc (speedup)'])
hbc_rf_stdev=list(df.loc[:,'hbc_rf (stdev)'])
hbc_rf_kmod_stdev=list(df.loc[:,'hbc_rf_kmod (stdev)'])
hbc_static_stdev=list(df.loc[:,'hbc_static (stdev)'])
hbc_stdev=list(df.loc[:,'hbc (stdev)'])

# Manually offset the number annotations spacing
hbc_rf_text = []
for i, s in enumerate(hbc_rf_speedups):
    if math.isnan(hbc_rf_stdev[i]):
        hbc_rf_text.append(hbc_rf_speedups[i])
        continue
    spaces = round(hbc_rf_stdev[i] / 0.2) * ' '
    hbc_rf_text.append(spaces + " " + str(hbc_rf_speedups[i]))
print(hbc_rf_text)

hbc_rf_kmod_text = []
for i, s in enumerate(hbc_rf_kmod_speedups):
    if math.isnan(hbc_rf_kmod_stdev[i]):
        hbc_rf_kmod_text.append(hbc_rf_kmod_speedups[i])
        continue
    spaces = round(hbc_rf_kmod_stdev[i] / 0.2) * ' '
    hbc_rf_kmod_text.append(spaces + " " + str(hbc_rf_kmod_speedups[i]))
print(hbc_rf_kmod_text)

hbc_static_text = []
for i, s in enumerate(hbc_static_speedups):
    if math.isnan(hbc_static_stdev[i]):
        hbc_static_text.append(hbc_static_speedups[i])
        continue
    spaces = round(hbc_static_stdev[i] / 0.2) * ' '
    hbc_static_text.append(spaces + " " + str(hbc_static_speedups[i]))
print(hbc_static_text)

hbc_text = []
for i, s in enumerate(hbc_speedups):
    if math.isnan(hbc_stdev[i]):
        hbc_text.append(hbc_speedups[i])
        continue
    spaces = round(hbc_stdev[i] / 0.2) * ' '
    hbc_text.append(spaces + " " + str(hbc_speedups[i]))
print(hbc_text)

del hbc_rf_stdev[-1]
del hbc_rf_kmod_stdev[-1]
del hbc_static_stdev[-1]
del hbc_stdev[-1]

# Some colourblind-aware colour palette that I found online
colors = ['#377eb8', '#ff7f00', '#4daf4a',
           '#f781bf', '#a65628', '#984ea3',
           '#999999', '#e41a1c', '#dede00']

# fig = go.Figure(data=[
#     go.Bar(name='hbc + RF Interrupt Ping Thread', x=benchmarks, y=hbc_rf_speedups, error_y=dict(type='data', array=hbc_rf_stdev), marker_color=colors[0], text=hbc_rf_speedups),
#     go.Bar(name='hbc + RF Kernel Module', x=benchmarks, y=hbc_rf_kmod_speedups, error_y=dict(type='data', array=hbc_rf_kmod_stdev), marker_color=colors[1], text=hbc_rf_kmod_speedups),
#     go.Bar(name='hbc + Static Chunking', x=benchmarks, y=hbc_static_speedups, error_y=dict(type='data', array=hbc_static_stdev), marker_color=colors[2], text=hbc_static_speedups),
#     go.Bar(name='hbc + ACC', x=benchmarks, y=hbc_speedups, error_y=dict(type='data', array=hbc_stdev), marker_color=colors[3], text=hbc_speedups)
# ])
# fig.update_layout(
#     autosize=False,
#     width=1200,
#     height=500,
#     margin=dict(
#         pad=4
#     )
# )

fig = go.Figure(data=[
    go.Bar(name='hbc + RF Interrupt Ping Thread', y=benchmarks, x=hbc_rf_speedups, error_x=dict(type='data', array=hbc_rf_stdev, thickness=1, width=1.5), marker_color=colors[0], text=hbc_rf_text, orientation='h'),
    go.Bar(name='hbc + RF Kernel Module', y=benchmarks, x=hbc_rf_kmod_speedups, error_x=dict(type='data', array=hbc_rf_kmod_stdev, thickness=1, width=1.5), marker_color=colors[1], text=hbc_rf_kmod_text, orientation='h'),
    go.Bar(name='hbc + Static Chunking', y=benchmarks, x=hbc_static_speedups, error_x=dict(type='data', array=hbc_static_stdev, thickness=1, width=1.5), marker_color=colors[2], text=hbc_static_text, orientation='h'),
    go.Bar(name='hbc + ACC', y=benchmarks, x=hbc_speedups, error_x=dict(type='data', array=hbc_stdev, thickness=1, width=1.5), marker_color=colors[3], text=hbc_text, orientation='h')
])
fig.update_traces(textposition='outside', textfont_size=9)


# Move legend
fig.update_layout(
    legend=dict(
        yanchor="middle",
        y=0.5,
        xanchor="right",
        x=0.99),
    xaxis_title="Program Speedup"
)

# fig['layout']['yaxis']['autorange'] = "reversed"
# fig.add_vline(x=9.5)
# fig.add_hline(y=1, line_dash="5", annotation_text="baseline", annotation_font_size=10, annotation_x=1.01, annotation_xanchor="left")
# fig.add_hline(y=64, line_dash="5", line_color="red", annotation_text="cores", annotation_font_color="red", annotation_font_size=10, annotation_x=1.01, annotation_xanchor="left")
fig.add_hline(y=9.5)
fig.add_vline(x=1, line_dash="5", annotation_text="baseline", annotation_font_size=10, annotation_textangle=315, annotation_x=0, annotation_y=0.99, annotation_yanchor="bottom")
fig.add_vline(x=64, line_dash="5", line_color="red", annotation_text="cores", annotation_font_color="red", annotation_font_size=10, annotation_textangle=315, annotation_x=63, annotation_y=0.99, annotation_yanchor="bottom", layer="below")

# Change the bar mode
fig.update_layout(barmode='group')
# fig.update_yaxes(dtick=10, showgrid=True, gridwidth=1, gridcolor='grey')
fig.update_xaxes(dtick=10, showgrid=True, gridwidth=1, gridcolor='grey')
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black')
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black')
fig['layout']['yaxis']['autorange'] = "reversed"
fig.write_image('plots/plot_hbc_versions.pdf', format='pdf')