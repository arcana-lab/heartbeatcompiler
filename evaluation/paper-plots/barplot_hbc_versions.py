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
# hbc_static_speedups=list(df.loc[:,'hbc_static (speedup)'])
hbc_speedups=list(df.loc[:,'hbc (speedup)'])
hbc_rf_stdev=list(df.loc[:,'hbc_rf (stdev)'])
hbc_rf_kmod_stdev=list(df.loc[:,'hbc_rf_kmod (stdev)'])
# hbc_static_stdev=list(df.loc[:,'hbc_static (stdev)'])
hbc_stdev=list(df.loc[:,'hbc (stdev)'])

# Manually offset the number annotations spacing
hbc_rf_text=hbc_rf_speedups
hbc_rf_text = []
for i, s in enumerate(hbc_rf_speedups):
    if math.isnan(hbc_rf_stdev[i]):
        hbc_rf_text.append(hbc_rf_speedups[i])
        continue
    spaces = round(hbc_rf_stdev[i] / 0.55) * ' '
    hbc_rf_text.append(spaces + " " + str(hbc_rf_speedups[i]))
print(hbc_rf_text)

hbc_rf_kmod_text=hbc_rf_kmod_speedups
hbc_rf_kmod_text = []
for i, s in enumerate(hbc_rf_kmod_speedups):
    if math.isnan(hbc_rf_kmod_stdev[i]):
        hbc_rf_kmod_text.append(hbc_rf_kmod_speedups[i])
        continue
    spaces = round(hbc_rf_kmod_stdev[i] / 0.55) * ' '
    hbc_rf_kmod_text.append(spaces + " " + str(hbc_rf_kmod_speedups[i]))
print(hbc_rf_kmod_text)

# hbc_static_text = []
# for i, s in enumerate(hbc_static_speedups):
#     if math.isnan(hbc_static_stdev[i]):
#         hbc_static_text.append(hbc_static_speedups[i])
#         continue
#     spaces = round(hbc_static_stdev[i] / 0.2) * ' '
#     hbc_static_text.append(spaces + " " + str(hbc_static_speedups[i]))
# print(hbc_static_text)

# hbc_text=hbc_speedups
hbc_text = []
for i, s in enumerate(hbc_speedups):
    if math.isnan(hbc_stdev[i]):
        hbc_text.append(hbc_speedups[i])
        continue
    spaces = round(hbc_stdev[i] / 0.55) * ' '
    hbc_text.append(spaces + " " + str(hbc_speedups[i]))
print(hbc_text)

benchmarks[-1] = '<b>geomean</b>'
hbc_text[-1] = '<b>' + str(hbc_text[-1]) + '</b>'
hbc_rf_text[-1] = '<b>' + str(hbc_rf_text[-1]) + '</b>'
hbc_rf_kmod_text[-1] = "<b>" + str(hbc_rf_kmod_text[-1]) + "</b>"
del hbc_rf_stdev[-1]
del hbc_rf_kmod_stdev[-1]
# del hbc_static_stdev[-1]
del hbc_stdev[-1]

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#4575b4']

# fig = go.Figure(data=[
#     go.Bar(name='hbc + RF Interrupt Ping Thread', y=benchmarks, x=hbc_rf_speedups, error_x=dict(type='data', array=hbc_rf_stdev, thickness=1, width=1.5), marker_color=colors[4], text=hbc_rf_text, orientation='h'),
#     go.Bar(name='hbc + RF Kernel Module', y=benchmarks, x=hbc_rf_kmod_speedups, error_x=dict(type='data', array=hbc_rf_kmod_stdev, thickness=1, width=1.5), marker_color=colors[5], text=hbc_rf_kmod_text, orientation='h'),
#     go.Bar(name='hbc + Static Chunking', y=benchmarks, x=hbc_static_speedups, error_x=dict(type='data', array=hbc_static_stdev, thickness=1, width=1.5), marker_color=colors[1], text=hbc_static_text, orientation='h'),
#     go.Bar(name='hbc + ACC', y=benchmarks, x=hbc_speedups, error_x=dict(type='data', array=hbc_stdev, thickness=1, width=1.5), marker_color=colors[2], text=hbc_text, orientation='h')
# ])
fig = go.Figure(data=[
    go.Bar(name='Interrupts (ping thread)', y=benchmarks, x=hbc_rf_speedups, error_x=dict(type='data', array=hbc_rf_stdev), marker_color=colors[5], text=hbc_rf_text, textposition='outside', orientation='h'),
    go.Bar(name='Interrupts (kernel module)', y=benchmarks, x=hbc_rf_kmod_speedups, error_x=dict(type='data', array=hbc_rf_kmod_stdev), marker_color=colors[4], text=hbc_rf_kmod_text, textposition='outside', orientation='h'),
    go.Bar(name='Software polling', y=benchmarks, x=hbc_speedups, error_x=dict(type='data', array=hbc_stdev), marker_color=colors[2], text=hbc_text, textposition='outside', orientation='h')
])
fig.update_traces(textposition='outside', textfont_size=13)


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
    uniformtext_minsize=13,
    uniformtext_mode='show',
    xaxis_title="Program Speedup",
    xaxis_tickvals=[1, 10, 20, 30, 40, 50, 60],
    plot_bgcolor="white",
    height=400,
    width=800,
    margin=dict(l=0,r=0,b=0,t=20)
)

fig.add_hline(y=6.5)
fig.add_vline(x=1, y1=1.1, line_dash="5", annotation_text="baseline", annotation_font_size=16, annotation_x=1.5, annotation_y=1, annotation_yanchor="bottom")
fig.add_vline(x=64, y1=1.1, line_dash="5", line_color="red", annotation_text="cores", annotation_font_color="red", annotation_font_size=16, annotation_x=63.5, annotation_xanchor='right', annotation_y=1, annotation_yanchor="bottom")

# Change the bar mode
fig.update_layout(barmode='group')
# fig.update_yaxes(dtick=10, showgrid=True, gridwidth=1, gridcolor='grey')
fig.update_xaxes(dtick=10, range=[0,69], showgrid=True, gridwidth=1, gridcolor='grey')
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black', titlefont_size=20, tickfont_size=14)
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickfont_size=20)
fig['layout']['yaxis']['autorange'] = "reversed"
fig.write_image('plots/plot_hbc_versions.pdf', format='pdf')