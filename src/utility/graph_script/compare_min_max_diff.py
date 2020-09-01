import pandas as pd
import sys

a = pd.read_csv(sys.argv[1] + '/result.csv', index_col=0)
b = pd.read_csv(sys.argv[2] + '/result.csv', index_col=0)

print (a)
print (b)

insert_ary = a.iloc[:,0] / b.iloc[:,0]
search_ary = a.iloc[:,1] / b.iloc[:,1]
delete_ary = a.iloc[:,2] / b.iloc[:,2]
print ("insert:\n" + str(insert_ary))
print ("search:\n" + str(search_ary))
print ("delete:\n" + str(delete_ary))
print ("insert: max = " + str(max(insert_ary)) + ", min = " + str(min(insert_ary)))
print ("search: max = " + str(max(search_ary)) + ", min = " + str(min(search_ary)))
print ("delete: max = " + str(max(delete_ary)) + ", min = " + str(min(delete_ary)))
