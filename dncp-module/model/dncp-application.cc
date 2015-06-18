/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
extern "C" {
#include "string.h"
}
#include "ns3/log.h"
#include "dncp-application.h"
#include "node-id-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DncpApplication");

NS_OBJECT_ENSURE_REGISTERED (DncpApplication);


TypeId
DncpApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DncpApplication")
    .SetParent<Application> ()
    .AddConstructor<DncpApplication>()
	.AddTraceSource ("NetworkHash",
	                 "the network hash value calculated by this node",
	                 MakeTraceSourceAccessor (&DncpApplication::net_hash))
	.AddTraceSource("PktRx",
					"Trace source indicating a packet was received",
					MakeTraceSourceAccessor(&DncpApplication::m_packetRxTrace),
					"ns3::DncpApplication::DncpCallback")
	.AddTraceSource("PktTx",
					"Trace source indicating a packet was sent",
					MakeTraceSourceAccessor(&DncpApplication::m_packetTxTrace),
					"ns3::DncpApplication::DncpCallback")
  ;
  return tid;
}

DncpApplication::DncpApplication ()
  : m_socket (0),
    m_nPackets (5),
    m_timeoutEvent (),
    m_running (false),
    m_packetsSent (0)
{
	NS_LOG_FUNCTION (this);
}

DncpApplication::~DncpApplication()
{
  m_socket = 0;
}


void
DncpApplication::DncpRun(dncp _o, int msecs)
{	NS_LOG_FUNCTION (this<<msecs);
	m_timeoutEvent=Simulator::Schedule(MilliSeconds(msecs), & DncpApplication::DncpDoRun,this,_o);

}

void
DncpApplication::DncpDoRun(dncp _o){
	NS_LOG_FUNCTION (this);
	if (m_running){
		dncp_run(_o);
		net_hash=dncp_hash64(&o->network_hash);
	}


}

void
DncpApplication::StartApplication (void)
{
	o=dncp_create(this);

	int num=this->GetNode()->GetNDevices();

	for(int i=1;i<num;i++){
		char *nom;
		char a[10];
		std::sprintf(a,"%d",i);
		nom=a;
		dncp_if_set_enabled(o, nom, 1);
	}

	if (inet_pton(AF_INET6, HNCP_MCAST_GROUP,
	                &o->profile_data.multicast_address) < 1)
	    {
	      NS_LOG_ERROR("unable to inet_pton multicast group address");
	    }

	m_running = true;
	m_packetsSent = 0;
}


void
DncpApplication::StopApplication (void)
{
	NS_LOG_FUNCTION (this);
	m_running = false;

	if (m_timeoutEvent.IsRunning ())
		{
	      Simulator::Cancel (m_timeoutEvent);
	    }

	if (m_socket)
	{
		m_socket->Close ();
		m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	}
	//dncp_destroy(o);
	NS_LOG_INFO ("At time " <<Simulator::Now ().GetSeconds () << "s Node "<<GetNode()->GetId()<<" stopped, " <<m_packetsSent
			<<" packets sent in total");

}


void
DncpApplication::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket<<o);
	dncp_poll(o);
}

bool
DncpApplication::Socket_init(uint16_t port)
{
	NS_LOG_FUNCTION (this<<port);

	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	m_socket =Socket::CreateSocket (GetNode (), tid);

	Inet6SocketAddress local=Inet6SocketAddress (Ipv6Address::GetAny (), port);

	if(m_socket->Bind(local)==-1)
		return false;

	m_socket->SetRecvPktInfo(true);
	m_socket->SetRecvCallback (MakeCallback (&DncpApplication::HandleRead, this));
	return true;
}
void
DncpApplication::Dncp_uninit(){

	NS_LOG_FUNCTION (this<<o);

	if (m_timeoutEvent.IsRunning ())
	{
      Simulator::Cancel (m_timeoutEvent);
    }

	if (m_socket)
	    {
	      m_socket->Close ();
	      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	    }
}

