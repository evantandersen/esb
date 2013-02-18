import argparse
import socket
import json
import math
import subprocess
import atexit
import time
import itertools
from collections import defaultdict
import os.path
import mmap
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plot
import matplotlib.ticker
import datetime
import decimal
from math import log10, floor, fabs


def round_sig(x, sig=2):
    if x == 0:
        return 0
    return round(x, sig-int(floor(log10(fabs(x))))-1)


def engNotation(x, pos):
    power = 0
    if x > 0:
        power = int(math.floor((math.floor(math.log10(x)))/3))
    a = {-4:"p", -3:"n", -2:"u", -1:"m", 0:"", 1:"K", 2:"M", 3:"G", 4:"T"}
    amount = round_sig(x/(10**(3*power)), 2)
    if amount >= 10:
        return "%d%s" % (amount, a[power])
    return "%.1f%s" % (amount, a[power])


def calculateStatistics(latencyDict):
    sortedData = [(k, latency[k]) for k in sorted(latency, key=int)]
    total = 0
    count = 0
    for key, value in sortedData:
        total += key * value
        count += value

    lowGoal = count * 0.025
    highGoal = count * 0.975
    temp = 0
    low = -1
    high = -1
    for key, value in sortedData:
        temp += value
        if temp > lowGoal and low == -1:
            low = key

        if temp > highGoal and high == -1:
            high = key
        
    return {"mean":total/(float(count)*10.0), "low":float(low)/10, "high":float(high)/10}

def plotData(path, x, y, xlabel, ylabel, xshort, yshort, xFormat=False, yFormat=False):

    plot.plot(x, y)
    
    #if flag is set, use engineering notation
    if xFormat:
        fmt = matplotlib.ticker.FuncFormatter(engNotation)
        plot.gca().xaxis.set_major_formatter(fmt)
    if yFormat:
        fmt = matplotlib.ticker.FuncFormatter(engNotation)
        plot.gca().yaxis.set_major_formatter(fmt)

    plot.xlabel(xlabel)
    plot.ylabel(ylabel)
    x1,x2,y1,y2 = plot.axis()
    plot.axis((0,x2,0,y2*1.25))
    plot.savefig(os.path.join(path, "%s_vs_%s.png" % (yshort, xshort)))
    plot.close()

def writeDataToFile(path, x, y, xshort, yshort):
    file = open(os.path.join(path, ("%s_vs_%s.txt" % (yshort, xshort))), "w")
    file.write("# <%s> <%s>\n" % (xshort, yshort))
    i = 0
    for xData in x:
        file.write("%f %f\n" % (xData, y[i]))
        i += 1
    file.close()

def splitNWays(count, n, i):
    return int(count/n) + ((count % n) > i)


def createPacket(buffer):
    str = json.dumps(buffer)
    return "%08d%s" % (len(str), str)


def dict_sum(a, b):
    d = defaultdict(int, a)
    for k, v in b.items():
        d[k] += v
    return dict(d)


def readPacket(recvSocket):
    buf = []
        
    result = recvSocket.recv(8, socket.MSG_WAITALL)
    size = int(result)
    if size > 1048576:
        if args.verbose:
            print "Packet too large (%d)" % size
        return None
    result = json.loads(recvSocket.recv(size, socket.MSG_WAITALL))
    if len(result['errors']) != 0:
        for error in result['errors']:
            print "Client Error: %s" % error
        raise Exception('ClientError')
    return result


def getJiffies(pid):
    stat = open("/proc/%d/stat" % pid, "r")
    #kernel and user time
    values = stat.readline().split()
    result = int(values[13]) + int(values[14])
    stat.close()
    return result


def getSystemJiffies():
    stat = open("/proc/stat", "r")
    line = stat.readline()
    while line[:3] != "cpu":
        line = stat.readline()
    
    jiffies = line.split()
    total = 0
    for value in jiffies[1:]:
            total += int(value)
    stat.close()
    return total


def getMemoryUsageOfPid(pid):
    memInfo = open("/proc/%d/statm" % pid, "r")
    result = (float((memInfo.readline().split()[1]))*mmap.PAGESIZE)/1048576
    memInfo.close()
    return result



