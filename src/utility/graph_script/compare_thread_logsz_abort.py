import pandas as pd
from matplotlib import pyplot as plt
import matplotlib as mpl
import japanize_matplotlib

colorlst = ['#fecc5c', '#fd8d3c', '#f03b20', '#bd0026']

type_list = ['fptree_concurrent_', 'bptree_concurrent_']
op_list = ['insert', 'delete', 'search']
op_list_j = ['挿入', '削除', '検索']
thr_list = [1, 2, 4, 8, 16] # TODO
log_list = [65824] # TODO
# memtypes = ['pmem', 'vmem']
memtypes = ['pmem']
font_size = 18

for mem in memtypes:
    for tree_type in type_list:
        if mem == 'vmem' and tree_type == 'bptree_concurrent_':
            continue
        for i in range(0, len(op_list)):
            ab_csv = pd.read_csv('abort/' + mem + '/' + tree_type + '/abort_' + op_list[i] + '.csv', index_col=0);
            others = ab_csv['abort'] - ab_csv['conflict'] - ab_csv['capacity'] - ab_csv['explicit']
            others.name = 'other'
            abort_df = pd.concat([ab_csv, others], axis=1)
            del abort_df['abort']
            ax = abort_df.plot(kind='bar', stacked=True, color=colorlst)
            if i == 0:
                plt.ylim(top=2200000)
            else:
                plt.ylim(top=1500000)
            ax.yaxis.offsetText.set_fontsize(font_size)
            ax.ticklabel_format(style="sci", axis="y", scilimits=(0,0))
            plt.xlabel('スレッド数', fontsize=font_size)
            plt.ylabel('アボート回数', fontsize=font_size)
            plt.tick_params(labelsize=font_size)
            plt.legend(fontsize=font_size)
            plt.tight_layout()
            # plt.title('スレッド数によるアボート回数の変化（' + op_list_j[i] + '）')
                #plt.savefig('abort_' + nvhtm_type + '_' + op_list[i] + '_' + str(logsz) + '.png')
            plt.savefig('abort/' + mem + '/' + tree_type + '/abort_' + op_list[i] + '.eps')
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
