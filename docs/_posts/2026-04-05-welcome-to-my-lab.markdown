---
layout: post
title:  "Welcome to my Lab"
date:   2026-04-05 12:07:33 +1000
categories: hardware, cisco
---

# Welcome to my Lab

I've built a small collection of hardware and software for exploring x.25 networking.  It contains Cisco routers from the 2500 series up to the more recent 1900s, along with an ISDN NTU.  I have a selection of network interfaces for them, some of which came with the routers and some were sourced separately.

## The Problem

X.25 networks used to be more common, but now they are rare and the hardware is harder to get.  In setting up this lab, it has been challenging to work out which interfaces work with which routers, under which ios versions and feature sets, to talk to what.

## Routers

### Cisco 1921/K9 - IOS 15.7(3)M9

This Cisco 1921 is running `Cisco IOS Software, C1900 Software (C1900-UNIVERSALK9-M), Version 15.7(3)M9, RELEASE SOFTWARE (fc1)` and is licensed as ipbasek9.  It has 512M of memory and 256M of flash.

{% highlight shell %}
c1921# show x25
X.25 software, Version 3.0.0.
...
XOT software, Version 2.0.0.
{% endhighlight %}

It has two EHWIC slots that can accept high-speed WAN interface cards (HWIC), Voice/WAN interface cards (VWIC) and WAN interface cards (WIC).

### Cisco 3845

...

### Cisco 2610 - IOS 12.1(27b)

This is a Cisco 2610 running `IOS (tm) C2600 Software (C2600-IS-M), Version 12.1(27b), RELEASE SOFTWARE (fc1)`.  It has 32M of memory and 16M of flash.

{% highlight shell %}
c2610#show x25
X.25 software, Version 3.0.0.
...
XOT software, Version 2.0.0.
{% endhighlight %}

It has two EHWIC slots that can accept high-speed WAN interface cards (HWIC), Voice/WAN interface cards (VWIC) and WAN interface cards (WIC).  It also has a Network Module slot.

A 2600-XM series device would have been a better choice, since later versions of IOS require more memory, and cards like the VWIC-1MFT-T1/E1 require at least IOS 12.3(14)T or 12.4(1).  This limits me to serial cards in the WIC slots.

The Network Module slot contains a PRI-2CE1B ISDN card, which I purchased to connect to an NTU (network termination unit).

[Cisco 2600 Series Hardware View][cisco_2600hw] is a good reference for discovering what cards work with this router.

### Cisco 2513

...

## WAN Cards

There are two main options for Cisco x.25 WAN cards:

*  **Synchronous Serial cards**.  Initially, I thought this was the only option.  Synchronous serial uses a shared clock signal for sending and receiving data.
*  **ISDN T1/E1 cards**.  I later learnt that PRI ISDN cards are a great way to connect routers and build x.25 networks.  They are more readily available than serial cards, and don't require special and expensive cables.  If you can crimp a network cable, you can build an ISDN crossover cable.

Both options permit multiplexing multiple virtual circuits over the same interface, usually at 2Mb/s.

### WIC-1T Synchronous Serial (DB-60)

![WIC-1T Serial Card](/assets/images/800/wic_1t_rear.jpg)
*WIC-1T Serial Card*

The WIC-1T supports a 2Mb/s serial connection.  It has a DB-60 receptacle on the card and supports different cables for:

* **Back to back** connection with other Cisco serial cards.
* **x.21**: A smaller connection typically used on external DCE/DTE devices for connecting to public networks.
* **V.35**: A large connector for x.25 public networks.

Cisco's [Understanding the 1-Port Serial WAN Interface Card (WIC-1T)][cisco_wic1t] page is a good starting point for this card.

#### Compatibility with my routers

| 2610     | 3845     | 1921          |
| ------   | ------   | ------------- |
| Detected | Detected | Not supported |

To use synchronous serial with the 1921, [this page][cisco_serial] says that I would need a HWIC-1T, HWIC-2T or a HWIC-2A/S card.


### WIC-2T Synchronous Serial (Smart Serial)

![WIC-2T Serial Card](/assets/images/800/wic_2t_rear.jpg)
*WIC-2T Serial Card*

The WIC-2T supports two 2Mb/s serial connections.  It has Smart Serial receptacle on the card and supports different cables for:

* **Back to back** connection with other Cisco serial cards.
* **x.21** and **V.35**
* **RS-232** and **RS-449** asynchronous serial.

Unlike the WIC-1T, this card does not have a CSU/DSU on board.  This may make it hard to use with x.25.  We will see.

Cisco's [Understanding the 2-Port Serial WAN Interface Card (WIC-2T)][cisco_wic2t] page is a good starting point for this card.

#### Compatibility with my routers

| 2610     | 3845     | 1921          |
| ------   | ------   | ------------- |
| ?        | ?        | Not supported |

To use synchronous serial with the 1921, [this page][cisco_serial] says that I would need a HWIC-1T, HWIC-2T or a HWIC-2A/S card.

###  PRI-2CE1B ISDN card

![PRI 2CE1B 2 Port Channelised E1/ISDN PRI](/assets/images/800/pri_2ce1b.jpg)
*PRI 2CE1B 2 Port Channelised E1/ISDN PRI*

The PRI-2CE1B supports two 2MB/s E1 connections with balanced or unbalanced 75 or 120 ohm connections.  It supports 30 E1 virtual channels (which is different to the thousands of x.25 channels that you can run over the connection).

I got this module because I wanted a card that supported G.703 to connect to my NTU.  Working out the pinout for the DB-15 (DA-15) connector was a pain.  The NM-2CE1T1-PRI was designed to replace this network module and comes with RJ-48C connectors.

Cisco's [ISDN PRI Network Modules][cisco_pri] page is a good starting point for this module.

#### Compatibility with my routers

| 2610     | 3845     | 1921          |
| ------   | ------   | ------------- |
| Yes      | ?        | No NM slots   |


[cisco_wic1t]: https://www.cisco.com/c/en/us/support/docs/interfaces-modules/1700-2600-3600-3700-1-port-serial-wan-interface-card/7265-hw-1t-wic.html
[cisco_wic2t]: https://www.cisco.com/c/en/us/support/docs/routers/3600-series-multiservice-platforms/7261-wic-2t.html
[cisco_serial]: https://www.cisco.com/c/en/us/products/collateral/interfaces-modules/high-speed-wan-interface-cards/datasheet_c78-491363.html
[cisco_2600hw]: https://www.cisco.com/web/ANZ/cpp/refguide/hview/router/2600.html
[cisco_pri]: https://www.cisco.com/c/en/us/td/docs/routers/access/interfaces/nm/hardware/installation/guide/ConntPRI.html
