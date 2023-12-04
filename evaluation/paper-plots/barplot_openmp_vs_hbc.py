import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd
import numpy as np
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_openmp_vs_hbc.csv')
print(df)

split=14
benchmarks_irregular=list(df.loc[:,'benchmarks'])[:split]
benchmarks_regular=list(df.loc[:,'benchmarks'])[split:]
openmp_static_speedups=list(df.loc[:,'openmp_static'])[split:]
openmp_dynamic_speedups=list(df.loc[:,'openmp_dynamic'])[:split]
# openmp_guided_speedups=list(df.loc[:,'openmp_guided'])[:split]
# openmp_best_speedups=list(df.loc[:,'openmp (best)'])
hbc_irregular_speedups=list(df.loc[:,'hbc'])[:split]
hbc_regular_speedups=list(df.loc[:,'hbc'])[split:]
openmp_static_stdev=list(df.loc[:,'openmp_static (stdev)'])[split:]
openmp_dynamic_stdev=list(df.loc[:,'openmp_dynamic (stdev)'])[:split]
# openmp_guided_stdev=list(df.loc[:,'openmp_guided (stdev)'])[:split]
# openmp_best_stdev=list(df.loc[:,'openmp_best (stdev)'])
hbc_irregular_stdev=list(df.loc[:,'hbc (stdev)'])[:split]
hbc_regular_stdev=list(df.loc[:,'hbc (stdev)'])[split:]

print(benchmarks_irregular)
print(benchmarks_regular)

# Sort benchmarks by increasing hbc speedup
# hbc_speedups[:-2], openmp_best_speedups[:-2], hbc_stdev[:-2], openmp_best_stdev[:-2], benchmarks[:-2] = (list(t) for t in zip(*sorted(zip(hbc_speedups[:-2], openmp_best_speedups[:-2], hbc_stdev[:-2], openmp_best_stdev[:-2], benchmarks[:-2]))))

# Manually offset the number annotations spacing
openmp_static_text = []
for i, s in enumerate(openmp_static_speedups):
    if math.isnan(openmp_static_stdev[i]):
        openmp_static_text.append(openmp_static_speedups[i])
        continue
    spaces = round(openmp_static_stdev[i] / 0.45) * ' '
    openmp_static_text.append(spaces + str(openmp_static_speedups[i]))
print(openmp_static_text)

openmp_dynamic_text = []
for i, s in enumerate(openmp_dynamic_speedups):
    if math.isnan(openmp_dynamic_stdev[i]):
        openmp_dynamic_text.append(openmp_dynamic_speedups[i])
        continue
    spaces = round(openmp_dynamic_stdev[i] / 0.45) * ' '
    openmp_dynamic_text.append(spaces + str(openmp_dynamic_speedups[i]))
print(openmp_dynamic_text)

# openmp_guided_text = []
# for i, s in enumerate(openmp_guided_speedups):
#     if math.isnan(openmp_guided_stdev[i]):
#         openmp_guided_text.append(openmp_guided_speedups[i])
#         continue
#     spaces = round(openmp_guided_stdev[i] / 0.45) * ' '
#     openmp_guided_text.append(spaces + str(openmp_guided_speedups[i]))
# print(openmp_guided_text)

hbc_irregular_text = []
for i, s in enumerate(hbc_irregular_speedups):
    if math.isnan(hbc_irregular_stdev[i]):
        hbc_irregular_text.append(hbc_irregular_speedups[i])
        continue
    spaces = round(hbc_irregular_stdev[i] / 0.45) * ' '
    hbc_irregular_text.append(spaces + str(hbc_irregular_speedups[i]))
print(hbc_irregular_text)

hbc_regular_text = []
for i, s in enumerate(hbc_regular_speedups):
    if math.isnan(hbc_regular_stdev[i]):
        hbc_regular_text.append(hbc_regular_speedups[i])
        continue
    spaces = round(hbc_regular_stdev[i] / 0.45) * ' '
    hbc_regular_text.append(spaces + str(hbc_regular_speedups[i]))
print(hbc_regular_text)

# Remove error bars for geomeans
# for i in range(0,3):
benchmarks_irregular[-1] = '<b>' + benchmarks_irregular[-1] + '</b>'
benchmarks_regular[-1] = '<b>' + benchmarks_regular[-1] + '</b>'
hbc_irregular_text[-1] = '<b>' + str(hbc_irregular_text[-1]) + '</b>'
hbc_regular_text[-1] = '<b>' + str(hbc_regular_text[-1]) + '</b>'
openmp_static_text[-1] = "<b>" + str(openmp_static_text[-1]) + "</b>"
openmp_dynamic_text[-1] = "<b>" + str(openmp_dynamic_text[-1]) + "</b>"
# openmp_guided_text[-1] = '<b>' + str(openmp_guided_text[-1]) + '</b>'
del openmp_static_stdev[-1]
del openmp_dynamic_stdev[-1]
# del openmp_guided_stdev[-1]
del hbc_irregular_stdev[-1]
del hbc_regular_stdev[-1]

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#4575b4']

