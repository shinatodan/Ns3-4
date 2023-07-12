/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* Red-DripTail.cc: simulate Red Queue's
 * SimClass Version
 * Author: F.Qian, Nov. 2012 <fei@kanto-gakuin.ac.jp>
 *
 *       Source         n2                               n5   Sink
 *    (10Mbps,2ms)        \                             / (10Mbps,2ms)
 *                         \ .1.0/24          .1.0/24  /
 *                          \     bottleneck link     /
 * net1(172.16.0.0/16) n2 --n0 -- net3(1Mbps,2ms) -- n1 -- n6 net2(172.18.0.0/16)
 *                .2.0/24   /     172.17.1.0/24       \    .2.0/24
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
#include "ns3/rr-queue.h"

#include "ns3/point-to-point-layout-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/netanim-module.h"
#include "customized-app.h"

#define NET_MASK        "255.255.255.0"
#define NET_ADDR1       "172.16.1.0"	// left leaf network
#define NET_ADDR2       "172.18.1.0"	// right leaf network
#define NET_ADDR3       "172.17.1.0"	// bottleneck link
#define FIRST_NO        "0.0.1.1"

#define SIM_START       0.10
#define SIM_STOP        10.0
#define DATA_MBYTES     500
#define PORT            50000
#define CW_INTERVAL     0.001     // The time interval in seconds for measurement cwnd
#define PROG_DIR	"local/rr-queue/"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RRQueueTest");

class SimClass {
    public :
	SimClass();
	void DoSim(int, char **);

    private :
	void Configure(int, char **);
	void CreateNetTopology();
	void SetInternetStack ();
	void SetApplications  ();
	void StartFlowMonitor ();
	void RunFlowMonitor   ();
	void LocalTraceStuff  ();
	void setCustomizedTcpApp(Ptr <Node>, Ptr <Node>);

	uint32_t    nLeftLeaf; // Number of left  side leaf nodes
	uint32_t   nRightLeaf; // Number of right side leaf nodes

	double	 max_start_time;

	PointToPointHelper  *p2pRouter;
	PointToPointHelper  *p2pLeaf;

  	PointToPointDumbbellHelper *dumbbell_net;

  	ApplicationContainer *source;
	ApplicationContainer *sink;

        Ptr<FlowMonitor>   monitor;
        FlowMonitorHelper *flowmon;

	Ptr<UniformRandomVariable> rnd;

	static void CwndTracer (Ptr<OutputStreamWrapper>, uint32_t, uint32_t);
	static void MyEventHandller (Ptr<Application>, Ptr<OutputStreamWrapper>);
	static void MyEventWrapper  (Ptr<Application>, Ptr<OutputStreamWrapper>);

	// for RED queue simulations
	std::string queueType; // Set Queue type to "DropTail" or "RED"
	uint32_t    randStart; // Start each session at same time:0 or at randomly:1
	uint32_t    modeBytes; // Set Queue mode to Packets:0 or Bytes:1

	static void CheckQueueSize (Ptr<Queue>, uint32_t, Ptr<OutputStreamWrapper>);

	std::string appDataRate;        // Set OnOff App DataRate
	std::string bottleNeckLinkBw;   // bandwidth of bottleneck link
	std::string bottleNeckLinkDelay;// propagation delay of bottleneck link
};

SimClass::SimClass() : nLeftLeaf(6), nRightLeaf(6), max_start_time(0)
{
	queueType  = "RR";	// Set default queue type to "RR"
	randStart  = 0;	    	// set default start time to constant
	modeBytes  = 0;         // Set Queue mode to Packets:0 or Bytes:1
	
	appDataRate = "10Mbps"; // Set OnOff App DataRate
	bottleNeckLinkBw    = "5Mbps";
	bottleNeckLinkDelay = "2ms";

	rnd = CreateObject<UniformRandomVariable> ();
	rnd->SetAttribute ("Min", DoubleValue(0.1));
	rnd->SetAttribute ("Max", DoubleValue(5.5));
}

