---
layout: post
title:  "Linux as an XOT Client"
date:   2026-04-08 11:00:00 +1000
categories: xot
---

# Linux as an XOT Client

I have built a small collection of hardware and software for exploring X.25 networking.  It contains routers, interfaces and bridges from the 1990s through to the early 2020s.

[Welcome to my Lab](/hardware/cisco/2026/04/05/welcome-to-my-lab.html) has an introduction to the various devices discussed below.

## The Problem

Can we use a modern Linux machine as an X.25 XOT client?

This builds on [Linux X.25 and XOT](/xot/2026/04/07/linux-x25.html).  You may want to finish that first.

### Goals

1. Create C code that connects to X.25 using the Linux X.25 stack.
   * Set the packet and window sizes.
   * Set the PID in the call user data to request a PAD connection.
2. Forward that connection to a router over XOT (X.25 over TCP).

## Create C code that connects to X.25

The [x25 Linux manual page][man_x25] describes how to interface with X.25 on Linux.  It's quite short.  Working from the [pad_svr][pad_svr] and `/usr/include/linux/x25.h` provides good guidance on the proper IOCTLs to use.

You can see the [completed code in my repo][gh_repo].

### Create an X.25 socket

Creating an X.25 socket is a lot like creating any other socket, except that we specify X25 and SEQPACKET.  Contrast this with HP-UX where we would use `socket(AF_CCITT, SOCK_STREAM, X25_PROTO_NUM)`

{% highlight c %}
int sock = socket(AF_X25, SOCK_SEQPACKET, 0);
if (sock < 0) {
	perror("socket");
	return -1;
}
{% endhighlight %}

### Configuring the local address

To ensure proper source addresses appear in `/proc/net/x25/socket` and at the remote end, we set the source address:

