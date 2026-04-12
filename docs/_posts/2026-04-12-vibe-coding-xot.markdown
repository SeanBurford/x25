---
layout: post
title:  "Vibe Coding XOT"
date:   2026-04-12 06:00:00 +1000
categories: xot
---

# Vibe Coding XOT

I have built a small collection of hardware and software for exploring X.25 networking.  It contains routers, interfaces and bridges from the 1990s through to the early 2020s.

[Welcome to my Lab](/hardware/cisco/2026/04/05/welcome-to-my-lab.html) has an introduction to the various devices discussed below.

## The Problem

I have a vision for an XOT router, but I don't have the time to bring it to life.  I outline it in [XOT to XOT Router (attempt 1)](/xot/2026/04/10/xot-to-xot-1.html)

## Vibe Coding a Solution

I would like to think that I have high standards for design and coding, and that X.25 and XOT are too complicated for a large language model.  I'm flexible though, let's give this a shot.

### Setup

I opened up Google AI Studio, selected Gemini 3 Flash Preview and started prompting.

### Things that impressed me

#### Prompt Assistance

When writing the initial prompt, additional sentences such as "The application needs to be robust and handle network errors gracefully." were auto suggested.  This set the tone for the level of detail that I should provide.

#### Debugging

After each round AI Studio tells you what has changed, which has given me insights that would have been learnt the hard way otherwise (hours of reading docs and third party code).  For example I asked:

> When another program tries to establish an X.25 call, it is able to bind a socket but hangs in connect. Here is what strace shows:
>
> ...
> 
> GoXOT shows "2026/04/12 15:16:04 Error parsing X.25 packet: packet too short" when that connect() occurs, and strace of GoXOT shows:
>
> [pid 6574] read(3, "\1", 4096) = 1
>
> I think that either Linux is sending the CALL REQUEST one byte at a time through the TUN interface ("\1" would be the LCI in that case), or the TUN interface has some kind of handshake required to accept a call? It is probably the former.

Which lead to me learning something:

> Link-Layer Handshake (main.go)
> *  Link-Up Detection: The gateway now detects the 1-byte 0x01 "Link Up" request from the Linux kernel. It responds by writing 0x01 back, which transitions the TUN link to the "UP" state.

I also learnt a bit about TUN handling:

> Connect still hangs because our connect ack still returns EINVAL.
>
> Here is the strace:
>
> [pid 8657] <... read resumed>"\1", 4096) = 1
>
> [pid 8657] futex(0x68a760, FUTEX_WAKE_PRIVATE, 1) = 1
>
> [pid 8654] <... futex resumed>) = 0
>
> [pid 8657] write(2, "2026/04/12 16:42:15 <TUN (Header: 0x01) \n", 412026/04/12 16:42:15 <TUN (Header: 0x01)
>
> ) = 41
>
> [pid 8654] epoll_pwait(4, <unfinished ...>
>
> [pid 8657] write(3, "\1", 1 <unfinished ...>
>
> [pid 8654] <... epoll_pwait resumed>[], 128, 0, NULL, 0) = 0
>
> [pid 8657] <... write resumed>) = -1 EINVAL (Invalid argument)

Resulting in a description of how TUN detects packet types:

> Root Cause Analysis:
>
> The EINVAL was caused by a specific behavior in the Linux kernel's TUN driver. When IFF_TUN and IFF_NO_PI are both set, the kernel attempts to "guess" the protocol of the outgoing packet by looking at the first 4 bits of the payload. It expects 0x40 (IPv4) or 0x60 (IPv6). Since X.25 pseudo-headers (like 0x01 for Connect Request) do not match these patterns, the kernel rejects the write with EINVAL.
>
>
> Solution:
> *  Removed IFF_NO_PI: I reverted the interface configuration to use the default PI (Packet Information) header.
> Manual PI Header Management:
> * Writes: I now prepend a 4-byte PI header to every packet written to the TUN device. This header explicitly sets the protocol to ETH_P_X25 (0x0805), which informs the kernel exactly how to handle the packet without guessing.
> * Reads: I updated ReadTun to strip the 4-byte PI header from incoming packets, ensuring the rest of the gateway logic receives the expected 1-byte X.25 pseudo-header and payload.

#### Less backlog

Nice to haves are easy, for example:

> When I start GoXOT I have to bring the interface up and give it a route by hand. The commands I have been using are "ip link set tun0 up" and "route --x25 add 701/3 tun0". Are these available as IOCTLs?

Results in:

> 1. Automated Interface & Route Setup (tun.go)
>    * BringUpInterface: Automatically sets the IFF_UP and IFF_RUNNING flags on the TUN interface using SIOCSIFFLAGS.
>    * AddX25Route: Configures X.25 routes via SIOCADDRT.
>    * SetupTun: Now automatically brings the interface up upon creation.

