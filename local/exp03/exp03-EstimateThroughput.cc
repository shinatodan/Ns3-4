/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* exp03-EstimateThroughput.cc: simulate TCP behaviours with a simple 4-nodes topology.
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

#define DATA_MBYTES 	500
#define SIM_START 	00.10
#define SIM_STOP  	20.10
#define PORT  		50000

#define	PROG_DIR	"local/exp03/data/"

#define TH_INTERVAL	0.1	// The time interval in seconds for measurement throughput
#define CW_INTERVAL	0.5	// The time interval in seconds for measurement cwnd

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("exp03-OnOffTCPThroughput");

void 
CwndTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldcwnd, uint32_t newcwnd)
{
	//fprintf(stdout,"%10.4f %6d %6d\n",Simulator::Now ().GetSeconds (),oldcwnd,newcwnd);
  	*stream->GetStream() << Simulator::Now ().GetSeconds () 
		<< " \t " << newcwnd << std::endl;
}

uint32_t oldTotalBytes=0;
uint32_t newTotalBytes;

void 
TraceThroughput (Ptr<Application> app, Ptr<OutputStreamWrapper> stream)
{
	Ptr <PacketSink> pktSink = DynamicCast <PacketSink> (app);
      	newTotalBytes = pktSink->GetTotalRx ();
	// messure throughput in Kbps
	//fprintf(stdout,"%10.4f %f\n",Simulator::Now ().GetSeconds (), 
	//	(newTotalBytes - oldTotalBytes)*8/0.1/1024);
  	*stream->GetStream() << Simulator::Now ().GetSeconds () 
		<< " \t " << (newTotalBytes - oldTotalBytes)*8.0/0.1/1024 << std::endl;
	oldTotalBytes = newTotalBytes;
	Simulator::Schedule (Seconds (TH_INTERVAL), &TraceThroughput, app, stream);
}

void 
MyEventHandller (Ptr<Application> app, Ptr<OutputStreamWrapper> stream)
{
        Ptr<Socket> src_socket  = app->GetObject<OnOffApplication>()->GetSocket();
        src_socket->TraceConnectWithoutContext("CongestionWindow", 
                MakeBoundCallback(&CwndTracer, stream));
}

int 
main (int argc, char *argv[])
{
	std::string tcpType = "TcpNewReno";
	std::string flowType = "Constant";
	CommandLine cmd;

  	//LogComponentEnable("TcpSocketBase", LOG_LEVEL_FUNCTION);

	cmd.AddValue("tcpType", 
		"TCP versions (TcpTahoe,TcpReno,TcpNewReno(default))", tcpType);
	cmd.AddValue("flowType", 
		"Flow type (Constant(default),Exponential,Uniform)", flowType);
	cmd.Parse(argc, argv);

  	if(argc > 1) {
		if(tcpType.compare("TcpTahoe") == 0) {
  			//LogComponentEnable("TcpTahoe", LOG_LEVEL_INFO);
			Config::SetDefault ("ns3::TcpL4Protocol::SocketType", 
				StringValue ("ns3::TcpTahoe"));
			std::cout << "use " << tcpType << std::endl;
		} else if(tcpType.compare("TcpReno") == 0) {
  			//LogComponentEnable("TcpReno", LOG_LEVEL_INFO);
			Config::SetDefault ("ns3::TcpL4Protocol::SocketType", 
				StringValue ("ns3::TcpReno"));
			std::cout << "use " << tcpType << std::endl;
		} else if(tcpType.compare("TcpNewReno") == 0) {
  			//LogComponentEnable("TcpNewReno", LOG_LEVEL_INFO);
			Config::SetDefault ("ns3::TcpL4Protocol::SocketType", 
				StringValue ("ns3::TcpNewReno"));
			std::cout << "use " << tcpType << std::endl;
		} else {
			std::cerr << "Invalid TCP version, use TCP NewReno" << std::endl;
			tcpType = "TcpNewReno";
		}
  	}

	Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (10));
	Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (1024));
        Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("100kb/s"));

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

	Address srcAddress (InetSocketAddress (ifs1.GetAddress (0), PORT));
        OnOffHelper ftp ("ns3::TcpSocketFactory", srcAddress);
        AddressValue remoteAddress (InetSocketAddress (ifs3.GetAddress(1, 0), PORT));
        ftp.SetAttribute ("Remote"    , remoteAddress);
        ftp.SetAttribute ("DataRate"  , StringValue ("450kbps"));
        ftp.SetAttribute ("PacketSize", UintegerValue (1024));
	if(flowType.compare("Exponential") == 0) {
		// set on/off time randomly with exponential random distribution
        	ftp.SetAttribute ("OnTime" , StringValue ("ns3::ExponentialRandomVariable[Mean=0.352]"));
        	ftp.SetAttribute ("OffTime", StringValue ("ns3::ExponentialRandomVariable[Mean=0.150]"));
	} else if(flowType.compare("Uniform") == 0) {
		// set On/Off time randomly with uniform random distribution(on/off in seconds)
        	ftp.SetAttribute ("OnTime" , StringValue ("ns3::UniformRandomVariable[Min=1.0,max=5.00]"));
        	ftp.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.1,max=0.80]"));
	} else {
        	ftp.SetAttribute ("OnTime" , StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        	ftp.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
	}

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

  	AsciiTraceHelper ascii;
	std::string afname = std::string(PROG_DIR) + "simple-tcp.tr";
        p2p2.EnableAsciiAll (ascii.CreateFileStream (afname));

	// make trace file's name
	std::string fname1 = std::string(PROG_DIR) + "exp03-" + tcpType + "-" + flowType + ".cwnd";
	Ptr<OutputStreamWrapper> stream1 = ascii.CreateFileStream(fname1);
	Simulator::Schedule (Seconds (CW_INTERVAL), 
                &MyEventHandller, net1_nodes.Get(0)->GetApplication(0), stream1);

	std::string fname2 = std::string(PROG_DIR) + "exp03-" + tcpType + "-" + flowType + ".throughput";
	Ptr<OutputStreamWrapper> stream2 = ascii.CreateFileStream(fname2);
	Simulator::Schedule (Seconds (TH_INTERVAL), 
		&TraceThroughput, net3_nodes.Get(1)->GetApplication(0), stream2);

  	// for PCAP Tracing
	//std::string pfname = std::string(PROG_DIR) + "simple-tcp";
        //p2p2.EnablePcapAll (pfname);

	Simulator::Stop (Seconds(SIM_STOP));
  	Simulator::Run ();
  	Simulator::Destroy ();

  	return 0;
}
