---
layout: page
title: References
permalink: /references/
---

# References

## Overview

*  [Is X.25 Still Alive][alive] puts X.25 into context.
*  🤩🤩🤩 [Configuring X.25 and LAPB][cisco_x25wan] is a long chapter of a long book which covers absolutely everything related to configuring X.25 on Cisco routers.  Other chapters cover serial interface failover, configuring PAD options, XOT and DNS based X.25 routing.
   *  Protip: Navigate to the Book Title Page and press the **Download** button to get the whole book as a PDF.
*  [Farsite][farsite] have a number of pages introducing X.25 and related stuff.
*  [Configuring the Cisco PAD Facility for X.25 Connections][emp] describes PAD commands (and configuration).

### Community

*  [x25.org][x25org] is the Telebahn web site for the community behind x25 at groups.io.
*  [x25 at groups.io][groupsio] mailing list is a community of people interested in X.25.
*  [Compu-Global Hyper Mega Net][cghmn] is an overnet that connects retro computing enthusiasts and their machines using Wireguard.  Probably interesting if you're into X.25.
   *  [Serivces people are running][cghmn_services] doesn't have any X.25 though.  You'll need to be on CGHMN to access `.retro`.

### Migration to X.25 over TCP (XOT)

*  🤩 [HP Migrating X.25 over PSI to X.25 over TCP (XOT)][hpxot] describes the benefits of XOT and how HP-UX 11iv3 patched to Sept 2011 (AR1109) can be configured to speak XOT natively.
   *  Sorry, you'll need an Itanium to run 11iv3.
*  🤩🤩 [Cisco Network Solutions for the Telco DCN: Transmission Equipment in X.25 Environments][cisco_dcn] (2005) sets out the plan to migrate from an X.25 core to an IP core, with an overview of Cisco XOT and specific configurations for different telecomms gear (Fujitsu, ADC, Alcatel, Tellabs, Applied Digital, Wiltron).

## Implementation Reference

### Command references

*  [Cisco IOS Wide-Area Networking Command Reference][cisco_cmd] `x25 accept-reverse` through to `x25 pvc` including setting the virtual circuit number ranges.
*  [Cisco IOS Wide-Area Networking Command Reference][cisco_cm2] `x25 pvc` through to `x29 inviteclear-time` including setting the protocol timers.
*  [X.25 over TCP Profiles][cisco_profiles] describes how to apply access control to XOT connections.

### Cisco Configuration examples

*  [X.25 to TCP Translation][cisco_xot1] has an example for `r1 --xot-- r2 --serial-- r3`.
*  [X25 over TCP/IP][cisco_xot] has an example for `r1 --serial-- r2 --xot-- r3 --serial-- r4`.
*  [X25 over TCP/IP with XOT Keepalives][cisco_xotka] has an example for `r1 --serial-- r2 --xot-- r3 --serial-- r4`.
*  [TCP over X.25][cisco_ip] has an example for running IP over an X.25 link.

### HP Unix

*  🤩 [HP X.25/9000 Programmers Guide][hpx25] describes HP's extensions to sockets, IOCTLs and signals to support X.25.  

## Standards

### RFC

These RFCs (Internet Request for Comments) mostly describe how to run X over Y:

*  **RFC 1356**: [Multiprotocol Interconnect on X.25 and ISDN in the Packet Mode][rfc1356] defines carriage of IP over X.25 (and ISDN).  This includes the *actual* PID value used for IP over X.25.
*  **RFC 1613**: [Cisco Systems X.25 over TCP][rfc1613] defines X.25 over TCP (XOT).

Less well known RFCs include:

*  🤩🤩 **RFC 1090**: [SMTP on X.25][rfc1090] which defines PID `C0F70000` for transport of email across X.25 links (without any IP carriage).  This sounds like fun.
*  🤩 **RFC 874**: [A Critique of X.25][rfc874]

### ISO/IEC

The ISO/IEC standards for X.25 are available for free under license.  ISO/IEC 8208 and 8878 define the base of the protocol stack.  You can click **Buy** to get the PDF versions for CHF0,00 here:

*  [ISO/IEC 8208:2000 Information technology — Data communications — X.25 Packet Layer Protocol for Data Terminal Equipment][iec8208]
*  💤 [ISO/IEC 8878:1992 Information technology — Telecommunications and information exchange between systems — Use of X.25 to provide the OSI Connection-mode Network Service][iec8878]

### ITU-T

ITU-T Recommendations (formerly CCITT) define the various X protocols.  There are more at [ITU-T X Series][xseries]:

* **Data Communications** defines electrical interfaces:
   * **X.21**: [Interface between DTE and DCE Equipment for Synchronous Operation on Public Data Networks][x21] defines the X.21 electrical interface used by X.25.
* **Data Networks** defines network protocols:
  * **X.25**: [Interface between DTE and DCE for terminals operating in the packet mode and connected to public data networks by dedicated circuit][x25] defines the network protocol.
