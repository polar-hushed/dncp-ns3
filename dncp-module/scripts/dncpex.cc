/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/dncp-module.h"
#include "ns3/csma-module.h"
#include <iostream>

NS_LOG_COMPONENT_DEFINE ("DncpExample");

// Default Network Topology

//			point-to-point
// ===================================
// |                                 |
// n0 =============== n1   n2   n3   n4
//    point-to-point  |    |    |    |
//                    ================
//                      LAN

using namespace ns3;


const std::string tlv[]= { "",
					"DNCP_T_REQ_NET_STATE",
					"DNCP_T_REQ_NODE_STATE",
					"DNCP_T_ENDPOINT_ID",
					"DNCP_T_NET_STATE",
					"DNCP_T_NODE_STATE",
					"DNCP_T_CUSTOM",
					"DNCP_T_FRAGMENT_COUNT",
					"DNCP_T_NEIGHBOR",
					"DNCP_T_KEEPALIVE_INTERVAL",
					"DNCP_T_TRUST_VERDICT"};

static void
NethashTrack(Ptr<OutputStreamWrapper> file,std::string context,const uint64_t oldvalue,const uint64_t newvalue){
	*file->GetStream() <<Simulator::Now ().GetSeconds ()<<" "<<context.substr(10,(context.find("/ApplicationList")-10))<<" "<<newvalue<<std::endl;
}

static void
PktTrace(Ptr<OutputStreamWrapper> file,std::string context, Ipv6Address saddr, Ipv6Address dstaddr,
		uint32_t deviceIndex,uint32_t pktsize,uint64_t uid,struct tlv_attr* msg,bool isReceive){
if (isReceive)
	*file->GetStream() <<Simulator::Now ().GetSeconds ()<< " R "<< (dstaddr.IsMulticast()? "M ":"U ")
			<<context.substr(10,(context.find("/ApplicationList")-10))<<" "<<saddr<<" "<<dstaddr<<" "<<deviceIndex<<" "<<uid<<" "<<pktsize<<" " ;
else
	*file->GetStream() <<Simulator::Now ().GetSeconds ()<< " S "<<(dstaddr.IsMulticast()? "M ":"U ")
			<<context.substr(10,(context.find("/ApplicationList")-10))<<" "<<saddr<<" "<<dstaddr<<" "<<deviceIndex<<" "<<uid<<" "<<pktsize<<" " ;

	tlv_init(msg, 0, pktsize + sizeof(struct tlv_attr));

	int counter[11]={0};
	struct tlv_attr *a;

	tlv_for_each_attr(a, msg){
		counter[tlv_id(a)]++;
	}

	if (counter[DNCP_T_ENDPOINT_ID]==1)
			if (counter[DNCP_T_REQ_NET_STATE]==1)
				*file->GetStream() <<"REQ_NET"<<std::endl;
			else if (counter[DNCP_T_REQ_NODE_STATE]==1)
				*file->GetStream() <<"REQ_NODE"<<std::endl;
				 else if (counter[DNCP_T_NET_STATE]==1)
					 *file->GetStream()<<"NET_STATE"<<std::endl;
					  else if ((counter[DNCP_T_NET_STATE]==0) && (counter[DNCP_T_NODE_STATE]==1))
						  *file->GetStream()<<"NODE_STATE"<<std::endl;
}

static void
PhyDropTrack(std::string context,const Ptr<const Packet> packet){
	NS_LOG_UNCOND(Simulator::Now ().GetSeconds ()<<" Packet dropped "<<context<<" "<<packet<<*packet<<"UID="<<packet->GetUid());
}

static void
MacDropTrack(std::string context,const Ptr<const Packet> packet){
	NS_LOG_UNCOND(Simulator::Now ().GetSeconds ()<<" Packet dropped "<<context<<" "<<packet<<*packet<<"UID="<<packet->GetUid());
}

enum Topology{LINK,MESH,STRING,TREE,DOUBLETREE};

