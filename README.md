# Description
Implementation of a network stack based on http://www.saminiir.com/lets-code-tcp-ip-stack-1-ethernet-arp/

# Implemented features
* Ethernet
* ARP

# Build and usage
Tested with gcc version 13.1.1
```
$ cmake build
$ cmake --build build
$ ./build/setup.sh 
$ ./build/network_stack 10.0.0.4
Created TAP device tap0
```
in another terminal:
```
$ ./build/setup_dev.sh tap0
$ sudo arping -I tap0 10.0.0.4
```
