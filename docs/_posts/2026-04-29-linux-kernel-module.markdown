---
layout: post
title:  "4.5x Faster Linux X.25 Handling"
date:   2026-05-03 06:00:00 +1000
categories: xot
---

# 4.5x Faster Linux X.25 Handling

When I started playing with X.25 I spotted a gap.  There didn't seem to be an existing solution for bridging [XOT to XOT](xot/2026/04/11/xot-to-xot-1.html) traffic, at least not in my price range.  Cisco routers won't bridge XOT to XOT, and it seems that software solutions did not either.

Since then I've been developing a suite of tools called [GoXOT][goxot], which can do XOT to XOT bridging and also bridge that traffic to and from the Linux X.25 subsystem for local applications.  This enables Linux X.25 applications to use XOT without modification, which is an existing feature of tools like [jh-xotd][jh-xotd].  It also enables creation of XOT to XOT routers, which is a unique feature.

[Welcome to my Lab](/hardware/cisco/2026/04/05/welcome-to-my-lab.html) has an introduction to the various devices discussed below.

## The Problem

I want to ensure that GoXOT is performant and reliable.  in practice, I was seeing lost packets and failed connections.  To address this I created a [stress test](/software/2026/04/18/x25-stress-test.html) tool.  This has uncovered and helped resolve a stream of issues:

*  Problems with GoXOT LCI management.
*  Null dereferences and synchronisation problems in the Linux X.25 kernel module.
*  Userland operation ordering problems and race conditions.
*  Coordination issues between components of GoXOT.

I've used Google Gemini and Claude Code analyze each of these aspects, report on their findings and help create fixes.

Along the way I've improved GoXOT's observability and added a dashboard to show various metrics collected from varz (including graphing).  Real time graphs turned out to be very handy for spotting and diagnosing performance issues in real time.

## GoXOT LCI management

The first problem that stress-test found was that GoXOT rapidly ran out of Logical Channel IDs (LCIs).  These numbers, between 1 and 4095, identify a call or connection as it flows across a link.  LCIs are local to a link, those used on a serial connection only hold meaning to the devices on each end, and may differ from those used to carry the same call across a Linux TUN interface (or example).

There are two ways to make X.25 calls in Linux.  One is a socket interface, similar to using TCP.  The alternative is to inject call request packets onto a TUN interface, which is desirable for us since it overcomes limitations in the socket interface.  X.25 Facilities are used in call request packets to negotiate things like window sizes, access to private call user groups and other stuff.  The socket interface has limited support for facilities, which we want to pass around intact.

One interesting finding (documented at [COMPAT006][COMPAT006]) is that while applications can assign LCI numbers to calls they inject directly, there is no way for an application to reserve an LCI range.  GoXOT ensures that the kernel is able to track the state and LCI for TUN injected calls, but does not track LCI numbers assigned by the kernel for socket calls, so there is an opportunity for conflict.

