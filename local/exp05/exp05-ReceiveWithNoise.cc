/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* exp05-ReceiveWithNoise: simulate TCP behaviours with a simple 4-nodes topology.
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
#include <unistd.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/tcp-header.h"

#define	NET_MASK  	"255.255.255.0"
#define	NET_ADD1  	"192.168.1.0"
#define	NET_ADD2  	"192.168.2.0"
#define	NET_ADD3  	"192.168.3.0"
#define	FIRST_NO  	"0.0.0.1"

#define	SIM_START 	00.10
#define	SIM_STOP  	20.10
#define DATA_MBYTES  	500
#define PORT		50000
#define QUEUE_SIZE	10

#define TH_INTERVAL     0.1 
#define CW_INTERVAL     0.5 

#define PROG_DIR	"local/exp05/data/"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("exp05-CheckQueueSize");

void CheckQueueSize (Ptr<Queue> queue, std::string mode, Ptr<OutputStreamWrapper> stream)
{
  	uint32_t qSize;
	if(mode.compare("Bytes") == 0)
  		qSize = StaticCast<DropTailQueue> (queue)->GetNBytes (); 
	else
		qSize = StaticCast<DropTailQueue> (queue)->GetNPackets (); 
  	Simulator::Schedule (Seconds (0.1), &CheckQueueSize, queue, mode, stream);

  	NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << " \t " << qSize);
  	*stream->GetStream() << Simulator::Now ().GetSeconds () << " \t " << qSize << std::endl;
}

void 
CwndTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldcwnd, uint32_t newcwnd)
{
        //fprintf(stdout,"%10.4f %6d %6d\n",Simulator::Now ().GetSeconds (),oldcwnd,newcwnd);
        *stream->GetStream() << Simulator::Now ().GetSeconds () << " \t " << newcwnd << std::endl;
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
        //     (newTotalBytes - oldTotalBytes)*8/0.1/1024);
        *stream->GetStream() << Simulator::Now ().GetSeconds () 
                << " \t " << (newTotalBytes - oldTotalBytes)*8.0/0.1/1024 << std::endl;
        oldTotalBytes = newTotalBytes;
        Simulator::Schedule (Seconds (TH_INTERVAL), &TraceThroughput, app, stream);
}

void 
MyEventHandller (Ptr<Application> app, Ptr<OutputStreamWrapper> stream)
{
        Ptr<Socket> src_socket  = app->GetObject<BulkSendApplication>()->GetSocket();
        src_socket->TraceConnectWithoutContext("CongestionWindow", 
                MakeBoundCallback(&CwndTracer, stream));
}

void
printUsage()
{
	std::cout 
	<< "--------------------------------------------------------------------------\n"
	<< "Usage:  --queueModel:Counte queue size in Bytes or Packets(default).\n"
	<< "\t--maxSize: Maximum queue size in packets(default is 10).\n"
	<< "\t--errorRate: Error Rate of ReceiveErrorModel (default is 0.0001).\n"
	<< "--------------------------------------------------------------------------\n";
}

int main (int argc, char *argv[])
{
	CommandLine 	cmd;
	std::string	queueMode = "Packets";
	std::string	queueType = "DropTailQueue";
	uint32_t	queueSize = QUEUE_SIZE;
	double		errorRate = 0.0001;

	if(argc < 2) printUsage();
	cmd.AddValue("queueMode", "Counte queue size in Bytes or Packets(default).", queueMode);
	cmd.AddValue("maxSize"  , "Maximum queue size in packets(default is 10)." , queueSize);
	cmd.AddValue("errorRate", "Error Rate of ReceiveErrorModel (default is 0.0001).", errorRate);
	cmd.Parse(argc, argv);
	Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (queueSize));

	SeedManager::SetSeed(getpid());
	SeedManager::SetRun (getpid());

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
  	p2p2.SetDeviceAttribute ("DataRate", StringValue ("800kbps"));
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

	AddressValue remoteAddress (InetSocketAddress (ifs3.GetAddress(1, 0), PORT));

	BulkSendHelper ftp ("ns3::TcpSocketFactory", Address());
	ftp.SetAttribute ("Remote", remoteAddress);
	ftp.SetAttribute ("MaxBytes", UintegerValue (int(DATA_MBYTES * 1024 * 1024)));

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

	// set error model
	Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  	em->SetAttribute ("ErrorRate", DoubleValue (errorRate));
  	em->SetAttribute ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0,max=1.0]"));
  	devices3.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

	AsciiTraceHelper ascii;

	// trace cwnds
        std::string fd1 = std::string(PROG_DIR) + "exp05.cwnd";
        Ptr<OutputStreamWrapper> stream1 = ascii.CreateFileStream(fd1);
        Simulator::Schedule (Seconds (CW_INTERVAL), 
		&MyEventHandller, net1_nodes.Get(0)->GetApplication(0), stream1);

	// trace throughputs
	std::string fd2 = std::string(PROG_DIR) + "exp05.throughput";
        Ptr<OutputStreamWrapper> stream2 = ascii.CreateFileStream(fd2);
        Simulator::ScheduleNow (&TraceThroughput, net3_nodes.Get(1)->GetApplication(0), stream2);

	// trace queue sizes
	std::string fd3 = std::string(PROG_DIR) + "exp05.qsize";
        Ptr<OutputStreamWrapper> stream3 = ascii.CreateFileStream(fd3);
	Ptr<PointToPointNetDevice> nd = StaticCast<PointToPointNetDevice> (devices3.Get (0));
	Ptr<Queue> queue = nd->GetQueue ();
	Simulator::ScheduleNow (&CheckQueueSize, queue, queueMode, stream3);

	Simulator::Stop (Seconds(SIM_STOP));
  	Simulator::Run ();
  	Simulator::Destroy ();
  	return 0;
}

