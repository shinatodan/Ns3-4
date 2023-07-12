/* -*- mode:c++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* exp08-LongFatPipeTcp: simulate tcp behaviours with a Long Fat Pipe.
 * F.Qian, Nov. 2012
 *
 *   source                           sink
 *     n0 -----net3(100Mbps,50ms)----- n1
 *         	 192.168.3.0/24        
 */

#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"

#define	NET_MASK  	"255.255.255.0"
#define	NET_ADD  	"192.168.1.0"
#define	FIRST_NO  	"0.0.0.1"

#define	SIM_START 	00.10
#define PORT		50000
#define CW_INTERVAL     0.01 	// The time interval in seconds for measurement cwnd
#define TH_INTERVAL     0.1     // The time interval in seconds for measurement throughput

#define	PROG_DIR	"local/exp08/data/"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("exp08-LongFatPipe");

uint32_t dl = 2; // propogation delay of bottleneck link

uint32_t oldTotalBytes=0;
uint32_t newTotalBytes;
double   throughput=0.0;

void 
TraceThroughput (Ptr<Application> app, Ptr<OutputStreamWrapper> stream)
{
        Ptr <PacketSink> pktSink = DynamicCast <PacketSink> (app);
        newTotalBytes = pktSink->GetTotalRx ();
	double th = (newTotalBytes - oldTotalBytes)*8.0/0.1/1024;
        *stream->GetStream() << Simulator::Now ().GetSeconds () << " \t " << th << std::endl;
	throughput = (throughput+th)/2.0;
        oldTotalBytes = newTotalBytes;
        Simulator::Schedule (Seconds (TH_INTERVAL), &TraceThroughput, app, stream);
}

void 
CwndTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldcwnd, uint32_t newcwnd)
{
	// cwnd is traced in byte unit ??
        *stream->GetStream() << dl << " \t" << Simulator::Now ().GetSeconds () 
                << " \t " << newcwnd << std::endl;
}

void 
MyEventHandller (Ptr<Application> app, Ptr<OutputStreamWrapper> st)
{
        Ptr<Socket> src_socket  = app->GetObject<OnOffApplication>()->GetSocket();
        src_socket->TraceConnectWithoutContext("CongestionWindow", 
                MakeBoundCallback(&CwndTracer, st));
}

void 
MyEventWrapper (Ptr<Application> app, Ptr<OutputStreamWrapper> st)
{
        Simulator::Schedule (Seconds (CW_INTERVAL), &MyEventHandller, app, st);
}

int main (int argc, char *argv[])
{
	uint32_t    pktSize          = 512;       // Set OnOff App Packet Size
        std::string appDataRate      = "100Mbps"; // Set OnOff App DataRate
	uint32_t    duration         = 100;       // simulation duration
	uint32_t    bw               = 100;       // bandwidth of bottleneck link
	uint32_t    bottleNeckLinkQS = 5;         // maximum length of droptail queue

	CommandLine cmd;
	cmd.AddValue("bw"      , "bandwidth of bottleneck link in Mbps", bw);
	cmd.AddValue("dl"      , "delay of bottleneck link in ms"      , dl);
	cmd.AddValue("duration", "simulation duration in seconds", duration);
	cmd.Parse(argc, argv);

	std::ostringstream s1,s2;
	s1 << bw << "Mbps";
	s2 << dl << "ms";
	std::string bottleNeckLinkBW = s1.str();
	std::string bottleNeckLinkDL = s2.str();

	Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (pktSize));
        Config::SetDefault ("ns3::OnOffApplication::DataRate"  , StringValue   (appDataRate));
	Config::SetDefault ("ns3::TcpL4Protocol::SocketType"   , StringValue   ("ns3::TcpNewReno"));

	NS_LOG_UNCOND("----------------------------------------------------------");
	NS_LOG_UNCOND("Simulation Environment:");
	NS_LOG_UNCOND("  - bottleneck link  ("<< bottleNeckLinkBW<<","<<bottleNeckLinkDL<<")");
	NS_LOG_UNCOND("  - OnOffApplication ("<< pktSize <<"bytes,"<<appDataRate<<")");
	NS_LOG_UNCOND("  - Socket Options   (TcpNewReno,"<< pktSize*2-42 <<"bytes"<<")");
	NS_LOG_UNCOND("  - simulation time  "<< SIM_START+duration << "s");
	NS_LOG_UNCOND("----------------------------------------------------------");

  	NodeContainer net;
  	net.Create (2);

  	PointToPointHelper p2p;
  	p2p.SetDeviceAttribute  ("DataRate", StringValue (bottleNeckLinkBW));
  	p2p.SetChannelAttribute ("Delay"   , StringValue (bottleNeckLinkDL));
	p2p.SetQueue ("ns3::DropTailQueue" , "MaxPackets", UintegerValue (bottleNeckLinkQS));

  	NetDeviceContainer dev;
  	dev= p2p.Install (net);

  	InternetStackHelper stack;
  	stack.InstallAll ();

  	Ipv4AddressHelper address;
  	address.SetBase (NET_ADD, NET_MASK, FIRST_NO);
  	Ipv4InterfaceContainer ifs = address.Assign (dev);
  	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	AddressValue remoteAddress (InetSocketAddress (ifs.GetAddress(1, 0), PORT));
	OnOffHelper ftp  ("ns3::TcpSocketFactory", Address());
	ftp.SetAttribute ("OnTime"               , StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        ftp.SetAttribute ("OffTime"              , StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
	ftp.SetAttribute ("Remote"               , remoteAddress);
	ApplicationContainer src = ftp.Install (net.Get(0));
        src.Start (Seconds (SIM_START));
        src.Stop  (Seconds (SIM_START+duration));

	// create a sink to recieve packets @ network's node (192.168.1.2).
  	Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), PORT));
  	PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkAddress);
        ApplicationContainer sink = sinkHelper.Install (net.Get(1));
	sink.Start (Seconds (SIM_START));
  	sink.Stop  (Seconds (SIM_START+duration ));

	// trace cwnd to file
	std::string fd = std::string(PROG_DIR) + "cwnd-" + bottleNeckLinkBW + "-" + bottleNeckLinkDL;
	AsciiTraceHelper ascii;
	Ptr<OutputStreamWrapper> st = ascii.CreateFileStream(fd);
	Simulator::Schedule (Seconds (0.1), 
		&MyEventWrapper, net.Get(0)->GetApplication(0), st);

	std::string fd2 = std::string(PROG_DIR) + "throughput-" + bottleNeckLinkBW + "-" + bottleNeckLinkDL;
        Ptr<OutputStreamWrapper> st2 = ascii.CreateFileStream(fd2);
        Simulator::Schedule (Seconds (TH_INTERVAL), 
                &TraceThroughput, net.Get(1)->GetApplication(0), st2);

	Simulator::Stop (Seconds(SIM_START+duration));
  	Simulator::Run ();

	NS_LOG_UNCOND("   Average Throughput : " << throughput << "kbps");
	NS_LOG_UNCOND("Average Utilities Link: " << throughput/(bw*1024.0));

  	Simulator::Destroy ();
  	return 0;
}
