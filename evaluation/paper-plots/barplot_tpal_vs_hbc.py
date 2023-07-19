import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_tpal_vs_hbc.csv')
print(df)

benchmarks=list(df.loc[:,'benchmarks'])
tpal_speedups=list(df.loc[:,'tpal (speedup)'])
print(tpal_speedups)
hbc_speedups=list(df.loc[:,'hbc (speedup)'])
print(hbc_speedups)
tpal_stdev=[i for i in df.loc[:,'tpal (stdev)'] if i == i]
print(tpal_stdev)
hbc_stdev=[i for i in df.loc[:,'hbc (stdev)'] if i == i]
print(hbc_stdev)

fig = go.Figure(data=[
    go.Bar(name='tpal', x=benchmarks, y=tpal_speedups, error_y=dict(type='data', array=tpal_stdev), text=tpal_speedups),
    go.Bar(name='hbc', x=benchmarks, y=hbc_speedups, error_y=dict(type='data', array=hbc_stdev), text=hbc_speedups)
])
fig.update_traces(textposition='outside', textfont_size=9)
# i=0
# text_height = 5
# for x_val in benchmarks:
#     fig.data[0].add_annotation(
#         x=x_val,
#         y=text_height, 
#         text=str(tpal_speedups[i]),
#         showarrow=False,
#         font=(dict(color='white'))
#     )
#     i = i+1
fig.update_xaxes(tickangle=270)
fig.add_vline(x=6.5)

fig.update_layout(barmode='group')
fig.write_image('plot_tpal_vs_hbc.pdf', format='pdf')