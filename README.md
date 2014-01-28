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

**Screenshots**
 - [Throughput: Actual vs Requested](http://i.imgur.com/pCI7Kwg.png)
 - [Memory vs Number of Keys](http://i.imgur.com/2jldbT9.png)
 - [Throughput vs Number of Keys](http://i.imgur.com/wcO4o3p.png)
 - [Latency vs Throughput](http://i.imgur.com/neWhbqi.png)

**How To Use**

Please see [the wiki](https://github.com/charliehorse55/esb/wiki) for detailed usage information

**Future Developments**
 - Measure network usage
 - Add more tests, such as repetitive add/remove cycles
 - Extend tester to support complex queries
