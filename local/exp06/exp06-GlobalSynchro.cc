/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* exp06-GlobalSynchro.cc: simulate TCP's Global Synchronization behaviours
 * SimClass Version
 * F.Qian, Nov. 2012
 *
 *       Source         n2                               n5   Sink
 *    (10Mbps,2ms)        \                             / (10Mbps,2ms)
 *                         \ .1.0/24          .1.0/24  /
 *                          \     bottleneck link     /
 * net1(172.16.0.0/16) n2 --n0 -- net3(1Mbps,2ms) -- n1 -- n6 net2(172.17.0.0/16)
 *                .2.0/24   /     172.18.1.0/24       \    .2.0/24
 *                         / .3.0/24          .3.0/24  \ 
 *                        /                             \
 *      Left Leaf       n3                               n7     Right Leaf
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <unistd.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/tcp-header.h"

#include "ns3/point-to-point-layout-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/random-variable-stream.h"

#define NET_MASK        "255.255.255.0"
#define NET_ADDR1       "172.16.0.0"	// left leaf network
#define NET_ADDR2       "172.18.0.0"	// right leaf network
#define NET_ADDR3       "172.17.0.0"	// bottleneck link
#define FIRST_NO        "0.0.1.1"

#define SIM_START       0.10
#define SIM_STOP        150.0
#define DATA_MBYTES     500
#define PORT            50000
#define QUEUE_SIZE	25

#define CW_INTERVAL     0.001 // The time interval in seconds for measurement cwnd

#define	PROG_DIR	"local/exp06/data/"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("exp06-TcpSynchronization");

class SimClass {
    public :
	SimClass();

	void Configure(int, char **);
	void CreateNetTopology();
	void SetInternetStack ();
	void SetApplications  ();
	void StartFlowMonitor ();
	void RunFlowMonitor   ();
	void LocalTraceStuff  ();

    private :
	uint32_t  randStart; // Start each session at same time:0 or at randomly:1

	uint32_t  nLeftLeaf; // Number of left  side leaf nodes
	uint32_t nRightLeaf; // Number of right side leaf nodes

	double	 max_start_time;

	PointToPointHelper  *p2pRouter;
	PointToPointHelper  *p2pLeaf;

  	PointToPointDumbbellHelper *dumbbell_net;

  	ApplicationContainer *source;
	ApplicationContainer *sink;

        FlowMonitorHelper *flowmon;
        Ptr<FlowMonitor>   monitor;

	Ptr<UniformRandomVariable> rnd;

	static void CwndTracer (Ptr<OutputStreamWrapper>, uint32_t, uint32_t);
	static void MyEventHandller (Ptr<Application>, Ptr<OutputStreamWrapper>);
	static void MyEventWrapper  (Ptr<Application>, Ptr<OutputStreamWrapper>);
};

SimClass::SimClass() : nLeftLeaf(3), nRightLeaf(3), max_start_time(0)
{
	randStart = 0;
}

void
SimClass::Configure(int argc, char *argv[])
{
	CommandLine cmd;
	cmd.AddValue ("randStart", "Start each session at same time <0> or randomly <1>", randStart);
	cmd.Parse(argc, argv);
	
	if(randStart) {
                SeedManager::SetSeed(getpid());
                SeedManager::SetRun (2);
		rnd = CreateObject<UniformRandomVariable> ();
		rnd->SetAttribute ("Min", DoubleValue(0.1));
		rnd->SetAttribute ("Max", DoubleValue(5.5));
        }

	Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (1024));
	Config::SetDefault ("ns3::OnOffApplication::DataRate"  , StringValue ("10Mbps"));
	Config::SetDefault ("ns3::DropTailQueue::MaxPackets"   , UintegerValue (100));

	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  	// 42 = headers size
  	Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1024 - 42));
  	Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  	GlobalValue::Bind  ("ChecksumEnabled", BooleanValue (false));

	// set bottkeneck link
	p2pRouter = new PointToPointHelper;
	p2pRouter->SetDeviceAttribute  ("DataRate", StringValue ("1Mbps"));
  	p2pRouter->SetChannelAttribute ("Delay"   , StringValue ("2ms"));
	p2pRouter->SetQueue ("ns3::DropTailQueue" , "MaxPackets", UintegerValue (QUEUE_SIZE));

	// set left & right leaf link
	p2pLeaf = new PointToPointHelper;
	p2pLeaf->SetDeviceAttribute  ("DataRate", StringValue ("10Mbps"));
  	p2pLeaf->SetChannelAttribute ("Delay"   , StringValue ("2ms"));
	p2pLeaf->SetQueue ("ns3::DropTailQueue" , "MaxPackets", UintegerValue (1000));
}

void
SimClass::CreateNetTopology()
{
	// make dumbble network topology
  	dumbbell_net = new PointToPointDumbbellHelper (nLeftLeaf, *p2pLeaf, nRightLeaf, *p2pLeaf, *p2pRouter);

	// Set the bounding box for animation
  	dumbbell_net->BoundingBox (1, 1, 100, 100);
}

void
SimClass::SetInternetStack()
{
  	InternetStackHelper stack;
  	dumbbell_net->InstallStack (stack);

  	dumbbell_net->AssignIpv4Addresses (
		Ipv4AddressHelper (NET_ADDR1, NET_MASK),
		Ipv4AddressHelper (NET_ADDR2, NET_MASK),
		Ipv4AddressHelper (NET_ADDR3, NET_MASK));
}

