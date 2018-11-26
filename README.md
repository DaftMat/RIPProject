# RIPProject - Network's courses technical project

This program executes a couple of virtual routers (via xterm). You have a few commands to manage the router and see the communication process between them.

## Compiling

Get in `src` directory then type the command :
```
make <test_topoX>
```
Where X must be between 1 and 5 :
```
topo1 : 
R1 -- R2 -- R3

topo2 :
R1 -- R2 -- R3
  \   |
   \- R4 -- R5
   
topo3 :
    R3
  /    \
 R1     R2
  \    /
    R4

topo4 :
R1 -- R2 -- R3 -- R4 -- R5
      |            |
      +- R6 -- R7 -+

topo5 :
R1 -- R2 -- R4 -- R5
  \    \
  R3 -- R6 -- R7
    \     \  /
     R9 -- R8
```
This will launch as much virtual routers (xterm processes) as there are in the specified topology.
The `logs` repertory will contain the communications between routers.
```
make clean
```
Will remove all the .o files, and all the logs files.

## Managing routers

First, you may be interrested in `help` command, here is what it will print all the possible commands :

* `ping <r>` 		: send echo request to the selected router.
* `pingforce <r>` 	: force the sending of a ping (until timeout, 1min).
* `traceroute <r>` 	: send a ping to r and show all the route it takes.
* `show ip route` 	: prints its routing table.
* `show ip neigh`	: prints its neighbors.
