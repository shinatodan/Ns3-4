/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/* exp09-ipv6-basic.cc simulate IPv6 basi behaviours 
 * F.Qian, Nov. 2012
 * Usage: ./waf --run ipv6-basic [--showIP=1] [--showRT=1]
 *
 *  lan1(2001:1::)   lan2(2001:2::)    lan3(2001:3::)
 *  ============= R0 ============== R1 =============
 *   |    |    |         CSMA           |    |    |
 *  n0   n1   n2                        m0   m1   m2
 *
 *  CSMA Links with MTU:1500bytes, Bandwidith:10Mbps, Delay:2ms
 *  Flows:
 *        n0 -- m0 : ping application (ICMP6)
 *        n1 -- m1 : UDP echo application
 *        n2 -- m2 : UDP On/OFF application
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"

#define	PORT 	  50000
#define	SIM_START 0.1
#define	SIM_STOP  20.1

#define PROG_DIR        "local/exp09/data/"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("exp09-Ipv6Basic");

void PrintRoutingTable (Ptr<Node>& n) {
	Ptr<Ipv6StaticRouting>  routing = 0;
	Ipv6StaticRoutingHelper routingHelper;
	Ptr<Ipv6> ipv6 = n->GetObject<Ipv6> ();
	uint32_t  nbRoutes = 0;
	Ipv6RoutingTableEntry route;

	routing = routingHelper.GetStaticRouting (ipv6);

	//std::cout << "Routing table of " << n << " : " << std::endl;
	std::cout << "Destination\t\t\t\t" << "Gateway\t\t\t\t\t" << "I/F   \t" 
		  <<  "Prefix to use" << std::endl;

	nbRoutes = routing->GetNRoutes ();
	NS_LOG_UNCOND ("Number of Neighbour Routers:" << nbRoutes);
	for (uint32_t i = 0; i < nbRoutes; i++) {
		route = routing->GetRoute (i);
		std::cout << route.GetDest () << "\t"
			  << route.GetGateway () << "\t "
			  << route.GetInterface () << "\t"
			  << route.GetPrefixToUse () << "\t"
			  << std::endl;
	}
} 

// set ICMP over IPv6 service
void 
//setPing6App(Ptr <Node> node, Ipv6Address nadr, Ipv6Address madr)
setPing6App(Ptr <Node> n, Ptr <Node> m)
{
	// ping application, from 2001:1::1(n0) to 2001:3::1(m0)
	uint32_t packetSize      = 4096;
	uint32_t maxPacketCount  = 500;
	Time interPacketInterval = Seconds (0.1);

	Ping6Helper ping6;
	// Ipv6Address ns3::Ipv6InterfaceContainer::GetAddress(ifsidx, addridx)
	// - ifsidx  interface index, generally index 0 is the loopback device
	// - addridx address index, generally index 0 is the link-local address

	Ipv6InterfaceAddress nadr = n->GetObject <Ipv6> ()->GetAddress(1, 1);
	Ipv6InterfaceAddress madr = m->GetObject <Ipv6> ()->GetAddress(1, 1);
	ping6.SetLocal  (nadr.GetAddress());
	ping6.SetRemote (madr.GetAddress());

	ping6.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
	ping6.SetAttribute ("Interval"  , TimeValue (interPacketInterval));
	ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));
	ApplicationContainer apps = ping6.Install (n);
	apps.Start (Seconds (SIM_START));
	apps.Stop  (Seconds (SIM_STOP ));
}

// UDP echo application, from n(with address nadr) to m
void 
//setUdpEchoApp(Ptr <Node> n, Ipv6Address nadr, Ptr <Node> m)
setUdpEchoApp(Ptr <Node> n, Ptr <Node> m)
{
	UdpEchoServerHelper echoServer (PORT);
	ApplicationContainer serverApps = echoServer.Install (n);
	serverApps.Start (Seconds (SIM_START));
	serverApps.Stop  (Seconds (SIM_STOP ));

	Ipv6InterfaceAddress nadr = n->GetObject <Ipv6> ()->GetAddress(1, 1);
	UdpEchoClientHelper echoClient (nadr.GetAddress(), PORT);
	echoClient.SetAttribute ("MaxPackets", UintegerValue (500));
	echoClient.SetAttribute ("Interval"  , TimeValue (Seconds (0.1)));
	echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

	ApplicationContainer clientApps = echoClient.Install (m);
	clientApps.Start (Seconds (SIM_START));
	clientApps.Stop  (Seconds (SIM_STOP ));
}

void
//setUdpOnOffApp(Ptr <Node> n, Ipv6Address null)
setUdpOnOffApp(Ptr <Node> n, Ptr <Node> m)
{
	Config::SetDefault ("ns3::OnOffApplication::PacketSize", StringValue ("1024"));
	Config::SetDefault ("ns3::OnOffApplication::DataRate"  , StringValue ("1Mbps"));

	Ipv6InterfaceAddress madr = m->GetObject <Ipv6> ()->GetAddress(1, 1);
	AddressValue remoteAddr (Inet6SocketAddress (madr.GetAddress(), PORT));

	OnOffHelper onoff  ("ns3::UdpSocketFactory", Address());
	onoff.SetAttribute ("OnTime"               , StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute ("OffTime"              , StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
	onoff.SetAttribute ("Remote"               , remoteAddr);
	ApplicationContainer onoffapps = onoff.Install (n);
	onoffapps.Start (Seconds (SIM_START));
	onoffapps.Stop  (Seconds (SIM_STOP ));
}

int main (int argc, char** argv)
{
	int nLan1 = 3, nLan2 = 3;
	bool dump_addr   = false;
	bool dump_rtable = false;
	CommandLine cmd;

	cmd.AddValue("showIP", "dump IPv6 address of nodes. true(1) or false(0).", dump_addr);
	cmd.AddValue("showRT", "dump IPv6 routing tables of routers. true(1) or false(0).", dump_rtable);
	cmd.Parse (argc, argv);

	Ptr<Node> R0 = CreateObject<Node> ();
	Ptr<Node> R1 = CreateObject<Node> ();

	// create nodes on lan1
	Ptr<Node> *n = new Ptr<Node> [nLan1];
	NodeContainer lan1;
	lan1.Add (R0);
	for(int i=0;i<nLan1;i++) {
		n[i] = CreateObject<Node> ();
		lan1.Add (n[i]);
	}

	// create nodes on lan2 (bottleneck link)
	NodeContainer lan2;
	lan2.Add (R0);
	lan2.Add (R1);

	// create nodes on lan3
	Ptr<Node> *m = new Ptr<Node> [nLan2];
	NodeContainer lan3;
	lan3.Add (R1);
	for(int i=0;i<nLan2;i++) {
		m[i] = CreateObject<Node> ();
		lan3.Add (m[i]);
	}

	// aggregate containers
	NodeContainer all;
	all.Add (R0); all.Add (R1);
	for(int i=0;i<nLan1;i++) all.Add (n[i]);
	for(int i=0;i<nLan2;i++) all.Add (m[i]);

	InternetStackHelper internetv6;
	internetv6.Install (all);

	CsmaHelper csma;
	csma.SetDeviceAttribute  ("Mtu"     , UintegerValue (1500));
	csma.SetChannelAttribute ("DataRate", StringValue   ("10Mbps"));
	csma.SetChannelAttribute ("Delay"   , StringValue   ("2ms"));
	NetDeviceContainer dev1 = csma.Install (lan1);
	NetDeviceContainer dev2 = csma.Install (lan2);
	NetDeviceContainer dev3 = csma.Install (lan3);

	Ipv6AddressHelper ipv6;
	ipv6.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));
	Ipv6InterfaceContainer if1 = ipv6.Assign (dev1);
	//if1.SetRouter (0, true); // used in the previous version 3.17
	if1.SetForwarding (0, true);
	//if1.SetDefaultRouteInAllNodes (0);

	ipv6.SetBase (Ipv6Address ("2001:2::"), Ipv6Prefix (64));
	Ipv6InterfaceContainer if2 = ipv6.Assign (dev2);
	//if2.SetRouter (0, true); // used in the previous version 3.17
	//if2.SetRouter (1, true); // used in the previous version 3.17
	if2.SetForwarding (0, true);
	//if2.SetDefaultRouteInAllNodes (0);
	if2.SetForwarding (1, true);
	//if2.SetDefaultRouteInAllNodes (1);
	
	ipv6.SetBase (Ipv6Address ("2001:3::"), Ipv6Prefix (64));
	Ipv6InterfaceContainer if3 = ipv6.Assign (dev3);
	//if3.SetRouter (0, true); // used in the previous version 3.17
	if3.SetForwarding (0, true);
	//if3.SetDefaultRouteInAllNodes (0);

	if(dump_addr) {
		for(int i=0;i<nLan1;i++)
			NS_LOG_UNCOND("n"<< i << "'s address:   " 
				<< n[i]->GetObject <Ipv6> ()->GetAddress(1, 1));
		NS_LOG_UNCOND("R0's address 1: " 
			<< R0->GetObject <Ipv6> ()->GetAddress(1, 1));
		NS_LOG_UNCOND("R0's address 2: " 
			<< R0->GetObject <Ipv6> ()->GetAddress(2, 1));
		NS_LOG_UNCOND("R1's address 1: " 
			<< R1->GetObject <Ipv6> ()->GetAddress(1, 1));
		NS_LOG_UNCOND("R1's address 2: " 
			<< R1->GetObject <Ipv6> ()->GetAddress(2, 1));
		for(int i=0;i<nLan2;i++)
			NS_LOG_UNCOND("m"<< i << "'s address:   " 
				<< m[i]->GetObject <Ipv6> ()->GetAddress(1, 1));
	}

	if(dump_rtable) {
		NS_LOG_UNCOND ("R0's Routing Table :");
		PrintRoutingTable (R0);
		NS_LOG_UNCOND ("R1's Routing Table :");
		PrintRoutingTable (R1);
	}

	// ping application, from 2001:1::1(n0) to 2001:3::1(m0)
	setPing6App(n[0], m[0]);

	// UDP echo application, from 2001:1::2(n1) to 2001:3::2(m1)
	setUdpEchoApp(n[1], m[1]);

	// UDP On/OFF application, from 2001:1::3(n2) to 2001:3::3(m2)
	setUdpOnOffApp(n[2], m[2]);

	AsciiTraceHelper ascii;
	std::string fd = std::string(PROG_DIR) + "ipv6-basic";
        csma.EnablePcapAll (fd, true);

	Simulator::Stop (Seconds(10.0));
	Simulator::Run ();

	Simulator::Destroy ();
	NS_LOG_UNCOND ("Done.");
}

