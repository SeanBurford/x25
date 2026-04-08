---
layout: post
title:  "Linux as an XOT Client"
date:   2088-04-07 11:00:00 +1000
categories: xot
---

# Linux as an XOT Client

I have built a small collection of hardware and software for exploring X.25 networking.  It contains routers, interfaces and bridges from the 1990s through to the early 2020s.

[Welcome to my Lab](/hardware/cisco/2026/04/05/welcome-to-my-lab.html) has an introduction to the various devices discussed below.

## The Problem

Can we use a modern Linux machine as an X.25 XOT client?

This builds on [Linux X.25 and XOT](/xot/2026/04/07/linux-x25.html).  You may want to finish that first.

### Goals

1. TODO

### Build and Install X25-utils

x25-utils comes from 1997, but 

{% highlight shell %}
cd ~/src
mkdir x25-utils
cd x25-utils
wget https://www.nic.funet.fi/pub/ham/packet/linux/ax25/x25-utils-2.1.20.tar.gz
tar xzvf x25-utils-2.1.20.tar.gz
cd x25-utils-2.1.20/
{% endhighlight %}

We need libbsd-dev since all of these tools rely on `/usr/include/bsd/bsd.h`:

{% highlight shell %}
sudo apt-get install libbsd-dev
{% endhighlight %}

In the Makefile we need to update the path to bsd.h:

{% highlight shell %}
# IBSD=-I/usr/include/bsd -include /usr/include/bsd/bsd.h
IBSD=-I/usr/include/x86_64-linux-gnu/ -include /usr/include/x86_64-linux-gnu/bsd/bsd.h
{% endhighlight %}

#### x25trace

`x25trace` is the easiest tool to build:

{% highlight shell %}
cd ~/src/x25-utils/x25-utils-2.1.20/trace
make
{% endhighlight %}

It acts a bit like tcpdump but for X.25:

{% highlight plaintext %}
sudo ./x25trace
Port: tun0
X.25: LCI 0FF : CALL REQUEST - 701001 -> 999888
Packet Size:     0B 0B
Window Size:      7  7
0000  01 00 00 00                                      | ....

Port: tun0
X.25: LCI 0FF : CLEAR CONFIRMATION

Port: tun0
X.25: LCI 0FF : CALL REQUEST - 701001 -> 999999
Packet Size:     0B 0B
Window Size:      7  7
0000  01 00 00 00                                      | ....

Port: tun0
X.25: LCI 0FF : RR R1

Port: tun0
X.25: LCI 0FF : RR R2

Port: tun0
X.25: LCI 0FF : DATA R2 S0 
0000  s
{% endhighlight %}

[ax25]: [https://www.nic.funet.fi/pub/ham/packet/linux/ax25/
