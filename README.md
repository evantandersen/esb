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
 - Slave supports Mac and Linux
 - [Jansson](http://www.digip.org/jansson/releases/jansson-2.4.tar.gz) (JSON Parser)

**How To Use**

esb requires 1 master, and at least 1 slave to function. The slave requires your implementation of the protocol defined in *storage_protocol.h*. Add your file into the slave directory under the name *storage_protocol.c*. After building the slave with the makefile, it can simply be started with no parameters. When it launches, it will print the local IP address of the computer. 

To run the master, it is required that a copy of your server executable be placed in the same directory as the master. As the master connects to every slave seperately, it must be told of each one. To launch the master:

python esb_master.py [slave IP addresses]

Output files are created in the working directory, labelled by the dependent variable. They can easily be graphed with gnuplot. 


**Screenshots**
 - [Number of Keys vs Memory Usage](http://i.imgur.com/YQoBY.png)
 - [Throughput vs Latency (With error bars)](http://i.imgur.com/LFR6c.png)
 - [Concurrent Clients vs Latency](http://i.imgur.com/fN86U.png)
 - [Requested vs Actual Throughput](http://i.imgur.com/0mKWC.png)
 - [Number of Keys vs CPU Utilization](http://i.imgur.com/F3EUo.png)

**Future Developments**
 - Measure network usage
 - Add more tests, such as repetitive add/remove cycles
 - Extend tester to support complex queries
 - gnuplot scripts for automated graphing
 - It would be a great addition to esb to have a windows port of the client, to increase the number of client nodes available