#start of program execution
parser = argparse.ArgumentParser()
parser.add_argument("slaves", nargs='+', help="Address of slaves")
parser.add_argument("-k", "--keys", help="Specify the number of keys", type=int, default=512)
parser.add_argument("-c", "--clients", help="Specify the number of clients", type=int, default=1)
parser.add_argument("-t", "--throughput", help="Specify the number of operations per second", type=int, default=32)
parser.add_argument("-i", "--independent-variable", default='k', choices=['c', 'k', 't'])
parser.add_argument("-l", "--length", help="Specify the length of the values stored", type=int, default=128)
parser.add_argument("-n", "--num-datapoints", help="Specify the number of datapoints", type=int, default=64)
parser.add_argument("-s", "--server-path", help="Filepath of server executable", default="./server")
parser.add_argument("-o", "--output-path", help="Output filepath", default="")
parser.add_argument("--error-checking", help="Check the server's output", action="store_true")
parser.add_argument("--raw-output", help="Save the raw data to files", action="store_true")

args = parser.parse_args()

#find our local IP
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(("8.8.8.8", 0))
hostIP = s.getsockname()[0]
s.close()

#create a config file
config = open("esb-test.config", "w")
config.write("server_host 0.0.0.0\nserver_port 3940\nusername someuser\npassword somepsswd\ntable test\n")
config.close()

#start the server
process = subprocess.Popen([args.server_path, "esb-test.config"])
print "Server spawned on pid: %d" % process.pid

#kill the server when we exit
atexit.register(process.terminate)

#give the server time to setup
time.sleep(0.5)

#connect to the slaves
numSlaves = 0
slaves = []
for slave in args.slaves:
    newSocket = socket.socket()
    newSocket.connect((slave, 4912))
    slaves.append(newSocket)
    numSlaves += 1

#send the initial details
i = 0;
for slave in slaves:
    init = {"command":"init","address": hostIP, "port":3940, "slave-id":i << 48, "username":"someuser", "password":"somepsswd", "table":"test","error-checking":args.error_checking, "value-length":args.length}
    slave.sendall(createPacket(init))
    i += 1

#client and throughput tests have the keys inserted at the start
if args.independent_variable == 'c' or args.independent_variable == 't':
    i = 0;
    for slave in slaves:
        command = {"command":"add","num-clients":splitNWays(args.clients, numSlaves, i), "amount":splitNWays(args.keys, numSlaves, i),"throughput":0}
        slave.sendall(createPacket(command))
        i += 1
    
    for slave in slaves:
        readPacket(slave)

#run the tests
dataPointCount = 0

ivar_filename = ''
ivar_title = ''

if args.independent_variable == 'c':
    dataPointCount = min(args.clients, args.num_datapoints)
    ivar_filename = "clients"
    ivar_title = "Concurrent Clients"

if args.independent_variable == 'k':
    dataPointCount = min(args.keys, args.num_datapoints)
    ivar_filename = "keys"
    ivar_title = "Number of Keys"

if args.independent_variable == 't':
    dataPointCount = min(args.throughput, args.num_datapoints)
    ivar_filename = "IOPs"
    ivar_title = "Requested Throughput (IOPs/s)"

percentDone = 0

ivar = []
latencyResults = []
memoryResults = []
throughputResults = []
cpuResults = []

