/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/dncp-module.h"
#include "ns3/csma-module.h"
#include <iostream>

NS_LOG_COMPONENT_DEFINE ("DncpExample");

// Default Network Topology
//
//
// n0 -------------- n1   n2   n3   n4
//    point-to-point  |    |    |    |
//                    ================
//                      LAN

using namespace ns3;
int
main (int argc, char *argv[]){
	Time::SetResolution (Time::MS);
	LogComponentEnable ("DncpApplication", LOG_LEVEL_INFO);
	Packet::EnableChecking();
	Packet::EnablePrinting();

	uint32_t nCsma=3;
	NodeContainer p2pNodes;
	p2pNodes.Create (2);

	NodeContainer csmaNodes;
	csmaNodes.Add (p2pNodes.Get (1));
	csmaNodes.Create (nCsma);

	NodeContainer p2pNodes1;
	p2pNodes1.Add (p2pNodes.Get (0));
	p2pNodes1.Add (csmaNodes.Get (nCsma));

	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
	csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

	InternetStackHelper stack;
	stack.Install (p2pNodes.Get (0));
	stack.Install (csmaNodes);

    NetDeviceContainer p2pDevices;
	p2pDevices = pointToPoint.Install (p2pNodes);

	NetDeviceContainer csmaDevices;
	csmaDevices = csma.Install (csmaNodes);

    NetDeviceContainer p2pDevices1;
	p2pDevices1 = pointToPoint.Install (p2pNodes1);

	Ipv6AddressHelper ipv6addr;
	ipv6addr.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));
	Ipv6InterfaceContainer interfaces =ipv6addr.Assign(p2pDevices);

	ipv6addr.SetBase (Ipv6Address ("2001:2::"), Ipv6Prefix (64));
	Ipv6InterfaceContainer csmaInterfaces =ipv6addr.Assign(csmaDevices);

	ipv6addr.SetBase (Ipv6Address ("2001:3::"), Ipv6Prefix (64));
	Ipv6InterfaceContainer Interfaces2 =ipv6addr.Assign(p2pDevices1);

	DncpApplicationHelper dncp;
	ApplicationContainer dncpApps = dncp.Install (p2pNodes.Get(0));
	dncpApps.Start (Seconds (2.0));
	dncpApps.Stop (Seconds (30.0));

	dncpApps=dncp.Install(csmaNodes);
	dncpApps.Start (Seconds (2.0));
	dncpApps.Stop (Seconds (200.0));

	Simulator::Stop (Seconds (200));
	Simulator::Run ();
	Simulator::Destroy ();
	return 0;

}
