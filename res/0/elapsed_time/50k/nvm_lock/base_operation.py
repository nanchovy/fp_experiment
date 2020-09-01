import statistics as st
import numpy as np
from matplotlib import pyplot as plt
import sys
# import decimal
import os
import subprocess
import csv
import pandas as pd
exefiles = ["insert_concurrent.exe", "search_concurrent.exe", "delete_concurrent.exe"]
exp_loop_times = range(3)
warmup_num = 50000 # 元からある要素の数
trial_num = 50000 # 検索・追加する回数
thread_num = range(1, 27, 5) # スレッド数
# vrampath = "/home/iiboshi/dramdir"
pmempath = "/mnt/nvmm/iiboshi/data"
pmemlogpath = "/mnt/nvmm/iiboshi/log"
# linestyles = ["ro-", "b.-", "gs-", "k+-", "y^-", "c*-", "m1-", "kD-", "kx-", "k3-"]

def exp_loop(filename, mode, mempath):
    result_array = [];
    print("executing: " + filename)
    try:
        for i in thread_num:
            print("thread_num: " + str(i))
            inner_result_array = [];
            cmd = ['./' + filename, str(warmup_num), str(trial_num), str(warmup_num + trial_num), str(i), pmempath, pmemlogpath]
            print(cmd)
            for j in exp_loop_times:
                print("trial " + str(j+1))
                spres = subprocess.run(cmd, stdout = subprocess.PIPE, stderr = None).stdout.decode("utf8")
                print(spres);
                inner_result_array.append(float(spres));
            result_array.append(inner_result_array);
    except NameError as err:
        print("NameError: {0}".format(err))
    except:
        print("execution error.", sys.exc_info());
    return result_array

for fn in exefiles:
    if not os.path.exists(fn):
        print(fn + " not exists.")
        sys.exit()

results = []
for fn in exefiles:
    results.append(exp_loop(fn, "i", ""))

print(results)
np.save('result_raw.npy', results)
results_dataframe = pd.DataFrame(np.median(results, axis=2), index=exefiles, columns=thread_num).T
print(results_dataframe)
print("max=", results_dataframe.max().max())
results_dataframe.to_csv('result.csv')
results_dataframe.plot(xlim=[1, max(thread_num)], ylim=[0, results_dataframe.max().max() * 1.1])
plt.savefig('result_fig.png')
# 
# isres.plot(style=linestyles, logy=True)
# plt.savefig('insres.png')
# insert_file = open('insert_result.csv', 'w')
# search_file = open('search_result.csv', 'w')
# 
# insert_writer = csv.writer(insert_file, lineterminator='\n')
# search_writer = csv.writer(search_file, lineterminator='\n')
# insert_writer.writerows(insert_results[0])
# search_writer.writerows(search_results[0])
# 
# insert_file.close()
# search_file.close()

# insert_vram_result_np = np.array(insert_vram_result)
# search_vram_result_np = np.array(search_vram_result)
# 
# insert_vram_result_np = np.sort(insert_vram_result, axis = 1)
# search_vram_result_np = np.sort(search_vram_result, axis = 1)
# 
# insert_vram_result_mean = np.mean(insert_vram_result_np, axis = 1)
# insert_vram_result_min10 = insert_vram_result_np[(insert_vram_result_np.shape[0]/10)]
# insert_vram_result_max90 = insert_vram_result_np[(insert_vram_result_np.shape[0]* 9/10)]
