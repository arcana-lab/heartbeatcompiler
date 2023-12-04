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
level_three=[0 if  math.isnan(x) else x for x in df.iloc[3]]

print(level_zero)
print(level_one)
print(level_two)
print(level_three)

totals = [i+j+k for i,j,k in zip(level_zero, level_one, level_two)]
level_zero_rel = [round(i / j * 100, 2) for i,j in zip(level_zero, totals)]
level_one_rel = [round(i / j * 100, 2) for i,j in zip(level_one, totals)]
level_two_rel = [round(i / j * 100, 2) for i,j in zip(level_two, totals)]
level_three_rel = [round(i / j * 100, 2) for i,j in zip(level_three, totals)]
level_zero_rel_text = level_zero_rel.copy()
# level_zero_rel_text[6] = '>99.99'
print(level_zero_rel)
print(level_one_rel)
print(level_two_rel)
print(level_three_rel)

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
# colors = ['#fee090','#fc8d59','#d73027','#e0f3f8','#91bfdb','#4575b4']
colors = ['#fee5d9','#fcbba1','#fc9272','#fb6a4a','#de2d26','#a50f15']      # reds

# Create the bar chart
fig = go.Figure(data=[
    go.Bar(name='Nesting level 0', y=benchmarks, x=level_zero_rel, marker_color=colors[0], text=level_zero_rel_text, textposition='inside', orientation='h'),
    go.Bar(name='Nesting level 1', y=benchmarks, x=level_one_rel, marker_color=colors[2], text=level_one_rel, textposition='inside', orientation='h'),
    go.Bar(name='Nesting level 2', y=benchmarks, x=level_two_rel, marker_color=colors[4], text=level_two_rel, textposition='inside', orientation='h'),
    go.Bar(name='Nesting level 3', y=benchmarks, x=level_three_rel, marker_color=colors[5], text=level_three_rel, textposition='inside', orientation='h')
])
# fig.update_traces(textposition='outside', texttemplate='%{text:.1f}')


# Move legend
fig.update_layout(
    legend=dict(
        font_size=16,
        yanchor="bottom",
        y=0.014,
        xanchor="left",
        x=0.008,
        bgcolor="#c2c2c2",
        bordercolor="Black",
        borderwidth=1),
    xaxis_title="Parallelism promotions generated (%)",
    uniformtext_minsize=14,
    uniformtext_mode='hide',
    plot_bgcolor="white",
    margin=dict(l=0,r=52,b=0,t=0),
    height=450,
    width=800
)

fig.add_annotation(text=level_zero_rel[0], font_color='black', y=0, xref='paper', x=-0.001, showarrow=False, font_size=14)
fig.add_annotation(text=level_zero_rel[3], font_color=colors[0], arrowcolor=colors[0], y=3, xref='paper', x=0.005, arrowhead=1, axref='x domain', ax=30, ayref='y domain', ay=0, font_size=14)
fig.add_annotation(text=level_one_rel[4], font_color=colors[2], arrowcolor=colors[2], y=4, xref='paper', x=1.01, arrowhead=1, axref='x domain', ax=30, ayref='y domain', ay=0, font_size=14)
fig.add_annotation(text=level_two_rel[5], font_color=colors[4], arrowcolor=colors[4], y=4.9, xref='paper', x=1.01, arrowhead=1, axref='x domain', ax=30, ayref='y domain', ay=-5, font_size=14)
fig.add_annotation(text=level_three_rel[5], font_color=colors[5], arrowcolor=colors[5], y=5.1, xref='paper', x=1.01, arrowhead=1, axref='x domain', ax=30, ayref='y domain', ay=5, font_size=14)
fig.add_annotation(text=level_two_rel[6], font_color=colors[4], arrowcolor=colors[4], y=5.9, xref='paper', x=1.01, arrowhead=1, axref='x domain', ax=30, ayref='y domain', ay=-5, font_size=14)
fig.add_annotation(text=level_three_rel[6], font_color=colors[5], arrowcolor=colors[5], y=6.1, xref='paper', x=1.01, arrowhead=1, axref='x domain', ax=30, ayref='y domain', ay=5, font_size=14)
fig.add_annotation(text=level_one_rel[7], font_color=colors[2], arrowcolor=colors[2], y=7, xref='paper', x=1.01, arrowhead=1, axref='x domain', ax=30, ayref='y domain', ay=0, font_size=14)
fig.add_annotation(text=level_one_rel[8], font_color=colors[2], arrowcolor=colors[2], y=8, xref='paper', x=1.01, arrowhead=1, axref='x domain', ax=30, ayref='y domain', ay=0, font_size=14)
fig.add_annotation(text=level_one_rel[9], font_color=colors[2], arrowcolor=colors[2], y=9, xref='paper', x=1.01, arrowhead=1, axref='x domain', ax=30, ayref='y domain', ay=0, font_size=14)
fig.add_annotation(text=level_one_rel[10], font_color=colors[2], arrowcolor=colors[2], y=10, xref='paper', x=1.01, arrowhead=1, axref='x domain', ax=30, ayref='y domain', ay=0, font_size=14)
fig.add_annotation(text=level_one_rel[11], font_color=colors[2], arrowcolor=colors[2], y=11, xref='paper', x=1.01, arrowhead=1, axref='x domain', ax=30, ayref='y domain', ay=0, font_size=14)
fig.add_annotation(text=level_one_rel[12], font_color=colors[2], arrowcolor=colors[2], y=12, xref='paper', x=1.01, arrowhead=1, axref='x domain', ax=30, ayref='y domain', ay=0, font_size=14)
# fig.add_annotation(text="<0.01", textangle=-90, font_color=colors[1], arrowcolor=colors[1], y=6, xref='paper', x=0.95, arrowhead=1, axref='x domain', ax=20, ayref='y domain', ay=0, font_size=14)
# fig.add_vline(x=6.5)
fig['layout']['yaxis']['autorange'] = "reversed"
# Change the bar mode
fig.update_layout(barmode='relative')
# fig.update_yaxes(dtick=2, showgrid=True, gridwidth=1, gridcolor='grey', range=[0, 12])
fig.update_xaxes(range=[0, 100.01], showline=True, mirror=True, linewidth=1, linecolor='black', tickfont_size=14, titlefont_size=20)
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickfont_size=20)
fig.write_image('plots/plot_parallelism_generation.pdf', format='pdf')