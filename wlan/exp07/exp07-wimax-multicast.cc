/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// Default network topology includes:
// - A base station (BS)
// - Some number of SSs specified by the variable nbSS (defaults to 10)
// - A multicast router (ASNGW)
// - A multicast streamer

// Two Lans are setup: The first one between the multicast streamer and the
// ASNGW, the second one between the ASNGW (router) and the base station

//      +-----+    +-----+    +-----+   +-----+    +-----+
//      | SS0 |    | SS1 |    | SS2 |   | SS3 |    | SS4 |
//      +-----+    +-----+    +-----+   +-----+    +-----+
//     10.1.0.1   10.1.0.2   10.1.0.3  10.1.0.4   10.1.0.5
//      --------  --------    -------   -------   --------
//        ((*))    ((*))       ((*))     ((*))       ((*))
//
//                              LAN2 (11.1.1.0)
//                          ===============
//        10.1.0.11          |          |
//              +------------+        ASNGW           multicast Streamer
//       ((*))==|Base Station|          |  (12.1.1.0)   |
//              +------------+         ==================
//                                            LAN1
//
//        ((*))    ((*))       ((*))        ((*))    ((*))
//       -------   --------   --------    -------   --------
//      10.1.0.6   10.1.0.7   10.1.0.8    10.1.0.9   10.1.0.10
//       +-----+    +-----+    +-----+    +-----+    +-----+
//       | SS5 |    | SS6 |    | SS7 |    | SS8 |    | SS9 |
//       +-----+    +-----+    +-----+    +-----+    +-----+

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wimax-module.h"
#include "ns3/csma-module.h"
#include <iostream>
#include "ns3/global-route-manager.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/vector.h"

#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"

#define PROG_DIR "wlan/exp07/data/"

NS_LOG_COMPONENT_DEFINE ("exp07-SimpleWimax");

#define MAXSS 	1000
#define MAXDIST 10 // km

using namespace ns3;

void show_pkt_count(UdpServerHelper udpServer[], int nbSS)
{
        for (int i = 0; i < nbSS; i++) {
                std::cout << "SS" << i << " Received: " 
		<< udpServer[i].GetServer()->GetReceived() 
                << "/" << udpServer[i].GetServer()->GetLost() 
		<< std::endl;
        }
}