int
main (int argc, char *argv[]){

	Time::SetResolution (Time::MS);

	log_level=0;
	bool verbose=false;
	double startTime=1;
	double endTime=50;
	uint32_t nNode=3;
	uint16_t topology=LINK;

	CommandLine cmd;
	cmd.AddValue ("log_level","log level in dncp code",log_level);
	cmd.AddValue ("verbose","set it to true to enable DncpApplication logging component",verbose);
	cmd.AddValue ("start_time","the beginning time of dncp application",startTime);
	cmd.AddValue ("end_time","the end time of dncp application",endTime);
	cmd.AddValue ("topology","the topology that the simulation is going to generate",topology);
	cmd.AddValue ("nNode", "number of nodes in the network",nNode);
	cmd.Parse (argc, argv);

	if(verbose)
		LogComponentEnable ("DncpApplication", LOG_LEVEL_INFO);

	Packet::EnablePrinting();

	switch(topology){

		case LINK:{

			NS_LOG_INFO("Creating LINK topology");

			NodeContainer nodes;
			nodes.Create(nNode);


			CsmaHelper csma;
			csma.SetChannelAttribute ("DataRate", StringValue ("12Mbps"));
			csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

			InternetStackHelper stack;
			stack.Install(nodes);

			NetDeviceContainer csmaDevices;
			csmaDevices = csma.Install (nodes);

			Ipv6AddressHelper ipv6addr;
			//ipv6addr.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));
			Ipv6InterfaceContainer interfaces =ipv6addr.AssignWithoutAddress(csmaDevices);

			DncpApplicationHelper dncp;
			ApplicationContainer dncpApps= dncp.Install(nodes);
		}
			break;

		case MESH:{
			NS_LOG_INFO("Creating MESH topology");

			NodeContainer nodes;
			nodes.Create(nNode);

			CsmaHelper csma;
			csma.SetChannelAttribute ("DataRate", StringValue ("12Mbps"));
			csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

			InternetStackHelper stack;
			stack.Install(nodes);

			Ipv6AddressHelper ipv6addr;

			NodeList::Iterator listEnd=NodeList::End();
			for(NodeList::Iterator i= NodeList::Begin(); i!=listEnd; i++){
				Ptr<Node> node= *i;
				for(NodeList::Iterator j=i+1;j!=listEnd; j++){

					NodeContainer nodePair;
					nodePair.Add(node);
					nodePair.Add(*j);

					NetDeviceContainer csmaDevices;
					csmaDevices = csma.Install (nodePair);
					ipv6addr.AssignWithoutAddress(csmaDevices);
				}
			}

			DncpApplicationHelper dncp;
			ApplicationContainer dncpApps= dncp.Install(nodes);

		}
			break;

		case STRING:{
			NS_LOG_INFO("Creating STRING topology");

			NodeContainer nodes;
			nodes.Create(nNode);

			CsmaHelper csma;
			csma.SetChannelAttribute ("DataRate", StringValue ("12Mbps"));
			csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

			InternetStackHelper stack;
			stack.Install(nodes);

			Ipv6AddressHelper ipv6addr;

			NodeList::Iterator listEnd=NodeList::End()-1;
			for(NodeList::Iterator i= NodeList::Begin(); i!=listEnd; i++){
				NodeContainer nodePair;
				nodePair.Add(*i);
				nodePair.Add(*(i+1));
				NetDeviceContainer csmaDevices;
				csmaDevices = csma.Install (nodePair);
				ipv6addr.AssignWithoutAddress(csmaDevices);
			}

			DncpApplicationHelper dncp;
			ApplicationContainer dncpApps= dncp.Install(nodes);
		}
			break;

		case TREE:{
			NS_LOG_INFO("Creating TREE topology");

			NodeContainer nodes;
			nodes.Create(nNode);

			CsmaHelper csma;
			csma.SetChannelAttribute ("DataRate", StringValue ("12Mbps"));
			csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

			InternetStackHelper stack;
			stack.Install(nodes);

			Ipv6AddressHelper ipv6addr;

			uint32_t N=nodes.GetN();

			if (N%2){
				for(int i=0;i<=(N-3)/2;i++){
					NodeContainer nodePair;
					nodePair.Add(nodes.Get(i));
					nodePair.Add(nodes.Get(2*i+1));
					NetDeviceContainer csmaDevices;
					csmaDevices = csma.Install (nodePair);
					ipv6addr.AssignWithoutAddress(csmaDevices);

					NodeContainer nodePair1;
					nodePair1.Add(nodes.Get(i));
					nodePair1.Add(nodes.Get(2*i+2));
					csmaDevices = csma.Install (nodePair1);
					ipv6addr.AssignWithoutAddress(csmaDevices);
				}
			}
			else{

				for(int i=0;i<(N-2)/2;i++){
					NodeContainer nodePair;
					nodePair.Add(nodes.Get(i));
					nodePair.Add(nodes.Get(2*i+1));
					NetDeviceContainer csmaDevices;
					csmaDevices = csma.Install (nodePair);
					ipv6addr.AssignWithoutAddress(csmaDevices);

					NodeContainer nodePair1;
					nodePair1.Add(nodes.Get(i));
					nodePair1.Add(nodes.Get(2*i+2));
					csmaDevices = csma.Install (nodePair1);
					ipv6addr.AssignWithoutAddress(csmaDevices);
				}

				int i=(N-2)/2;
				NodeContainer nodePair;
				nodePair.Add(nodes.Get(i));
				nodePair.Add(nodes.Get(2*i+1));
				NetDeviceContainer csmaDevices;
				csmaDevices = csma.Install (nodePair);
				ipv6addr.AssignWithoutAddress(csmaDevices);

			}
			DncpApplicationHelper dncp;
			ApplicationContainer dncpApps= dncp.Install(nodes);

		}
			break;

		case DOUBLETREE:{
			NS_LOG_INFO("Creating DOUBLETREE topology");

			NodeContainer nodes;
			nodes.Create(nNode);

			CsmaHelper csma;
			csma.SetChannelAttribute ("DataRate", StringValue ("12Mbps"));
			csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

			InternetStackHelper stack;
			stack.Install(nodes);

			Ipv6AddressHelper ipv6addr;

			NodeContainer nodePair;
			nodePair.Add(nodes.Get(0));
			nodePair.Add(nodes.Get(1));
			NetDeviceContainer csmaDevices;
			csmaDevices = csma.Install (nodePair);
			ipv6addr.AssignWithoutAddress(csmaDevices);

			int N=nodes.GetN();

			for(int i=0;i<=2*((N-2)/4)-1;i++){
				if(i%2){
					for (int j=2*i;j<=2*i+3;j++){
						NodeContainer nodePair;
						nodePair.Add(nodes.Get(i));
						nodePair.Add(nodes.Get(j));
						NetDeviceContainer csmaDevices;
						csmaDevices = csma.Install (nodePair);
						ipv6addr.AssignWithoutAddress(csmaDevices);
					}
				}
				else{
					for (int j=2*i+2;j<=2*i+5;j++){
						NodeContainer nodePair;
						nodePair.Add(nodes.Get(i));
						nodePair.Add(nodes.Get(j));
						NetDeviceContainer csmaDevices;
						csmaDevices = csma.Install (nodePair);
						ipv6addr.AssignWithoutAddress(csmaDevices);
					}
				}
			}

			for(int i=1;i<=(N-2)%4;i++){

				int index=2*((N-2)/4);
				NetDeviceContainer csmaDevices;

				NodeContainer nodePair;
				nodePair.Add(nodes.Get(index));
				nodePair.Add(nodes.Get(2*index+1+i));
				csmaDevices = csma.Install (nodePair);
				ipv6addr.AssignWithoutAddress(csmaDevices);

				index=index+1;
				NodeContainer nodePair1;
				nodePair1.Add(nodes.Get(index));
				nodePair1.Add(nodes.Get(2*index-1+i));
				csmaDevices = csma.Install (nodePair1);
				ipv6addr.AssignWithoutAddress(csmaDevices);
			}

			DncpApplicationHelper dncp;
			ApplicationContainer dncpApps= dncp.Install(nodes);
		}
		break;

		default:
			NS_LOG_ERROR("Unknown topology");

	}


	AsciiTraceHelper asciiTraceHelper;
	Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("log.networkhash");
	Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream ("log.packets");

	Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::DncpApplication/NetworkHash", MakeBoundCallback (&NethashTrack, stream));
	Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::DncpApplication/PktRx", MakeBoundCallback (&PktTrace, stream1));
	Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::DncpApplication/PktTx", MakeBoundCallback (&PktTrace, stream1));
	//Config::Connect ("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/PhyTxDrop", MakeCallback (&PhyDropTrack));
	//Config::Connect ("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/MacTxDrop", MakeCallback (&MacDropTrack));


	Config::Set("/NodeList/*/ApplicationList/*/$ns3::DncpApplication/StartTime",TimeValue (Seconds (startTime)));
	Config::Set("/NodeList/*/ApplicationList/*/$ns3::DncpApplication/StopTime",TimeValue (Seconds (endTime)));

	Simulator::Stop (Seconds (endTime+1));
	Simulator::Run ();
	Simulator::Destroy ();
	return 0;

}
