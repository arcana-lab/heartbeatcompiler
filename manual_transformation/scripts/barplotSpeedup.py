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

def collectSpeedups(benchmark, pathToBenchmarkSuite, techniques, configs):
  speedups = {}
  for technique in techniques:
    speedups[technique] = []
    for config in configs:
      speedup = 0.0
      fileName = pathToBenchmarkSuite + '/' + benchmark + '/' + technique + '/time' + config + '.txt'
      if os.path.isfile(fileName) and getMedianTime(fileName) != 0.0:
        speedup = round(baselineTime[benchmark] / getMedianTime(fileName), 2)
      speedups[technique].append(speedup)
  
  return speedups

def getMedianTime(pathToTimeFile):
  if os.path.isfile(pathToTimeFile):
    times = []
    with open(pathToTimeFile, 'r') as f:
      for line in f:
        try:
          times.append(float(line))
        except:
          return 0.0 # take 0.0 if execution time is missing/wrong
    return statistics.median(times)

  return 0.0

def findMaxSpeedUp(speedups):
    maxSpeedUp = 1.0
    for lst in speedups.values():
        for item in lst:
            if float(item) > maxSpeedUp:
                maxSpeedUp = float(item)
    return maxSpeedUp

def plot(benchmark, speedups, techniques, configs, maxSpeedUp):
  colors = ["tomato", "darkorange", "yellowgreen", "turquoise", "cornflowerblue", "purple"]

  lineWidth = 0.5
  fontSize = 6

  fig = plt.figure()
  ax = fig.add_subplot(111)

  matplotlib.rcParams.update({'font.size': fontSize})
  ax.set_ylabel('Program Speedup', fontsize = fontSize)

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
    rects.append(ax.bar(xTicks, speedups[technique], barWidth, color = colors[i], label = name))
    xTicks = [elem + barIncrement for elem in xTicks]
    xTicksShifted = [elem - barWidth/2 - gap/2 for elem in xTicks]
    xTicksAcc.append(xTicksShifted)
    i += 1

  x = []
  for i in range(len(configs)):
    x.append(np.median([elem[i] for elem in xTicksAcc]))

  ymin = 0
  ymax = int(maxSpeedUp * 1.2)
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

  ax.legend(fontsize = fontSize - 1.5, fancybox = False, framealpha = 1, ncol = int(math.ceil(len(techniques)/3)), bbox_to_anchor=(1, 1.02), loc = 'lower right', borderpad = 0.2)

  matplotlib.rcParams['pdf.fonttype'] = 42
  matplotlib.rcParams['ps.fonttype'] = 42

  ax.set_aspect(0.3)

  plt.tight_layout()
  plt.savefig('./plots/' + benchmark + '.pdf', format = 'pdf')

# Global settings
pathToBenchmarkSuite = 'results/authors_machine/time'
techniques = ['openmp', 'openmp_dynamic', 'openmp_guided', 'opencilk', 'heartbeat_branches', 'heartbeat_versioning']
technique_names = ['OpenMP (static)', 'OpenMP (dynamic)', 'OpenMP (guided)', 'OpenCilk', 'Heartbeat (branches)', 'Heartbeat (versioning)']
configs = ['1', '2', '4', '8', '16', '28', '56']
config_names = ['1 Physical Core', '2 Physical Cores', '4 Physical Cores', '8 Physical Cores', '16 Physical Cores', '28 Physical Cores', '56 Physical Cores']

# Collect all benchmarks
benchmarks = collectBenchmarks(pathToBenchmarkSuite)

# Calculate baseline average time
baselineTime = {}
for benchmark in benchmarks:
  baselineTime[benchmark] = getMedianTime(pathToBenchmarkSuite + '/' + benchmark + '/baseline/time1.txt')
print(baselineTime)

# Plot speedup plot per benchmark
for benchmark in benchmarks:
  speedups = collectSpeedups(benchmark, pathToBenchmarkSuite, techniques, configs)
  print(benchmark, speedups)
  maxSpeedUp = findMaxSpeedUp(speedups)
  plot(benchmark, speedups, techniques, configs, maxSpeedUp)
