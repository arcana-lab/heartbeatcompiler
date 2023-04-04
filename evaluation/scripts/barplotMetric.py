import os
import sys
import numpy as np
import math
import statistics
import matplotlib
import matplotlib.pyplot as plt

def collectBenchmarks(pathToBenchmarkSuite):
  benchmarks = []
  for benchmark in os.listdir(pathToBenchmarkSuite):
    benchmarks.append(benchmark)

  return benchmarks

def getMedianMetric(pathToMetricFile):
  if os.path.isfile(pathToMetricFile):
    metrics = []
    with open(pathToMetricFile, 'r') as f:
      for line in f:
        try:
          metrics.append(int(line))
        except:
          return 0 # take 0 if metric is missing/wrong
    return statistics.median(metrics)

  return 0

def getMinMetric(pathToMetricFile):
  if os.path.isfile(pathToMetricFile):
    metrics = []
    with open(pathToMetricFile, 'r') as f:
      for line in f:
        try:
          metrics.append(int(line))
        except:
          return 0 # take 0 if metric is missing/wrong
    return min(metrics)

  return 0

def getMaxMetric(pathToMetricFile):
  if os.path.isfile(pathToMetricFile):
    metrics = []
    with open(pathToMetricFile, 'r') as f:
      for line in f:
        try:
          metrics.append(int(line))
        except:
          return 0 # take 0 if metric is missing/wrong
    return max(metrics)

  return 0

def getStdevMetric(pathToMetricFile, benchmark):
  if os.path.isfile(pathToMetricFile):
    metrics = []
    with open(pathToMetricFile, 'r') as f:
      for line in f:
        try:
          metrics.append(int(line))
        except:
          return 0.0 # take 0.0 if metric is missing/wrong
    return round(statistics.stdev([metric / baselineMetric[benchmark] for metric in metrics]), 4)

  return 0.0

def collectIncreases(benchmark, pathToBenchmarkSuite, techniques, configs):
  increases = {}
  for technique in techniques:
    increases[technique] = {}
    increases[technique]['median'] = []
    increases[technique]['min'] = []
    increases[technique]['max'] = []
    increases[technique]['stdev'] = []
    for config in configs:
      medianIncrease = 0.0
      minIncrease = 0.0
      maxIncrease = 0.0
      stdev = 0.0
      fileName = pathToBenchmarkSuite + '/' + benchmark + '/' + technique + '/' + metric + config + '.txt'
      if os.path.isfile(fileName) and getMedianMetric(fileName) != 0:
        medianIncrease = round(getMedianMetric(fileName) / baselineMetric[benchmark], 2)
        minIncrease = round(getMaxMetric(fileName) / baselineMetric[benchmark], 2)
        maxIncrease = round(getMinMetric(fileName) / baselineMetric[benchmark], 2)
        stdev = getStdevMetric(fileName, benchmark)
      increases[technique]['median'].append(medianIncrease)
      increases[technique]['min'].append(minIncrease)
      increases[technique]['max'].append(maxIncrease)
      increases[technique]['stdev'].append(stdev)

  return increases

def findMaxIncrease(increases):
  maxIncrease = 1.0
  for dic in increases.values():
      for lst in dic.values():
        for item in lst:
            if int(item) > maxIncrease:
              maxIncrease = int(item)

  return maxIncrease

def plot(benchmark, increases, techniques, configs, maxIncrease):
  lineWidth = 0.5
  fontSize = 6

  fig = plt.figure()
  ax = fig.add_subplot(111)

  matplotlib.rcParams.update({'font.size': fontSize})
  ax.set_ylabel(metric + ' increase (' + benchmark + ')', fontsize = fontSize)

  gap = 0.02
  numBars = float(len(techniques))
  barWidth = 0.5/(numBars + numBars*gap)
  barIncrement = barWidth + gap
  xTicks = range(len(configs))
  xTicksShifted = [elem - barWidth/2 - gap/2 for elem in xTicks]
  xTicksAcc = []
  xTicksAcc.append(xTicksShifted)
  rects = []
  i = 0
  for technique, name in zip(techniques, technique_names):
    rects.append(ax.bar(xTicks, increases[technique]['median'], yerr=increases[technique]['stdev'], width=barWidth, color = colors[i], label = name))
    xTicks = [elem + barIncrement for elem in xTicks]
    xTicksShifted = [elem - barWidth/2 - gap/2 for elem in xTicks]
    xTicksAcc.append(xTicksShifted)
    i += 1

  x = []
  for i in range(len(configs)):
    x.append(np.median([elem[i] for elem in xTicksAcc]))

  ymin = 0
  ymax = int(maxIncrease * 2)
  ystep = 2

  plt.xticks(x, config_names, fontsize = 5, rotation = 45, ha = 'right')
  plt.yticks(range(ymin, ymax + 1, ystep), fontsize = fontSize)

  mul = 50
  xmin = xTicksAcc[0][0] - gap*mul
  xmax = xTicksAcc[-1][-1] + gap*mul

  ax.set_xlim(xmin = xmin, xmax = xmax)
  ax.set_ylim(ymin = ymin, ymax = ymax + 1)

  ax.plot([xmin, xmax], [1, 1], '-', color = 'red', linewidth = lineWidth + 0.1, zorder = 0)

  ax.yaxis.grid(True, color = 'gray', ls = '--', lw = lineWidth)
  ax.set_axisbelow(True)

  ax.tick_params(axis = 'x', direction = 'out', top = False)

  ax.legend(fontsize = fontSize - 1.5, fancybox = False, framealpha = 1, ncol = int(math.ceil(len(techniques)/3)), loc = 'upper right', borderpad = 0.2)

  matplotlib.rcParams['pdf.fonttype'] = 42
  matplotlib.rcParams['ps.fonttype'] = 42

  ax.set_aspect(0.3)

  plt.tight_layout()
  plt.savefig('./plots/' + metric + '/' + benchmark + '.pdf', format = 'pdf')

# Global settings
metric = sys.argv[1]
pathToBenchmarkSuite = 'results/' + metric
colors = ["tomato", "darkorange", "yellowgreen", "turquoise", "cornflowerblue", "purple", "pink"]
techniques = ['opencilk', 'openmp', 'openmp_dynamic', 'openmp_guided', 'heartbeat_versioning', 'heartbeat_versioning_optimized', 'heartbeat_branches']
technique_names = ['OpenCilk', 'OpenMP (Static)', 'OpenMP (Dynamic)', 'OpenMP (Guided)', 'Heartbeat (versioning)', 'Heartbeat (versioning optimized)', 'Heartbeat (branches)']
configs = ['1', '2', '4', '8', '16']
config_names = ['1 Physical Core', '2 Physical Cores', '4 Physical Cores', '8 Physical Cores', '16 Physical Cores']

# Collect all benchmarks
benchmarks = collectBenchmarks(pathToBenchmarkSuite)

# Calculate baseline median metric
baselineMetric = {}
for benchmark in benchmarks:
  baselineMetric[benchmark] = getMedianMetric(pathToBenchmarkSuite + '/' + benchmark + '/baseline/' + metric + '1.txt')
print(baselineMetric)

# Plot increase plot per benchmark
for benchmark in benchmarks:
  increases = collectIncreases(benchmark, pathToBenchmarkSuite, techniques, configs)
  print(benchmark, increases)
  maxIncrease = findMaxIncrease(increases)
  plot(benchmark, increases, techniques, configs, maxIncrease)