Leading to:

> Routes should be added and removed based on the prefix fields in config.json, rather than as a separate section in the config.

Which was immediately taken care of.

#### Refactoring

> Let's refactor out the tun handling into it's own process, tun-gateway. This simplifies xot-server and xot-gateway and also allows them to run unprivileged.
>
> xot-server can then interface between incoming XOT sessions and either xot-gateway or tun-gateway using unixpacket sockets. This enables us to simplify xot-server to run unprivileged. It listens for incoming TCP connections, accepts them, and spawns a process handle the connection. If the destination X.121 address is in the existing config file, xot-gateway makes a unixpacket socket connection to xot-gateway and relays XOT packets back and forth. If the destination is not in the config file, xot-server instead connects to tun-gateway. xot-server no longer needs to manage LCIs,
>
> ...
>
> It is very important to remain RFC 1613 compliant.
>
> Finally, update the README.

Done.

### Things that will improve

#### Access to the Golang Tool Chain

Gemini does not have a Golang tool chain available to it, so it cannot run tests and will sometimes fail to update package names and dependencies during refactors.  After a refactor you usually have to give it the build time errors and have it fix them up.

Asking for "Test coverage should aim for 80%" is meaningless since Gemini apparently can't measure test coverage without a Golang tool chain.  It's a while since I asked it to improve the tests, and coverage cuurently sits at 50%:

{% highlight shell %}
go test -cover .
ok  	xot-gateway	0.108s	coverage: 50.2% of statements
{% endhighlight %}

#### You have to be precise

Gemini doesn't ask clarifying questions.  I gave it apparently conflicting requests over time which resulted in bugs.  For example "If a call does not match any entries in the config file, the application will log the request but not respond to it." so that TUN to TUN calls would not be rejected, and then later I asked for "When shutting down, active XOT calls should be terminated with CLEAR packets."  This caused the first command to be overridden.

#### Sometimes it is wrong

Just like me, sometimes it is wrong:

> We can now connect to the Cisco router over XOT, however the router sends data that the client never receives.  Here are the logs:

To which Gemini replied:

> I have added comprehensive trace logging for all outgoing data to both the TUN and XOT interfaces. When the -trace flag is enabled, you will now see entries starting with > indicating data being sent, for example:
>
> ...
> 
> Analysis of the Data Relay Issue:
>
> Based on the logs you provided and the Cisco router's output, it appears the data is being forwarded to the TUN interface, but the Linux kernel is rejecting it with a Reset Request (10 01 1B 00 00).
> 
> A likely reason for this is the Q-bit (Qualified Data) in the first data packet (90 01 00 04). In X.25, the Q-bit is often used for PAD (Packet Assembler/Disassembler) control. If the Linux X.25 stack is not configured to handle PAD traffic or if the application isn't expecting it, the kernel might see this as an invalid packet for the current state and reset the circuit.

I went to investigate that, and in the mean time asked:

> Check our code for compliance with RFC 1613.

Which revealed the problem:

> I have completed the RFC 1613 compliance check and implemented several critical improvements to ensure interoperability and protocol transparency.
> 
> RFC 1613 Compliance & Key Improvements:
> 
> X.25 Packet Transparency (Resolved Data Relay Issue):
> * The Problem: Our previous implementation was zeroing out the 3rd byte of the X.25 header for data packets during serialization. This byte contains the P(S) and P(R) sequence numbers. By resetting these to 0, we were causing sequence errors in the Linux X.25 stack, leading it to immediately send a Reset Request and hang the connection.
> * The Fix: I refactored x25.go to preserve the entire 3rd byte. I added IsData() and GetBaseType() helpers to X25Packet to allow the gateway to identify packet types (like Call Request or RR) using bitmasks while keeping the sequence bits intact.

## Summary

Gemini has dramatically sped up my development time on this project.  It takes care of looking through code and documentation to resolve bugs, and it has suggested some good approaches to the problems that we have encountered.  Refactoring and implementing nice to have features just requires clearly describing what you need.  I do have to keep reminding it to maintain RFC compliance, update tests and documentation and things like that.

Best of all, in less than a day of effort I have a working XOT to XOT Router prototype:

{% highlight plaintext %}
❯ ~/src/xotpad/target/debug/xotpad -g 192.0.2.100 -a 127001 701001||stty sane

c1921.lan on line 137.

C i s c o  S y s t e m s
      ||        ||
      ||        ||    
     ||||      ||||   
 ..:||||||:..:||||||:..      



User Access Verification

Username: 
Password: 

% Authentication failed
{% endhighlight %}