int main (int argc, char *argv[])
{
	// default values
	int nbSS = 10, duration = 20, schedType = 0, clientStart = 6.0;
	bool verbose = false;
	WimaxHelper::SchedulerType scheduler = WimaxHelper::SCHED_TYPE_SIMPLE;

	CommandLine cmd;
	cmd.AddValue ("nbSS", "number of subscriber station to create", nbSS);
	cmd.AddValue ("scheduler", "type of scheduler to use with the netdevices", schedType);
	cmd.AddValue ("duration", "duration of the simulation in seconds", duration);
	cmd.AddValue ("verbose", "turn on all WimaxNetDevice log components", verbose);
	cmd.Parse (argc, argv);

	std::cout << "Simulation Parameters ------------------------------------\n" 
		  << "Subscriber Stations: " << nbSS
		  << ", Duration: "          << duration 
		  << ", Scheduler type: "    << schedType
		  << std::endl;
	std::cout << "----------------------------------------------------------\n";
		
	//LogComponentEnable ("UdpTraceClient", LOG_LEVEL_INFO);
	//LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);

	switch (schedType) {
	    case 0:
		scheduler = WimaxHelper::SCHED_TYPE_SIMPLE;
		break;
	    case 1:
		scheduler = WimaxHelper::SCHED_TYPE_MBQOS;
		break;
	    case 2:
		scheduler = WimaxHelper::SCHED_TYPE_RTPS;
		break;
	    default:
		scheduler = WimaxHelper::SCHED_TYPE_SIMPLE;
	}

	NodeContainer ssNodes;
	ssNodes.Create (nbSS);

	NodeContainer bsNodes;
	bsNodes.Create (1);

	NodeContainer Streamer_Node;
	Streamer_Node.Create (1);

	NodeContainer ASNGW_Node;
	ASNGW_Node.Create (1);

	Ptr<SimpleOfdmWimaxChannel> channel;
	channel = CreateObject<SimpleOfdmWimaxChannel> ();
	channel->SetPropagationModel (SimpleOfdmWimaxChannel::COST231_PROPAGATION);

	WimaxHelper wimax;

	NetDeviceContainer ssDevs;
	ssDevs = wimax.Install (ssNodes,
		WimaxHelper::DEVICE_TYPE_SUBSCRIBER_STATION,
		WimaxHelper::SIMPLE_PHY_TYPE_OFDM,
		channel,
		scheduler);
	Ptr<WimaxNetDevice> dev = wimax.Install (bsNodes.Get (0),
		WimaxHelper::DEVICE_TYPE_BASE_STATION,
		WimaxHelper::SIMPLE_PHY_TYPE_OFDM,
		channel,
		scheduler);

	Ptr<ConstantPositionMobilityModel> BSPosition;
	BSPosition = CreateObject<ConstantPositionMobilityModel> ();

	BSPosition->SetPosition (Vector (1000, 0, 0));
	bsNodes.Get (0)->AggregateObject (BSPosition);

	NetDeviceContainer bsDevs, bsDevsOne;
	bsDevs.Add (dev);

	if (verbose) 
		wimax.EnableLogComponents ();  // Turn on all wimax logging

	Ptr<SubscriberStationNetDevice> ss[MAXSS];
	Ptr<RandomWaypointMobilityModel> SSPosition[MAXSS];
	Ptr<RandomRectanglePositionAllocator> SSPosAllocator[MAXSS];
	for (int i = 0; i < nbSS; i++) {
		SSPosition[i] = CreateObject<RandomWaypointMobilityModel> ();
		SSPosAllocator[i] = CreateObject<RandomRectanglePositionAllocator> ();
		Ptr<UniformRandomVariable> xVar = CreateObject<UniformRandomVariable> ();
		xVar->SetAttribute ("Min", DoubleValue ((i / 40.0) * 2000));
		xVar->SetAttribute ("Max", DoubleValue ((i / 40.0 + 1) * 2000));
		SSPosAllocator[i]->SetX (xVar);
		Ptr<UniformRandomVariable> yVar = CreateObject<UniformRandomVariable> ();
		yVar->SetAttribute ("Min", DoubleValue ((i / 40.0) * 2000));
		yVar->SetAttribute ("Max", DoubleValue ((i / 40.0 + 1) * 2000));
		SSPosAllocator[i]->SetY (yVar);
		SSPosition[i]->SetAttribute ("PositionAllocator", PointerValue (SSPosAllocator[i]));
		SSPosition[i]->SetAttribute ("Speed", StringValue ("ns3::UniformRandomVariable[Min=10.3|Max=40.7]"));
		SSPosition[i]->SetAttribute ("Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.01]"));

		ss[i] = ssDevs.Get (i)->GetObject<SubscriberStationNetDevice> ();
		ss[i]->SetModulationType (WimaxPhy::MODULATION_TYPE_QAM16_12);
		ssNodes.Get (i)->AggregateObject (SSPosition[i]);
	}

	Ptr<BaseStationNetDevice> bs;
	bs = bsDevs.Get (0)->GetObject<BaseStationNetDevice> ();

	CsmaHelper csmaASN_BS;
	CsmaHelper csmaStreamer_ASN;

	// First LAN BS and ASN
	NodeContainer LAN_ASN_BS;
	LAN_ASN_BS.Add (bsNodes.Get (0));
	LAN_ASN_BS.Add (ASNGW_Node.Get (0));

	csmaASN_BS.SetChannelAttribute ("DataRate", DataRateValue (DataRate (10000000)));
	csmaASN_BS.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
	csmaASN_BS.SetDeviceAttribute ("Mtu", UintegerValue (1500));
	NetDeviceContainer LAN_ASN_BS_Devs = csmaASN_BS.Install (LAN_ASN_BS);

	NetDeviceContainer BS_CSMADevs;
	BS_CSMADevs.Add (LAN_ASN_BS_Devs.Get (0));

	NetDeviceContainer ASN_Devs1;
	ASN_Devs1.Add (LAN_ASN_BS_Devs.Get (1));

	// Second LAN ASN-GW and Streamer
	NodeContainer LAN_ASN_STREAMER;
	LAN_ASN_STREAMER.Add (ASNGW_Node.Get (0));
	LAN_ASN_STREAMER.Add (Streamer_Node.Get (0));

	csmaStreamer_ASN.SetChannelAttribute ("DataRate", DataRateValue (DataRate (10000000)));
	csmaStreamer_ASN.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
	csmaStreamer_ASN.SetDeviceAttribute ("Mtu", UintegerValue (1500));

	NetDeviceContainer LAN_ASN_STREAMER_Devs = csmaStreamer_ASN.Install (LAN_ASN_STREAMER);

	NetDeviceContainer STREAMER_Devs;
	STREAMER_Devs.Add (LAN_ASN_STREAMER_Devs.Get (1));

	NetDeviceContainer ASN_Devs2;
	ASN_Devs2.Add (LAN_ASN_STREAMER_Devs.Get (0));

	MobilityHelper mobility;
	mobility.Install (bsNodes);
	mobility.Install (ssNodes);

	InternetStackHelper stack;
	stack.Install (bsNodes);
	stack.Install (ssNodes);
	stack.Install (Streamer_Node);
	stack.Install (ASNGW_Node);

	Ipv4AddressHelper address;

	address.SetBase ("10.1.0.0", "255.255.255.0");
	bsDevsOne.Add (bs);

	Ipv4InterfaceContainer BSinterfaces;
	BSinterfaces = address.Assign (bsDevsOne);

	Ipv4InterfaceContainer SSinterfaces;
	SSinterfaces = address.Assign (ssDevs);

	address.SetBase ("11.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer BSCSMAInterfaces = address.Assign (BS_CSMADevs);
	Ipv4InterfaceContainer ASNCSMAInterfaces1 = address.Assign (ASN_Devs1);

	address.SetBase ("12.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer ASNCSMAInterfaces2 = address.Assign (ASN_Devs2);
	Ipv4InterfaceContainer StreamerCSMAInterfaces = address.Assign (STREAMER_Devs);

	Ipv4Address multicastSource ("12.1.1.2");
	Ipv4Address multicastGroup ("224.30.10.1");

	Ipv4StaticRoutingHelper multicast;
	// 1) Configure a (static) multicast route on ASNGW (multicastRouter)
	Ptr<Node> multicastRouter = ASNGW_Node.Get (0); // The node in question
	Ptr<NetDevice> inputIf = ASN_Devs2.Get (0); // The input NetDevice

	multicast.AddMulticastRoute (multicastRouter, multicastSource, multicastGroup, inputIf, ASN_Devs1);

	// 2) Set up a default multicast route on the sender n0
	Ptr<Node> sender = Streamer_Node.Get (0);
	Ptr<NetDevice> senderIf = STREAMER_Devs.Get (0);
	multicast.SetDefaultMulticastRoute (sender, senderIf);

	// 1) Configure a (static) multicast route on ASNGW (multicastRouter)
	multicastRouter = bsNodes.Get (0); // The node in question
	inputIf = BS_CSMADevs.Get (0); // The input NetDevice

	multicast.AddMulticastRoute (multicastRouter, multicastSource, multicastGroup, inputIf, bsDevsOne);

	uint16_t multicast_port = 100;

	UdpServerHelper udpServer[MAXSS];
	ApplicationContainer serverApps[MAXSS];
	for (int i = 0; i < nbSS; i++) {
		udpServer[i] = UdpServerHelper (multicast_port);
		serverApps[i] = udpServer[i].Install (ssNodes.Get (i));
		serverApps[i].Start (Seconds (clientStart));
		serverApps[i].Stop (Seconds (duration));
	}

	UdpTraceClientHelper udpClient;
	//udpClient = UdpTraceClientHelper (multicastGroup, multicast_port, "");
	udpClient = UdpTraceClientHelper (multicastGroup, multicast_port, "wlan/exp07/Verbose_Simpsons_10_14_18.dat");

	ApplicationContainer clientApps;
	clientApps = udpClient.Install (Streamer_Node.Get (0));
	clientApps.Start (Seconds (clientStart));
	clientApps.Stop (Seconds (duration));

	std::string trbs= std::string(PROG_DIR) + "wimax-bs";
	wimax.EnableAscii (trbs, bsDevs);
	wimax.EnablePcap  (trbs, bsNodes.Get (0)->GetId (), bs->GetIfIndex ());

	IpcsClassifierRecord MulticastClassifier (
		Ipv4Address ("0.0.0.0"),
		Ipv4Mask ("0.0.0.0"),
		multicastGroup,
		Ipv4Mask ("255.255.255.255"),
		0,
		65000,
		multicast_port,
		multicast_port,
		17,
		1);

	ServiceFlow MulticastServiceFlow = wimax.CreateServiceFlow (
		ServiceFlow::SF_DIRECTION_DOWN,
		//ServiceFlow::SF_TYPE_UGS,
		ServiceFlow::SF_TYPE_RTPS,
		MulticastClassifier);

	bs->GetServiceFlowManager ()->AddMulticastServiceFlow (MulticastServiceFlow, WimaxPhy::MODULATION_TYPE_QPSK_12);

	// show received pkts on each seaver
	Simulator::Schedule (Seconds (duration), &show_pkt_count, udpServer, nbSS);

	/*
	AnimationInterface::SetConstantPosition(Streamer_Node.Get (0), 100.0, 100.0);
        AnimationInterface::SetNodeDescription (Streamer_Node.Get (0), "Streamer");
        AnimationInterface::SetConstantPosition(ASNGW_Node.Get (0), 100.0, 200.0);
        AnimationInterface::SetNodeDescription (ASNGW_Node.Get (0), "ASN-Gateway");
        AnimationInterface::SetConstantPosition(bsNodes.Get (0), 200.0, 300.0);
        AnimationInterface::SetNodeDescription (bsNodes.Get (0), "BS");
        std::string xf = std::string(PROG_DIR) + "wimax-multicast.xml";
        AnimationInterface anim (xf);
	*/

	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();

	NS_LOG_INFO ("Starting simulation.....");
	Simulator::Stop (Seconds (duration + 0.1));
	Simulator::Run ();

	// Print per flow statistics
	NS_LOG_UNCOND ("flow status ----------------------------------------------");
	monitor->CheckForLostPackets (); 
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats (); 
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i=stats.begin (); i!=stats.end (); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
		std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
		std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
		std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
		std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (duration - clientStart) / 1024 / 1024  << " Mbps\n";
	}

	AsciiTraceHelper ascii;
	std::string mf = std::string(PROG_DIR) + "wimax-multicast-flowmon.xml";
	Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(mf);
	monitor->SerializeToXmlFile (mf, true, true);

	for (int i = 0; i < nbSS; i++) {
		ss[i] = 0;
		SSPosition[i] = 0;
		SSPosAllocator[i] = 0;
	}
	bs = 0;

	Simulator::Destroy ();
	NS_LOG_INFO ("Done.");

	return 0;
}
