#ifndef DNCP_IO_NS3
#define DNCp_IO_NS3

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
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/ipv6.h"
#include "ns3/application.h"
#include "ns3/dncp-application.h"
#include "ns3/simulator.h"
#include "ns3/ipv6-packet-info-tag.h"
#include <iostream>

#define HNCP_MAXIMUM_MULTICAST_SIZE (1280-40-8)

using namespace ns3;

struct tlv_attr *dncp_profile_node_validate_data(dncp_node n,
                                          struct tlv_attr *a)
{
	struct tlv_attr *a_valid = a;
	return a_valid;
}


void dncp_profile_link_send_network_state(dncp_link l){
	struct sockaddr_in6 dst;
	dst.sin6_family = AF_INET6;
	dst.sin6_addr = l->dncp->profile_data.multicast_address;
	dst.sin6_port = l->dncp->udp_port;

	if (!(dst.sin6_scope_id = l->ifindex))
		if (!(dst.sin6_scope_id = if_nametoindex(l->ifname)))
	    {
			L_ERR("dncp_profile_link_send_network_state: Unable to find index ");
	        return;
	    }

	dncp_link_send_network_state(l, &dst, HNCP_MAXIMUM_MULTICAST_SIZE);
}

/* Profile hook to allow overriding collision handling. */
bool dncp_profile_handle_collision(dncp o){

	dncp_node_identifier_s ni;
	int i;
	for (i = 0; i < DNCP_NI_LEN; i++)
		ni.buf[i] = random() % 256;
	dncp_set_own_node_identifier(o, &ni);
	return true;
}


bool dncp_io_init(dncp o){
	void *t=o->userdata;
	ns3::DncpApplication *app;
	app=static_cast<ns3::DncpApplication*>(t);
	Ptr<DncpApplication> app_p = Ptr<DncpApplication>(app);

	if (!o->udp_port)
		o->udp_port = HNCP_PORT;

	if(app_p->Socket_init(o->udp_port)) {
		return true;
	}
	return false;
}

void dncp_io_uninit(dncp o){
	void *t=o->userdata;
	ns3::DncpApplication *app;
	app=static_cast<ns3::DncpApplication*>(t);
	Ptr<DncpApplication> app_p=Ptr<DncpApplication>(app);

	app_p->Dncp_uninit();
}

bool dncp_io_set_ifname_enabled(dncp o, const char *ifname, bool enabled){

	dncp_link l = dncp_find_link_by_name(o, ifname, false);

	if (!l )
		return false;

	l->ifindex=std::atoi(ifname);
	return true;
}

int dncp_io_get_hwaddrs(unsigned char *buf, int buf_left){
	unsigned char buf1[4];
	int i;
	for(i=0;i<4;i++){
		buf1[i]=random()%256;
	}
	memcpy(buf, buf1, 12);
	return 12;
}

void dncp_io_schedule(dncp o, int msecs){

	void *t=o->userdata;
	ns3::DncpApplication *app;
	app=static_cast<ns3::DncpApplication*>(t);
	Ptr<DncpApplication> app_p = Ptr<DncpApplication>(app);
	app_p->DncpRun(o,msecs);

}

hnetd_time_t dncp_io_time(dncp o){

	return (((hnetd_time_t)ns3::Simulator::Now ().GetMilliSeconds () ));
}

ssize_t dncp_io_recvfrom(dncp o, void *buf, size_t len,
                         char *ifname,
                         struct sockaddr_in6 *src,
                         struct in6_addr *dst){
	dncp_time(o);
	void *t=o->userdata;
	ns3::DncpApplication *app;
	app=static_cast<ns3::DncpApplication*>(t);
	Ptr<DncpApplication> app_p = Ptr<DncpApplication>(app);
	return app_p->Dncp_Recvfrom(buf,len,ifname,src,dst);

}


ssize_t dncp_io_sendto(dncp o, void *buf, size_t len,
                       const struct sockaddr_in6 *dst,
					   const struct in6_pktinfo *src){
	void *t=o->userdata;
	ns3::DncpApplication *app;
	app=static_cast<ns3::DncpApplication*>(t);
	Ptr<DncpApplication> app_p = Ptr<DncpApplication>(app);
	char addrBuffer1[48];
	memcpy(addrBuffer1,buf,48);
	app_p->Dncp_Sendto(o,buf,len,dst);
	return 0;
}


void _hnetd_log(int priority, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	vprintf(format, argptr);
	printf("\n");
	va_end(argptr);
}

int log_level=0;
void (*hnetd_log)(int priority, const char *format, ...) = _hnetd_log;


#endif
