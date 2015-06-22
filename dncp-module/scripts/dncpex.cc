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
using namespace std;


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

/*Callback to trace the packets sent and received in dcnp*/
static void
PktTrace(Ptr<OutputStreamWrapper> file,std::string context, Ipv6Address saddr, Ipv6Address dstaddr,
		uint32_t deviceIndex,uint32_t pktsize,uint64_t uid,struct tlv_attr* msg,bool isReceive){
if (isReceive)
	*file->GetStream() <<Simulator::Now ().GetSeconds ()<< " R "<<context.substr(10,(context.find("/ApplicationList")-10))<<
	" "<<uid<< (dstaddr.IsMulticast()? " M ":" U ")<<saddr<<" "<<dstaddr<<" "<<deviceIndex<<" "<<pktsize<<" " ;
else
	*file->GetStream() <<Simulator::Now ().GetSeconds ()<< " S "<<context.substr(10,(context.find("/ApplicationList")-10))<<
	" "<<uid<<(dstaddr.IsMulticast()? " M ":" U ")<<saddr<<" "<<dstaddr<<" "<<deviceIndex<<" "<<pktsize<<" " ;

	tlv_init(msg, 0, pktsize + sizeof(struct tlv_attr));

	int counter[11]={0};
	struct tlv_attr *a;

	tlv_for_each_attr(a, msg){
		counter[tlv_id(a)]++;
	}

/*determine the type of the packet*/
	if (counter[DNCP_T_ENDPOINT_ID]==1)
			if (counter[DNCP_T_REQ_NET_STATE]==1)
				*file->GetStream() <<"REQ_NET"<<std::endl;
			else if (counter[DNCP_T_REQ_NODE_STATE]==1)
				*file->GetStream() <<"REQ_NODE"<<std::endl;
				 else if (counter[DNCP_T_NET_STATE]==1)
					 *file->GetStream()<<"NET_STATE"<<std::endl;
					  else if ((counter[DNCP_T_NET_STATE]==0) && (counter[DNCP_T_NODE_STATE]==1))
						  *file->GetStream()<<"NODE_STATE"<<std::endl;
					  	  else NS_LOG_ERROR("unknown TLV");
	else NS_LOG_ERROR("unknown TLV");

}

/*Callbacks to trace the packets dropped by lower layers */
static void
NethashTrack(Ptr<OutputStreamWrapper> file,std::string context,const uint64_t oldvalue,const uint64_t newvalue){
	*file->GetStream() <<Simulator::Now ().GetSeconds ()<<" "<<context.substr(10,(context.find("/ApplicationList")-10))<<" "<<newvalue<<std::endl;
}

static void
PktTrace1(Ptr<OutputStreamWrapper> file,std::string context,const Ptr<const Packet> packet){
	*file->GetStream()<<Simulator::Now ().GetSeconds ()<<" "<<context.substr(context.find_last_of("/")+1)<<
		" "<<context.substr(10,context.find("/",11)-10)<<" "<<packet->GetUid()<<std::endl;
}

/*function that links two given nodes with csma channel*/
NetDeviceContainer
JoinTwoNodes(Ptr<Node> node1,Ptr<Node> node2,CsmaHelper csma){
	NodeContainer nodePair;
	nodePair.Add(node1);
	nodePair.Add(node2);

	NetDeviceContainer csmaDevices;
	csmaDevices = csma.Install (nodePair);
	return csmaDevices;

}


enum Topology{LINK,MESH,STRING,TREE,DOUBLETREE};

