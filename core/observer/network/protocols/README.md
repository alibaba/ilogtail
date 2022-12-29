# How to add a new protocol parser?

1. Add the specific protocol parser with unique dir, such as the redis dir.
2. Add the new protocol aggregator to ProtocolEventAggregators class.
3. Bind dispatcher to ConnectionObserver.h.
4. Bind statistics to interface/statistics.h