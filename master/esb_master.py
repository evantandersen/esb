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
            

parser = argparse.ArgumentParser()
parser.add_argument("slaves", nargs='+', help="Address of slaves")
parser.add_argument("--error-checking", help="Check the server's output", action="store_true")
parser.add_argument("-p", "--port", help="Port used to communicate with slaves", type=int, default=4912)
parser.add_argument("-k", "--keys", help="Specify the number of keys", type=int, default=8192)
parser.add_argument("-c", "--clients", help="Specify the number of clients", type=int, default=64)
parser.add_argument("-l", "--length", help="Specify the length of the values stored", type=int, default=64)
parser.add_argument("-t", "--throughput", help="Specify the number of operations per second", type=int, default=1024)
parser.add_argument("-n", "--num-datapoints", help="Specify the number of datapoints", type=int, default=64)
parser.add_argument("-i", "--independent-variable", default='k', choices=['c', 'k', 't'])
parser.add_argument("-s", "--server-file-path", help="Filepath of server executable", default="server")
parser.add_argument("-o", "--output-path", help="Output filepath", default="")

args = parser.parse_args()

#open the results file
outfile = open(args.output_path + "latency.txt", "w")
outfile.write("# <x> <y> <low-error> <high-error>\n")

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

#kill the server when we exit
atexit.register(process.terminate)

print "Server spawned on pid: %d" % process.pid

#give the server time to setup
time.sleep(0.5)

#connect to the slaves
numSlaves = 0
slaves = []
for slave in args.slaves:
    newSocket = socket.socket()
    newSocket.connect((slave, args.port))
    slaves.append(newSocket)
    numSlaves += 1

#send the initial details
i = 0;
for slave in slaves:
    init = {"command":"init","address": hostIP, "port":3940, "slave-id":i << 48, "username":"someuser", "password":"somepsswd", "table":"test","error-checking":args.error_checking, "value-length":args.length}
    slave.sendall(createPacket(init))
    i += 1

#vary the number of clients
if args.independent_variable == 'c':

    #insert all the keys
    i = 0;
    for slave in slaves:
        command = {"command":"add","num-clients":args.clients,"amount":(args.keys/numSlaves) + ((args.keys % numSlaves) > i), "throughput":0}
        slave.sendall(createPacket(command))
        i += 1

    for slave in slaves:
        readPacket(slave)
            
    #vary the number of clients
    dataPointCount = min(args.clients, args.num_datapoints)
    for i in xrange(1, dataPointCount + 1):
        numClients = int((args.clients/dataPointCount)*i/numSlaves)
        for slave in slaves:
            command = {"command":"test","num-clients":numClients, "amount":2*args.throughput/numSlaves, "workload":[0.8], "throughput":args.throughput/numSlaves}
            slave.sendall(createPacket(command))

        latency = defaultdict(lambda:0)
        for slave in slaves:
            result = readPacket(slave)
            for k, v in zip(*[iter(result['latency'])] * 2):
                latency[k] += v

        stats = calculateStatistics(latency)
        outfile.write("%d, %f, %f, %f\n" % (numClients, stats["mean"], stats["low"], stats["high"]))

#vary the number of keys
if args.independent_variable == 'k':
    
    dataPointCount = min(args.keys, args.num_datapoints)
    totalKeys = 0
    for i in xrange (1, dataPointCount + 1):
        numKeys = int((args.keys/dataPointCount)/numSlaves)
        totalKeys += numKeys
        for slave in slaves:
            command = {"command":"add","num-clients":args.clients/numSlaves,"amount":numKeys, "throughput":0}
            slave.sendall(createPacket(command))

        for slave in slaves:
            readPacket(slave)
                
        for slave in slaves:
            command = {"command":"test","num-clients":args.clients/numSlaves, "amount":2*args.throughput/numSlaves, "workload":[0.8], "throughput":args.throughput/numSlaves}
            slave.sendall(createPacket(command))

        latency = defaultdict(lambda:0)
        for slave in slaves:
            result = readPacket(slave)
            for k, v in zip(*[iter(result['latency'])] * 2):
                latency[k] += v

        stats = calculateStatistics(latency)
        outfile.write("%d, %f, %f, %f\n" % (totalKeys, stats["mean"], stats["low"], stats["high"]))

#vary the number of requests per second
if args.independent_variable == 't':

    for slave in slaves:
        command = {"command":"add","num-clients":args.clients/numSlaves,"amount":args.keys/numSlaves, "throughput":0}
        slave.sendall(createPacket(command))
    for slave in slaves:
        readPacket(slave)

    dataPointCount = min(args.throughput, args.num_datapoints)
    for i in xrange(1, dataPointCount + 1):
        for slave in slaves:
            command = { "command":"test",
                        "num-clients":args.clients/numSlaves,
                        "amount":2*i*(args.throughput/dataPointCount)/numSlaves,
                        "workload":[0.8],
                        "throughput":i*(args.throughput/dataPointCount)/numSlaves}

            slave.sendall(createPacket(command))
        
        latency = defaultdict(lambda:0)
        for slave in slaves:
            result = readPacket(slave)
            for k, v in zip(*[iter(result['latency'])] * 2):
                latency[k] += v
        
        stats = calculateStatistics(latency)
        outfile.write("%d, %f, %f, %f\n" % (i*(args.throughput/dataPointCount), stats["mean"], stats["low"], stats["high"]))


#done writing data
outfile.close()

#close the connections to the slaves
for slave in slaves:
    command = {"command":"quit"}
    slave.sendall(createPacket(command))
    slave.close()