int
main (int argc, char *argv[]){

	Time::SetResolution (Time::MS);

	log_level=0;
	bool verbose=false;
	double startTime=1;
	double stopTime=50;
	uint32_t nNode=3;
	uint16_t topology=LINK;

	string runID;
	string trialID;

    stringstream sstr;
    sstr << time (NULL);
    trialID = sstr.str ();

	CommandLine cmd;
	cmd.AddValue ("log_level","log level in dncp code",log_level);
	cmd.AddValue ("verbose","set it to true to enable DncpApplication logging component",verbose);
	cmd.AddValue ("start_time","the time that dncp applications begin to run",startTime);
	cmd.AddValue ("stop_time","the stop time of dncp application",stopTime);
	cmd.AddValue ("topology","the topology that the simulation is going to generate",topology);
	cmd.AddValue ("nNode", "number of nodes in the network",nNode);
	cmd.AddValue ("trialID","the ID of this trial",trialID);
	cmd.Parse (argc, argv);

	if(verbose){
		LogComponentEnable ("DncpApplication", LOG_LEVEL_INFO);
		LogComponentEnable ("DncpExample", LOG_LEVEL_INFO);
	}

	Packet::EnablePrinting();

	switch(topology){

		case LINK:{

		    stringstream sstr;
		    sstr << "link-" << nNode<<"-"<<trialID;
		    runID = sstr.str ();

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

		    stringstream sstr;
		    sstr << "mesh-" << nNode<<"-"<<trialID;
		    runID = sstr.str ();

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

					NetDeviceContainer csmaDevices=JoinTwoNodes(node,*j,csma);
					ipv6addr.AssignWithoutAddress(csmaDevices);
				}
			}

			DncpApplicationHelper dncp;
			ApplicationContainer dncpApps= dncp.Install(nodes);

		}
			break;

		case STRING:{

		    stringstream sstr;
		    sstr << "string-" << nNode<<"-"<<trialID;
		    runID = sstr.str ();

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
				NetDeviceContainer csmaDevices=JoinTwoNodes(*i,*(i+1),csma);
				ipv6addr.AssignWithoutAddress(csmaDevices);
			}

			DncpApplicationHelper dncp;
			ApplicationContainer dncpApps= dncp.Install(nodes);
		}
			break;

		case TREE:{

		    stringstream sstr;
		    sstr << "tree-" << nNode<<"-"<<trialID;
		    runID = sstr.str ();

			NS_LOG_INFO("Creating TREE topology");

			NodeContainer nodes;
			nodes.Create(nNode);

			CsmaHelper csma;
			csma.SetChannelAttribute ("DataRate", StringValue ("12Mbps"));
			csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

			InternetStackHelper stack;
			stack.Install(nodes);

			Ipv6AddressHelper ipv6addr;

			int N=nodes.GetN();

			if (N%2){
				for(int i=0;i<=(N-3)/2;i++){

					NetDeviceContainer csmaDevices=JoinTwoNodes(nodes.Get(i),nodes.Get(2*i+1),csma);
					ipv6addr.AssignWithoutAddress(csmaDevices);
					csmaDevices=JoinTwoNodes(nodes.Get(i),nodes.Get(2*i+2),csma);
					ipv6addr.AssignWithoutAddress(csmaDevices);
				}
			}
			else{

				for(int i=0;i<(N-2)/2;i++){

					NetDeviceContainer csmaDevices=JoinTwoNodes(nodes.Get(i),nodes.Get(2*i+1),csma);
					ipv6addr.AssignWithoutAddress(csmaDevices);
					csmaDevices=JoinTwoNodes(nodes.Get(i),nodes.Get(2*i+2),csma);
					ipv6addr.AssignWithoutAddress(csmaDevices);
				}

				int i=(N-2)/2;
				NetDeviceContainer csmaDevices=JoinTwoNodes(nodes.Get(i),nodes.Get(2*i+1),csma);
				ipv6addr.AssignWithoutAddress(csmaDevices);

			}
			DncpApplicationHelper dncp;
			ApplicationContainer dncpApps= dncp.Install(nodes);

		}
			break;

		case DOUBLETREE:{

		    stringstream sstr;
		    sstr << "doubletree-" << nNode<<"-"<<trialID;
		    runID = sstr.str ();

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
						NetDeviceContainer csmaDevices=JoinTwoNodes(nodes.Get(i),nodes.Get(j),csma);
						ipv6addr.AssignWithoutAddress(csmaDevices);
					}
				}
				else{
					for (int j=2*i+2;j<=2*i+5;j++){
						NetDeviceContainer csmaDevices=JoinTwoNodes(nodes.Get(i),nodes.Get(j),csma);
						ipv6addr.AssignWithoutAddress(csmaDevices);
					}
				}
			}

			for(int i=1;i<=(N-2)%4;i++){

				int index=2*((N-2)/4);
				NetDeviceContainer csmaDevices=JoinTwoNodes(nodes.Get(index),nodes.Get(2*index+1+i),csma);
				ipv6addr.AssignWithoutAddress(csmaDevices);

				index=index+1;
				csmaDevices=JoinTwoNodes(nodes.Get(index),nodes.Get(2*index-1+i),csma);
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
	Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (runID+".networkhash");
	Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (runID+".packets");
	//Ptr<OutputStreamWrapper> stream2 =asciiTraceHelper.CreateFileStream (runID+".packetTrace");

	Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::DncpApplication/NetworkHash", MakeBoundCallback (&NethashTrack, stream));
	Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::DncpApplication/PktRx", MakeBoundCallback (&PktTrace, stream1));
	Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::DncpApplication/PktTx", MakeBoundCallback (&PktTrace, stream1));

	Config::Connect ("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/MacTx", MakeBoundCallback (&PktTrace1,stream1));
	Config::Connect ("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/MacTxDrop", MakeBoundCallback (&PktTrace1,stream1));
	Config::Connect ("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/PhyTxDrop", MakeBoundCallback (&PktTrace1,stream1));
	//Config::Connect ("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/PhyTxEnd", MakeBoundCallback (&PhyDropTrack,stream2));
	//Config::Connect ("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/MacRx", MakeBoundCallback (&PhyDropTrack,stream2));
	//Config::Connect ("/NodeList/*/$ns3::UdpL4Protocol/SocketList/*/Drop", MakeBoundCallback (&PhyDropTrack,stream2));

	Config::Set("/NodeList/*/ApplicationList/*/$ns3::DncpApplication/StartTime",TimeValue (Seconds (startTime)));
	Config::Set("/NodeList/*/ApplicationList/*/$ns3::DncpApplication/StopTime",TimeValue (Seconds (stopTime)));

	Simulator::Stop (Seconds (stopTime+1));
	Simulator::Run ();
	Simulator::Destroy ();
	return 0;

}
