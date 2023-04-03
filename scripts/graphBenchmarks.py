import os
import numpy as np
import math
import statistics
import matplotlib
import matplotlib.ticker
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
    return statistics.median(times)

  return 0.0

def plotGraphs(benchmark_runtimes, benchmarks, heartbeat_rates):
  markers = ["+", "x", "s", "*", "^", "d", "v"]
  fig, ax = plt.subplots()

  ax.set_xscale("log")
  ax.set_xticks(sorted(heartbeat_rates))
  ax.get_xaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())
  plt.xlabel("TASKPARTS_KAPPA_USEC ( \u03BCs)")
  plt.ylabel("Median runtime (s)")

  for benchmark, m in zip(benchmark_runtimes, markers):
    runtimes = sorted(benchmark_runtimes[benchmark].items())

    x, y = zip(*runtimes)
    ax.plot(x, y, marker = m, label = benchmark)
  plt.legend()
  path = '../plots/heartbeat_rate/'
  if not os.path.exists(path):
    os.makedirs(path)
  plt.savefig('../plots/heartbeat_rate/graphs.pdf', format = 'pdf')

# Global settings
pathToBenchmarkSuite = '../results/heartbeat_rate'
inputSize = 'tpal'

# Collect all benchmarks
benchmarks = collectBenchmarks(pathToBenchmarkSuite)

# Calculate baseline median time
benchmark_runtimes = {}
heartbeat_rates = {2, 10, 30, 100, 1000, 10000, 100000}
for benchmark in benchmarks:
  runtimes = {}
  for kappa in heartbeat_rates:
    runtimes[kappa] = getMedianTime(pathToBenchmarkSuite + '/' + benchmark + '/' + inputSize + '/heartbeat_compiler_software_polling/time_' + str(kappa) + '.txt')
  benchmark_runtimes[benchmark] = runtimes
  
# print(benchmark_runtimes)

plotGraphs(benchmark_runtimes, benchmarks, heartbeat_rates)