{% highlight c %}
struct sockaddr_x25 laddr;
memset(&laddr, 0, sizeof(laddr));
laddr.sx25_family = AF_X25;
strncpy(laddr.sx25_addr.x25_addr, local_addr, X25_ADDR_LEN);
if (bind(sock, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
	perror("bind");
	return -1;
}
{% endhighlight %}

### Set up the Call User Data to request a PAD connection

There are a heap of protocols that can be used over an X.25 connection:

*  **PAD**: An interactive session, typically to a console on a router or similar.
*  **IP**: Send and receive IP packets over an X.25 link.
*  **SMTP**: Send and receive email without using TCP/IP (RFC 1090).

The call user data field is used to request a protocol. 0x01000000 is PAD.

{% highlight c %}
struct x25_calluserdata cud;
memset(&cud, 0, sizeof(cud));
cud.cudlength = 4;
cud.cuddata[0] = 0x01;
cud.cuddata[1] = 0x00;
cud.cuddata[2] = 0x00;
cud.cuddata[3] = 0x00;
if (ioctl(sock, SIOCX25SCALLUSERDATA, &cud) < 0) {
	perror("ioctl(SIOCX25SCALLUSERDATA)");
	return -1;
}
{% endhighlight %}

### Request specific window sizes and packet sizes

If we care about window and packet sizes we can request them in the facilities field.

I noticed that there is no support for negotiating access to Closed User Groups, however if needed a `map` on the router side could take care of that.

{% highlight c %}
struct x25_facilities facilities;
memset(&facilities, 0, sizeof(facilities));
facilities.winsize_in = 4;
facilities.winsize_out = 4;
facilities.pacsize_in = 9; // 2^9 = 512 bytes
facilities.pacsize_out = 9;
if (ioctl(sock, SIOCX25SFACILITIES, &facilities) < 0) {
	perror("ioctl(SIOCX25SFACILITIES)");
	return -1;
}
{% endhighlight %}

### Connect to a remote address

Finally, we use connect with a `sockaddr_x25` to connect to the remote end.

{% highlight c %}
// Connect to remote address.
struct sockaddr_x25 raddr;
memset(&raddr, 0, sizeof(raddr));
raddr.sx25_family = AF_X25;
strncpy(raddr.sx25_addr.x25_addr, remote_addr, X25_ADDR_LEN);
if (connect(sock, (struct sockaddr *)&raddr, sizeof(raddr)) < 0) {
	perror("connect");
	show_cause_x25(sock);
	return -1;
}
{% endhighlight %}

### Cause and diagnostic codes

If there is a failure, X.25 communicates a cause and diag code to help work out what the problem is.  This is exposed through an IOCTL on Linux:

{% highlight c %}
void show_cause_x25(int sock) {
        struct x25_causediag causediag;
        memset(&causediag, 0, sizeof(causediag));
        if (ioctl(sock,SIOCX25GCAUSEDIAG, &causediag) < 0) {
                perror("ioctl(SIOCX25GCAUSEDIAG)");
                return;
        }
        printf("C: %d D: %d\n", causediag.cause, causediag.diagnostic);
}
{% endhighlight %}

### Querying negotiated connection parameters

We can also query the connection parameters via another IOCTL:

{% highlight c %}
void describe_x25(int sock) {
        struct x25_facilities facilities;
        memset(&facilities, 0, sizeof(facilities));
        if (ioctl(sock,SIOCX25GFACILITIES, &facilities) < 0) {
                perror("ioctl(SIOCX25GFACILITIES)");
                return;
        }
        printf("Packet sizes: in: %d, out: %d\n", 1<<facilities.pacsize_in, 1<<facilities.pacsize_out);
        printf("Window sizes: in: %d, out: %d\n", facilities.winsize_in, facilities.winsize_out);
}
{% endhighlight %}

## Forward the connection to a router over XOT

Depending on what X.25 routing is in place, the connection will flow through a physical serial or ISDN card, or through software to an XOT destination.

If you follow [Linux X.25 and XOT](/xot/2026/04/07/linux-x25.html), running `jh-xotd` establishes a `tun` interface that is the default route.  Traffic sent to that interface is then tunneled over X.25 over TCP (XOT) to a router.

{% highlight c %}
cat /proc/net/x25/route
Address          Digits  Device
000000000000000  0       tun0 
{% endhighlight %}

## Results

### Successful connection

Here is a successful connection:

{% highlight plaintext %}
./a.out 999888 701001
Connected
Packet sizes: in: 512, out: 512
Window sizes: in: 4, out: 4

c1921.lan on line 137.

C i s c o  S y s t e m s      
      ||        ||
      ||        ||        
     ||||      ||||      
 ..:||||||:..:||||||:..



User Access Verification

Username: Timed out waiting for data.
{% endhighlight %}

During the call, `/proc/net/x25/socket` showed our outbound connection:

{% highlight plaintext %}
cat /proc/net/x25/socket
dest_addr  src_addr   dev   lci st vs vr va   t  t2 t21 t22 t23 Snd-Q Rcv-Q inode
701001     999888     tun0  001  3  0  3  0   1   3 200 180 180     0     0 22468
*          999999     ???   000  0  0  0  0   0   3 200 180 180     0     0 11408
{% endhighlight %}

The router logs:

{% highlight plaintext %}
Apr  8 06:18:04.532: [192.0.2.100,27764/192.0.2.250,1998]: XOT I P/Inactive Call (23) 8 lci 1
Apr  8 06:18:04.532:   From (6): 999888 To (6): 701001
Apr  8 06:18:04.532:   Facilities: (8)
Apr  8 06:18:04.532:     Packet sizes: 512 512
Apr  8 06:18:04.532:     Window sizes: 4 4
Apr  8 06:18:04.532:    ITU-T facility marker
Apr  8 06:18:04.532:   Call User Data (4): 0x01000000 (pad)
Apr  8 06:18:04.532: [192.0.2.100,27764/192.0.2.250,1998]: XOT O P3 Call Confirm (11) 8 lci 1
Apr  8 06:18:04.532:   From (0):  To (0): 
Apr  8 06:18:04.532:   Facilities: (6)
Apr  8 06:18:04.532:     Packet sizes: 512 512
Apr  8 06:18:04.532:     Window sizes: 4 4
Apr  8 06:18:04.536: [192.0.2.100,27764/192.0.2.250,1998]: XOT I P4 Clear (5) 8 lci 1
Apr  8 06:18:04.536:   Cause 0, Diag 0 (DTE originated/No additional information)
Apr  8 06:18:04.536: [192.0.2.100,27764/192.0.2.250,1998]: XOT O P7 Clear Confirm (3) 8 lci 1
{% endhighlight %}

### Unsuccessful connection

If we attempt to connect to an address that the router does not know:

{% highlight shell %}
./a.out 999888 701000
connect: Connection refused
C: 13 D: 64
{% endhighlight %}

The router logs:

{% highlight plaintext %}
Apr  8 06:18:19.296: [192.0.2.100,27664/192.0.2.250,1998]: XOT I P/Inactive Call (23) 8 lci 1
Apr  8 06:18:19.296:   From (6): 999888 To (6): 701000
Apr  8 06:18:19.296:   Facilities: (8)
Apr  8 06:18:19.296:     Packet sizes: 512 512
Apr  8 06:18:19.296:     Window sizes: 4 4
Apr  8 06:18:19.296:    ITU-T facility marker
Apr  8 06:18:19.296:   Call User Data (4): 0x01000000 (pad)
Apr  8 06:18:19.296: [192.0.2.100,27664/192.0.2.250,1998]: XOT O P3 Clear (5) 8 lci 1
Apr  8 06:18:19.296:   Cause 13, Diag 64 (Not obtainable/Call or Clear problem)
{% endhighlight %}

## Summary

You can see the [completed code in my repo][gh_repo].

I could not see a way to get access to a Closed User Group (CUG), which is normally negotiated via facilities.  As a work around this would have to be performed on the Cisco side using a map.

1.  [x] Create C code that connects to X.25 using the Linux X.25 stack.
2.  [x] Set the packet and window sizes.
3.  [x] Set the PID in the call user data to request a PAD connection.
4.  [x] Forward that connection to a router over XOT (X.25 over TCP).


[man_x25]: https://man7.org/linux/man-pages/man7/x25.7.html
[pad_svr]: https://github.com/BAN-AI-X25/pad_svr/blob/02013e0995885d487b1fab8243425181dbff82c2/open_x25.c
[gh_repo]: https://github.com/SeanBurford/x25/blob/main/x25_client/main.c
