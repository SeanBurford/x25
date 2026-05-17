---
layout: post
title:  "Linux X.25 Loopback"
date:   2026-05-17 06:00:00 +1000
categories: software
---

# Linux X.25 Loopback

I have been developing a range of X.25 software for Linux, mostly under [goxot][goxot].  The trigger for this was that I wanted to bridge inbound XOT traffic back out to XOT destinations, something that wasn't possible at the time.

More recently I wanted to host some of my software as a tech demo, however I encountered another problem with this which would require running multiple machines.  I wanted to:

1.  Accept email connections.
2.  Bridge the email to X.25 using an RFC 1090 sender (`AF_X25 connect()`).
3.  Accept the email using an RFC 1090 receiver (`AF_X25 listen()`).
4.  Store the email and make it accessible.

Going from step 2 to step 3 proved challenging.  Linux makes it easy to:

*  Accept X.25 traffic on an **interface** (serial, ISDN, TUN) and send it to a **local process**.
*  Accept X.25 traffic from a **local process** and send it to an **interface**.
*  Accept X.25 traffic from an **interface** and send it out another **interface** (if the sysctl `net.x25.x25_forward` is enabled)

Linux does not have provision to accept X.25 traffic from a **local process** and send it to **another local process**.  I didn't want to maintain two machines for the demo.

## Goals

Determine why we can't get traffic between local processes, and then enable such traffic.

Not only will this help with my deployment, it will make development of X.25 software easier (by containing development to a single machine).

## Why it doesn't work

I cloned the latest kernel source and had Claude create `CLAUDE.md` files at various levels under `/net/`, `/Documentation/` and `/drivers/` to facilitate cheaper faster analysis of this kind of problem.  With that in place, I asked for an analysis of four routing scenarios (local to local, local to interface, interface to local and interface to interface).  You can read that analysis at [Linux X.25 Routing][x25-routing].

This identified the problem:

*  Linux routes new `connect()` calls based on the routing table, which supports sending frames to interfaces.  It doesn't consider listening sockets as possible destinations.
*  The recieve path for incoming frames starts at `x25_lapb_receive_frame()`, which is not reachable from an `AF_X25` socket.

The analysis also showed that this problem could not be addressed by receiving and retransmitting frames using a single TUN device (e.g. by adding functionality to `tun-gateway`).  The problem there is that, since the kernel already knows the connection ID (LCI), reinjecting the packet through the same interface will cause the packet to be dropped as a duplicate call request.

## Making it work

[goxot][goxot] now has a new tool: [tun-loopback][tun-loopback].  It uses the same `config.json` file that is shared with the rest of goxot, where it is configured with:

*  A range of LCIs to use.  It is smart to use a different range for `tun-gateway` (incoming calls from XOT) and `tun-loopback` so that they don't allocate LCIs that conflict with the other process.  It is OK to reuse the same LCI on different loopback TUNs, since these communicate directly with the target process (and the kernel is happy for LCI reuse to occur across different neighbours (TUN devices)).
*  A list of addresses to allocate.

You can read the [tun-loopback README][tun-loopback] for usage details.

For each address, `tun-loopback` creates a TUN device and adds a route for that address pointing to the TUN.  As it receives traffic, it finds the right TUN to send that traffic out.  Very simple.  The basic idea is expressed in this diagram:
```
X.25 application A          X.25 application B
  connect("addrB")            listen("addrB")
       │                             │
  AF_X25 socket                AF_X25 socket
       │                             │
  tunlb0 (route: addrA)       tunlb1 (route: addrB)
       │                             │
       └──── tun-loopback relay ─────┘
              (reads tunlb0, writes tunlb1 and vice versa)
```

### Stress Test Results

I created a goxot `config.json` that allocated a range of connection numbers (LCI) to the loopback and started the loopback process:
```json
{
  "tun-gateway": {
    "lci_start": 1024,
    "lci_end": 3071,
    "stats-port": 8003
  },
  "tun-loopback": {
    "lci_start": 3072,
    "lci_end": 4095,
    "stats-port": 8004,
    "routes": ["127001", "127002"]
  },
...
```

```
sudo go run cmd/tun-loopback/main.go ../dist/config.json 
2026/05/17 14:20:08 Configuration reloaded from config.json
Stats server listening on :8004
2026/05/17 14:20:08 TUN interface tunlb0 ready
2026/05/17 14:20:08 tun-loopback: tunlb0 → address 127001 (LCI range 3072-4095)
2026/05/17 14:20:08 TUN interface tunlb1 ready
2026/05/17 14:20:08 tun-loopback: tunlb1 → address 127002 (LCI range 3072-4095)
2026/05/17 14:20:08 tunlb0: link operational
2026/05/17 14:20:08 tunlb1: link operational
```

Then I ran a stress test.  I started the receiver with `./stress_test -r -a 127002 -W 7 -P 4096 ` and the sender with `./stress_test -l 8192 -N 128 -d 127002,127002 -b 50 -T 20 -n 100000 -W 7 -P 2048`.

On the first run, performance was terrible (about 500KB/s).  On the second run it was worse (about 90KB/s).

When performance gets worse on each run, you're probably leaking sockets, so I checked `/proc/net/x25/socket` and found thousands of sockets in state 2 (partially torn down, waiting for `CLR_CONF`).  This was becase `tun-loopback` was clearing its internal state for a connection when it was a `CLR_REQ` request packet, then never forwarding to the kernels `CLR_CONF`.  Session tracking has to be maintained until `CLR_CONF` has been relayed.  There were a couple of other session setup/teardown races that were also fixed.

With that fix in, performance is good:
```
./stress_test -l 8192 -N 32 -d 127002,127002 -b 50 -T 20 -n 100000 -W 7 -P 2048

--- Stress Test Summary ---
Run Time: 19.75 seconds
Calls Made: 50605
Calls Received: 0
Calls Failed: 9
Packets Sent: 50596
Packets Received: 50596
Bytes Sent: 206526055
Bytes Received: 206526055
Data Mismatches: 0

--- Errors/Timeouts ---
Socket Errors: 0
Setsockopt Errors: 0
Bind Errors: 0
Facilities Errors: 0
Short Receives: 0
Write Errors: 0
Accept Errors: 0
Connect Timeouts: 0
Packet Size Negotiated (In):  Min: 2048, Max: 2048
Packet Size Negotiated (Out): Min: 2048, Max: 2048
Window Size Negotiated (In):  Min: 7, Max: 7
Window Size Negotiated (Out): Min: 7, Max: 7
Average Bandwidth (Sent): 10213.05 KB/s
Average Bandwidth (Recv): 10213.05 KB/s
---------------------------
```

This demonstrates the value of having a stress test tool.  Without that, the slow build up of improperly closed sockets would not have been noticed at low load (since they time out after 200 seconds), but would have impacted performance at higher load.

## Summary

[tun-loopback][tun-loopback]:

*  Providing a fast and reliable local X.25 connection between processes.
*  Enables development and testing on one machine.
*  Enables deployment of tech demos using one machine.

[goxot]: https://github.com/SeanBurford/goxot/
[x25-routing]: https://github.com/SeanBurford/goxot/blob/main/docs/tech/linux_x25_routing.md
[tun-loopback]: https://github.com/SeanBurford/goxot/tree/main/src/cmd/tun-loopback