void
SimClass::Configure(int argc, char *argv[])
{
	LogComponentEnable("Socket", LogLevel(LOG_LEVEL_DEBUG| 
              LOG_PREFIX_NODE|LOG_PREFIX_TIME|LOG_PREFIX_FUNC)); 
	LogComponentEnable("Ipv4L3Protocol", LogLevel(LOG_LEVEL_DEBUG| 
              LOG_PREFIX_NODE|LOG_PREFIX_TIME|LOG_PREFIX_FUNC)); 
	LogComponentEnable("UdpSocketImpl", LogLevel(LOG_LEVEL_DEBUG| 
              LOG_PREFIX_NODE|LOG_PREFIX_TIME|LOG_PREFIX_FUNC)); 
	LogComponentEnable("OnOffApplication", LogLevel(LOG_LEVEL_DEBUG| 
              LOG_PREFIX_NODE|LOG_PREFIX_TIME|LOG_PREFIX_FUNC)); 
	LogComponentEnable("NetDevice", LogLevel(LOG_LEVEL_DEBUG| 
              LOG_PREFIX_NODE|LOG_PREFIX_TIME|LOG_PREFIX_FUNC)); 

  	uint32_t    maxPackets  = 1000;      // Max Packets allowed in the queue
	uint32_t    pktSize     = 512;      // Set OnOff App Packet Size

	CommandLine cmd;
	cmd.AddValue ("modeBytes", "Set Queue mode to packets <0> or bytes <1>", modeBytes);
	cmd.AddValue ("queueType", "Set Queue type to DropTail or RR", queueType);
	cmd.AddValue ("randStart", "Start each session at same time <0> or at randomly <1>", randStart);
	cmd.AddValue ("bandwidth", "Set bandwidth of bottleneck link in Mbps", bottleNeckLinkBw);
	cmd.Parse(argc, argv);
	
	Packet::EnablePrinting ();
	Packet::EnableChecking ();

	if(randStart) {
		SeedManager::SetSeed(getpid());
		SeedManager::SetRun (2);
	}

	if (queueType == "RR") { // set ARED queue parameters
		Config::SetDefault ("ns3::RRQueue::MaxPackets"       , UintegerValue (maxPackets));
		Config::SetDefault ("ns3::RRQueue::NumFlows"         , UintegerValue (6));
		Config::SetDefault ("ns3::RRQueue::Flow1_Source"     , StringValue ("172.16.1.1"));
		Config::SetDefault ("ns3::RRQueue::Flow2_Source"     , StringValue ("172.16.2.1"));
		Config::SetDefault ("ns3::RRQueue::Flow3_Source"     , StringValue ("172.16.3.1"));
		Config::SetDefault ("ns3::RRQueue::Flow4_Source"     , StringValue ("172.16.4.1"));
		Config::SetDefault ("ns3::RRQueue::Flow5_Source"     , StringValue ("172.16.5.1"));
		Config::SetDefault ("ns3::RRQueue::Flow6_Source"     , StringValue ("172.16.6.1"));
		Config::SetDefault ("ns3::RRQueue::Flow1_Destination", StringValue ("172.18.1.1"));
		Config::SetDefault ("ns3::RRQueue::Flow2_Destination", StringValue ("172.18.2.1"));
		Config::SetDefault ("ns3::RRQueue::Flow3_Destination", StringValue ("172.18.3.1"));
		Config::SetDefault ("ns3::RRQueue::Flow4_Destination", StringValue ("172.18.4.1"));
		Config::SetDefault ("ns3::RRQueue::Flow5_Destination", StringValue ("172.18.5.1"));
		Config::SetDefault ("ns3::RRQueue::Flow6_Destination", StringValue ("172.18.6.1"));
		if (modeBytes) {
			Config::SetDefault ("ns3::RRQueue::Mode", 
				EnumValue (ns3::Queue::QUEUE_MODE_BYTES));
			Config::SetDefault ("ns3::RRQueue::MaxBytes", 
				UintegerValue (maxPackets * pktSize));
		}
	} else if (queueType == "DropTail") { // set DropTail queue parameters
		Config::SetDefault ("ns3::DropTailQueue::Mode", EnumValue (ns3::Queue::QUEUE_MODE_PACKETS));
		Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (maxPackets));
		if (modeBytes) {
			Config::SetDefault ("ns3::DropTailQueue::Mode", EnumValue (ns3::Queue::QUEUE_MODE_BYTES));
			Config::SetDefault ("ns3::DropTailQueue::MaxBytes", UintegerValue (maxPackets * pktSize));
		}
	} else {
		NS_ABORT_MSG ("Invalid queue type: Use --queueType=[RR][DropTail]");
		exit(1);
	}

	Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (pktSize));
	Config::SetDefault ("ns3::OnOffApplication::DataRate"  , StringValue (appDataRate));

	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  	Config::SetDefault ("ns3::TcpSocket::SegmentSize"   , UintegerValue (pktSize*2 - 42));// 42 = headers size
  	Config::SetDefault ("ns3::TcpSocket::DelAckCount"   , UintegerValue (1));
  	GlobalValue::Bind  ("ChecksumEnabled", BooleanValue (false));

	// set bottkeneck link
	p2pRouter = new PointToPointHelper;
	p2pRouter->SetDeviceAttribute  ("DataRate", StringValue (bottleNeckLinkBw));
  	p2pRouter->SetChannelAttribute ("Delay"   , StringValue (bottleNeckLinkDelay));
	if(queueType == "RR")
		p2pRouter->SetQueue ("ns3::RRQueue");
	else
		p2pRouter->SetQueue ("ns3::DropTailQueue");

	// set left & right leaf link
	p2pLeaf = new PointToPointHelper;
	p2pLeaf->SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  	p2pLeaf->SetChannelAttribute ("Delay"  , StringValue ("2ms"));
	p2pLeaf->SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (100));
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
	// Install Stack
  	InternetStackHelper stack;
  	dumbbell_net->InstallStack (stack);

	// Assign IP Addresses
  	dumbbell_net->AssignIpv4Addresses (
		Ipv4AddressHelper (NET_ADDR1, NET_MASK),
		Ipv4AddressHelper (NET_ADDR2, NET_MASK),
		Ipv4AddressHelper (NET_ADDR3, NET_MASK));

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
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
		// Create an on/off app sending packets to the left side
      		AddressValue remoteAddress (InetSocketAddress 
			(dumbbell_net->GetRightIpv4Address (i), PORT));
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
		Address sinkAddress (InetSocketAddress 
			(dumbbell_net->GetRightIpv4Address (i), PORT));
       	 	PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkAddress);
      		sinkHelper.SetAttribute ("Local", AddressValue (sinkAddress));
		sink[i].Add (sinkHelper.Install (dumbbell_net->GetRight (i)));

		sink[i].Start (Seconds (SIM_START));
		sink[i].Stop  (Seconds (SIM_STOP ));
	}
}

