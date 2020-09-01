import pandas as pd
from matplotlib import pyplot as plt
import matplotlib as mpl
import japanize_matplotlib

colorlst = ['#fecc5c', '#fd8d3c', '#f03b20', '#bd0026']

# type_list = ['bptree_nvhtm_0', 'bptree_nvhtm_1']
type_list = ['bptree_nvhtm_0']
# type_list = ['bptree_nvhtm_1']
op_list = ['insert', 'delete', 'search']
op_list_j = ['挿入', '削除', '検索']
thr_list = [1, 2, 4, 8, 16] # TODO
# log_list = [33056, 65824, 131360, 262432, 524576, 1048864] # TODO
# log_list = [2621728, 5243168, 10486048, 20971808, 41943328, 83886368] # TODO
# log_list = [10486048, 20971808, 41943328, 83886368] # TODO
log_list = [41943328] # TODO
mem_list = ['pmem', 'vmem']
font_size = 18

for memtype in mem_list:
    for nvhtm_type in type_list:
        for i in range(0, len(op_list)):
            ab_csv = pd.read_csv('abort/' + memtype + '/' + nvhtm_type + '/abort_' + op_list[i] + '.csv', index_col=1, usecols=[1,2,3,4,5,6]);
            max_val = max(ab_csv['abort'])
            print(max_val)
            others = ab_csv['abort'] - ab_csv['conflict'] - ab_csv['capacity'] - ab_csv['explicit']
            others.name = 'other'
            abort_df = pd.concat([ab_csv, others], axis=1)
            for logsz in log_list:
                tmp_df = abort_df[abort_df['logsize'] == logsz]
                del tmp_df['abort']
                del tmp_df['logsize']
                ax = tmp_df.plot(kind='bar', stacked=True, color=colorlst, edgecolor='k')
                # if i == 0:
                #     plt.ylim(top=2200000)
                # else:
                #     plt.ylim(top=1500000)
                ax.yaxis.offsetText.set_fontsize(font_size)
                ax.ticklabel_format(style="sci", axis="y", scilimits=(0,0))
                plt.xlabel('スレッド数', fontsize=font_size)
                plt.ylabel('総アボート回数', fontsize=font_size)
                plt.tick_params(labelsize=font_size)
                plt.xticks(rotation=0)
                plt.legend(fontsize=font_size)
                plt.tight_layout()
                # plt.title('スレッド数によるアボート回数の変化（' + op_list_j[i] + '）')
                #plt.savefig('abort_' + nvhtm_type + '_' + op_list[i] + '_' + str(logsz) + '.png')
                plt.savefig('abort/' + memtype + '/' + nvhtm_type + '/total_abort_' + op_list[i] + '_logsz_' + str(logsz) + '.pdf')
                plt.close()

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
