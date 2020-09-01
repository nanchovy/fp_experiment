import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import matplotlib as mpl

line_style = ['ro-', 'gv-', 'b^-', 'k*-']

result_df = pd.read_csv("result.csv", index_col=0)

result_df.plot.line(style=line_style)
plt.xlabel('Number of thread')
plt.ylabel('Elapsed time (sec.)')
plt.xlim([1, result_df.index.max()])
plt.ylim([0, result_df.max().max() * 1.1])
plt.xticks(result_df.index, result_df.index)
plt.savefig('result.png')
plt.savefig('result.eps')
result_df.plot.line(style=line_style)
plt.xlabel('Number of thread')
plt.ylabel('Elapsed time (sec.)')
plt.xticks(result_df.index, result_df.index)
plt.xscale('log')
plt.yscale('log')
plt.savefig('result_log.png')
plt.savefig('result_log.eps')