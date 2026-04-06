---
layout: post
title:  "Serial and Smart Serial Back to Back"
date:   2026-04-06 12:00:00 +1000
categories: hardware serial
---

# Serial and Smart Serial Back to Back

I have built a small collection of hardware and software for exploring X.25 networking.  It contains routers, interfaces and bridges from the 1990s through to the early 2020s.

## The Problem

Cisco's [Understanding the 2-Port Serial WAN Interface Card (WIC-2T)][cisco_wic2t] page says that the WIC-1T does not have a CSU/DSU on board:

> Note: There are no framing, clocking or linecode parameters or commands being used here. This is because this card does not have an integrated channel service unit/data service unit (CSU/DSU). You need to use an external CSU/DSU.

A CSU/DSU (channel service unit/data service unit) is similar to a modem for a WAN, for example to connect it to ISDN.

If we want to connect two routers back to back using Smart Serial or DB60 connectors, do we need a CSU/DSU?




[cisco_wic2t]: https://www.cisco.com/c/en/us/support/docs/routers/3600-series-multiservice-platforms/7261-wic-2t.html
