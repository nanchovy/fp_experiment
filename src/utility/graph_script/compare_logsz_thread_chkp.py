import pandas as pd
from matplotlib import pyplot as plt
import matplotlib as mpl
import japanize_matplotlib
import sys
import os

line_style = ['ro-', 'gv-', 'b^-', 'k*-']

# type_list = ['bptree_nvhtm_0', 'bptree_nvhtm_1']
# type_list = ['bptree_nvhtm_0']
type_list = ['bptree_nvhtm_1']
op_list = ['insert', 'delete', 'search']
op_list_j = ['挿入', '削除', '検索']
thr_list = [1, 2, 4, 8, 16] # TODO
# log_list = [33056, 65824, 131360, 262432, 524576, 1048864] # TODO
log_list = [2621728, 5243168, 10486048, 20971808, 41943328, 83886368] # TODO

root_dir = sys.argv[1]
graph_root = sys.argv[2]

for nvhtm_type in type_list:
    for thr in thr_list:
        for op_index in range(len(op_list)):
            chkp_times_lst = []
            for logsz in log_list:
                os.makedirs(graph_root + '/' + nvhtm_type, exist_ok=True)
                for trial in range(1, 6):
                    chkp_time = 0
                    with open(root_dir + '/elapsed_time/' + nvhtm_type + '/logsz_' + str(logsz) + '/' + op_list[op_index] + '_concurrent.exe.thr.' + str(thr) + '.trial.' + str(trial) + '.dmp') as f:
                        chkp_time += [int(line.strip('[FORKED_MANAGER] Nb. checkpoints ')) for line in f.readlines() if line.startswith('[FORKED_MANAGER] Nb.')][0]
                    chkp_time /= 5.0
                chkp_times_lst.append(chkp_time)
            df = pd.DataFrame(chkp_times_lst, index=log_list)
            df.index = list(map(lambda x: str(int(x/(1024 * 1024))), df.index))
            df.plot(kind='bar', legend=False)
            plt.xlabel('ログ容量（MiB）')
            plt.ylabel('Checkpoint回数')
            plt.xticks(rotation=0)
            plt.title('ログ容量の大きさによるCheckpoint回数の変化（' + op_list_j[op_index] + '）')
            plt.savefig(graph_root + '/' + nvhtm_type + '/' + op_list[op_index] + '.thr.' + str(thr) + '.eps')
        plt.close('all')
