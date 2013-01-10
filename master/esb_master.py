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
parser.add_argument("--error-checking", help="Check the server's output", action="store_true")
parser.add_argument("-k", "--keys", help="Specify the number of keys", type=int, default=8192)
parser.add_argument("-c", "--clients", help="Specify the number of clients", type=int, default=64)
parser.add_argument("-l", "--length", help="Specify the length of the values stored", type=int, default=64)
parser.add_argument("-t", "--throughput", help="Specify the number of operations per second", type=int, default=1024)
parser.add_argument("-n", "--num-datapoints", help="Specify the number of datapoints", type=int, default=64)
parser.add_argument("-i", "--independent-variable", default='k', choices=['c', 'k', 't'])
parser.add_argument("-s", "--server-file-path", help="Filepath of server executable", default="server")
parser.add_argument("-o", "--output-path", help="Output filepath", default="")

args = parser.parse_args()

#open the results files
latencyFile = open(args.output_path + "latency.txt", "w")
latencyFile.write("# <x> <mean> <low-error> <high-error>\n")

throughputFile = open(args.output_path + "throughput.txt", "w")
throughputFile.write("# <x> <mean>\n")

memoryFile = open(args.output_path + "memory.txt", "w")
memoryFile.write("# <x> <memory usage (MB)>\n")

cpuFile = open(args.output_path + "cpu.txt", "w")
cpuFile.write("# <x> <cpu utilization>\n")

#find our local IP
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(("8.8.8.8", 0))
hostIP = s.getsockname()[0]
s.close()

#create a config file
config = open("esb-test.config", "w")
config.write("server_host 0.0.0.0\nserver_port 3940\nusername someuser\npassword somepsswd\n")
config.close()

#start the server
process = subprocess.Popen([args.server_file_path, "esb-test.config"])
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

if args.independent_variable == 'c':
    dataPointCount = min(args.clients, args.num_datapoints)

if args.independent_variable == 'k':
    dataPointCount = min(args.keys, args.num_datapoints)

if args.independent_variable == 't':
    dataPointCount = min(args.throughput, args.num_datapoints)

percentDone = 0

ivar = 0
for i in xrange(1, dataPointCount + 1):

    if args.independent_variable == 'k':
        numKeys = splitNWays(args.keys, dataPointCount, i)
        ivar += numKeys
        
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
        ivar = numClients

    throughput = args.throughput/numSlaves
    if args.independent_variable == 't':
        throughput = i*(args.throughput/dataPointCount)/numSlaves
        ivar = throughput

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
    stats = calculateStatistics(latency)
    latencyFile.write("%d, %f, %f, %f\n" % (ivar, stats["mean"], stats["low"], stats["high"]))

    #calculate the throughput
    throughput = (amount * numSlaves)/(time.time() - start)
    throughputFile.write("%f, %f\n" % (ivar, (throughput * numSlaves)))

    #CPU usage
    cpuUsage = (getJiffies(process.pid) - startServJiffies)/float(getSystemJiffies() - startSysJiffies)
    cpuFile.write("%f, %f\n" % (ivar, cpuUsage))

    #memory usage
    memoryFile.write("%f, %f\n" % (ivar, getMemoryUsageOfPid(process.pid)))

    #if the % changed, print it
    if int(i*100/dataPointCount) > percentDone:
        percentDone = int(i*100/dataPointCount)
        print "%d%%" % percentDone,
        sys.stdout.flush()


#done writing data
latencyFile.close()
throughputFile.close()
memoryFile.close()
cpuFile.close()

#close the connections to the slaves
for slave in slaves:
    command = {"command":"quit"}
    slave.sendall(createPacket(command))
    slave.close()













