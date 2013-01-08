esb
===

The ECE297 Server Benchmark faclilitates quick and accurate distributed benchmarking. esb isolates a single variable and measures the effect is has on performance. A master coordinates with slaves to carry out the tests. 

**Indepdent Variables**
 - Number of active keys
 - Number of concurrent clients
 - Total server throughput (IOPs/s)

**Depedent Variables**
 - Reuqest Latency (mean / 95% confidence interval)
 - More to come

**Requirements**
 - Master requires linux
 - Slave supports Mac and Linux
 - [Jansson](http://www.digip.org/jansson/releases/jansson-2.4.tar.gz) (JSON Parser)
 - [Argtable](http://prdownloads.sourceforge.net/argtable/argtable2-13.tar.gz) (Argument Parsing)

**How To Use**

esb requires 1 master, and at least 1 slave to function. The slave requires your implementation of the protocol defined in *storage_protocol.h*. Add your file into the slave directory under the name *storage_protocol.c*. After building the slave with the makefile, it can simply be started with no parameters. When it launches, it will print the local IP address of the computer. 

To run the master, it is required that a copy of your server executable be placed in the same directory as the master. As the master connects to every slave seperately, it must be told of each one. To launch the master:

python esb_master.py [slave IP addresses]

An output file will be created in the working directory named "latency.txt" that contains the results. It can easily be graphed with gnuplot. 


**Screenshots**
 - [Number of Keys vs Latency](http://i.imgur.com/fLnUe.png)
 - [Throughput vs Latency (With error bars)](http://i.imgur.com/LFR6c.png)
 - [Concurrent Clients vs Latency](http://i.imgur.com/9qP0A.png)

**Future Developments**
 - Measure CPU/Memory resource usage (SOON)
 - Measure total throughput (SOON)
 - Measure network usage
 - Add more tests, such as repetitive add/remove cycles
 - Extend tester to support complex queries
 - gnuplot scripts for automated graphing

