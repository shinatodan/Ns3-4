/* -*- Mode:C:++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* exp02-BulkTCP-with-cwnd-trace.cc: simulate TCP behaviours with a simple 4-nodes topology.
 * F.Qian, Nov. 2012
 *
 * n0(tcp source) 
 *  \.1
 *   \ net1(25Mbps,2ms)192.168.1.0/24
 *  .2\
 *     n1 -----net3(10Mbps,5ms)----- n3(tcp sink)
 *  .1/  .1	192.168.3.0/24      .2
 *   / net2(25Mbps,2ms)192.168.2.0/24
 *  /.2
 * n2(tcp source)
 */

#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"

#define	NET_MASK  "255.255.255.0"
#define	NET_ADD1  "192.168.1.0"
#define	NET_ADD2  "192.168.2.0"
#define	NET_ADD3  "192.168.3.0"
#define	FIRST_NO  "0.0.0.1"

#define DATA_MBYTES 500
#define	PORT  	    50000

#define SIM_START 00.10
#define SIM_STOP  10.10

#define PROG_DIR "local/exp02/data/"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("exp02-SimpleTCPwithCwnd");

void 
CwndTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldcwnd, uint32_t newcwnd)
{
	//fprintf(stdout,"%10.4f %6d %6d\n",Simulator::Now ().GetSeconds (),oldcwnd,newcwnd);
  	*stream->GetStream() << Simulator::Now ().GetSeconds () 
		<< " \t " << newcwnd << std::endl;
}

void 
TraceDropedPkt (Ptr<const Packet> pkt)
{
	fprintf(stdout,"%10.4f droped a packet (%4d bytes) with sequence no:%6d\n",
		Simulator::Now ().GetSeconds (),
		(int)pkt->GetSize(),
		(int)pkt->GetUid());
}

void 
MyEventHandller (Ptr<Application> app, Ptr<OutputStreamWrapper> stream)
{
	// get socket according to each application helper.
	// for BulkSenderHelper, the application object is "BulkSendApplication".
	// for more information about this, ref. Doxygen documentation system,
	// and check the class list, select "ns::Application", looking at the
	// Inheritance diagram for "ns::Application:".
	
	Ptr<Socket> src_socket  = app->GetObject<BulkSendApplication>()->GetSocket();

	// ref. Doxygen documentation system, chesk the list of all trace sources
	// at "modules"->"C++ Constructs Used by All Modules"->"Debugging"->
	// "the list of all trace sources".
	src_socket->TraceConnectWithoutContext("CongestionWindow", 
		MakeBoundCallback(&CwndTracer, stream));
}

