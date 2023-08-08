import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_parallelism_generation.csv')
print(df)

benchmarks=list(df.columns)
level_zero=[0 if math.isnan(x)  else x for x in df.iloc[0]]
level_one=[0 if  math.isnan(x) else x for x in df.iloc[1]]
level_two=[0 if  math.isnan(x) else x for x in df.iloc[2]]

print(level_zero)
print(level_one)
print(level_two)

totals = [i+j+k for i,j,k in zip(level_zero, level_one, level_two)]
level_zero_rel = [round(i / j * 100, 1) for i,j in zip(level_zero, totals)]
level_one_rel = [round(i / j * 100, 1) for i,j in zip(level_one, totals)]
level_two_rel = [round(i / j * 100, 1) for i,j in zip(level_two, totals)]
print(level_zero_rel)
print(level_one_rel)
print(level_two_rel)

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#4575b4']

# Create the bar chart
fig = go.Figure(data=[
    go.Bar(name='Nesting level 0', y=benchmarks, x=level_zero_rel, marker_color=colors[0], text=level_zero_rel, textposition='inside', orientation='h'),
    go.Bar(name='Nesting level 1', y=benchmarks, x=level_one_rel, marker_color=colors[1], text=level_one_rel, textposition='inside', orientation='h'),
    go.Bar(name='Nesting level 2', y=benchmarks, x=level_two_rel, marker_color=colors[2], text=level_two_rel, textposition='inside', orientation='h')
])
# fig.update_traces(textposition='outside', texttemplate='%{text:.1f}')


# Move legend
fig.update_layout(
    legend=dict(
        font_size=16,
        yanchor="bottom",
        y=0.03,
        xanchor="left",
        x=0.01,
        bgcolor="#e8e8e8",
        bordercolor="Black",
        borderwidth=1),
    xaxis_title="Parallelism promotions generated (%)",
    uniformtext_minsize=14,
    uniformtext_mode='show',
    plot_bgcolor="white",
    margin=dict(l=0,r=0,b=0,t=0),
    height=200,
    width=800
)

fig.add_annotation(text=level_zero_rel[1], font_color=colors[0], arrowcolor=colors[0], yref='paper', y=0.7, xref='paper', x=0.005, arrowhead=1, axref='x domain', ax=40, ayref='y domain', ay=0, font_size=14)
# fig.add_vline(x=6.5)
fig['layout']['yaxis']['autorange'] = "reversed"
# Change the bar mode
fig.update_layout(barmode='relative')
# fig.update_yaxes(dtick=2, showgrid=True, gridwidth=1, gridcolor='grey', range=[0, 12])
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickfont_size=14, titlefont_size=20)
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickfont_size=20)
fig.write_image('plots/plot_parallelism_generation.pdf', format='pdf')