void
SimClass::SetApplications()
{
	double start_time = SIM_START;

	// Install on/off app on all right side nodes
  	OnOffHelper  tcp ("ns3::TcpSocketFactory", Address ());
  	tcp.SetAttribute ("OnTime"    , StringValue   ("ns3::ConstantRandomVariable[Constant=1]"));
  	tcp.SetAttribute ("OffTime"   , StringValue   ("ns3::ConstantRandomVariable[Constant=0]"));
	tcp.SetAttribute ("DataRate"  , StringValue   ("1Mbps"));
	tcp.SetAttribute ("PacketSize", UintegerValue (1024));

  	source = new ApplicationContainer [nLeftLeaf];
	for (uint32_t i = 0; i < dumbbell_net->LeftCount (); i++) {
      		// Create an on/off app sending packets to the same leaf right side
      		AddressValue remoteAddress (InetSocketAddress (dumbbell_net->GetRightIpv4Address (i), PORT));
      		tcp.SetAttribute ("Remote", remoteAddress);
      		source[i].Add (tcp.Install (dumbbell_net->GetLeft (i)));

		if(randStart) {
			start_time = rnd->GetValue();
			max_start_time = (start_time > max_start_time)? start_time : max_start_time;
		}
		std::cout << "Flow " << i << " started at " << start_time <<std::endl;

		source[i].Start (Seconds (start_time));
  		source[i].Stop  (Seconds (SIM_STOP));
    	}

	sink = new ApplicationContainer [nRightLeaf];
	for (uint32_t i = 0; i < dumbbell_net->RightCount (); i++) {
		Address sinkAddress (InetSocketAddress (dumbbell_net->GetRightIpv4Address (i), PORT));
       	 	PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkAddress);
      		sinkHelper.SetAttribute ("Local", AddressValue (sinkAddress));
		sink[i].Add (sinkHelper.Install (dumbbell_net->GetRight (i)));

		sink[i].Start (Seconds (SIM_START));
		sink[i].Stop  (Seconds (SIM_STOP ));
	}

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

void
SimClass::StartFlowMonitor()
{
	// set Flowmonitor atributes and mount it to all nodes
	flowmon = new FlowMonitorHelper;
	flowmon->SetMonitorAttribute("PacketSizeBinWidth", ns3::DoubleValue(20));
	flowmon->SetMonitorAttribute("JitterBinWidth"    , ns3::DoubleValue(0.001));
	flowmon->SetMonitorAttribute("DelayBinWidth"     , ns3::DoubleValue(0.001));
	monitor = flowmon->InstallAll();
}

void
SimClass::RunFlowMonitor()
{
	double sum = 0.0, throughput;

        monitor->CheckForLostPackets ();
        Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon->GetClassifier ());
        std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

        std::cout << "--------------------------------------" << std::endl;
        for(std::map<FlowId,FlowMonitor::FlowStats>::const_iterator i=stats.begin(); i!=stats.end(); ++i) {
                Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

                if((t.sourceAddress=="172.16.1.1" && t.destinationAddress == "172.18.1.1")
                 ||(t.sourceAddress=="172.16.2.1" && t.destinationAddress == "172.18.2.1")
                 ||(t.sourceAddress=="172.16.3.1" && t.destinationAddress == "172.18.3.1")) {
                        std::cout << "Flow " << i->first  << " (" << t.sourceAddress 
                                << " -> " << t.destinationAddress << ")\n";
                        throughput = i->second.rxBytes * 8.0 
                                / (i->second.timeLastRxPacket.GetSeconds() 
                                - i->second.timeFirstRxPacket.GetSeconds())/1024;
                        std::cout << "  Throughput: " << throughput << "Kbps\n";
                        std::cout << "--------------------------------------" << std::endl;
			sum += throughput;
                }
        }
	
	std::cout << "       Total throughput: " << sum << "kbps\n";
	std::cout << "Bottleneck Link Utility: " << sum/1024 << std::endl;
}

void
SimClass::LocalTraceStuff()
{
	// make trace file's name
	AsciiTraceHelper ascii;
	char	fname[128];

        Ptr<OutputStreamWrapper> *st;
        st = new Ptr<OutputStreamWrapper> [nLeftLeaf];

	for(uint32_t i=0; i<dumbbell_net->LeftCount (); i++) {
		sprintf(fname,"%sexp06-n%d.cwnd",PROG_DIR, i);
        	st[i] = ascii.CreateFileStream(fname);
		Simulator::Schedule (Seconds (1.0+max_start_time), 
			 &MyEventWrapper, dumbbell_net->GetLeft(i)->GetApplication(0), st[i]);
	}
}

void 
SimClass::CwndTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldcwnd, uint32_t newcwnd)
{
        *stream->GetStream() << Simulator::Now ().GetSeconds () 
                << " \t " << newcwnd << std::endl;
}

void 
SimClass::MyEventHandller (Ptr<Application> app, Ptr<OutputStreamWrapper> st)
{
        Ptr<Socket> src_socket  = app->GetObject<OnOffApplication>()->GetSocket();
        src_socket->TraceConnectWithoutContext("CongestionWindow", 
                MakeBoundCallback(&CwndTracer, st));
}

void 
SimClass::MyEventWrapper (Ptr<Application> app, Ptr<OutputStreamWrapper> st)
{
        Simulator::Schedule (Seconds (CW_INTERVAL), &MyEventHandller, app, st);
}

int 
main (int argc, char *argv[])
{
	SimClass sim;

	sim.Configure(argc, argv);
	sim.CreateNetTopology();
	sim.SetInternetStack();
	sim.SetApplications();
	sim.LocalTraceStuff();

	sim.StartFlowMonitor();

	Simulator::Stop (Seconds(SIM_STOP));
	Simulator::Run ();

	sim.RunFlowMonitor();

  	Simulator::Destroy ();
  	return 0;
}

