import os
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

def getMedianTime(pathToTimeFile):
  if os.path.isfile(pathToTimeFile):
    times = []
    with open(pathToTimeFile, 'r') as f:
      for line in f:
        try:
          times.append(float(line))
        except:
          return 0.0 # take 0.0 if execution time is missing/wrong
    return statistics.median(times) if (len(times) > 0) else 0.0

  return 0.0

def getMinTime(pathToTimeFile):
  if os.path.isfile(pathToTimeFile):
    times = []
    with open(pathToTimeFile, 'r') as f:
      for line in f:
        try:
          times.append(float(line))
        except:
          return 0.0 # take 0.0 if execution time is missing/wrong
    return min(times) if (len(times) > 0) else 0.0

  return 0.0

def getMaxTime(pathToTimeFile):
  if os.path.isfile(pathToTimeFile):
    times = []
    with open(pathToTimeFile, 'r') as f:
      for line in f:
        try:
          times.append(float(line))
        except:
          return 0.0 # take 0.0 if execution time is missing/wrong
    return max(times) if (len(times) > 0) else 0.0

  return 0.0

def getStdevTime(pathToTimeFile, benchmark):
  if os.path.isfile(pathToTimeFile):
    times = []
    with open(pathToTimeFile, 'r') as f:
      for line in f:
        try:
          times.append(float(line))
        except:
          return 0.0 # take 0.0 if execution time is missing/wrong
    return round(statistics.stdev([baselineTime[benchmark] / time for time in times]), 4)

  return 0.0

def collectSpeedups(benchmark, pathToBenchmarkSuite, inputSize, techniques, configs):
  speedups = {}
  for technique in techniques:
    speedups[technique] = {}
    speedups[technique]['median'] = []
    speedups[technique]['min'] = []
    speedups[technique]['max'] = []
    speedups[technique]['stdev'] = []
    for config in configs:
      medianSpeedup = 0.0
      minSpeedup = 0.0
      maxSpeedup = 0.0
      stdev = 0.0
      fileName = pathToBenchmarkSuite + '/' + benchmark + '/' + inputSize + '/' + technique + '/time_' + config + '.txt'
      if os.path.isfile(fileName) and getMedianTime(fileName) != 0.0:
        medianSpeedup = round(baselineTime[benchmark] / getMedianTime(fileName), 2)
        minSpeedup = round(baselineTime[benchmark] / getMaxTime(fileName), 2)
        maxSpeedup = round(baselineTime[benchmark] / getMinTime(fileName), 2)
        stdev = getStdevTime(fileName, benchmark)
      speedups[technique]['median'].append(medianSpeedup)
      speedups[technique]['min'].append(minSpeedup)
      speedups[technique]['max'].append(maxSpeedup)
      speedups[technique]['stdev'].append(stdev)

  return speedups

def findMaxSpeedUp(speedups):
  maxSpeedUp = 1.0
  for dic in speedups.values():
      for lst in dic.values():
        for item in lst:
            if float(item) > maxSpeedUp:
              maxSpeedUp = float(item)

  return maxSpeedUp

def plot(benchmark, inputSize, speedups, techniques, configs, maxSpeedUp):
  lineWidth = 0.5
  fontSize = 6

  fig = plt.figure()
  ax = fig.add_subplot(111)

  matplotlib.rcParams.update({'font.size': fontSize})
  ax.set_ylabel('Program Speedup (' + benchmark + ')', fontsize = fontSize)

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
    rects.append(ax.bar(xTicks, speedups[technique]['median'], yerr=speedups[technique]['stdev'], width=barWidth, color = colors[i], label = name))
    xTicks = [elem + barIncrement for elem in xTicks]
    xTicksShifted = [elem - barWidth/2 - gap/2 for elem in xTicks]
    xTicksAcc.append(xTicksShifted)
    i += 1

  x = []
  for i in range(len(configs)):
    x.append(np.median([elem[i] for elem in xTicksAcc]))

  ymin = 0
  ymax = int(maxSpeedUp * 1.2)
  # ymax = 17
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
  plt.savefig('plots/scaling/' + benchmark + '_' + inputSize + '.pdf', format = 'pdf')

# Global settings
pathToBenchmarkSuite = 'results/scaling'
inputSize = 'tpal'
# https://matplotlib.org/stable/gallery/color/named_colors.html
colors = [
  'tomato',
  'turquoise',
  'cornflowerblue',
  'mediumpurple',
  'purple',
  'pink'
]
techniques = [
  'openmp',
  'opencilk',
  'heartbeat_compiler_software_polling',
  'heartbeat_compiler_rollforward'
]
technique_names = [
  'OpenMP',
  'OpenCilk',
  'Heartbeat Compiler (Software Polling)',
  'Heartbeat Compiler (Rollforward)'
]
configs = ['1', '2', '4', '8', '16', '28', '56']
config_names = ['1 Core', '2 Cores', '4 Cores', '8 Cores', '16 Cores', '28 Cores', '56 Cores']

# Collect all benchmarks
benchmarks = collectBenchmarks(pathToBenchmarkSuite)

# Calculate baseline median time
baselineTime = {}
for benchmark in benchmarks:
  baselineTime[benchmark] = getMedianTime(pathToBenchmarkSuite + '/' + benchmark + '/' + inputSize + '/baseline/time.txt')
print(baselineTime)

# Plot speedup plot per benchmark
for benchmark in benchmarks:
  speedups = collectSpeedups(benchmark, pathToBenchmarkSuite, inputSize, techniques, configs)
  print(benchmark, speedups)
  maxSpeedUp = findMaxSpeedUp(speedups)
  plot(benchmark, inputSize, speedups, techniques, configs, maxSpeedUp)