Both the default [Linux X.25 module](https://github.com/SeanBurford/linux_x25_module/blob/d7f70ff4f80dc99b481bd1dc395aec08f6225a16/x25-6.12.74%2Bdeb13%2B1-amd64/af_x25.c#L335-L350) and my [modified version](https://github.com/SeanBurford/linux_x25_module/blob/main/x25-6.12.74%2Bdeb13%2B1-amd64/af_x25.c#L336-L361) assign LCIs using the first free value from 1 through to 4095.  I reduced the chance of collisions by using a higher LCI range (1024-4095).  An alternative would be to assign from the top of the range downwards, or for `tun-gateway` to poll `/proc/net/x25/sockets` and use that for LCI discovery and deconfliction, but this has not been necessary so far.

Tracking free LCI numbers and using the first free one made other resource management problems worse later on, since rapid LCI reuse execerbates poor state management, but it turned out to be a good choice since those bugs would have been harder to discover otherwise.  Once the "last" LCI related bug was fixed I changed to a strategy of rotating through available LCIs to reduce stress on components I don't control.

I've decided that the maturity of an X.25 stack is directly reflected in its LCI management approach.

## Synchronisation in the Linux X.25 kernel module

Synchronising multiple processes is a hard task in computer science.

In order to establish a common understanding of the basics I asked Claude Code to build a [catalogue of synchronisation primitives][mod_sync] used in the Linux X.25 module, along with whether each primitive was currently best practice and when it should be used or not used.  This was interesting because the agent also found **notable usage patterns** where locks were misused or had dual roles.  All of the locks were superceded spin locks (which burn CPU time waiting) instead of more recent RCU locks (better in read-mostly situations).

With the catalogue in hand, I asked for [prioritised recommendations][mod_rec] and fixes.

There are a total of 7 synchronisation bugs identified.  I committed each fix as a separate pull request, then continued testing with just the first two fixes in place.  I was still experiencing connection issues under load and didn't feel comfortable with the full set of fixes until GoXOT was more battle tested.  In particular SYNC007 switched to RCU locks and the [Review Checklist for RCU Patches][rcu_checklist] was quite daunting.

You can find my DKMS ready module with the fixes at [https://github.com/SeanBurford/linux_x25_module/](https://github.com/SeanBurford/linux_x25_module/)

## Userland operation ordering

With the most frequently hit kernel bugs fixed, I was still seeing lost connections in the stress-test.

Next I ran an analysis of how different X.25 operations interact in user land:

*  **[Userland operations used for X.25 connection management][userland_operations]**: What functions are called to open an interface, establish a call, destroy a call, etc.  What is seen on the TUN interface, what side effects do these operations have?
*  **[Compatible of those X.25 operations][userland_compat] with each other**:  Which operations conflict when run in parallel and why?  Are these userland issues or kernel issues?
*  **Hunt for [session handling][userland_session] bugs** in GoXOT based on the previous analysis.
*  Somewhat redundantly, what **[socket handling][userland_socket]** bugs are evident?

The first document is also one of the oldest in the repo.  It describes all of the fundamentals and structures used to interact with X.25 from Linux pretty well (though it's coverage of `x25_subscrip_struct` is lacking).  It's the missing manual for Linux X.25.

The second document says that many operations have conflicts within the same process (and it identifies two operations that can conflict across processes):

|      | Open X.25 Packet Socket | Open X.25 Connection | Close X.25 Connection | Receive Close Notification | Receive Disconnect Notification | Clear Conns and Shut Down |
|:-----|:----|:----|:----|:----|:----|:----|
| Open X.25 Packet Socket  | —   | COMPAT003, COMPAT004 | COMPAT004 | COMPAT004 | COMPAT005 | COMPAT004 |
| Open X.25 Connection  |     | COMPAT006 | COMPAT007 | COMPAT007 | COMPAT008 | COMPAT009 |
| Close X.25 Connection  |     |     | Safe | COMPAT007 | Safe | Safe |
| Receive Close Notification  |     |     |     | Safe | Safe | Safe |
| Receive Disconnect Notification  |     |     |     |     | Safe | COMPAT009 |
| Clear Conns and Shut Down  |     |     |     |     |     | — |

None of these are particularly devastating, but you can see that creating connections presents plenty of opportunities for failure.  stress-test creates a lot of connections.

For a bit of variety I had Gemini develop fixes for these issues, then asked Claude to [review the results][gemini_claude]

## Stress Test Results

Now that I was happy that GoXOT was stable, I decided to run stress tests against the fixes for the X.25 kernel module that I had identified earlier in "[prioritised recommendations][mod_rec]".

I quickly observed several things:

*  TCP port exhaustion was the next limit.
*  Fixing SYNC004 provided a huge performance improvement.
*  Once most (all?) of the major bugs were fixed, real time graphs went from bumpy to smooth.

![Calls per 20 seconds by Kernel Patch](/assets/images/Calls_20s_By_Patch.svg)

### TCP TIME_WAIT and port exhaustion

Fixing **SYNC004** caused `stress_test` results to become lumpy and unpredictable.  `xot-gateway` was reporting:

{% highlight shell %}
Connection to 192.0.2.1:1998 failed: dial tcp 192.0.2.1:1998: connect: cannot assign requested address
{% endhighlight %}

I soon realised that it was related to the number of TCP connections I was burning through, since each X.25 call uses a separate TCP connection.  Each socket is identified by four numbers (local and remote IP address, local and remote port).  The local and remote IP address and remote port are constant during testing, leaving only the local port to identify each connection.  My machine was configured to originate connections from around 30k local ports, and could reuse ports once every 60 seconds (closed ports are kept in TIME_WAIT for that period to ensure both sides are in sync).

Increasing the number of local ports used to establish connections and asking for more aggressive reuse of TIME_WAIT connections helped:

{% highlight shell %}
sydo sysctl -w net.ipv4.ip_local_port_range="8192 64000"
sydo sysctl -w net.ipv4.tcp_tw_reuse="1"
{% endhighlight %}

I have not yet looked at safe ways to multiplex X.25 calls on a single connection, there are subtle issues to consider.  That would resolve this properly.

### X.25 kernel SYNC-001 through SYNC-003

SYNC-001, SYNC-002 and SYNC-003 relate to bugs that I have previously observed in dmesg output:

*  **SYNC-001: Write operation performed under read lock on x25->neighbour**.  This bug means that one thread might delete neighbour data that another thread is relying on.  I had already observed x25->neighbour NULL pointer dereference stack traces in dmesg under load, but had not identified where neighbour was being set to NULL.
*  **SYNC-002: TOCTOU in x25_forward_call: duplicate-check and insert are not atomic**.  This bug means that two threads might be allocated the same LCI for different connections.  I had already observed messages in dmesg about call request packets being received on active connections, which were caused by this bug.
*  **SYNC-003: x25_kill_by_neigh drops the list write lock mid iteration**.  This bug means that, while x25_kill_by_neigh is working through a list, that list might change.  I didn't notice this happening in practice.

Fixing these bugs resulted in fewer X25 failures in `dmesg` but no real change in performance.

### X.25 kernel SYNC-004 TOCTOU in `x25_new_lci`

With the above fixes in place, I was still getting one kernel message that indicated that the kernel was confused about the state of connections: `X25: unknown 0f in state 3`.

This was caused by a "time of check, time of use" bug.  This is a race condition, where there is a gap between the time a value is checked and the time when it is used, and the value can change within that gap.

The old code would start with `lci` 1, then call `x25_find_socket(lci, ...)` over and over (incrementing lci as it went) looking for a free LCI number.  `x25_find_socket()` does three important things:

1.  Holds a read lock on the socket list.
2.  Looks for an entry in the list that matches the requested LCI/neighbour.
3.  Increments the usage count on the matching socket and returns it.

{% highlight c %}
static unsigned int x25_new_lci(struct x25_neigh *nb)
{
	unsigned int lci = 1;
	struct sock *sk;

	while ((sk = x25_find_socket(lci, nb)) != NULL) {
		sock_put(sk);
		if (++lci == 4096) {
			lci = 0;
			break;
		}
		cond_resched();
	}

	return lci;
}
{% endhighlight %}

The caller of `x25_new_lci()` could then assume that (when x25_new_lci was running) the returned LCI was not in use, so the caller can probably use it.

The problems are that:

1.  There is no lock protecting the caller, and LCIs are allocated from 1 upwards, two concurrent callers are likely to be allocated the same LCI.
2.  During heavy load the list is frequently modified, and x25_find_socket churns through read locks on that list.

Here is the modified code:

{% highlight c %}
static unsigned int x25_new_lci(struct sock *owner, struct x25_neigh *nb)
{
	unsigned int lci = 1;
	struct sock *s;

	write_lock_bh(&x25_list_lock);
	while (lci < 4096) {
		bool in_use = false;

		sk_for_each(s, &x25_list) {
			if (s != owner &&
			    x25_sk(s)->lci == lci &&
			    x25_sk(s)->neighbour == nb) {
				in_use = true;
				break;
			}
		}
		if (!in_use) {
			x25_sk(owner)->lci = lci;
			break;
		}
		lci++;
	}
	write_unlock_bh(&x25_list_lock);
	return (lci < 4096) ? lci : 0;
}
{% endhighlight %}

This code only uses the lock once (as a write lock), rather than once per open connection (as a read lock).  It properly protects the LCI, setting it as in-use within the write lock protected region.

I have not confirmed why `stress-test` is 4.5 times faster with this fix in place, it is probably a combination of:

*  `stress-test` punishes lost connections (e.g. where the kernel gets confused and reuses an LCI).
*  One write lock is probably faster than many read locks in this circumstance.  Since the write lock will always be used eventually (to allocate the LCI), the read locks are just wasting cycles.  These read locks are spin locks, which burn CPU waiting.

Graphs of connections/second in the dashboard went from lumpy:

![SYNC-003](/assets/images/800/bumpy_graph.png)

to smooth with SYNC-004 in place:

![SYNC-004](/assets/images/800/smooth_graph.png)

### X.25 kernel SYNC-005, SYNC-006, SYNC-007

These patches maintained or improved the performance of the previous set.  Notably, SYNC-007 improves data throughput:

![Bytes per second by Kernel Patch](/assets/images/Bytes_Sec_By_Patch.svg)

SYNC-007 replaces spin locks (which burn CPU waiting for the lock) with Read/Copy/Update locks (essentially free for read locking).  Since one or more of these read locks are acquired on every packet processed, and these locks are mostly used as read locks, this is a worthwhile performance improvement.

## Dashboard

Google Gemini has been keen to develop a dashboard for GoXOT since the very first prompt.  During this work I decided to let it show me what it could do.

The first attempt was a disaster.  I asked Gemini to import GoXOT into the workspace and start developing a dashboard.  It reorganised all of the files and directories in the repo around the dashboard concept, deleting unnecessary documentation and source code.  I eventually had to give up on this attempt once it became irrecoverable.  I did learn how to give Gemini standing instructions in AI Studio, which is a handy tool, and Gemini did have some good ideas about reorganising the varz metrics which I adopted.

On the second attempt I decided to let Gemini do what it wanted (create a new dashboard project), so I asked it to do that with reference to GoXOT.  I dropped the idea of creating a diagram showing `xot-server` receiving calls, `tun-gateway` interfacing with local applications and `xot-gateway` forwarding outbound connections since Gemini struggled with positioning the elements (CSS, right?).  Instead I specified that I wanted tables showing what was happening on each interface of each server instead.  For graphing, I asked that each metric could be clicked on to graph it.

Between the first attempt and the second I switched from asking for "1990s futuristic" to "vt100", which I think was a good choice.

The dashboard turned out well, you can:

*  See the availability of each GoXOT component and it's uptime.
*  See per server and interface metrics and how they have changed since the last update.
*  See a summary of the types of packets handled.
*  Click a metric to start graphing it.

![New Dashboard](/assets/images/800/dashboard.png)

If you don't like it, just scrape the varz metrics with your boring metrics collection tool instead.

## Conclusion

It's been a long journey but it looks like all of the major bugs have been addressed.

I no longer get dmesg output indicating that the kernel is confused about the state of X.25 calls, nor any back traces for crashes.  `stress-test` performance has increased 4.5x.

I can now lab test serial and ISDN links with confidence that most new findings will be outside of GoXOT and the Linux kernel.

[goxot]: https://github.com/SeanBurford/goxot/
[jh-xotd]: https://github.com/BAN-AI-X25/jh-xotd/
[COMPAT006]: https://github.com/SeanBurford/goxot/blob/main/docs/tech/x25_operation_compatibility.md#compat006--lci-collision-between-gateway-sessionmanager-and-kernel-x25_new_lci
[mod_sync]: https://github.com/SeanBurford/linux_x25_module/blob/main/analysis/synchronisation.md
[mod_rec]: https://github.com/SeanBurford/linux_x25_module/blob/main/analysis/synchronisation-recommendations.md
[userland_operations]: https://github.com/SeanBurford/goxot/blob/main/docs/tech/linux_x25_and_tun.md#connection-operations
[userland_session]: https://github.com/SeanBurford/goxot/blob/main/docs/analysis/x25_session_handling.md
[userland_socket]: https://github.com/SeanBurford/goxot/blob/main/docs/analysis/x25_socket_handling.md
[userland_compat]: https://github.com/SeanBurford/goxot/blob/main/docs/tech/x25_operation_compatibility.md
[rcu_checklist]: https://docs.kernel.org/RCU/checklist.html
[gemini_claude]: https://github.com/SeanBurford/goxot/commit/235cdab385621e44ab5be0347f7ced34113b7820
