import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_nested_parallelism.csv')
print(df)

benchmarks=list(df.loc[:,'benchmarks'])
openmp_dynamic_speedup=list(df.loc[:,'openmp_dynamic_speedup'])
nested_parallelism_speedup=list(df.loc[:,'nested_parallelism_speedup'])

openmp_dynamic_speedup_text = []
for s in openmp_dynamic_speedup:
    if math.isnan(s):
        openmp_dynamic_speedup_text.append("   DNF")
    else:
        openmp_dynamic_speedup_text.append(str(s))
print(openmp_dynamic_speedup_text)

nested_parallelism_speedup_text = []
for s in nested_parallelism_speedup:
    if math.isnan(s):
        nested_parallelism_speedup_text.append("   DNF")
    else:
        nested_parallelism_speedup_text.append(str(s))
print(nested_parallelism_speedup_text)

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
# colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#4575b4']
colors = ['#e0f3f8','#c6dbef','#9ecae1','#6baed6','#4292c6','#2171b5','#084594'] #blues

# Create the bar chart
fig = go.Figure(data=[
    go.Bar(name='outermost DOALL loop only', y=benchmarks, x=openmp_dynamic_speedup, marker_color=colors[4], text=openmp_dynamic_speedup_text, orientation='h'),
    go.Bar(name='all DOALL loops', y=benchmarks, x=nested_parallelism_speedup, marker_color=colors[2], text=nested_parallelism_speedup_text, orientation='h')
])
fig.update_traces(textposition='outside')

# Move legend
fig.update_layout(
    legend=dict(
        font_size=16,
        yanchor="bottom",
        y=0.02,
        xanchor="right",
        x=0.976,
        bgcolor="#c2c2c2",
        bordercolor="Black",
        borderwidth=1),
    uniformtext_minsize=13,
    uniformtext_mode='show',
    xaxis_title="Program speedup",
    plot_bgcolor="white",
    margin=dict(l=0,r=0,b=0,t=20),
    height=250,
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
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black', ticks="outside", tickfont_size=16)
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickfont_size=14, titlefont_size=20)
fig.write_image('plots/plot_nested_parallelism.pdf', format='pdf')