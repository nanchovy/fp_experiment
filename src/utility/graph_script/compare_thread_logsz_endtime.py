import pandas as pd
from matplotlib import pyplot as plt
import matplotlib as mpl
import japanize_matplotlib

colorlst = ['#fecc5c', '#fd8d3c', '#f03b20', '#bd0026']

# type_list = ['plain', 'db']
type_list = ['plain']
# type_list = ['ca']
op_list = ['insert', 'delete', 'search']
op_list_j = ['挿入', '削除', '検索']
thr_list = [1, 2, 4, 8, 16] # TODO
# log_list = [33056, 65824, 131360, 262432, 524576, 1048864] # TODO
# log_list = [2621728, 5243168, 10486048, 20971808, 41943328, 83886368] # TODO
# log_list = [10486048, 20971808, 41943328, 83886368] # TODO
log_list = [41943328] # TODO

bar_width=0.2
fontsize = 18

for nvhtm_type in type_list:
    for i in range(0, len(op_list)):
        for logsz in log_list:
            wait_csv = pd.read_csv('wait_' + op_list[i] + '_' + nvhtm_type + '.logsize.' + str(logsz) + '.csv', index_col=0);
            del(wait_csv['HTM-block'])
            del(wait_csv['Checkpoint-block'])
            del(wait_csv['Abort'])
            ax = wait_csv.plot(kind='bar', stacked=True, legend=False, color=colorlst)
            plt.xticks(rotation=0)
            plt.xlabel('スレッド数', fontsize=fontsize)
            plt.ylabel('Checkpointプロセスの終了待ち時間 (秒)', fontsize=fontsize)
            plt.tick_params(labelsize=fontsize)
            # plt.legend(fontsize=fontsize)
            plt.tight_layout()
            # plt.title('スレッド数別の終了待ち時間の変化（' + op_list_j[i] + ', ログ容量' + str((logsz - 288)/(1024 * 1024)) + 'MiB' + '）')
            #plt.savefig('abort_' + nvhtm_type + '_' + op_list[i] + '_' + str(logsz) + '.png')
            plt.savefig('endwait_' + nvhtm_type + '_' + op_list[i] + '.logsize.' + str(logsz) + '.eps')
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