int 
main (int argc, char *argv[])
{
	std::string tcpType = "TcpNewReno";
	CommandLine cmd;

  	//LogComponentEnable("TcpSocketBase", LOG_LEVEL_FUNCTION);

	cmd.AddValue("tcpType", "TCP versions (TcpTahoe,TcpReno,TcpNewReno(default))", tcpType);
	cmd.Parse(argc, argv);

  	if(argc > 1) {
		if(tcpType.compare("TcpTahoe") == 0) {
  			//LogComponentEnable("TcpTahoe", LOG_LEVEL_INFO);
			Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpTahoe"));
		} else if(tcpType.compare("TcpReno") == 0) {
  			//LogComponentEnable("TcpReno", LOG_LEVEL_INFO);
			Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpReno"));
		} else if(tcpType.compare("TcpNewReno") == 0) {
  			//LogComponentEnable("TcpNewReno", LOG_LEVEL_INFO);
			Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
		} else {
			std::cerr << "Invalid TCP version, use TCP NewReno" << std::endl;
			tcpType = "TcpNewReno";
		}
  	}
	std::cout << "use " << tcpType << std::endl;

	Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (10));

  	NS_LOG_DEBUG("Creating Topology");

  	NodeContainer net1_nodes;
  	net1_nodes.Create (2);

  	NodeContainer net2_nodes;
	net2_nodes.Add (net1_nodes.Get(1));
  	net2_nodes.Create (1);

  	NodeContainer net3_nodes;
	net3_nodes.Add (net1_nodes.Get(1));
  	net3_nodes.Create (1);

  	PointToPointHelper p2p1;
  	p2p1.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  	p2p1.SetChannelAttribute ("Delay", StringValue ("2ms"));
	p2p1.SetQueue ("ns3::DropTailQueue");

  	NetDeviceContainer devices1;
 	// for simplex link do :
  	//devices1 = p2p1.Install (net1_nodes.Get(0), net1_nodes.Get(1));
  	//devices1 = p2p1.Install (net1_nodes.Get(1), net1_nodes.Get(0));
 	// for duplex link do :
  	devices1 = p2p1.Install (net1_nodes);

  	NetDeviceContainer devices2;
  	devices2 = p2p1.Install (net2_nodes);

  	PointToPointHelper p2p2;
  	p2p2.SetDeviceAttribute ("DataRate", StringValue ("800Kbps"));
  	p2p2.SetChannelAttribute ("Delay", StringValue ("5ms"));
	p2p2.SetQueue ("ns3::DropTailQueue");

  	NetDeviceContainer devices3;
  	devices3 = p2p2.Install (net3_nodes);

  	InternetStackHelper stack;
  	stack.InstallAll ();

  	Ipv4AddressHelper address;

  	address.SetBase (NET_ADD1, NET_MASK, FIRST_NO);
  	Ipv4InterfaceContainer ifs1 = address.Assign (devices1);
  	NS_LOG_INFO ("Network 1: " << ifs1.GetAddress(0, 0) << " - " << ifs1.GetAddress(1, 0));

  	address.SetBase (NET_ADD2, NET_MASK, FIRST_NO);
  	Ipv4InterfaceContainer ifs2 = address.Assign (devices2);
  	NS_LOG_INFO ("Network 2: " << ifs2.GetAddress(0, 0) << " - " << ifs2.GetAddress(1, 0));

  	address.SetBase (NET_ADD3, NET_MASK, FIRST_NO);
  	Ipv4InterfaceContainer ifs3 = address.Assign (devices3);
  	NS_LOG_INFO ("Network 3: " << ifs3.GetAddress(0, 0) << " - " << ifs3.GetAddress(1, 0));

	NS_LOG_INFO ("Initialize Global Routing.");
  	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	// get n3's peer address as remote(sink) address
	AddressValue remoteAddress (InetSocketAddress (ifs3.GetAddress(1, 0), PORT));

	// create FTP like flows
	BulkSendHelper ftp ("ns3::TcpSocketFactory", Address());
	ftp.SetAttribute ("MaxBytes", UintegerValue (int(DATA_MBYTES * 1024 * 1024)));
	ftp.SetAttribute ("Remote", remoteAddress);

	ApplicationContainer sourceApp1;
	sourceApp1 = ftp.Install (net1_nodes.Get(0));
        sourceApp1.Start (Seconds (SIM_START+0.1));
        sourceApp1.Stop  (Seconds (SIM_STOP -0.1));

	ApplicationContainer sourceApp2;
	sourceApp2 = ftp.Install (net2_nodes.Get(1));
        sourceApp2.Start (Seconds (SIM_START+0.1));
        sourceApp2.Stop  (Seconds (SIM_STOP -0.1));

	// create a sink to recieve packets @ network3's node 1(192.168.3.2).
  	Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), PORT));
  	PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkAddress);

	//sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
        ApplicationContainer sink_apps = sinkHelper.Install (net3_nodes.Get(1));

	sink_apps.Start (Seconds (SIM_START+0.1));
  	sink_apps.Stop  (Seconds (SIM_STOP -0.1));

	// Trace changes to the congestion window
	// Note :
	// However, by using the BulkSendHelper the TCP socket is not created until the 
	// simulation is already running (i.e. after Simulator::Run () is called), and 
	// therefore we cannot add the "CongestionWindow" callback on configuration time.
	// as following,
	// if(printCwnd.compare("True") == 0) {
  	//	Config::ConnectWithoutContext (
	//		"/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", 
	//		MakeCallback (&CwndTracer));
	// }
	// On the NS-3 tutorial, it is proposed two alternatives, which are 
	// 1) create a simulator event that is run after the dynamic object is created 
	//    and hook the trace when that event is executed; 
	// 2) create the dynamic object at configuration time, hook it then, and give 
	//    the object to the system to use during simulation time. 
	// at here, the 2nd. approach is taken.

  	AsciiTraceHelper ascii;
	//std::string afname = std::string(PROG_DIR) + "simple-bulk-tcp.tr";
  	//p2p2.EnableAsciiAll (ascii.CreateFileStream (afname));

	// make trace file's name
	std::string fname = std::string(PROG_DIR) + tcpType + "-bulk.cwnd";
	Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(fname);

	Simulator::Schedule (Seconds (0.5)+NanoSeconds (1.0), 
		&MyEventHandller, net1_nodes.Get(0)->GetApplication(0), stream);

	// trace droped packet from queue
	Ptr<DropTailQueue> q = CreateObject<DropTailQueue> ();
	q->SetAttribute ("Mode", EnumValue (DropTailQueue::QUEUE_MODE_PACKETS));
	devices3.Get (0)->SetAttribute ("TxQueue", PointerValue (q));
	q->TraceConnectWithoutContext ("Drop", MakeCallback (&TraceDropedPkt));

  	// for PCAP Tracing
	//std::string pfname = std::string(PROG_DIR) + "bulk-simple-tcp";
  	//p2p2.EnablePcapAll (pfname);

	Simulator::Stop (Seconds(SIM_STOP));
  	Simulator::Run ();
  	Simulator::Destroy ();

  	return 0;
}