# Create the bar chart
fig1 = go.Figure(data=[
    go.Bar(name='OpenMP (dynamic)', y=benchmarks_irregular, x=openmp_dynamic_speedups, error_x=dict(type='data', array=openmp_dynamic_stdev), marker_color=colors[4], text=openmp_dynamic_text, textposition="outside", orientation='h'),
    # go.Bar(name='OpenMP (guided)', y=benchmarks_irregular, x=openmp_guided_speedups, error_x=dict(type='data', array=openmp_guided_stdev), marker_color=colors[5], text=openmp_guided_text, textposition="outside", orientation='h'),
    go.Bar(name='HBC', y=benchmarks_irregular, x=hbc_irregular_speedups, error_x=dict(type='data', array=hbc_irregular_stdev), marker_color=colors[2], text=hbc_irregular_text, textposition="outside", orientation='h')
], layout_xaxis_range=[0,69])

fig2 = go.Figure(data=[
    go.Bar(name='OpenMP (static)', y=benchmarks_regular, x=openmp_static_speedups, error_x=dict(type='data', array=openmp_static_stdev), marker_color=colors[4], text=openmp_static_text, textposition="outside", orientation='h'),
    go.Bar(name='HBC', y=benchmarks_regular, x=hbc_regular_speedups, error_x=dict(type='data', array=hbc_regular_stdev), marker_color=colors[2], text=hbc_regular_text, textposition="outside", orientation='h')
], layout_xaxis_range=[0,69])

for fig in [fig1, fig2]:
    # Add shading
    # fig.add_hrect(y0=-0.5, y1=4.5, 
    #             annotation_text="Irregular workloads", annotation_font_size=16, annotation_y=2.5, annotation_yanchor="middle", annotation_x=0.73, annotation_xanchor="center",
    #             fillcolor="LightGrey", opacity=0.8, line_width=0, layer='below')
    # fig.add_hrect(y0=4.5, y1=8.5, 
    #             annotation_text="Regular workloads", annotation_font_size=16, annotation_y=6.5, annotation_yanchor="middle", annotation_x=0.735, annotation_xanchor="center",
    #             fillcolor="LightGrey", opacity=0.3, line_width=0, layer='below')

    # Add lines
    fig.add_vline(x=1, y1=1.3, line_dash="5", annotation_text="baseline", annotation_font_size=16, annotation_x=1.5, annotation_y=1, annotation_yanchor="bottom")
    fig.add_vline(x=64, y1=1.3, line_dash="5", line_color="red", annotation_text="cores", annotation_font_color="red", annotation_font_size=16, annotation_x=63.5, annotation_xanchor='right', annotation_y=1, annotation_yanchor="bottom")

    # Other changes to graph
    fig['layout']['yaxis']['autorange'] = "reversed"
    fig.update_layout(barmode='group', yaxis2=dict(overlaying='y'))
    fig.update_xaxes(dtick=10, showgrid=True, gridwidth=1, gridcolor='grey')
    fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black', titlefont_size=20, tickfont_size=14)
    fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickfont_size=20)
    
fig1.update_layout(
    uniformtext_minsize=13,
    uniformtext_mode='show',
    xaxis_title="Program speedup",
    xaxis_tickvals=[1, 10, 20, 30, 40, 50, 60],
    plot_bgcolor="white",
    height=450,
    width=800,
    margin=dict(l=0,r=0,b=0,t=20)
)
fig2.update_layout(
    uniformtext_minsize=13,
    uniformtext_mode='show',
    xaxis_title="Program speedup",
    xaxis_tickvals=[1, 10, 20, 30, 40, 50, 60],
    plot_bgcolor="white",
    height=250,
    width=800,
    margin=dict(l=0,r=0,b=0,t=20)
)

fig1.update_xaxes(titlefont_size=20)
fig2.update_xaxes(titlefont_size=20)

fig1.add_hline(y=len(benchmarks_irregular) - 1.5)
fig2.add_hline(y=len(benchmarks_regular) - 1.5)
fig1.update_layout(
    legend=dict(
        font_size=17,
        yanchor="bottom",
        y=0.03,
        xanchor="auto",
        x=0.92,
        bgcolor="#e8e8e8",
        bordercolor="Black",
        borderwidth=1))
fig2.update_layout(
    legend=dict(
        font_size=17,
        yanchor="top",
        y=0.96,
        xanchor="auto",
        x=0.92,
        bgcolor="#e8e8e8",
        bordercolor="Black",
        borderwidth=1))
fig1.write_image('plots/plot_openmp_vs_hbc_irregular.pdf', format='pdf')
fig2.write_image('plots/plot_openmp_vs_hbc_regular.pdf', format='pdf')