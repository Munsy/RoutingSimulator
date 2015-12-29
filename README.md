# RoutingSimulator
A simulation of router communications over a network. Written in C++.

## Purpose
This project was designed with the following goals in mind:
- To understand the dynamic behavior of the distance-vector algorithm.
- To learn about programming with datagram sockets.
- To learn about socket demultiplexing and time management using the **select()** system call.

## Specifications
The project consists of a single program that is implemented on multiple, communicating 'routers,' which are emulated by processes. Each router instance will determine its local links by reading from files when the the `Run()` command is executed. 
One file, which is labled as `routers` in each `test` directory, contains a list of all the routers in the system, the hostname on which they run, and the beginning port number that they use. Each of the other files in the given `test` directory will specify the link information for each router. Those directories, labled `A.cfg`, `B.cfg`, `C.cfg`, etc, correspond to routers named `A`, `B`, `C`, etc, respectively. The predefined test cases use `localhost` for the hostname and a fixed set of port numbers. *You are free to edit this file to allow different routers to run on different hosts or to use different port numbers*.

## Running the project
Each router should be invoked like this:

`
router [-p] testdir routername
`

Where `testdir` is the name of the directory containing the configuration files and `routername` is a single-letter router name corresponding to an entry in the list of routers. The `-p` switch controls the use of poisoned-reverse.
The router initializes its routing table and distance-vector with entries for all directly-connected neighbors. Non-neighbors can either be added dynamically or entered initially with an infinite cost. *For brevity's sake, infinity is defined as 64.*

## Message Protocol
###Router update messages
Each router learns of better paths to other routers by receiving distance vector updates from its neighbors. The neighbors send distance vector update messages that look like:

`
U d1 cost1 d2 cost2 … dn costn
`

That is, the letter `U` followed by a space, followed by a single-letter destination name, followed by a space, followed by the cost of reaching the destination from the neighbor expressed as a sequence of ascii decimal digits. The di and costi fields are repeated for as many entries as the sending router has in its own distance vector.

###Link cost messages
The routers may receive link-cost changes as well. These look like:

`
L n cost
`

Where `n` is the single-letter name of a neighbor, and `cost` is the new cost of reaching that neighbor directly (as decimal digits). Link cost messages will only be received for existing links, but the cost of a link may change to infinity and back. Upon receiving either of these messages, the router makes the appropriate changes to its own distance-vector and to its routing table, and if there are changes, it sends the changed distance-vector immediately to its neighbors using `U` messages.

###Output
Whenever a routing table entry is added or changed, the program will print a message in the following format:

`
<r> - dest: <d> cost: <c> nexthop: <n>
`

Where `<r>` is the name of the router making the change, `<d>`, `<c>`, and `<n>` are the destination name, cost, and nexthop name. The remainder of the pattern, `<n>`, consists of literal characters.
Each router responds to an incoming datagram consisting of:

`
P d
`

by printing its routing table entry for destination `d`. If the `d` is omitted, all of the entries in the routing table will be printed.

##Implementation Notes
The links between neighboring routers are simulated using connected datagram sockets, using a separate socket to communicate with each neighbor. Although it is not usually necessary to call `connect()` for datagram sockets (after calling `bind()` to establish the port number for the local end), do in this project for the sockets used to communicate between routers. There are three reasons for this: 

1. We can use `send()` and `recv()` instead of `sendto()` and `recvfrom()`.
2. We don't have to look up the full IP address/port number when messages are received in order to know where they came from.
3. This forces us to use `select()` in order to handle multiple sockets, which is one of the key points of this project.

Connected datagram sockets send and receive datagrams only to/from the host/port to which they are connected. Unlike with TCP, calling `connect()` for a datagram socket does **not** exchange setup messages with the other end. Instead, it simply sets the destination and port for the local socket. If datagrams are sent to a connected socket from an address/port that it is not connected to, the messages are simply discarded.
In addition to these connected sockets, the router also creates another unconnected datagram socket bound to the local baseport to receive `L` and `P` messages (never `U` messages – those come only from neighbors on the appropriate connected sockets).
Each router has a socket associated with `baseport`, and additional sockets with portnumbers `baseport + 1, baseport + 2, …, baseport + numneighbors`. **If you run multiple routers on the same machine you must ensure that these ranges of port numbers do not overlap**.

###Using the select() system call
At any given time, the next message that the router has to process may arrive on any of the sockets. Additionally, each router is required to send its entire distance vector to its neighbors, even in the absence of incoming messages,
every 30 seconds. To implement this behavior, we use the `select()` system call to find out which socket descriptor(s) have available messages and issue `recv()` calls only for those descriptors. Also, `select()` provides the mechanism we need for determining when 30 seconds have passed.

##Extra Resources
Extra programs have been provided to easily send the `P` and `L` control messages. These are Python scripts, so they will work on any machine where the Python interpreter is installed in /usr/bin/python.
To send 'print' messages, use the `printtables` script:

```
printtables testdir r
printtables testdir
```

The former will trigger the printing of router `r`’s table, while the latter will trigger the printing of all routers’ tables, by sending `P` messages to them.

To update the cost of a link use, the `changelink` script:

`
changelink testdir r1 r2 c
`

In the above example, the `changelink` script will send the appropriate `L` messages to `r1` and `r2`.
The `testdir` argument is needed because the program reads the router's configuration file in order to find the appropriate host and port destination for the messages that each router sends.

Another handy program is `generateTest`, which you can use to make up your own test networks:

`
generateTest testdir
`

In the above example, `generateTest` reads the file `testdir/links` and produces the files `testdir/routers` along with `testdir/<routername>.cfg` for each router. `testdir/links` is just a list of links, one per line, consisting of the names of the two endpoints and the cost of the link. If any node of the graph has a degree greater than 9, this program will still work, but the generated configuration file will have problems. You can fix this by changing `port = port + 10` near the bottom to `port = port + 20`.

## References
https://tools.ietf.org/html/rfc1058
http://linux.die.net/man/2/select
https://en.wikipedia.org/wiki/Distance-vector_routing_protocol
https://en.wikipedia.org/wiki/Route_poisoning

## Co-Authors
- Justin Sloan
- Alex Kerr
- Tim Munson
