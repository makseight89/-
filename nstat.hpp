#ifndef NSTAT_HPP
#define NSTAT_HPP

#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <windows.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib") // Link against the Winsock library

struct NetworkStatistics {
    std::string interfaceName;
    unsigned long IpInReceives;                     // Number of IP packets received by the interface.
    unsigned long IpInDelivers;                     // Number of IP packets delivered to the upper layers (e.g., TCP, UDP).
    unsigned long IpOutRequests;                    // Number of IP packets requested to be sent by the upper layers.
    unsigned long IcmpInMsgs;                       // Number of ICMP messages received.
    unsigned long IcmpInDestUnreachs;               // Number of ICMP Destination Unreachable messages received.
    unsigned long IcmpOutMsgs;                      // Number of ICMP messages sent.
    unsigned long IcmpOutDestUnreachs;              // Number of ICMP Destination Unreachable messages sent.
    unsigned long IcmpMsgInType3;                   // Number of ICMP Type 3 messages received.
    unsigned long IcmpMsgOutType3;                  // Number of ICMP Type 3 messages sent.
    unsigned long TcpActiveOpens;                   // Number of times TCP connections have made a direct transition to the SYN-SENT state from the CLOSED state.
    unsigned long TcpInSegs;                        // Number of TCP segments received.
    unsigned long TcpOutSegs;                       // Number of TCP segments sent.
    unsigned long UdpInDatagrams;                   // Number of UDP datagrams received.
    unsigned long UdpNoPorts;                       // Number of received UDP datagrams for which there was no application at the destination port.
    unsigned long UdpOutDatagrams;                  // Number of UDP datagrams sent.
    unsigned long UdpIgnoredMulti;                  // Number of UDP multicast datagrams ignored.
    unsigned long Ip6OutRequests;                   // Number of IPv6 packets requested to be sent by the upper layers.
    unsigned long Ip6OutNoRoutes;                   // Number of IPv6 packets discarded because no route could be found to transmit them to their destination.
    unsigned long TcpExtTCPOrigDataSent;            // Number of TCP segments sent containing original data.
    unsigned long TcpExtTCPDelivered;               // Number of TCP segments delivered to the receiving application.
    unsigned long IpExtInOctets;                    // Number of octets received by the interface.
    unsigned long IpExtOutOctets;                   // Number of octets sent by the interface.
    unsigned long speed;                            // Interface speed in bits per second.
    unsigned long mtu;                              // Maximum transmission unit size.         
};

std::vector<NetworkStatistics> getNetworkStatistics() {
    std::vector<NetworkStatistics> stats;

    // Allocate memory for the interface table
    ULONG bufferSize = 0;
    GetInterfaceInfo(nullptr, &bufferSize);
    IP_INTERFACE_INFO* pIfTable = (IP_INTERFACE_INFO*)malloc(bufferSize);
    

    if (GetInterfaceInfo(pIfTable, &bufferSize) == NO_ERROR) {
        for (int i = 0; i < pIfTable->NumAdapters; ++i) {
            IP_ADAPTER_INDEX_MAP& adapter = pIfTable->Adapter[i];
            MIB_IFROW ifRow;
            ifRow.dwIndex = adapter.Index;

            if (GetIfEntry(&ifRow) == NO_ERROR) {
                NetworkStatistics stat;
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, adapter.Name, -1, NULL, 0, NULL, NULL);
                std::string interfaceName(size_needed, 0);
                WideCharToMultiByte(CP_UTF8, 0, adapter.Name, -1, &interfaceName[0], size_needed, NULL, NULL);
                stat.interfaceName = interfaceName;
                stat.speed = ifRow.dwSpeed;
                stat.mtu = ifRow.dwMtu;

                // Get IP statistics
                MIB_IPSTATS ipStats;
                if (GetIpStatistics(&ipStats) == NO_ERROR) {
                    stat.IpInReceives = ipStats.dwInReceives;
                    stat.IpInDelivers = ipStats.dwInDelivers;
                    stat.IpOutRequests = ipStats.dwOutRequests;
                }

                // Get ICMP statistics
                MIB_ICMP icmpStats;
                if (GetIcmpStatistics(&icmpStats) == NO_ERROR) {
                    stat.IcmpInMsgs = icmpStats.stats.icmpInStats.dwMsgs;
                    stat.IcmpInDestUnreachs = icmpStats.stats.icmpInStats.dwDestUnreachs;
                    stat.IcmpOutMsgs = icmpStats.stats.icmpOutStats.dwMsgs;
                    stat.IcmpOutDestUnreachs = icmpStats.stats.icmpOutStats.dwDestUnreachs;
                }

                // Get TCP statistics
                MIB_TCPSTATS tcpStats;
                if (GetTcpStatistics(&tcpStats) == NO_ERROR) {
                    stat.TcpActiveOpens = tcpStats.dwActiveOpens;
                    stat.TcpInSegs = tcpStats.dwInSegs;
                    stat.TcpOutSegs = tcpStats.dwOutSegs;
                }

                // Get UDP statistics
                MIB_UDPSTATS udpStats;
                if (GetUdpStatistics(&udpStats) == NO_ERROR) {
                    stat.UdpInDatagrams = udpStats.dwInDatagrams;
                    stat.UdpNoPorts = udpStats.dwNoPorts;
                    stat.UdpOutDatagrams = udpStats.dwOutDatagrams;
                }

                // Get IPv6 statistics
                MIB_IPSTATS ip6Stats;
                if (GetIpStatisticsEx(&ip6Stats, AF_INET6) == NO_ERROR) {
                    stat.Ip6OutRequests = ip6Stats.dwOutRequests;
                    stat.Ip6OutNoRoutes = ip6Stats.dwOutNoRoutes;
                }

                // Get extended IP statistics
                MIB_IFROW ifRowExt;
                ifRowExt.dwIndex = adapter.Index;
                if (GetIfEntry(&ifRowExt) == NO_ERROR) {
                    stat.IpExtInOctets = ifRowExt.dwInOctets;
                    stat.IpExtOutOctets = ifRowExt.dwOutOctets;
                }

                stats.push_back(stat);
            }
        }
    } else {
        std::cerr << "Error retrieving network statistics." << std::endl;
    }

    free(pIfTable);
    return stats;
}

#endif // NSTAT_HPP
          