void
SimClass::setCustomizedTcpApp(Ptr <Node> n, Ptr <Node> m)
{
        // get node m's IP address as remote site address
        Ipv4InterfaceAddress madr = m->GetObject <Ipv4> ()->GetAddress(1, 0);
        Address remoteAddr (InetSocketAddress (madr.GetLocal(), PORT));

        Ipv4InterfaceAddress nadr = n->GetObject <Ipv4> ()->GetAddress(1, 0);
        Ipv4Address src = nadr.GetLocal();
        Ipv4Address dst = madr.GetLocal();

        Ptr<Socket> socket = Socket::CreateSocket (n, TcpSocketFactory::GetTypeId ());
        Ptr<CustomizedApp> app = CreateObject<CustomizedApp> ();
        app->Setup (socket, remoteAddr, 1040, DataRate ("1Mbps"), src, dst);
        n->AddApplication (app);
        app->SetStartTime (Seconds (SIM_START));
        app->SetStopTime  (Seconds (SIM_STOP ));

        // Create an optional packet sink to receive these packets
        PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory",
                Address (InetSocketAddress (Ipv4Address::GetAny (), PORT)));
        ApplicationContainer sink = sinkHelper.Install (m);
        sink.Start (Seconds (SIM_START));
        sink.Stop  (Seconds (SIM_STOP ));
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
	double sum = 0.0, rxTotal=0, throughput;
	double var[16]; // maximum number of flows is 16.
	uint16_t n=0;   // used to caculate the fairness index

        monitor->CheckForLostPackets ();
        Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon->GetClassifier ());
        std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

	for(int i=0;i<16;i++)var[i]=0.0;
        std::cout << "-------------------------------------------------------------" << std::endl;
        for(std::map<FlowId,FlowMonitor::FlowStats>::const_iterator i=stats.begin(); i!=stats.end(); ++i) {
                Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

                if((t.sourceAddress=="172.16.1.1" && t.destinationAddress == "172.18.1.1")
                 ||(t.sourceAddress=="172.16.2.1" && t.destinationAddress == "172.18.2.1")
                 ||(t.sourceAddress=="172.16.3.1" && t.destinationAddress == "172.18.3.1")
                 ||(t.sourceAddress=="172.16.4.1" && t.destinationAddress == "172.18.4.1")
                 ||(t.sourceAddress=="172.16.5.1" && t.destinationAddress == "172.18.5.1")
                 ||(t.sourceAddress=="172.16.6.1" && t.destinationAddress == "172.18.6.1")) {
                        std::cout << "Flow " << i->first  << ":(" << t.sourceAddress 
                                << " -> " << t.destinationAddress << ") ";
                        throughput = i->second.rxBytes * 8.0 
                                / (i->second.timeLastRxPacket.GetSeconds() 
                                - i->second.timeFirstRxPacket.GetSeconds())/1024;
                        std::cout << "Throughput: " << throughput << "Kbps\n";
			sum += throughput;
			rxTotal += i->second.rxBytes;

			var[n++] = throughput;
                }
        }

	double a = 0.0, b = 0.0;
	for(uint16_t i=0;i<n+1;i++) {
		a += var[i];
		b += (var[i])*(var[i]);
	}

        std::cout << "-------------------------------------------------------------" << std::endl;
	
	//double now = Simulator::Now ().GetSeconds ();
	std::cout << "             Queue Type: " << queueType << std::endl;
	std::cout << "       Total throughput: " << sum << " kbps\n";
	std::cout << "Bottleneck Link Utility: " << sum/(5*1024) << std::endl;
	std::cout << "         Fairness index: " << (a*a)/((n)*b) << std::endl;
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
		sprintf(fname,"%s/data/rr-queue-n%d.cwnd", PROG_DIR, i);
        	st[i] = ascii.CreateFileStream(fname);
		Simulator::Schedule (Seconds (1.0+max_start_time), 
			 &MyEventWrapper, dumbbell_net->GetLeft(i)->GetApplication(0), st[i]);
	}

	// trace queue sizes
        std::string fd = std::string(PROG_DIR) + "data/rr-queue.qsize";
        Ptr<OutputStreamWrapper> s = ascii.CreateFileStream(fd);

        // Get node pointer from hellper
	Ptr< Node > r0 = dumbbell_net->GetLeft();
        // Get device from the node with dynamic casting
	Ptr <PointToPointNetDevice> dev =  DynamicCast<PointToPointNetDevice> (r0->GetDevice (0));
	// Get queue from the device
        Ptr<Queue> queue = dev->GetQueue ();
        Simulator::Schedule (Seconds (1.0+max_start_time), &CheckQueueSize, queue, modeBytes, s);
}

