/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DNCP_APPLICATION_H
#define DNCP_APPLICATION_H

extern "C" {
#include <dncp_i.h>
#include <dncp_profile.h>
#include <hncp_proto.h>
#include <tlv.h>
#include <hnetd.h>
#include <stdarg.h>
#undef LOG_INFO
#undef LOG_WARN
#undef LOG_DEBUG
}
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include "ns3/type-id.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "ns3/abort.h"
#include "ns3/ipv6.h"
#include "ns3/application.h"
#include "ns3/simulator.h"
#include "ns3/ipv6-packet-info-tag.h"
#include <iostream>
#include "ns3/object.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"

namespace ns3 {

class Socket;
class Packet;

class DncpApplication : public Application
{
public:
	static TypeId GetTypeId (void);
	DncpApplication();
	virtual ~DncpApplication();
	void DncpRun(dncp _o, int msecs);
	bool Socket_init(uint16_t port);
	void Dncp_uninit();
	void Dncp_Sendto(dncp o, void *buf, size_t len, const struct sockaddr_in6 *dst);
	ssize_t Dncp_Recvfrom( void *buf, size_t len,  char *ifname,
          	  	  	  	  struct sockaddr_in6 *src, struct in6_addr *dst);
	void PutTLV();

private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);
	void DncpDoRun(dncp _o);
	void HandleRead (Ptr<Socket> socket);
	void MsgReceivedCallback (dncp_subscriber s,
            const char *ifname,
            struct sockaddr_in6 *src,
            struct in6_addr *dst,
            struct tlv_attr *msg);

	ns3::Ptr<ns3::Socket>     	m_socket;
	uint32_t        			m_nPackets;
	ns3::EventId         	    m_timeoutEvent;
	bool            			m_running;
	uint32_t        			m_packetsSent;
	dncp   				 	    o;
	TracedValue<uint64_t> 		net_hash;
	TracedCallback<Ipv6Address,Ipv6Address,uint32_t,uint32_t,uint64_t,struct tlv_attr*,bool >    m_packetRxTrace;
	TracedCallback<Ipv6Address,Ipv6Address,uint32_t,uint32_t,uint64_t,struct tlv_attr*,bool >    m_packetTxTrace;
	TracedCallback<Ptr<Packet>,uint32_t,bool>																	   m_packetRxTrace1;


	typedef void(* DncpCallback)(Ipv6Address saddr, Ipv6Address dstaddr,
			uint32_t deviceIndex,uint32_t pktsize,uint64_t uid,struct tlv_attr* msg,bool receive);

	typedef void(* DncpCallback1)(Ptr<Packet>,uint32_t deviceIndex,bool receive);

};

/* ... */
}


#endif /* DNCP_APPLICATION_H */

