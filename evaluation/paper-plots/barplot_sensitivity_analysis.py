import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_sensitivity_analysis.csv')
print(df)

benchmarks=list(df.loc[:,'benchmarks'])
speedups_1=list(df.loc[:,'openmp_dynamic_speedup'])
speedups_2=list(df.loc[:,'chunksize_2_speed'])
speedups_4=list(df.loc[:,'chunksize_4_speed'])
speedups_8=list(df.loc[:,'chunksize_8_speed'])
speedups_16=list(df.loc[:,'chunksize_16_speed'])
speedups_32=list(df.loc[:,'chunksize_32_speed'])
stdev_1=list(df.loc[:,'openmp_dynamic_stdev'])
stdev_2=list(df.loc[:,'chunksize_2_stdev'])
stdev_4=list(df.loc[:,'chunksize_4_stdev'])
stdev_8=list(df.loc[:,'chunksize_8_stdev'])
stdev_16=list(df.loc[:,'chunksize_16_stdev'])
stdev_32=list(df.loc[:,'chunksize_32_stdev'])

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
# colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#4575b4']
colors = ['#e0f3f8','#c6dbef','#9ecae1','#6baed6','#4292c6','#2171b5','#084594'] #blues

speedups_1_text = []
for i, s in enumerate(speedups_1):
    if math.isnan(stdev_1[i]):
        speedups_1_text.append(speedups_1[i])
        continue
    spaces = round(stdev_1[i] / 0.43) * ' '
    speedups_1_text.append(spaces + str(speedups_1[i]))
print(speedups_1_text)

speedups_2_text = []
for i, s in enumerate(speedups_2):
    if math.isnan(stdev_2[i]):
        speedups_2_text.append(speedups_2[i])
        continue
    spaces = round(stdev_2[i] / 0.43) * ' '
    speedups_2_text.append(spaces + str(speedups_2[i]))
print(speedups_2_text)

speedups_4_text = []
for i, s in enumerate(speedups_4):
    if math.isnan(stdev_4[i]):
        speedups_4_text.append(speedups_4[i])
        continue
    spaces = round(stdev_4[i] / 0.43) * ' '
    speedups_4_text.append(spaces + str(speedups_4[i]))
print(speedups_4_text)

speedups_8_text = []
for i, s in enumerate(speedups_8):
    if math.isnan(stdev_8[i]):
        speedups_8_text.append(speedups_8[i])
        continue
    spaces = round(stdev_8[i] / 0.43) * ' '
    speedups_8_text.append(spaces + str(speedups_8[i]))
print(speedups_8_text)

speedups_16_text = []
for i, s in enumerate(speedups_16):
    if math.isnan(stdev_16[i]):
        speedups_16_text.append(speedups_16[i])
        continue
    spaces = round(stdev_16[i] / 0.43) * ' '
    speedups_16_text.append(spaces + str(speedups_16[i]))
print(speedups_16_text)

speedups_32_text = []
for i, s in enumerate(speedups_32):
    if math.isnan(stdev_32[i]):
        speedups_32_text.append(speedups_32[i])
        continue
    spaces = round(stdev_32[i] / 0.43) * ' '
    speedups_32_text.append(spaces + str(speedups_32[i]))
print(speedups_32_text)

# Create the bar chart
fig = go.Figure(data=[
    go.Bar(name='1 (default)', y=benchmarks, x=speedups_1, error_x=dict(type='data', array=stdev_1), marker_color=colors[5], text=speedups_1_text, orientation='h'),
    go.Bar(name='2', y=benchmarks, x=speedups_2, error_x=dict(type='data', array=stdev_2), marker_color=colors[4], text=speedups_2_text, orientation='h'),
    go.Bar(name='4', y=benchmarks, x=speedups_4, error_x=dict(type='data', array=stdev_4), marker_color=colors[3], text=speedups_4_text, orientation='h'),
    go.Bar(name='8', y=benchmarks, x=speedups_8, error_x=dict(type='data', array=stdev_8), marker_color=colors[2], text=speedups_8_text, orientation='h'),
    go.Bar(name='16', y=benchmarks, x=speedups_16, error_x=dict(type='data', array=stdev_16), marker_color=colors[1], text=speedups_16_text, orientation='h'),
    go.Bar(name='32', y=benchmarks, x=speedups_32, error_x=dict(type='data', array=stdev_32), marker_color=colors[0], text=speedups_32_text, orientation='h')
])
fig.update_traces(textposition='outside')

# Move legend
fig.update_layout(
    legend=dict(
        orientation='h',
        title="           OpenMP (dynamic) chunk size",
        title_side="top",
        font_size=16,
        yanchor="bottom",
        y=0.01,
        xanchor="right",
        x=0.99,
        bgcolor="#c2c2c2",
        bordercolor="Black",
        borderwidth=1),
    uniformtext_minsize=13,
    uniformtext_mode='show',
    bargap=0.1,
    xaxis_title="Program speedup",
    plot_bgcolor="white",
    margin=dict(l=0,r=0,b=0,t=20),
    height=400,
    width=800
)

# Add cut-off annotations
# for i, o in enumerate(no_chunking_overheads):
#     if o > 10:
#         fig.add_annotation(xref='paper', x=(0.03 + i/7), axref='x domain', ax=45, yref='paper', y=1, ayref='y', ay=9, text=str(no_chunking_overheads_text[i]), font_size=13)

# fig.add_vline(x=6.5)
fig['layout']['yaxis']['autorange'] = "reversed"
fig.add_vline(x=1, y1=1.3, line_dash="5", annotation_text="baseline", annotation_font_size=16, annotation_x=1.5, annotation_y=1, annotation_yanchor="bottom")
fig.add_vline(x=64, y1=1.3, line_dash="5", line_color="red", annotation_text="cores", annotation_font_color="red", annotation_font_size=16, annotation_x=63.5, annotation_xanchor='right', annotation_y=1, annotation_yanchor="bottom")
# Change the bar mode
fig.update_layout(barmode='group')
fig.update_xaxes(range=[0, 65], showgrid=True, gridwidth=1, gridcolor='grey')
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black', ticks="outside", tickfont_size=18)
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickfont_size=14, titlefont_size=18)
fig.write_image('plots/plot_sensitivity_analysis.pdf', format='pdf')