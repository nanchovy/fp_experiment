import statistics as st
import numpy as np
from matplotlib import pyplot as plt
import sys
# import decimal
import os
import subprocess
import csv
import pandas as pd

def execute_command(command):
    command_result = []
    try:
        command_result = subprocess.run(command, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
    except NameError as err:
        print("NameError: {0}".format(err))
    except subprocess.CalledProcessError as err:
        print("CalledProcessError:")
        print(err.stdout)
        print(err.stderr)
    except ValueError as err:
        print(e)
        print(spres.stdout)
        print(spres.stderr)
    except Exception as e:
        print("execution error.", sys.exc_info())
        print(e)
    return command_result

def exp_loop(filename, mode, exp_loop_times, warmup_num, trial_num, thread_list, cpthread_list, datapath, logpath):
    raw_result_list = []
    average_result_list = []
    print("executing: " + filename)
    print("warming up")
    cmd = ['./' + filename, str(warmup_num), str(trial_num), str(warmup_num + trial_num), str(4), datapath, logpath]
    execute_command(cmd)
    for cpthr in cpthread_list:
        for thr in thread_list:
            print("thread_num: " + str(thr))
            cmd = ['./' + filename, str(warmup_num), str(trial_num), str(warmup_num + trial_num), str(thr), datapath, logpath, str(cpthr)]
            print("command: " + str(cmd))
            average_result_list_tmp = []
            for trial in exp_loop_times:
                print("trial " + str(trial+1))
                spres = execute_command(cmd)
                print("trial " + str(trial+1) + " finished")
                result_time = float(spres.stdout.decode("utf8"));
                raw_result_list.append([cpthr, thr, trial, result_time])
                average_result_list_tmp.append(result_time)
                with open(filename + ".cpthr." + str(cpthr) + ".thr." + str(thr) + ".trial." + str(trial+1) + ".dmp", mode='w', encoding="utf-8") as f:
                    f.write(spres.stderr.decode("utf8"))
                    f.write(spres.stdout.decode("utf8"))
            trial_average = sum(average_result_list_tmp)/len(average_result_list_tmp)
            average_result_list.append([cpthr, thr, trial_average])
    return (raw_result_list, average_result_list)

def thrlist_generator(min_thr:int, max_thr:int):
    if (min_thr <= max_thr):
        p_list = thrlist_generator(min_thr * 2, max_thr)
        p_list.insert(0, min_thr)
        return p_list
    else:
        return list()

def main():
    exenames =          ["insert", "search", "delete", "mixed"]
    exefiles =          ["insert_concurrent.exe", "search_concurrent.exe", "delete_concurrent.exe", "mixed_concurrent.exe"]
    exp_loop_times =    range(3)
    exec_dir =          sys.argv[1] # 実行可能ファイルがある場所
    warmup_num =        int(sys.argv[2]) # 元からある要素の数
    trial_num =         int(sys.argv[3]) # 検索・追加する回数
    thread_stt =        int(sys.argv[4]) # 開始スレッド数
    thread_end =        int(sys.argv[5]) # 終了スレッド数
    cpthread_stt =      int(sys.argv[6]) # 開始Checkpointスレッド数
    cpthread_end =      int(sys.argv[7]) # 終了Checkpointスレッド数
    datapath =          sys.argv[8]
    logpath =           sys.argv[9]
    thread_list =       thrlist_generator(thread_stt, thread_end)
    cpthread_list =     thrlist_generator(cpthread_stt, cpthread_end)
    exefiles = list(map(lambda x: exec_dir + '/' + x, exefiles))
    print("exec:" + str(exefiles))

    for fn in exefiles:
        if not os.path.exists(fn):
            print("executable file \"" + fn + "\" not exists.")
            sys.exit()

    for (fn, name) in zip(exefiles, exenames):
        (raw_result, average_result) = exp_loop(fn, "i", exp_loop_times, warmup_num, trial_num, thread_list, cpthread_list, datapath, logpath)
        result_df = pd.DataFrame(average_result, columns=["cpthread", "thread", "time"])
        result_df.to_csv(exec_dir + '/' + name + '.csv')
        np.save(exec_dir + '/' + name + '_raw.npy', raw_result)

main()
