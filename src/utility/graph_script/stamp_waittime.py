import numpy as np
from matplotlib import pyplot as plt
import pandas as pd
import japanize_matplotlib
import sys
import os

def plot_graph(plot_dir, bench_name, result_files, plot_text, colorlst, labels, font_size, time_class):
    result_dataframes = []
    for result_file in result_files:
        result_dataframes.append(pd.read_csv(result_file, index_col=0))

    os.makedirs(plot_dir, exist_ok=True)

    xtick = np.arange(0, len(result_dataframes[0].index))
    x_position = -1 * len(result_files) / 2.0
    barwidth = 0.3
    top_of_bar = 0
    fig = plt.figure(figsize=(7, 7))
    for df in result_dataframes:
        xtick_tmp = list(map(lambda x: x + barwidth*x_position, xtick))
        # for (color, label, tc) in zip(colorlst, labels, time_class):
        for (color, tc) in zip(colorlst, time_class):
            bar_bottom = 0
            # plt.bar(xtick+barwidth*x_position, df[tc], bottom=bar_bottom, width=barwidth, edgecolor = 'k', color = color, label = label)
            plt.bar(xtick_tmp, df[tc], bottom=bar_bottom, width=barwidth, edgecolor = 'k', color = color, align='edge')
            bar_bottom += df[tc]
        top_of_bar = max([max(bar_bottom), top_of_bar])
        x_position += 1
    plt.xticks(xtick, result_dataframes[0].index)
    plt.xlabel('スレッド数', fontsize=font_size)
    # plt.xlabel('Number of Threads', fontsize=font_size)
    plt.ylabel('消費時間 (秒/スレッド)', fontsize=font_size)
    # plt.ylabel('Wasted Time (sec. / thread)', fontsize=font_size)
    plt.tick_params(labelsize=font_size)
    # plt.legend(bbox_to_anchor=(0.50, -0.20), loc='center', borderaxespad=0, ncol=2, fontsize=font_size-4)
    plt.legend(labels, bbox_to_anchor=(0.45, -0.30), loc='center', borderaxespad=0, fontsize=font_size-4)
    plt.text(-0.5, top_of_bar-(top_of_bar * 0.1), plot_text, fontsize=font_size-4)
    plt.tight_layout()
    plt.savefig(plot_dir + '/' + bench_name + '_wait_ja.pdf')
    plt.close()

def main():
    bench_names = ['genome', 'intruder', 'kmeans-high', 'kmeans-low', 'labyrinth', 'ssca2', 'vacation-high', 'vacation-low', 'yada']
    result_file_template = ['use_mmap', 'para_cp', 'log_comp']
    result_file_generator = lambda extype: 'waittime/' + extype + '/wait_' + bench_name + '.csv'
    plot_target_dir = "graphs/waittime"
    colorlst = ['#fecc5c', '#fd8d3c', '#f03b20', '#bd0026']
    labels = ['Checkpointプロセスによるログ処理の待ち時間', 'トランザクションのアボートによる消費時間', 'スレッド間同期に費やした時間']
    font_size = 18
    plot_text = '左：NVM(1スレッド), 中央：NVM(8スレッド),\n右：NVM(ログ圧縮)'
    time_class = ['Checkpoint-block', 'Abort', 'HTM-block']
    for bench_name in bench_names:
        result_files = list(map(result_file_generator, result_file_template))
        plot_graph(plot_target_dir, bench_name, result_files, plot_text, colorlst, labels, font_size, time_class)

main()
