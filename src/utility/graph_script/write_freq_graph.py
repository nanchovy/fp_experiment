import pandas as pd
from matplotlib import pyplot as pl
from matplotlib import ticker as tck
import japanize_matplotlib
import sys
import os
from statistics import mean, median,variance,stdev

root_dir = sys.argv[1]
graph_dir = sys.argv[2]
conc_list = ['bptree_concurrent_0', 'fptree_concurrent_0']
# conc_list = []
# nvhtm_list = ['bptree_nvhtm_0', 'bptree_nvhtm_1']
nvhtm_list = ['bptree_nvhtm_0']
# nvhtm_list = ['bptree_nvhtm_1']
# log_list = [33056, 65824, 131360, 262432, 524576, 1048864]
# log_list = [33056, 65824, 524576, 1048864]
# log_list = [2621728, 5243168, 10486048, 20971808, 41943328, 83886368] # TODO
log_list = [41943328] # TODO
thr_list = [1, 2, 4, 8, 16]
# thr_list = [4]

def make_graph(root, graph_root, logsz_list, thr_list):
    for logsize in logsz_list:
        os.makedirs(graph_root + '/logsz_' + str(logsize), exist_ok=True)
        for thr in thr_list:
            worker_timestamps = []
            checkpoint_timestamps = []
            with open(root + '/logsz_' + str(logsize) + '/write_freq' + str(thr) + '.txt') as f:
                lines = f.readlines()
                for line in lines:
                    if (line[0] == 'w'):
                        worker_timestamps.append(float(line[1:]))
                    else:
                        checkpoint_timestamps.append(float(line[1:]))

            worker_diff = []
            worker_freq = []
            worker_passed_time = []
            start = worker_timestamps[0]
            prev = worker_timestamps[0]
            for ts in worker_timestamps[1:]:
                tsdiff = ts - prev
                if (tsdiff == 0):
                    print('0 division!')
                    tsdiff =  0.000000001
                worker_diff.append(tsdiff)
                worker_freq.append(1024 * 1024 / tsdiff)
                prev = ts
                worker_passed_time.append(ts - start)

            # print("timestamps =", timestamps)
            # print("diff =", diff)
            # print("freq =", diff)
            # print(passed_time)

            if (len(worker_timestamps) > 1):
                worker_freq_index = list(range(1, len(worker_timestamps)))
                worker_major_fmt = list(map(str, worker_passed_time))
                worker_major_fmt.insert(0, "")

                freq_df = pd.DataFrame(worker_freq, index=worker_passed_time, columns=['sec'])
                # freq_df = pd.DataFrame(freq, columns=['sec'])
                ax = freq_df.plot(legend=False)
                ax.set_xlabel('経過時間(s)')
                ax.set_ylabel('書き込み頻度(byte/s)')
                # pl.title('Workerプロセスの書き込み頻度')
                ax.ticklabel_format(style="sci", axis="y", scilimits=(0,0))
                # ax.ticklabel_format(style='plain', axis='x')
                pl.ylim([0, freq_df.max().max() * 1.1])
                # pl.savefig(graph_root + '/logsz_' + str(logsize) + '/worker_write_freq.thr.' + str(thr) + '.png')
                # pl.savefig(graph_root + '/logsz_' + str(logsize) + '/worker_write_freq.thr.' + str(thr) + '.eps')
                pl.close()

            if (len(checkpoint_timestamps) > 1):
                start = checkpoint_timestamps[0]
                prev = checkpoint_timestamps[0]
                checkpoint_diff = []
                checkpoint_freq = []
                checkpoint_passed_time = []
                for ts in checkpoint_timestamps[1:]:
                    tsdiff = ts - prev
                    if (tsdiff == 0):
                        print('0 division!')
                        tsdiff =  0.000000001
                    checkpoint_diff.append(tsdiff)
                    checkpoint_freq.append(1024 * 1024 / (tsdiff))
                    prev = ts
                    checkpoint_passed_time.append(ts - start)
                print("checkpoint: logsize = " + str(logsize) + ", thread = " + str(thr))
                print("mean of freq = " + str(mean(checkpoint_freq)/(1024 * 1024)))
                print("interval = " + str(checkpoint_timestamps[-1] - checkpoint_timestamps[0]))

                checkpoint_freq_index = list(range(2, len(checkpoint_timestamps)))
                checkpoint_major_fmt = list(map(str, worker_passed_time))
                checkpoint_major_fmt.insert(0, "")

                freq_df = pd.DataFrame(checkpoint_freq, index=checkpoint_passed_time, columns=['sec'])
                # freq_df = pd.DataFrame(freq, columns=['sec'])
                ax = freq_df.plot(legend=False)
                ax.set_xlabel('経過時間(s)', fontsize=18)
                ax.set_ylabel('書き込み頻度(byte/s)', fontsize=18)
                # pl.title('Checkpointプロセスの書き込み頻度', fontsize=18)
                ax.yaxis.offsetText.set_fontsize(18)
                ax.ticklabel_format(axis="", scilimits=(0,0))
                ax.tick_params(labelsize=16)
                # ax.ticklabel_format(style='plain', axis='x')
                pl.ylim(bottom=0)
                # pl.ylim([0, 100000000])
                pl.tight_layout()
                # pl.savefig(graph_root + '/logsz_' + str(logsize) + '/checkpoint_write_freq.thr.' + str(thr) + '.png')
                # pl.show()
                # pl.savefig(graph_root + '/logsz_' + str(logsize) + '/checkpoint_write_freq.thr.' + str(thr) + '.eps')
                pl.close()

for nhlst in nvhtm_list:
    graph_nh_dir = graph_dir + '/' + nhlst
    os.makedirs(graph_nh_dir, exist_ok=True)
    make_graph(root_dir + '/write_freq/' + nhlst, graph_nh_dir, log_list, thr_list)
for conc in conc_list:
    graph_cc_dir = graph_dir + '/' + conc
    os.makedirs(graph_cc_dir, exist_ok=True)
    make_graph(root_dir + '/write_freq/' + conc, graph_cc_dir, [65824], thr_list)
