import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import matplotlib as mpl
import japanize_matplotlib
import sys
import os
import re

font_size = 18
operations = ["insert", "search", "delete"]
threads = [1, 2, 4, 8, 16]

def get_logsz_dir(path):
    files = os.listdir(path)
    logsz_dirs = []
    for f in files:
        if (os.path.isdir(os.path.join(path, f)) and re.match(r'logsz_.\d+', f)):
            logsz_dirs.append(path + f)
    return sorted(logsz_dirs, key=lambda s: int(s.split('logsz_')[1]))

line_style = ['ro-', 'gv-', 'b^-', 'k*-']

# result_file_dirs = sys.argv[1:]
# if (len(result_file_dirs) < 2):
#     print("too few arguments")
#     sys.exit()

# result_file_dirs_1 = get_logsz_dir(result_file_dirs[0])
# result_file_dirs_2 = get_logsz_dir(result_file_dirs[1])

# for memtype in ['pmem', 'vmem']:
for memtype in ['pmem']:
    result_file_dirs_1 = get_logsz_dir('../' + memtype + '/elapsed_time/bptree_nvhtm_0/')
    # result_file_dirs_2 = get_logsz_dir('../' + memtype + '/elapsed_time/bptree_nvhtm_1/')

    # print(result_file_dirs_1)
    # print(result_file_dirs_2)

    log_sizes = []
    csvs1 = []
    for res_dirname in result_file_dirs_1:
        log_sizes.append(float(os.path.basename(res_dirname).strip("logsz_"))/(1024*1024))
        csvs1.append(pd.read_csv(res_dirname + "/result.csv", index_col=0))
    # csvs2 = []
    # for res_dirname in result_file_dirs_2:
    #     csvs2.append(pd.read_csv(res_dirname + "/result.csv", index_col=0))
    # print(log_sizes)

    # print(csvs1)
    # print(csvs2)

    for thr in range(len(threads)):
        for i in range(3):
            results = []
            for j in range(len(log_sizes)):
                results.append([])
                results[j].append(csvs1[j].iat[thr,i])
            # for j in range (len(log_sizes)):
            #    results[j].append(csvs2[j].iat[thr,i])
            # print(results)
            # df = pd.DataFrame(results, index=log_sizes, columns=['bptree-nvhtm', 'bptree-nvhtm-2buff']);
            df = pd.DataFrame(results, index=log_sizes, columns=['bptree-nvhtm']);
            # print(df)
            ax = df.plot()
            plt.xlabel('ログサイズ (MB)', fontsize=font_size)
            plt.ylabel('実行時間 (秒)', fontsize=font_size)
            plt.title('ログサイズによる実行時間の変化', fontsize=font_size)
            # ax.legend(['B${^+}$-Tree${_{NH}}$', "B${^+}$-Tree${_{DB}}$"], fontsize=font_size)
            ax.legend(['B${^+}$-Tree${_{NH}}$', "B${^+}$-Tree${_{CA}}$"], fontsize=font_size)
            # plt.ylim(bottom=0, top=2.5)
            plt.ylim(bottom=0)
            plt.tick_params(labelsize=font_size)
            plt.tight_layout()
            plt.savefig('logsize/' + memtype + '/' + operations[i] + '_result_logsize.' + str(threads[thr]) + '.png')
            plt.savefig('logsize/' + memtype + '/' + operations[i] + '_result_logsize.' + str(threads[thr]) + '.eps')
        plt.close('all')
    # for i in range(3):
    #     p_df1 = result_df1.iloc[:, [i]]
    #     p_df2 = result_df2.iloc[:, [i]]
    #     p_df3 = result_df3.iloc[:, [i]]
    #     colname = p_df1.columns[0]
    #     p_df1.columns = [result_files[1]]
    #     p_df2.columns = [result_files[2]]
    #     result_df = pd.concat([p_df1, p_df2, p_df3], axis=1)
    #     result_df.plot.line(style=line_style)
    #     plt.xlabel('Log size (MB)')
    #     plt.ylabel('Elapsed time (sec.)')
    #     plt.xlim([1, result_df.index.max()])
    #     plt.ylim([0, result_df.max().max() * 1.1])
    #     plt.xticks(result_df.index, result_df.index)
    #     plt.savefig(colname + '.png')
    #     plt.savefig(colname + '.eps')
    # 
    #     result_df.plot.line(style=line_style)
    #     plt.xlabel('Number of thread')
    #     plt.ylabel('Elapsed time (sec.)')
    #     plt.xticks(result_df.index, result_df.index)
    #     plt.xscale('log')
    #     plt.yscale('log')
    #     plt.savefig('result_log.png')
    #     plt.savefig('result_log.eps')
