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

**Screenshots**
 - [CPU Utilization vs Concurrent Clients](http://i.imgur.com/frS7wFq.png)
 - [Memory vs Concurrent Clients](http://i.imgur.com/G9GENpZ.png)
 - [Throughput vs Number of Keys](http://i.imgur.com/gnfyEiZ.png)
 - [Latency vs Number of Keys](http://i.imgur.com/JG8L38z.png)

**How To Use**

Please see [the wiki](https://github.com/charliehorse55/esb/wiki) for detailed usage information

**Future Developments**
 - Measure network usage
 - Add more tests, such as repetitive add/remove cycles
 - Extend tester to support complex queries
 - Reduce number of threads spawned for high numbers of clients
 - It would be a great addition to esb to have a windows port of the client, to increase the number of client nodes available
