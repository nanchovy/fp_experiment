import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import matplotlib as mpl
import japanize_matplotlib
import sys
import os
import re

colorlst = ['#fecc5c', '#fd8d3c', '#f03b20', '#bd0026']
markerlst = ['o', 'v', '^', 's']

def get_logsz_dir(path):
    files = os.listdir(path)
    logsz_dirs = []
    for name in files:
        if (os.path.isdir(os.path.join(path, name)) and re.match(r'logsz_.\d+', name)):
            logsz_dirs.append(name)
    return sorted(logsz_dirs, key=lambda s: int(s.split('logsz_')[1]))

font_size = 18
line_style = ['ro-', 'gv-', 'b^-', 'k*-']
ops = ['挿入', '検索', '削除']

def plot_graph(log_size_str, result_file1, result_file2, result_file3, result_file4, cols, memtype):
    result_df1 = pd.read_csv(result_file1 + '/result.csv', index_col=0)
    result_df2 = pd.read_csv(result_file2 + '/result.csv', index_col=0)
    result_df3 = pd.read_csv(result_file3 + '/result.csv', index_col=0)
    result_df4 = pd.read_csv(result_file4 + '/result.csv', index_col=0)

    os.makedirs(log_size_str, exist_ok=True)

    for i in range(3):
        p_df1 = result_df1.iloc[:, [i]]
        p_df2 = result_df2.iloc[:, [i]]
        p_df3 = result_df3.iloc[:, [i]]
        p_df4 = result_df4.iloc[:, [i]]
        colname = p_df1.columns[0]
        p_df1.columns = [result_file1.split('../' + memtype + '/elapsed_time/')[1]]
        p_df2.columns = [result_file2.split('../' + 'vmem' + '/elapsed_time/')[1]]
        # p_df2.columns = [result_file2.split('../' + memtype + '/elapsed_time/')[1]]
        p_df3.columns = [result_file3.split('../' + memtype + '/elapsed_time/')[1]]
        p_df4.columns = [result_file4.split('../' + memtype + '/elapsed_time/')[1]]
        result_df = pd.concat([p_df1, p_df2, p_df3, p_df4], axis=1)
        result_df.columns = ['col1', 'col2', 'col3', 'col4']
        # result_df = p_df1
        # result_df = pd.concat([p_df1, p_df3, p_df4], axis=1)
        # ax = result_df.plot.line(style=line_style)
        ax = plt.axes()
        for (col, color, marker) in zip(result_df, colorlst, markerlst):
            tmp = eval('result_df.' + col)
            tmp.plot(ax=ax, color=color, marker=marker, figsize=(6,6))
        plt.xlabel('スレッド数', fontsize=font_size)
        plt.ylabel('実行時間 (秒)', fontsize=font_size)
        plt.xlim([1, result_df.index.max()])
        # plt.ylim([0, 7])
        plt.ylim(bottom = 0)
        plt.xticks(result_df.index, result_df.index)
        plt.legend(cols, bbox_to_anchor=(0.40, -0.30), loc='center', borderaxespad=0, ncol=2, fontsize=font_size)
        plt.tick_params(labelsize=font_size)
        # plt.title('スレッド数による実行時間の変化 (' + ops[i] + ')', fontsize=font_size)
        plt.tight_layout()
        # plt.savefig(log_size_str + '/' + colname + '.png')
        plt.savefig(log_size_str + '/' + colname.split('_concurrent.exe')[0] + '.pdf')
        plt.close()

        # result_df.plot.line(style=line_style)
        # plt.xlabel('スレッド数')
        # plt.ylabel('実行時間 (秒)')
        # plt.xticks(result_df.index, result_df.index)
        # plt.xscale('log')
        # plt.yscale('log')
        # plt.title('スレッド数による実行時間の変化（' + ops[i] + '）')
        # plt.savefig(log_size_str + '/' + colname + '.result_log.png')
        # plt.savefig(log_size_str + '/' + colname + '.result_log.eps')

# result_files = sys.argv
# if (len(result_files) < 4) :
#     print("too few arguments")
#     sys.exit()

for memtype in ['pmem']: # ['vmem']: # ['pmem', 'vmem']:
    result_file_dirs_1 = get_logsz_dir('../' + memtype + '/elapsed_time/bptree_nvhtm_0/')
    # result_file_dirs_2 = get_logsz_dir('../' + memtype + '/elapsed_time/bptree_nvhtm_1/')
    result_file_dirs_2 = get_logsz_dir('../' + 'vmem' + '/elapsed_time/bptree_nvhtm_0/')
    result_file_dirs_3 = get_logsz_dir('../' + memtype + '/elapsed_time/fptree_concurrent_0/')
    result_file_dirs_4 = get_logsz_dir('../' + memtype + '/elapsed_time/bptree_concurrent_0/')

    for i in range(len(result_file_dirs_1)):
        result_file_dir1 = '../' + memtype + '/elapsed_time/bptree_nvhtm_0/' + result_file_dirs_1[i]
        # result_file_dir2 = '../' + memtype + '/elapsed_time/bptree_nvhtm_1/' + result_file_dirs_2[i]
        result_file_dir2 = '../' + 'vmem' + '/elapsed_time/bptree_nvhtm_0/' + result_file_dirs_2[i]
        result_file_dir3 = '../' + memtype + '/elapsed_time/fptree_concurrent_0/' + result_file_dirs_3[0]
        result_file_dir4 = '../' + memtype + '/elapsed_time/bptree_concurrent_0/' + result_file_dirs_4[0]
        # plot_graph('scaling/' + memtype + '/' + result_file_dirs_1[i], result_file_dir1, result_file_dir2, result_file_dir3, result_file_dir4, ["B${^+}$-Tree${_{NH}}$", "B${^+}$-Tree${_{DB}}$", "FPTree", "B${^+}$-Tree${_{C}}$"], memtype)
        # plot_graph('scaling/' + memtype + '/' + result_file_dirs_1[i], result_file_dir1, 0, result_file_dir3, result_file_dir4, ["B${^+}$-Tree${_{NH}}$", "FPTree", "B${^+}$-Tree${_{C}}$"], memtype)
        # plot_graph('scaling/' + memtype + '/' + result_file_dirs_1[i], result_file_dir1, result_file_dir2, result_file_dir3, result_file_dir4, ["B${^+}$-Tree${_{NH}}$", "B${^+}$-Tree${_{CA}}$", "FPTree", "B${^+}$-Tree${_{C}}$"], memtype)
        # plot_graph('scaling/' + memtype + '/' + result_file_dirs_1[i], result_file_dir1, result_file_dir2, result_file_dir3, result_file_dir4, ["B${^+}$-Tree${_{NH}}$", "B${^+}$-Tree${_{CA}}$", "FPTree", "B${^+}$-Tree${_{C}}$"], memtype)
        plot_graph('scaling/' + memtype + '/' + result_file_dirs_1[i], result_file_dir1, result_file_dir2, result_file_dir3, result_file_dir4, ["B${^+}$-Tree${_{NH}}$", "B${^+}$-Tree${_{NH}}$(DRAM)", "FPTree", "B${^+}$-Tree${_{C}}$"], memtype)
        # plot_graph('scaling/' + memtype + '/' + result_file_dirs_1[i], result_file_dir1, 0, 0, 0, ["B${^+}$-Tree${_{NH}}$"], memtype)
plt.close('all')