void
DncpApplication::Dncp_Sendto(dncp o,void *buf, size_t len, const struct sockaddr_in6 *dst)
{
	NS_LOG_FUNCTION (this<<o);
	if(m_running)
	{
		/*Get the destination address, port, and device*/
		uint32_t deviceIndex=dst->sin6_scope_id;
		char addrBuffer[INET6_ADDRSTRLEN];
		inet_ntop(dst->sin6_family,&dst->sin6_addr,addrBuffer,sizeof(addrBuffer));

		Ipv6Address dstAddr (addrBuffer);
		if (Ipv6Address::IsMatchingType (dstAddr)){

			uint16_t dstPort=dst->sin6_port;

			Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6> ();
			uint32_t interfaceIndex=ipv6->GetInterfaceForDevice(GetNode()->GetDevice (deviceIndex));

			Ptr<Packet> packet = Create<Packet> (static_cast<uint8_t *>(buf),len);

			NodeIdTag nodeId;
			nodeId.SetNodeId (GetNode()->GetId());
			packet->AddByteTag (nodeId);


			NS_LOG_INFO ("At time " <<Simulator::Now ().GetSeconds () << "s Node "<<GetNode()->GetId()<<" sent " << len << " bytes to "
						<<Ipv6Address::ConvertFrom (dstAddr) << " port " <<dstPort<<" through interface "<<deviceIndex);

			struct tlv_attr *msg =container_of(static_cast<char(*)[0]>(buf), tlv_attr, data);





			m_packetTxTrace(ipv6->GetAddress(interfaceIndex,0).GetAddress(),Ipv6Address::ConvertFrom (dstAddr),deviceIndex,len,packet->GetUid(),msg,false);

			m_socket->SendTo (packet,0,Inet6SocketAddress(Ipv6Address::ConvertFrom(dstAddr), dstPort),
				GetNode()->GetDevice (deviceIndex));

		}

		else{

			NS_LOG_WARN("trying to send a packet to an ipv4 address in dncp");
			return;
		}

		m_packetsSent++;
	}
}

ssize_t
DncpApplication::Dncp_Recvfrom(void *buf, size_t len, char *ifname,
						struct sockaddr_in6 *src, struct in6_addr *dst){

	NS_LOG_FUNCTION (this<<o);

	Ptr<Packet> packet;
	Address from;

	while ((packet = m_socket->RecvFrom (from)))
	{
		if (Inet6SocketAddress::IsMatchingType (from))
	    {
			Ipv6PacketInfoTag interfaceInfo;
			if (!packet->RemovePacketTag (interfaceInfo))
			{
			   NS_ABORT_MSG ("No incoming interface on RIPng message, aborting.");
			}

			NodeIdTag nodeidtag;
			uint32_t sourceNodeId;
			if (packet->FindFirstMatchingByteTag (nodeidtag)) {
				sourceNodeId= nodeidtag.GetNodeId ();
			}
			else{
				NS_ABORT_MSG ("No nodeid tagged to the packet, aborting.");
			}

			struct sockaddr_in6 peer;
			Ipv6Address sourceAddr;
			uint32_t pktSize=packet->GetSize ();

			/*Get destination information,first incoming interface index*/
			uint32_t incomingIf = interfaceInfo.GetRecvIf ();

			/*Then source address, port*/
			sourceAddr=Inet6SocketAddress::ConvertFrom (from).GetIpv6 ();
			uint16_t port=Inet6SocketAddress::ConvertFrom (from).GetPort ();
			char addrBuffer[INET6_ADDRSTRLEN];
			std::stringstream stream;
			stream<<sourceAddr;
			stream>>addrBuffer;

			/*put source information in sockaddr_in6 struct*/
			if (inet_pton(AF_INET6, addrBuffer,&peer.sin6_addr)<1)
				NS_LOG_ERROR("unable to inet_pton");
			peer.sin6_family = AF_INET6;
			peer.sin6_port = port;
			peer.sin6_scope_id=incomingIf;

			char addrBuffer1[INET6_ADDRSTRLEN];
			std::stringstream stream1;
			stream1<<interfaceInfo.GetAddress();
			stream1>>addrBuffer1;
			if (inet_pton(AF_INET6, addrBuffer1,dst)<1)
				NS_LOG_ERROR("unable to inet_pton");

			packet->CopyData(static_cast<uint8_t*>(buf),pktSize);
			memcpy(src,&peer,sizeof(struct sockaddr_in6));
			std::sprintf(ifname,"%d",incomingIf);


			struct tlv_attr *msg =container_of(static_cast<char(*)[0]>(buf), tlv_attr, data);

			NS_LOG_INFO("At time " << Simulator::Now ().GetSeconds () << "s Node "<<GetNode()->GetId()<<
					" received " << packet->GetSize () << " bytes from node "<<sourceNodeId<<": "<<addrBuffer << " port " <<port<<
					" from device "<<incomingIf);

			m_packetRxTrace(sourceAddr,interfaceInfo.GetAddress(),incomingIf,pktSize,packet->GetUid(),msg,true);

			//m_packetRxTrace1(packet,incomingIf,true);

			return pktSize;

	    }
		else
		{
			NS_LOG_WARN("Ipv4 packet received in dncp");
			return 0;
		}
	}

	return 0;
}

void
DncpApplication::MsgReceivedCallback (dncp_subscriber s, const char *ifname, struct sockaddr_in6 *src,
									  struct in6_addr *dst, struct tlv_attr *msg){
	std::cout<<Simulator::Now ().GetSeconds ()<<" receive callback"<<std::endl;
}

void
DncpApplication::PutTLV(){
	char *data = "The answer";
	dncp_tlv tlv = dncp_add_tlv(o, 42, data, 3000, 0);
	if(!(tlv)) {
		NS_LOG_ERROR("Could not publish TLV");
		}
}

}/*End ns3 namespace*/



