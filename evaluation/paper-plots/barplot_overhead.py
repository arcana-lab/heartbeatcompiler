import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_overhead.csv')
print(df)

benchmarks=list(df.loc[:,'benchmarks'])
benchmarks2=[x + " ." for x in benchmarks]
benchmarks3=[x + " .." for x in benchmarks]
tpal_environment_overhead=list(df.loc[:,'tpal_environment_overhead'])
outline_overhead=list(df.loc[:,'outline_overhead'])
environment_overhead=list(df.loc[:,'environment_overhead'])
loop_chunking_overhead=list(df.loc[:,'loop_chunking_overhead'])
chunk_transferring_overhead=list(df.loc[:,'chunk_transferring_overhead'])
zeros=[0,]*len(benchmarks)
blanks=[' ',]*len(benchmarks)

benchmarks_text = [val for pair in zip(blanks, benchmarks, blanks) for val in pair]
benchmarks = [val for pair in zip(benchmarks, benchmarks2, benchmarks3) for val in pair]
tpal_environment_overhead = [val for pair in zip(tpal_environment_overhead, zeros, zeros) for val in pair]
outline_overhead = [val for pair in zip(zeros, outline_overhead, zeros) for val in pair]
environment_overhead = [val for pair in zip(zeros, environment_overhead, zeros) for val in pair]
loop_chunking_overhead = [val for pair in zip(zeros, loop_chunking_overhead, zeros) for val in pair]
chunk_transferring_overhead = [val for pair in zip(zeros, chunk_transferring_overhead, zeros) for val in pair]

print(benchmarks)
print(tpal_environment_overhead)
print(outline_overhead)
print(environment_overhead)
print(loop_chunking_overhead)
print(chunk_transferring_overhead)

print(benchmarks_text)

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#4575b4']

# Create the bar chart
fig = go.Figure(data=[
        go.Bar(orientation='h', name='Loop Closure Generation', y=benchmarks, x=tpal_environment_overhead, marker_color=colors[5], offsetgroup=0, legendgroup="tpal", legendgrouptitle_text="TPAL"),
        go.Bar(orientation='h', name='Loop Closure Generation', y=benchmarks, x=environment_overhead, marker_color=colors[4], offsetgroup=1, legendgroup="hbc", legendgrouptitle_text="HBC"),
        go.Bar(orientation='h', name='Loop Outlining', y=benchmarks, x=outline_overhead, marker_color=colors[0], offsetgroup=1, legendgroup="hbc", legendgrouptitle_text="HBC"),
        go.Bar(orientation='h', name='Loop Chunking Transformation', y=benchmarks, x=loop_chunking_overhead, marker_color=colors[1], offsetgroup=1, legendgroup="hbc", legendgrouptitle_text="HBC"),
        go.Bar(orientation='h', name='Chunksize Transferring', y=benchmarks, x=chunk_transferring_overhead, marker_color=colors[2], offsetgroup=1, legendgroup="hbc", legendgrouptitle_text="HBC")
    ])

# Move legend
fig.update_layout(
    legend=dict(
        # orientation='h',
        tracegroupgap=0,
        grouptitlefont_size=18,
        font_size=16,
        yanchor="bottom",
        y=0.01,
        xanchor="left",
        x=0.7,
        bgcolor="#e8e8e8",
        bordercolor="Black",
        borderwidth=1),
    bargap=0.3,
    xaxis_title="Overhead Compared to Baseline (%)",
    plot_bgcolor="white",
    height=350,
    width=800,
    margin=dict(l=0,r=0,b=0,t=0)
)

fig.update_traces(width=1)

# Add lines
for i in range(0, len(benchmarks)//3 - 1):
    fig.add_hline(y=i*3+2, line_width=1, line_color='grey')
# fig.add_vline(x=1, y1=1.03, line_dash="5", annotation_text="baseline", annotation_font_size=12, annotation_x=1.5, annotation_y=1, annotation_yanchor="bottom")
# fig.add_vline(x=64, y1=1.03, line_dash="5", line_color="red", annotation_text="cores", annotation_font_color="red", annotation_font_size=12, annotation_x=63.5, annotation_xanchor='right', annotation_y=1, annotation_yanchor="bottom")

fig['layout']['yaxis']['autorange'] = "reversed"
fig.update_layout(barmode='relative')
fig.update_xaxes(dtick=10, range=[-10, 57], showgrid=True, gridwidth=1, gridcolor='grey', zerolinecolor='black', zerolinewidth=3)
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickfont_size=16)
fig.update_yaxes(dtick=3, tick0=0.4, tickvals=benchmarks, ticktext=benchmarks_text)
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black', titlefont_size=20)
fig.write_image('plots/plot_overhead.pdf', format='pdf')