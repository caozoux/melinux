import time
import os
from collections import OrderedDict

def meminfo():
    ''' Return the information in /proc/meminfo
    as a dictionary '''
    meminfo=OrderedDict()

    with open('/proc/meminfo') as f:
        for line in f:
            meminfo[line.split(':')[0]] = line.split(':')[1].strip()
    return meminfo

def get_cur_time_string():
    """TODO: Docstring for get_cur_time_string.

    :arg1: TODO
    :returns: TODO

    """
    now = int(time.time()) 
    timeStruct = time.localtime(now) 
    strTime = time.strftime("%Y-%m-%d_%H:%M:%S", timeStruct) 
    return strTime


def dirt_writefile(size):
    """TODO: Docstring for dirt_writefile.
    :returns: TODO

    """
    old = time.time()
    filename="test_"+get_cur_time_string()
    os.system("dd if=/dev/zero of="+filename+" bs=1M count="+str(size)+" 2>/dev/null")
    new = time.time()
    return new-old


print "Cached      ", "write_time" 
cnt=10
while cnt>0:
    get_cur_time_string()
    memDict=meminfo()
    ms=dirt_writefile(1000)
    print(("%-12s %s")%(memDict["Cached"], ms))
    cnt -= 1
