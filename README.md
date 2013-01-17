esb
===

The ECE297 Server Benchmark faclilitates quick and accurate distributed benchmarking. esb isolates a single variable and measures it's effect on server performance. It currently supports: 

**Indepdent Variables**
 - Number of active keys
 - Number of concurrent clients
 - Total server throughput (IOPs/s)

**Depedent Variables**
 - Reuqest Latency (mean / 95% confidence interval)
 - Throughput
 - Memory Usage
 - CPU Usage

**Requirements**
 - Master requires linux
 - Slave supports most POSIX systems
 - [Jansson](http://www.digip.org/jansson/) (JSON Parser Library)
 - [Matplotlib](http://matplotlib.org/index.html) (Python lib for grahping)

**How To Use**

esb has two components - the master and the slave. The master handles executing the server, distributing work and collecting the results. The slave(s) execute the commands from the master and provide the workload for the server. 

The slave needs to be compiled with a client library before it can be used. Add the file `libstorage.a` to the `slave` folder, and then run make. 

To run a test, copy your `server` executable into the master directory. Launch at least one slave. When it lanches, it will print the local IP address of the computer. Pass these IP(s) as arguments to the master, so that it knows which computers to connect to:

python esb_master.py [slave IPs]

By default, esb will measure the effect on performance caused by varying the number of keys from 0 to 8192. The results will be placed in a folder labelled with the current date and time.


**Screenshots**
 - [CPU Utilization vs Number of Keys](http://i.imgur.com/zS5Uc.png)
 - [Memory vs Concurrent Clients](http://i.imgur.com/snGcS.png)
 - [Actual vs Requested Throughput](http://i.imgur.com/2V8DB.png)
 - [Latency vs Number of Keys](http://i.imgur.com/XxcDL.png)

**Future Developments**
 - Measure network usage
 - Add more tests, such as repetitive add/remove cycles
 - Extend tester to support complex queries
 - It would be a great addition to esb to have a windows port of the client, to increase the number of client nodes available
