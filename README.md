# miniddb
A microimplementation of ddbs

# Motivation
The goals of this project are to explore some basic themes:
* tcp client/server
* databases

The motivation comes from trying to find a use case for a TCP server. I decided to create a design for a ddb system.

# Implementation
My initial outlook requires:
* the system is scalable, it needs to run on a virtualized network of nodes which can be configured to connect to one another
* the system uses basic tcp/ip system calls

My extended vision:
* the system can be used to test c library implementations of database implementations
* the system can be configured to exaggerate networking conditions and network traffic to reveal the strengths and weaknesses of various database implementations 
