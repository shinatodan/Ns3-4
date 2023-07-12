# IGPSR

## Definition of IGPSR

Greedy Perimeter Stateless Routing, IGPSR, is a responsive and efficient routing protocol for mobile, wireless networks. Unlike established routing algorithms before it, which use graph-theoretic notions of shortest paths and transitive reachability to find routes, IGPSR exploits the correspondence between geographic position and connectivity in a wireless network, by using the positions of nodes to make packet forwarding decisions. IGPSR uses greedy forwarding to forward packets to nodes that are always progressively closer to the destination. In regions of the network where such a greedy path does not exist (i.e., the only path requires that one move temporarily farther away from the destination), IGPSR recovers by forwarding in perimeter mode, in which a packet traverses successively closer faces of a planar subgraph of the full radio network connectivity graph, until reaching a node closer to the destination, where greedy forwarding resumes. 

## Install NS3

https://www.nsnam.org/wiki/index.php/Installation

## Add IGPSR model

Download code

add IGPSR dictoray to ns3/src dictory 