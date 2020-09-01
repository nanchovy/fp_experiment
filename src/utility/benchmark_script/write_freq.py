import statistics as st
import numpy as np
from matplotlib import pyplot as plt
import sys
# import decimal
import os
import subprocess
import csv
import pandas as pd
import shutil
exefiles = ["insert_concurrent.exe"]
warmup_num = int(sys.argv[1]) # 元からある要素の数
trial_num = int(sys.argv[2]) # 検索・追加する回数
thread_num = eval(sys.argv[3]) # スレッド数
# vrampath = "/home/iiboshi/dramdir"
pmempath = sys.argv[4]
pmemlogpath = sys.argv[5]
# linestyles = ["ro-", "b.-", "gs-", "k+-", "y^-", "c*-", "m1-", "kD-", "kx-", "k3-"]

print(thread_num)

def exp_loop():
    try:
        for i in thread_num:
            cmd = ['./' + exefiles[0], str(warmup_num), str(trial_num), str(warmup_num + trial_num), str(i), pmempath, pmemlogpath]
            print(cmd)
            spres = subprocess.run(cmd, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
            print(spres.stderr.decode("utf8"))
            print(spres.stdout.decode("utf8"))
            shutil.move('./write_freq.txt', './write_freq' + str(i) + '.txt')
            with open(exefiles[0] + ".thr." + str(i) + ".dmp", mode='w', encoding="utf-8") as f:
                f.write(spres.stderr.decode("utf8"))
                f.write(spres.stdout.decode("utf8"))
    except NameError as err:
        print("NameError: {0}".format(err))
    except subprocess.CalledProcessError as err:
        print("CalledProcessError:")
        print(err.stdout)
        print(err.stderr)
    except Exception as e:
        print("execution error.", sys.exc_info())
        print(e)

for fn in exefiles:
    if not os.path.exists(fn):
        print(fn + " not exists.")
        sys.exit()

exp_loop()
