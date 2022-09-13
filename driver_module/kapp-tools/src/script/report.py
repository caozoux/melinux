#!/bin/python3

import os
import sys
from API import api
from optparse import OptionParser

from optparse import OptionParser

parser = OptionParser()
parser.add_option("-e", "--cmd", type="string", dest="cmd",
                  help="--cmd stat_event ")
parser.add_option("-l", "--list", action="store_true", dest="list",
                  help="--list list all benchmark test name") 
parser.add_option("-f", "--file", type="string", dest="file",
                  help="--file report log file")
(options, args) = parser.parse_args()

pidtop={}
delay100=[]
delay200=[]
delay400=[]
delay800=[]
delay3000=[]

#rce recomile with syntastic


reportdict = {
"100-200":delay100,
"200-400":delay200,
"400-800":delay400,
"800-3000":delay800,
"3000-100000":delay3000
}

def HandleReportLog(file):
    listbuf = os.popen("cat " + file + " | grep 调度被延迟").readlines()
    for line in listbuf:
        item={}
        #print(line[:-1])
        linelist=line.split(" ")
        item["delay"]= int(linelist[1])
        item["cur_pid"]= int(linelist[4][:-1])
        item["cur_comm"]= linelist[6][:-1]
        item["dealy_com"]= linelist[10][:-1]

        if item["delay"] >= 100 and item["delay"] < 200:
            #print(item)
            delay100.append(item)
        elif item["delay"] >= 200 and item["delay"] < 400:
            delay200.append(item)
        elif item["delay"] >= 400 and item["delay"] < 800:
            delay400.append(item)
        elif item["delay"] >= 800 and item["delay"] < 3000:
            delay800.append(item)
        else:
            delay3000.append(item)
        #print(linelist[1], linelist[4][:-1], linelist[6][:-1], linelist[10][:-1])

    #logbuf=api.FileRead(file)
    #listbuf=logbuf.split("\n")
    #for line in listbuf:
    #    print(line[2], line[5], line[7], line[11])
    #print(version)

def ReportLog(delaylist):
    piditem = {
            "comm":"",
            "min":-1,
            "max":0,
            "avg":0,
            "cnt":0,
            "total":0
    }
    for item in delaylist:
        pid = item["cur_pid"]
        dictitem={}
        if not pid in pidtop.keys():
            #print(item)
            dictitem['comm'] = item['cur_comm']
            dictitem['min'] = item['delay']
            dictitem['max'] = item['delay']
            dictitem['avg'] = item['delay']
            dictitem['cnt'] = 1
            dictitem['total'] =  item['delay']
            pidtop[pid]=dictitem
        else:
            if pidtop[pid]['min'] > item['delay']:
                pidtop[pid]['min'] = item['delay']
            if pidtop[pid]['max'] < item['delay']:
                #print("update max", item, pidtop[pid])
                pidtop[pid]['max'] = item['delay']
            pidtop[pid]['cnt'] += 1
            pidtop[pid]['total'] += item['delay']


if options.file:
    HandleReportLog(options.file)
    ReportLog(delay100)
    ReportLog(delay200)
    ReportLog(delay400)
    ReportLog(delay800)
    ReportLog(delay3000)


    for item in pidtop.keys():
        print("%-20s avg:%-10d min:%-10d max:%-10d cnt:%-10d"%(pidtop[item]['comm'], pidtop[item]['total']/pidtop[item]['cnt'], \
            pidtop[item]['min'], pidtop[item]['max'],pidtop[item]['cnt']))
            
   

