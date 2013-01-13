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
 - [Jansson](http://www.digip.org/jansson/) (JSON Parser)
 - [Matplotlib](http://matplotlib.org/index.html) (Python lib for grahping)

**How To Use**



**Screenshots**
 - [CPU Utilization vs Number of Keys](http://i.imgur.com/zS5Uc.png)
 - [Memory vs Concurrent Clients](http://i.imgur.com/snGcS.png)
 - [Actual vs Requested Throughput](http://i.imgur.com/2V8DB.png)
 - [Latency vs Number of Keys](http://i.imgur.com/F3EUo.png)

**Future Developments**
 - Measure network usage
 - Add more tests, such as repetitive add/remove cycles
 - Extend tester to support complex queries
 - It would be a great addition to esb to have a windows port of the client, to increase the number of client nodes available
