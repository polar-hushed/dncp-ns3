/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/log.h"
#include "dncp-application.h"

NS_LOG_COMPONENT_DEFINE ("DncpApplication");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DncpApplication);

TypeId
DncpApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DncpApplication")
    .SetParent<Application> ()
    .AddConstructor<DncpApplication>()
  ;
  return tid;
}

DncpApplication::DncpApplication ()
  : m_socket (0),
    m_packetSize (10),
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
	if (m_running)
		dncp_run(_o);
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
DncpApplication::Dncp_Sendto(dncp o,void *buf, size_t len, const struct sockaddr_in6 *dst){

	NS_LOG_FUNCTION (this<<o);
	if(m_running)
	{
		/*Get the destination address, port, and device*/
		uint32_t interfaceIndex=dst->sin6_scope_id;
		char addrBuffer[INET6_ADDRSTRLEN];
		inet_ntop(dst->sin6_family,&dst->sin6_addr,addrBuffer,sizeof(addrBuffer));

		Ipv6Address dstAddr (addrBuffer);
		if (Ipv6Address::IsMatchingType (dstAddr)){

			uint16_t dstPort=dst->sin6_port;

			Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6> ();
			uint8_t *buf1;
			buf1=static_cast<uint8_t *>(buf);
			std::cout<<*buf1<<std::endl;
			Ptr<Packet> packet = Create<Packet> (buf1,len);

			m_socket->SendTo (packet,0,Inet6SocketAddress(Ipv6Address::ConvertFrom(dstAddr), dstPort),
				ipv6->GetNetDevice (interfaceIndex));

			NS_LOG_INFO ("At time " <<Simulator::Now ().GetSeconds () << "s Node "<<GetNode()->GetId()<<" sent " << len << " bytes to "
						<<Ipv6Address::ConvertFrom (dstAddr) << " port " <<dstPort<<" through interface "<<interfaceIndex);
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
			struct sockaddr_in6 peer;

			/*Get destination information,first incoming interface index*/
			uint32_t incomingIf = interfaceInfo.GetRecvIf ();
			Ptr<NetDevice> dev=GetNode()->GetDevice(incomingIf);
			Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
			uint32_t ipInterfaceIndex = ipv6->GetInterfaceForDevice (dev);

			/*Then source address, port*/
			uint16_t port=Inet6SocketAddress::ConvertFrom (from).GetPort ();
			char addrBuffer[INET6_ADDRSTRLEN];
			std::stringstream stream;
			stream<<Inet6SocketAddress::ConvertFrom (from).GetIpv6 ();
			stream>>addrBuffer;

			/*put source information in sockaddr_in6 struct*/
			if (inet_pton(AF_INET6, addrBuffer,&peer.sin6_addr)<1)
				NS_LOG_ERROR("unable to inet_pton");
			peer.sin6_family = AF_INET6;
			peer.sin6_port = port;
			peer.sin6_scope_id=ipInterfaceIndex;

			/*Get the destination address*/
			char addrBuffer1[INET6_ADDRSTRLEN];
			std::stringstream stream1;
			stream1<<interfaceInfo.GetAddress();
			stream1>>addrBuffer1;
			if (inet_pton(AF_INET6, addrBuffer1,dst)<1)
				NS_LOG_ERROR("unable to inet_pton");

			packet->CopyData(static_cast<uint8_t*>(buf),packet->GetSize ());

			memcpy(src,&peer,sizeof(struct sockaddr_in6));
			std::sprintf(ifname,"%d",ipInterfaceIndex);

			NS_LOG_INFO("At time " << Simulator::Now ().GetSeconds () << "s Node "<<GetNode()->GetId()<<
					" received " << packet->GetSize () << " bytes from "<<addrBuffer << " port " <<port<<
					" from ipInterface "<<ipInterfaceIndex<<" from device interface "<<incomingIf);

			return packet->GetSize ();

	    }
		else
		{
			NS_LOG_WARN("Ipv4 packet received in dncp");
			return 0;
		}
	}

	return 0;
}

}/*End ns3 namespace*/



