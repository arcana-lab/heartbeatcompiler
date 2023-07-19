import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_hbc_versions.csv')
print(df)

benchmarks=list(df.loc[:,'benchmarks'])
hbc_rf_speedups=list(df.loc[:,'hbc_rf (speedup)'])
print(hbc_rf_speedups)
hbc_rf_kmod_speedups=list(df.loc[:,'hbc_rf_kmod (speedup)'])
hbc_static_speedups=list(df.loc[:,'hbc_static (speedup)'])
hbc_speedups=list(df.loc[:,'hbc (speedup)'])
hbc_rf_stdev=list(df.loc[:,'hbc_rf (stdev)'])
hbc_rf_kmod_stdev=list(df.loc[:,'hbc_rf_kmod (stdev)'])
hbc_static_stdev=list(df.loc[:,'hbc_static (stdev)'])
hbc_stdev=list(df.loc[:,'hbc (stdev)'])

print(hbc_rf_stdev)
print(hbc_rf_kmod_stdev)
print(hbc_static_stdev)
print(hbc_stdev)

fig = go.Figure(data=[
    go.Bar(name='hbc + RF Interrupt Ping Thread', x=benchmarks, y=hbc_rf_speedups, error_y=dict(type='data', array=hbc_rf_stdev), text=hbc_rf_speedups),
    go.Bar(name='hbc + RF Kernel Module', x=benchmarks, y=hbc_rf_kmod_speedups, error_y=dict(type='data', array=hbc_rf_kmod_stdev), text=hbc_rf_kmod_speedups),
    go.Bar(name='hbc + Static Chunking', x=benchmarks, y=hbc_static_speedups, error_y=dict(type='data', array=hbc_static_stdev), text=hbc_static_speedups),
    go.Bar(name='hbc + ACC', x=benchmarks, y=hbc_speedups, error_y=dict(type='data', array=hbc_stdev), text=hbc_speedups)
])
fig.update_traces(textposition='outside', textfont_size=9)
fig.update_layout(
    autosize=False,
    width=1200,
    height=500,
    margin=dict(
        pad=4
    )
)

fig.update_xaxes(tickangle=270)
fig.add_vline(x=9.5)
# Change the bar mode
fig.update_layout(barmode='group')
fig.write_image('plot_hbc_versions.pdf', format='pdf')