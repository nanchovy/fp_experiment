import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import matplotlib as mpl
import japanize_matplotlib
import sys
import os
import re

def plot_graph(plot_dir, bench_name, result_files, ledgends, colorlst, markerlst, font_size):
    result_dataframes = []
    for result_file in result_files:
        result_dataframes.append(pd.read_csv(result_file, index_col=0))

    os.makedirs(plot_dir, exist_ok=True)

    ax = plt.axes()
    for (df, color, marker) in zip(result_dataframes, colorlst, markerlst):
        df.plot(ax=ax, color=color, marker=marker, figsize=(8,6))
    plt.xlabel('スレッド数', fontsize=font_size)
    # plt.xlabel('Number of Threads', fontsize=font_size)
    plt.ylabel('実行時間 (秒)', fontsize=font_size)
    # plt.ylabel('Execution Time (sec.)', fontsize=font_size)
    # plt.xlim([1, result_df.index.max()])
    # plt.ylim([0, 7])
    plt.ylim(bottom = 0)
    # plt.xticks(result_df.index, result_df.index)
    plt.legend(ledgends, bbox_to_anchor=(0.40, -0.30), loc='center', borderaxespad=0, ncol=2, fontsize=font_size)
    plt.tick_params(labelsize=font_size)
    # plt.title('スレッド数による実行時間の変化 (' + ops[i] + ')', fontsize=font_size)
    plt.tight_layout()
    # plt.savefig(log_size_str + '/' + colname + '.png')
    plt.savefig(plot_dir + '/' + bench_name + '_threads_ja.pdf')
    plt.close()


def main():
    bench_names = ['genome', 'intruder', 'kmeans-high', 'kmeans-low', 'labyrinth', 'ssca2', 'vacation-high', 'vacation-low', 'yada']
    # result_file_template = ['use_mmap', 'para_cp', 'log_comp', 'emulator']
    result_file_template = ['use_mmap', 'para_cp', 'log_comp']
    result_file_generator = lambda extype: 'scaling/' + extype + '/time_' + bench_name + '.csv'
    plot_target_dir = "graphs/scaling"
    colorlst = ['#fecc5c', '#fd8d3c', '#f03b20', '#bd0026']
    markerlst = ['o', 'v', '^', 's']
    font_size = 18
    # ledgends = ['NVM(1スレッド)', 'NVM(8スレッド)', 'NVM(ログ圧縮)', 'エミュレータ(1スレッド)']
    ledgends = ['NVM(1スレッド)', 'NVM(8スレッド)', 'NVM(ログ圧縮)']
    for bench_name in bench_names:
        result_files = list(map(result_file_generator, result_file_template))
        plot_graph(plot_target_dir, bench_name, result_files, ledgends, colorlst, markerlst, font_size)

main()
