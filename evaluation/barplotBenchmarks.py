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

def plotGraph(runtimes, benchmark):
  fig, ax = plt.subplots()

  plt.xlabel(experiment)
  plt.ylabel("Median runtime (s)")

  baseline = runtimes.pop('time_false.txt')       # baseline filename
  x, y = zip(*sorted(runtimes.items()))
  plt.bar(x, y)
  plt.axhline(y=baseline,linewidth=1, color='k')
  plt.xticks(rotation=30, ha='right')
  plt.title(benchmark)
  plt.tight_layout()
  path = './plots/' + experiment + '/'
  if not os.path.exists(path):
    os.makedirs(path)
  plt.savefig(path + benchmark + '.pdf', format = 'pdf')

# Global settings
experiment = 'chunk_size'
pathToBenchmarkSuite = './results/' + experiment
inputSize = 'tpal'

# Collect all benchmarks
benchmarks = collectBenchmarks(pathToBenchmarkSuite)

for benchmark in benchmarks:
  runtimes = {}
  time_path = pathToBenchmarkSuite + '/' + benchmark + '/' + inputSize + '/heartbeat_compiler_software_polling/'
  for f in os.listdir(time_path):
    if f.startswith("time"):
      # Calculate baseline median time
      runtimes[f] = getMedianTime(time_path + f)
  # print(runtimes)

  # Plot barplots for benchmark
  plotGraph(runtimes, benchmark)