import plotly.graph_objects as go
import plotly.io as pio
import pandas as pd
import math

pio.kaleido.scope.mathjax = None

df = pd.read_csv('data_overhead.csv')
print(df)

benchmarks=list(df.loc[:,'benchmarks'])
tpal_environment_overhead=list(df.loc[:,'tpal_environment_overhead'])
outline_overhead=list(df.loc[:,'outline_overhead'])
closure_generation_overhead=list(df.loc[:,'closure_generation_overhead'])
loop_chunking_overhead=list(df.loc[:,'loop_chunking_overhead'])
promotion_insertion_overhead=list(df.loc[:,'promotion_insertion_overhead'])
chunksize_transferring_overhead=list(df.loc[:,'aca_chunksize_transferring_overhead'])
ac_polling_overhead=list(df.loc[:,'ac_polling_overhead'])
rf_kmod_overhead=list(df.loc[:,'rf_kmod_overhead'])

def base(overheads, y):
    base = []
    for i, val in enumerate(y):
        if val < 0:
            base.append(sum([l[i] if l[i] < 0 else 0 for l in overheads]))
        else:
            base.append(sum([l[i] if l[i] >= 0 else 0 for l in overheads]))
    return base

o = base([outline_overhead], closure_generation_overhead)
oc = base([outline_overhead, closure_generation_overhead], loop_chunking_overhead)
ocl = base([outline_overhead, closure_generation_overhead, loop_chunking_overhead], promotion_insertion_overhead)
oclp = base([outline_overhead, closure_generation_overhead, loop_chunking_overhead, promotion_insertion_overhead], chunksize_transferring_overhead)
oclpc = base([outline_overhead, closure_generation_overhead, loop_chunking_overhead, promotion_insertion_overhead, chunksize_transferring_overhead], ac_polling_overhead)

print(o)
print(oc)
print(ocl)
print(oclp)
print(oclpc)

print(benchmarks)

# https://colorbrewer2.org/#type=diverging&scheme=RdYlBu&n=6
# colors = ['#d73027','#fc8d59','#fee090','#e0f3f8','#91bfdb','#4575b4']
# others = ['#762a83','#af8dc3']
colors = ['#fee5d9','#fcbba1','#fc9272','#fb6a4a','#de2d26','#a50f15']      # reds
others = ['#4575b4','#91bfdb']

# Create the bar chart
fig = go.Figure(data=[
        go.Bar(orientation='h', y=benchmarks, x=tpal_environment_overhead, name='Closure generation', marker_color=others[0], offsetgroup=0, legendgroup="tpal", legendgrouptitle_text="TPAL"),
        go.Bar(orientation='h', y=benchmarks, x=rf_kmod_overhead, name='Kernel module', marker_color=others[1], offsetgroup=1, legendgroup="hbc1", legendgrouptitle_text="HBC (interrupt-based)"),
        go.Bar(orientation='h', y=benchmarks, x=outline_overhead, name='Loop outlining', marker_color=colors[0], offsetgroup=2, legendgroup="hbc2", legendgrouptitle_text="HBC (software polling)"),
        go.Bar(orientation='h', y=benchmarks, x=closure_generation_overhead, name='Closure generation', marker_color=colors[1], offsetgroup=2, base=o, legendgroup="hbc2", legendgrouptitle_text="HBC (software polling)"),
        go.Bar(orientation='h', y=benchmarks, x=loop_chunking_overhead, name='Loop chunking transformation', marker_color=colors[2], offsetgroup=2, base=oc, legendgroup="hbc2", legendgrouptitle_text="HBC (software polling)"),
        go.Bar(orientation='h', y=benchmarks, x=promotion_insertion_overhead, name='Promotion insertion', marker_color=colors[3], offsetgroup=2, base=ocl, legendgroup="hbc2"),
        go.Bar(orientation='h', y=benchmarks, x=chunksize_transferring_overhead, name='Chunksize transferring', marker_color=colors[4], offsetgroup=2, base=oclp, legendgroup="hbc2"),
        go.Bar(orientation='h', y=benchmarks, x=ac_polling_overhead, name='AC polling overhead', marker_color=colors[5], offsetgroup=2, base=oclpc, legendgroup="hbc2")
    ])

# Move legend
fig.update_layout(
    # showlegend=False,
    legend=dict(
        # orientation='h',
        tracegroupgap=0,
        grouptitlefont_size=16,
        font_size=14,
        # yanchor="middle",
        # y=0.36,
        # xanchor="right",
        # x=1.5,
        bgcolor="#e8e8e8",
        bordercolor="Black",
        borderwidth=1),
    xaxis_title="Overhead over Baseline (%)",
    plot_bgcolor="white",
    height=300,
    width=800,
    margin=dict(l=0,r=0,b=0,t=0)
)

# fig.update_traces(width=1)

# Add lines
for i in range(0, len(benchmarks) - 1):
    fig.add_hline(y=i+0.5, line_width=1, line_color='grey')

# fig.add_annotation(text=str(ocplc[1] + ac_polling_overhead[1]), font_size=14, xref='paper', x=1, yref='paper', y=0.765, axref='x domain', ax=-35, ayref='y domain', ay=-10)

fig['layout']['yaxis']['autorange'] = "reversed"
# fig.update_layout(barmode='relative')
fig.update_xaxes(range=[-10, 65], dtick=10, showgrid=True, gridwidth=1, gridcolor='grey', zerolinecolor='grey', zerolinewidth=1)
fig.update_yaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickfont_size=16)
# fig.update_yaxes(tickangle=-20, ticks="outside")
fig.update_xaxes(showline=True, mirror=True, linewidth=1, linecolor='black', tickfont_size=14, titlefont_size=20)
fig.write_image('plots/plot_overhead.pdf', format='pdf')