* **Public Data Networks** defines interoperability:
  * **X.28**: [DTE/DCE interface for start/stop mode data terminal equipment accessing the Packet Assembly/Disassembly facility (PAD) in public data network situated in the same country][x28] defines PAD (including PAD parameter settings).
  * **X.29**: [Procedures for the exchange of control information and user data between a Packet Assembly/Disassembly (PAD) facility and a packet mode DTE or another PAD][x29] defines PAD settings, interrupts, error handling and packet formats.
  * **X.30**: [Support of X.21, X.21 bis and X.20 bis based data terminal equipments (DTEs) by and Integrated Services Digital Network (ISDN)][x30] defines carrying X.21 on ISDN.
  * **X.121**: [International numbering plan for public data networks][x121] defines the format of X.25 addresses.
* **Information Technology**:
  * **X.263**: [Protocol Identification in the Network Layer][x263] defines the PID field of CALL packets.
* **Open Systems Interconnection** defines interoperability from an OSI point of view:
  * **X.222**: [Use of X.25 LAPB compatible data link procedures to provide the OSI connection-mode data link service][x222].

## Misc

I think of [V.35][v35] as a gigantic connector, but the ITU-T says it is "Data transmission at 48 kilobits per second using 60-108kHz group band circuits" and includes a schematic in the standard:

![Schematic from ITU-T Recommendation V.35](/assets/images/800/v35-000.jpg)
*Schematic from ITU-T Recommendation V.35*


[alive]: https://blog.ipspace.net/2022/04/x25-still-alive/
[cghmn]: https://cghmn.org/
[cghmn_services]: https://wiki.cursedsilicon.net/wiki/Services_people_are_running
[cisco_cm2]: https://www.cisco.com/c/en/us/td/docs/ios-xml/ios/wan/command/wan-cr-book/wan-x2.html
[cisco_cmd]: https://www.cisco.com/c/en/us/td/docs/ios-xml/ios/wan/command/wan-cr-book/x25_accept_reverse_through_x25_pvc_xot.html
[cisco_dcn]: https://www.cisco.com/c/en/us/td/docs/ios/solutions_docs/telco_dcn/tlctex25.html
[cisco_ip]: https://www.cisco.com/c/en/us/support/docs/wan/x25-protocols/9362-tcp-over-x25.html
[cisco_profiles]: https://www.cisco.com/c/en/us/td/docs/ios-xml/ios/wan_smxl/configuration/12-4/wan-smxl-12-4-book/wan-x25otcp-pro.pdf
[cisco_x25wan]: https://www.cisco.com/c/en/us/td/docs/ios-xml/ios/wan_smxl/configuration/xe-16-10/wan_smxl_xe16_10_book/wan-cfg-x25-lapb.html
[cisco_xot1]: https://www.cisco.com/c/en/us/support/docs/wan/x25-protocols/14216-x25-tcptrans.html
[cisco_xot]: https://www.cisco.com/c/en/us/support/docs/wan/x25-protocols/9363-x25-over-tcpip.html
[cisco_xotka]: https://www.cisco.com/c/en/us/support/docs/ip/x25-over-tcp-xot/21120-xot-keepalives.html
[emp]: https://www.employees.org/univercd/Feb-1998/cc/td/doc/product/software/ios113ed/113ed_cr/dial_c/dcprt2/dcpad.htm
[farsite]: https://farsite.com/product-support/x-25-networking-guide/
[groupsio]: https://groups.io/g/x25
[hpx25]: https://support.hpe.com/hpesc/public/docDisplay?docId=c02013089&docLocale=en_US
[hpxot]: https://support.hpe.com/hpesc/public/docDisplay?docLocale=en_US&docId=c03682619
[iec8208]: https://www.iso.org/obp/ui/en/#iso:std:iso-iec:8208:ed-4:v1:en
[iec8878]: https://www.iso.org/obp/ui/en/#iso:std:iso-iec:8878:ed-2:v1:en
[rfc1090]: https://datatracker.ietf.org/doc/html/rfc1090
[rfc1356]: https://datatracker.ietf.org/doc/html/rfc1356
[rfc1613]: https://datatracker.ietf.org/doc/html/rfc1613
[rfc874]: https://datatracker.ietf.org/doc/html/rfc874
[v35]: https://www.itu.int/rec/T-REC-V.35/en
[x121]: https://www.itu.int/rec/T-REC-X.121/en
[x21]: https://www.itu.int/rec/T-REC-X.21/en
[x222]: https://www.itu.int/rec/T-REC-X.222/en
[x25]: https://www.itu.int/rec/T-REC-X.25/en
[x25org]: https://x25.org/
[x263]: https://www.itu.int/rec/T-REC-X.263/en
[x28]: https://www.itu.int/rec/T-REC-X.28/en
[x29]: https://www.itu.int/rec/T-REC-X.29/en
[x30]: https://www.itu.int/rec/T-REC-X.30/en
[xseries]: https://www.itu.int/rec/T-REC-X/en
