# remoteDistributedHashtable
#### Source code for a remote, distributed hashtable spread over an arbitrary number of computers as well as the system of bash scripts used to implement and test the code remotely. 

## src file
There are two main packages of code, client-side code (called "sender") and server-side code (called "receiver"). The server network collectively receives, stores, and updates a collection of key-value pairs sent by a set of clients. Each client establishes and maintains a unique TCP connection with each node in the server network. A particular key-value pair update or query is mapped to a particular node in the server network according to a hash on the key. Keys and values are randomly generated by the client nodes.

### Client functionality
The client sends puts and gets for key, value pairs to an abstracted, remote distributed hashtable, where for an individual put or get, a client must interact with two servers because all key value pairs are stored redundantly on two servers. The client executable uses 8 threads to separately generate get and put commands for the system. Each thread is bound to 5 sockets, each socket being used to connect it to one of the five servers in the system. With 8 threads, the client executable binds to 40 total sockets, and with 5 clients, there are 200 TCP connections in the whole system.

Each thread will generate a put or a get on the distributed hash table. I call this an abstract hashtable because it is implemented over 5 different nodes. The key of the put/get determines which of the 5 nodes will be responsible for maintaining the key, value pair. It selects the servers for the put or get based on the value of a key, which is a string.

The client executable takes up to three parameters: the file which lists the static IP addresses of the server nodes (mandatory), the size of the key string in all key-value pairs (I set this between 1 and 4), and the size of key range, which is used to allocate key value pairs to specific server nodes (it was not expected to play a role in performance, but it does appear to) as can be seen in the second pair of graphs.

The client, when it completes it’s series of commands, writes the results of the commands to a file and terminates. This client also can write commands to a log, which was used for debugging.

### Server functionality 
The main thread listens for incoming connections and binds sockets for dealing with new  incoming connections to memory. Each server is bound to 40 sockets to deal with communicating with 5 clients’ 8 threads. I use a threadpool with 8 threads to deal with each of these 40 sockets. Strangely enough, the system still works. I would have expected to need 40 threads. Each TCP connection is somehow managed by a thread which deals with incoming and outgoing messages between the server and a particular client. The server executable can take a single parameter, the size of the key range used to allocate key-value pairs to specific server nodes (it was not expected to play a role in performance but it appears to). Each server is responsible for a redundant range of keys.

### TCP connections/Two-phase commit
The servers and clients each maintain a set of connections to each other which are maintained until the server is finished running and breaks the connection. 

Messages are passed between servers and clients via a two-phase commit, which is necessary to keep consistency on the server system across the redundant key, value pairs which are stored in different nodes. As a result in order for a client to complete a command, or learn the result of the command it executed, 8 messages must be sent, which is four messages exchanged between the two client-server relationships that are relevant to the command. These commands are sent over TCP connections, using a buffer of 256 char characters. Each char is 1 byte large, so 256 bytes x 8 messages x 8 bits per byte = 16,384 bits per complete command. With a maximal rate of 100Mbps, the best I could hope for is 6,102 commands per second. The rate that I got was 2,400 commands per second, roughly 40 Mbps. Some bitrate must be lost to overhead as well. Some loss could be due to latency in the binary executables.

For this project, the data is stored redundantly, on two machines. These machines must share the same information at all times so if one machine dies, no data is lost. 

Visualizing all possible communication patterns between the clients and servers was very useful for creating and verifying the two-phase commit protocol. When a client starts communicating with a set of servers in order to put or get a key-value pair from the remote distributed hashtable, a state-space machine is implicitly created, which is represented in the visual below.
![Remote Distributed Hashtable](https://user-images.githubusercontent.com/31664870/134083045-5edf62df-1cfd-4719-9eff-396598f30a93.jpg)

### Lessons 
* Printing out very organized message statements with full information was an innovation that made debugging more manageable easier. 
* Giving identifiers to each of the messages and mapping out the different paths was a key innovation in debugging as well.
Use three debugging environments: local machine server to local client
* One server (one thread), two clients
* One server (multiple threads), two clients
* Full 5 node system

### scripting files
I used a set of 9 bash scripts to ease the process of updating and managing the code on each of the remote servers/clients while I was debugging, as well as the collection and assembly of performance data. When running experiments with the system, the server-side binaries were first started, then the client-side binaries were started. 

### implementation architecture
In my implementation I used 5 raspberry pi nodes that are connected over Ethernet via an Ethernet switch (using a TCP connection). The maximal rate of flow into between any two nodes is 100 Mbps full duplex. The communication rate is also potentially bottlenecked constricted by the 100Mbps bitrate of each Ethernet switch’s outgoing ports. The pis are connected to the switch via USB 3.0 to Ethernet converters, which have a bitrate of 300+ Mbps. The pis can also write at a rate of roughly 300 Mbps. Each machine runs two binary executables: a client binary, and a server binary. See image below. 

<img width="189" alt="Capture" src="https://user-images.githubusercontent.com/31664870/134083145-e2f94eb8-a311-4f8f-816c-1ab88b5995cf.PNG">




