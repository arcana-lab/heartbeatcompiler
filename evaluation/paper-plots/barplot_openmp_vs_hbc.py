import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd
import numpy as np
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_openmp_vs_hbc.csv')
print(df)

benchmarks=list(df.loc[:,'benchmarks'])
# openmp_static_speedups=list(df.loc[:,'openmp_static (speedup)'])
# openmp_static_nested_speedups=list(df.loc[:,'openmp_static_nested (speedup)'])
# openmp_dynamic_speedups=list(df.loc[:,'openmp_dynamic (speedup)'])
# openmp_dynamic_nested_speedups=list(df.loc[:,'openmp_dynamic_nested (speedup)'])
# openmp_guided_speedups=list(df.loc[:,'openmp_guided (speedup)'])
# openmp_guided_nested_speedups=list(df.loc[:,'openmp_guided_nested (speedup)'])
openmp_best_speedups=list(df.loc[:,'openmp_best (speedup)'])
hbc_speedups=list(df.loc[:,'hbc (speedup)'])
# openmp_static_stdev=list(df.loc[:,'openmp_static (stdev)'])
# openmp_static_nested_stdev=list(df.loc[:,'openmp_static_nested (stdev)'])
# openmp_dynamic_stdev=list(df.loc[:,'openmp_dynamic (stdev)'])
# openmp_dynamic_nested_stdev=list(df.loc[:,'openmp_dynamic_nested (stdev)'])
# openmp_guided_stdev=list(df.loc[:,'openmp_guided (stdev)'])
# openmp_guided_nested_stdev=list(df.loc[:,'openmp_guided_nested (stdev)'])
openmp_best_stdev=list(df.loc[:,'openmp_best (stdev)'])
hbc_stdev=list(df.loc[:,'hbc (stdev)'])

# Sort benchmarks by increasing hbc speedup
# hbc_speedups[:-2], openmp_best_speedups[:-2], hbc_stdev[:-2], openmp_best_stdev[:-2], benchmarks[:-2] = (list(t) for t in zip(*sorted(zip(hbc_speedups[:-2], openmp_best_speedups[:-2], hbc_stdev[:-2], openmp_best_stdev[:-2], benchmarks[:-2]))))

# Manually offset the number annotations spacing
openmp_text = []
for i, s in enumerate(openmp_best_speedups):
    if math.isnan(openmp_best_stdev[i]):
        openmp_text.append(openmp_best_speedups[i])
        continue
    spaces = round(openmp_best_stdev[i] / 0.4) * ' '
    openmp_text.append(spaces + str(openmp_best_speedups[i]))
print(openmp_text)

hbc_text = []
for i, s in enumerate(hbc_speedups):
    if math.isnan(hbc_stdev[i]):
        hbc_text.append(hbc_speedups[i])
        continue
    spaces = round(hbc_stdev[i] / 0.4) * ' '
    hbc_text.append(spaces + str(hbc_speedups[i]))
print(hbc_text)

# Remove error bars for geomeans
del openmp_best_stdev[-1]
del openmp_best_stdev[-1]
del hbc_stdev[-1]
del hbc_stdev[-1]

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#4575b4']

# Create the bar chart
fig = go.Figure(data=[
    go.Bar(name='OpenMP (best scheduling)', y=benchmarks, x=openmp_best_speedups, error_x=dict(type='data', array=openmp_best_stdev), marker_color=colors[5], text=openmp_text, textposition="outside", textfont_size=9, orientation='h'),
    go.Bar(name='Heartbeat Compiler', y=benchmarks, x=hbc_speedups, error_x=dict(type='data', array=hbc_stdev), marker_color=colors[2], text=hbc_text, textposition="outside", textfont_size=9, orientation='h')
], layout_xaxis_range=[0,69])

# Add shading
fig.add_hrect(y0=-0.5, y1=4.5, 
              annotation_text="irregular workloads", annotation_y=4.25, annotation_yanchor="middle", annotation_x=0.75, annotation_xanchor="center",
              fillcolor="LightGrey", opacity=0.8, line_width=0, layer='below')
fig.add_hrect(y0=4.5, y1=8.5, 
              annotation_text="regular workloads", annotation_y=4.75, annotation_yanchor="middle", annotation_x=0.75, annotation_xanchor="center",
              fillcolor="LightGrey", opacity=0.3, line_width=0, layer='below')

# Move legend
fig.update_layout(
    legend=dict(
        yanchor="bottom",
        y=0.01,
        xanchor="auto",
        x=0.99,
        bgcolor="#e8e8e8",
        bordercolor="Black",
        borderwidth=1),
    xaxis_title="Program Speedup",
    plot_bgcolor="white",
    height=450,
    width=800,
    margin=dict(l=0,r=0,b=0,t=35)
)

# Add lines
fig.add_hline(y=8.5)
fig.add_vline(x=1, line_dash="5", annotation_text="baseline", annotation_font_size=10, annotation_textangle=315, annotation_x=0, annotation_y=0.99, annotation_yanchor="bottom")
fig.add_vline(x=64, line_dash="5", line_color="red", annotation_text="cores", annotation_font_color="red", annotation_font_size=10, annotation_textangle=315, annotation_x=63, annotation_y=0.99, annotation_yanchor="bottom")

# Other changes to graph
fig['layout']['yaxis']['autorange'] = "reversed"
fig.update_layout(barmode='group', yaxis2=dict(overlaying='y'))
fig.update_xaxes(dtick=10, showgrid=True, gridwidth=1, gridcolor='grey')
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black')
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black')
fig.write_image('plots/plot_openmp_vs_hbc.pdf', format='pdf')