import numpy as np
from matplotlib import pyplot as plt
import pandas as pd
import japanize_matplotlib
import sys
ops = ['insert', 'search', 'delete']
logsizes = ['41943328']

colorlst = ['#ffeda0', '#feb24c', '#f03b20']
fontsize = 18
labels = ['Checkpoint-block', 'Abort', 'HTM-block']

for logsize in logsizes:
    for op in ops:
        pmem_file = pd.read_csv(sys.argv[1] + '/wait_' + op + '_plain.logsize.' + logsize + '.csv', index_col=0)
        vmem_file = pd.read_csv(sys.argv[2] + '/wait_' + op + '_plain.logsize.' + logsize + '.csv', index_col=0)

        # print(pmem_file)
        # print(vmem_file)
        # print(pmem_file.index)

        xtick = np.array(range(len(pmem_file.index)))
        barwidth = 0.3

        cpt_bottom_p = np.zeros(len(pmem_file.index), dtype=int)
        abt_bottom_p = cpt_bottom_p + pmem_file['Checkpoint-block']
        htm_bottom_p = abt_bottom_p + pmem_file['Abort']
        top_p = htm_bottom_p + pmem_file['HTM-block']
        print(op)
        print(max(top_p))
        print('cpt + end:')
        print(pmem_file['Checkpoint-block'] + pmem_file['End'])

        cpt_bottom_v = np.zeros(len(vmem_file.index), dtype=int)
        abt_bottom_v = cpt_bottom_v + vmem_file['Checkpoint-block']
        htm_bottom_v = abt_bottom_v + vmem_file['Abort']
        print('cpt-ratio:')
        print(vmem_file['Checkpoint-block']/pmem_file['Checkpoint-block'])
        print('end-ratio:')
        print(vmem_file['End']/pmem_file['End'])

        fig = plt.figure(figsize=(7, 6))
        plt.xticks(xtick, pmem_file.index)
        plt.bar(xtick-barwidth/2, pmem_file['Checkpoint-block'], bottom=cpt_bottom_p, width=barwidth, edgecolor = 'k', color = colorlst[0], label = labels[0])
        plt.bar(xtick-barwidth/2, pmem_file['Abort']           , bottom=abt_bottom_p, width=barwidth, edgecolor = 'k', color = colorlst[1], label = labels[1])
        plt.bar(xtick-barwidth/2, pmem_file['HTM-block']       , bottom=htm_bottom_p, width=barwidth, edgecolor = 'k', color = colorlst[2], label = labels[2])

        plt.bar(xtick+barwidth/2, vmem_file['Checkpoint-block'], bottom=cpt_bottom_v, width=barwidth, edgecolor = 'k', color = colorlst[0])
        plt.bar(xtick+barwidth/2, vmem_file['Abort']           , bottom=abt_bottom_v, width=barwidth, edgecolor = 'k', color = colorlst[1])
        plt.bar(xtick+barwidth/2, vmem_file['HTM-block']       , bottom=htm_bottom_v, width=barwidth, edgecolor = 'k', color = colorlst[2])

        plt.xlabel('スレッド数', fontsize=fontsize)
        plt.ylabel('スレッドあたりの消費時間 (秒)', fontsize=fontsize)
        plt.tick_params(labelsize=fontsize)
        plt.legend(bbox_to_anchor=(0.50, -0.20), loc='center', borderaxespad=0, ncol=3, fontsize=fontsize-4)
        # plt.text(1.75, max(top_p)-1, '左：不揮発性メモリ，右：DRAM', fontsize=fontsize-4)
        plt.text(1, max(top_p)-max(top_p)/10, '左：B${^+}$-Tree${_{NH}}$，右：B${^+}$-Tree${_{NH}}$(DRAM)', fontsize=fontsize-4)
        plt.tight_layout()
        plt.savefig('wait_' + op + '_plain_logsize_' + logsize + '.pdf')
        plt.close()

        fig, ax = plt.subplots()
        plt.xticks(xtick, pmem_file.index)
        plt.bar(xtick-barwidth/2, pmem_file['End']       , width=barwidth, edgecolor = 'k', color = colorlst[1], label = 'B${^+}$-Tree${_{NH}}$')
        plt.bar(xtick+barwidth/2, vmem_file['End']       , width=barwidth, edgecolor = 'k', color = colorlst[2], label = 'B${^+}$-Tree${_{NH}}$(DRAM)')
        plt.xlabel('スレッド数', fontsize=fontsize)
        plt.ylabel('処理時間 (秒)', fontsize=fontsize)
        ax.yaxis.offsetText.set_fontsize(18)
        ax.ticklabel_format(style="sci", axis="y", scilimits=(0,0))
        plt.tick_params(labelsize=fontsize)
        plt.legend(fontsize=fontsize-2)
        plt.tight_layout()
        plt.savefig('endwait_' + op + '_plain_logsize_' + logsize + '.pdf')
        plt.close()
