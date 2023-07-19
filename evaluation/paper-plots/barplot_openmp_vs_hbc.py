import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd

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

fig = go.Figure(data=[
    go.Bar(name='OpenMP (best scheduling)', x=benchmarks, y=openmp_best_speedups, error_y=dict(type='data', array=openmp_best_stdev), text=openmp_best_speedups),
    go.Bar(name='hbc', x=benchmarks, y=hbc_speedups, error_y=dict(type='data', array=hbc_stdev), text=hbc_speedups)
])
fig.update_traces(textposition='outside', textfont_size=9)
# fig.update_layout(
#     autosize=False,
#     width=1200,
#     height=500,
#     margin=dict(
#         pad=4
#     )
# )

fig.update_xaxes(tickangle=270)
fig.add_vline(x=9.5)
# Change the bar mode
fig.update_layout(barmode='group')
fig.write_image('plot_openmp_vs_hbc.pdf', format='pdf')