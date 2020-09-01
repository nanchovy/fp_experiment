import pandas as pd
from matplotlib import pyplot as plt
import matplotlib as mpl
import japanize_matplotlib

line_style = ['ro-', 'gv-', 'b^-', 'k*-']

# type_list = ['plain', 'db']
type_list = ['plain']
# type_list = ['ca']
op_list = ['insert', 'delete', 'search']
op_list_j = ['挿入', '削除', '検索']
thr_list = [1, 2, 4, 8, 16] # TODO
# log_list = [33056, 65824, 131360, 262432, 524576, 1048864] # TODO
log_list = [10486048, 20971808, 41943328, 83886368] # TODO

fontsize=16
for nvhtm_type in type_list:
    for i in range(0, len(op_list)):
        for thr in thr_list:
            wait_csv = pd.read_csv('wait_' + op_list[i] + '_' + nvhtm_type + '.thr.' + str(thr) + '.csv', index_col=0);
            wait_csv.index /= 1024 * 1024
            wait_csv.index = list(map(lambda x: str(int(x)), list(wait_csv.index)))
            ax = wait_csv.plot(kind='bar', stacked=True)
            plt.xticks(rotation=0)
            plt.xlabel('ログ容量（MiB）', fontsize=fontsize)
            plt.ylabel('スレッドあたりの待ち時間（s）', fontsize=fontsize)
            # plt.ylim(top=2.5)
            plt.title('ログ容量数別の待ち時間の変化（' + op_list_j[i] + '，スレッド数' + str(thr) + '）', fontsize=fontsize)
            #plt.savefig('abort_' + nvhtm_type + '_' + op_list[i] + '_' + str(logsz) + '.png')
            ax.legend(fontsize=fontsize)
            plt.tick_params(labelsize=fontsize)
            plt.tight_layout()
            plt.savefig('wait_' + nvhtm_type + '_' + op_list[i] + '.thr.' + str(thr) + '.eps')
        plt.close('all')

#         ind = np.arange(len(thr_list))
#         for thr in thr_list:
#             thr_filter = abort_df['thread'] == thr
#             i = 0
#             for logsz in log_list:
#                 plt.bar(ind+i*bar_width, abort_df['conflict'], width=bar_width, color='r', label='conflict')
#                 btm = abort_df['conflict'].values
#                 plt.bar(ind+i*bar_width, abort_df['capacity'], width=bar_width, bottom=btm, color='r', label='capacity')
        # ab_csv.plot(kind='bar', stacked=True)
        # plt.show()
