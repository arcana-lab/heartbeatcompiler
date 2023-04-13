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

def collectSpeedups(benchmark, pathToBenchmarkSuite, inputSize, techniques):
  speedups = {}
  for technique in techniques:
    speedups[technique] = {}
    medianSpeedup = 0.0
    minSpeedup = 0.0
    maxSpeedup = 0.0
    stdev = 0.0
    fileName = pathToBenchmarkSuite + '/' + benchmark + '/' + inputSize + '/' + technique + '/time_64.txt'
    if os.path.isfile(fileName) and getMedianTime(fileName) != 0.0:
      medianSpeedup = round(baselineTime[benchmark] / getMedianTime(fileName), 2)
      minSpeedup = round(baselineTime[benchmark] / getMaxTime(fileName), 2)
      maxSpeedup = round(baselineTime[benchmark] / getMinTime(fileName), 2)
      stdev = getStdevTime(fileName, benchmark)
    speedups[technique]['median'] = medianSpeedup
    speedups[technique]['min'] = minSpeedup
    speedups[technique]['max'] = maxSpeedup
    speedups[technique]['stdev'] = stdev

  return speedups

def findMaxSpeedUp(speedups):
  maxSpeedUp = 1.0
  for dic in speedups.values():
      for lst in dic.values():
        for item in lst.values():
            if float(item) > maxSpeedUp:
              maxSpeedUp = float(item)

  return maxSpeedUp


def plot(benchmarks, inputSize, speedups, techniques, maxSpeedUp):
  lineWidth = 0.5
  fontSize = 6

  fig = plt.figure()
  ax = fig.add_subplot(111)

  matplotlib.rcParams.update({'font.size': fontSize})
  ax.set_ylabel('Program Speedup', fontsize = fontSize)

  gap = 0.02
  numBars = float(len(techniques))
  barWidth = 0.6/(numBars + numBars*gap)
  barIncrement = barWidth + gap
  xTicks = range(len(benchmarks))
  xTicksShifted = [elem + barWidth/2 - gap/2 for elem in xTicks]
  xTicksAcc = []
  xTicksAcc.append(xTicksShifted)
  rects = []

  benchmarkSpeedups = {}
  benchmarkStdevs = {}
  for benchmark in benchmarks:
    for technique in techniques:
      benchmarkSpeedups[technique] = []
      benchmarkStdevs[technique] = []
  for benchmark in benchmarks:
    for technique in techniques:
      benchmarkSpeedups[technique].append(speedups[benchmark][technique]['median'])
      benchmarkStdevs[technique].append(speedups[benchmark][technique]['stdev'])

  i = 0
  for technique, name in zip(techniques, technique_names):
    rects.append(ax.bar(xTicks, benchmarkSpeedups[technique], yerr=benchmarkStdevs[technique], width=barWidth, color = colors[i], label = name))
    xTicks = [elem + barIncrement for elem in xTicks]
    xTicksShifted = [elem - barWidth/2 - gap/2 for elem in xTicks]
    xTicksAcc.append(xTicksShifted)
    i += 1

  x = []
  for i in range(len(benchmarks)):
    x.append(np.median([elem[i] for elem in xTicksAcc]))

  ymin = 0
  ymax = int(maxSpeedUp * 1.1)
  # ymax = 17
  ystep = 2

  plt.xticks(x, benchmarks, fontsize = 5, rotation = 45, ha = 'right')
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
  plt.savefig('plots/scaling/all_benchmarks_' + inputSize + '.pdf', format = 'pdf')

# Global settings
pathToBenchmarkSuite = 'results/scaling'
inputSize = 'benchmarking'
# https://matplotlib.org/stable/gallery/color/named_colors.html
colors = [
  'tomato',
  'turquoise',
  'cornflowerblue',
  'mediumpurple',
  'purple',
  'pink',
  'gold',
  'orchid'
]
techniques = [
  'openmp',
  # 'opencilk',
  'heartbeat_compiler_rollforward',
  'heartbeat_compiler_kmod',
  'heartbeat_compiler_software_polling',
]
technique_names = [
  'OpenMP',
  # 'OpenCilk',
  'HBC (Rollforward)',
  'HBC (Kernel Module)',
  'HBC (Software Polling)',
]

# Collect all benchmarks
benchmarks = sorted(collectBenchmarks(pathToBenchmarkSuite))

# Calculate baseline median time
baselineTime = {}
for benchmark in benchmarks:
  baselineTime[benchmark] = getMedianTime(pathToBenchmarkSuite + '/' + benchmark + '/' + inputSize + '/baseline/time.txt')
# print(baselineTime)

# Plot speedup plot per benchmark
speedups = {}
for benchmark in benchmarks:
  speedups[benchmark] = collectSpeedups(benchmark, pathToBenchmarkSuite, inputSize, techniques)
# print(speedups)

maxSpeedUp = findMaxSpeedUp(speedups)
# print(maxSpeedUp)
plot(benchmarks, inputSize, speedups, techniques, maxSpeedUp)