temp = 0
for i in xrange(1, dataPointCount + 1):

    if args.independent_variable == 'k':
        numKeys = splitNWays(args.keys, dataPointCount, i)
        temp += numKeys
        ivar.append(temp)
        
        j = 0
        for slave in slaves:
            command = {"command":"add","num-clients":splitNWays(args.clients, numSlaves, j),"amount":splitNWays(numKeys, numSlaves, j), "throughput":0}
            slave.sendall(createPacket(command))
            j += 1
        
        for slave in slaves:
            readPacket(slave)

    numClients = args.clients
    if args.independent_variable == 'c':
        numClients = (args.clients/dataPointCount)*i
        ivar.append(numClients)

    throughput = args.throughput/numSlaves
    if args.independent_variable == 't':
        throughput = i*(args.throughput/dataPointCount)/numSlaves
        ivar.append(throughput)

    amount = 2*throughput

    #start a timer for throughput
    start = time.time()
    startSysJiffies = getSystemJiffies()
    startServJiffies = getJiffies(process.pid)
    
    j = 0
    for slave in slaves:
        command = {"command":"test","num-clients":splitNWays(numClients, numSlaves, j),"amount":amount,"workload":[0.8],"throughput":throughput}
        slave.sendall(createPacket(command))
        j += 1

    latency = defaultdict(lambda:0)
    for slave in slaves:
        result = readPacket(slave)
        for k, v in zip(*[iter(result['latency'])] * 2):
            latency[k] += v

    #calculate the latency mean and confidence interval
    latencyResults.append(calculateStatistics(latency))

    #calculate the throughput
    throughputResults.append((amount * numSlaves)/(time.time() - start))

    #CPU usage
    cpuResults.append(((getJiffies(process.pid) - startServJiffies)/float(getSystemJiffies() - startSysJiffies)) * 100)

    #memory usage
    memoryResults.append(getMemoryUsageOfPid(process.pid))

    #if the % changed, print it
    if int(i*100/dataPointCount) > percentDone:
        percentDone = int(i*100/dataPointCount)
        print "%d%%" % percentDone,
        sys.stdout.flush()

#create a folder for results
outputFilePath = os.path.join(args.output_path, datetime.datetime.now().strftime("%Y-%m-%d %H.%M"))
os.mkdir(outputFilePath)

#write out the configuration used
configFile = open(os.path.join(outputFilePath, "config.txt"), "w")
configFile.write("#\n# esb tests performed at %s\n" % datetime.datetime.now().strftime("%R on %A, %B %e"))
configFile.write("#\n")
configFile.write("# Configuration Settings:\n")
configFile.write("#    clients:%d\n" % args.clients)
configFile.write("#    keys:%d\n" % args.keys)
configFile.write("#    throughput:%d\n" % args.throughput)
configFile.write("#    data points:%d\n" % dataPointCount)
configFile.write("#    value size:%d\n" % args.length)
configFile.write("#\n")
configFile.write("# Independent Variable:\n")
configFile.write("#    %s\n" % ivar_filename)
configFile.write("#\n")
configFile.write("#\n")
configFile.close()

if args.raw_output:    
    #write out
    writeDataToFile(outputFilePath, ivar, cpuResults, ivar_filename, "cpu")
    writeDataToFile(outputFilePath, ivar, memoryResults, ivar_filename, "mem")
    writeDataToFile(outputFilePath, ivar, throughputResults, ivar_filename, "IOPs")

    #latency is a special case
    file = open(os.path.join(outputFilePath, ("latency_vs_%s.txt" % ivar_filename)), "w")
    file.write("# <%s> <latency <ymin> <ymax>\n" % ivar_filename)
    i = 0
    for item in latencyResults:
        file.write("%f, %f, %f, %f\n" % (ivar[i], item["mean"], item["low"], item["high"]))
        i += 1
    file.close()

#create graphs
plotData(outputFilePath, ivar, memoryResults, ivar_title, "Memory (MB)", ivar_filename, "mem", xFormat=True)
plotData(outputFilePath, ivar, cpuResults, ivar_title, "CPU Utilization (%)", ivar_filename, "cpu", xFormat=True)
plotData(outputFilePath, ivar, throughputResults, ivar_title, "Actual Throughput (IOPs/s)", ivar_filename, "IOPs", xFormat=True, yFormat=True)

latencyMean = []
for item in latencyResults:
    latencyMean.append(item["mean"])

plotData(outputFilePath, ivar, latencyMean, ivar_title, "Latency (ms)", ivar_filename, "Latency", xFormat=True)


#close the connections to the slaves
for slave in slaves:
    command = {"command":"quit"}
    slave.sendall(createPacket(command))
    slave.close()