void 
SimClass::CheckQueueSize (Ptr<Queue> queue, uint32_t mode, Ptr<OutputStreamWrapper> stream)
{
        uint32_t qSize; // current queue size

        if(!mode)
                qSize = StaticCast<DropTailQueue> (queue)->GetNPackets (); 
        else
                qSize = StaticCast<DropTailQueue> (queue)->GetNBytes (); 

        Simulator::Schedule (Seconds (0.1), &CheckQueueSize, queue, mode, stream);

        //NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << " \t " << qSize);
        *stream->GetStream() << Simulator::Now ().GetSeconds () << " \t " << qSize << std::endl;
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

void
SimClass::DoSim (int argc, char *argv[])
{
	Configure(argc, argv);
	CreateNetTopology();
	SetInternetStack();
	//SetApplications();

	setCustomizedTcpApp(dumbbell_net->GetLeft(0), dumbbell_net->GetRight(0));
	setCustomizedTcpApp(dumbbell_net->GetLeft(1), dumbbell_net->GetRight(1));
	setCustomizedTcpApp(dumbbell_net->GetLeft(2), dumbbell_net->GetRight(2));
	setCustomizedTcpApp(dumbbell_net->GetLeft(3), dumbbell_net->GetRight(3));
	setCustomizedTcpApp(dumbbell_net->GetLeft(4), dumbbell_net->GetRight(4));
	setCustomizedTcpApp(dumbbell_net->GetLeft(5), dumbbell_net->GetRight(5));

	//LocalTraceStuff();
	StartFlowMonitor();

        std::string af = std::string(PROG_DIR) + "rr-queue.xml";
	AnimationInterface anim (af);

	Simulator::Stop (Seconds(SIM_STOP));
	Simulator::Run ();
	RunFlowMonitor();
  	Simulator::Destroy ();
}

int 
main (int argc, char *argv[])
{
	SimClass sim;
	sim.DoSim(argc, argv);
  	return 0;
}

