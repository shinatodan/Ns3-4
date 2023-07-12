/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* exp04-FlowMonitoring.cc: simulate TCP behaviours with flow monitor
 * F.Qian, Nov. 2012
 *
 *       Source         n2                               n5   Sink
 *    (10Mbps,1ms)        \                             / (10Mbps,1ms)
 *                         \ .1.0/24          .1.0/24  /
 *                          \     bottleneck link     /
 * net1(172.16.0.0/16) n2 --n0 --net3(0.8Mbps,5ms)-- n1 -- n6 net2(172.17.0.0/16)
 *                .2.0/24   /     172.18.1.0/24       \    .2.0/24
 *                         / .3.0/24          .3.0/24  \ 
 *                        /                             \
 *      Left Leaf       n3                               n7     Right Leaf
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/tcp-header.h"

#include "ns3/point-to-point-layout-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"

#define NET_MASK        "255.255.255.0"
#define NET_ADDR1       "172.16.1.0"	// left leaf network
#define NET_ADDR2       "172.18.1.0"	// right leaf network
#define NET_ADDR3       "172.17.1.0"	// bottleneck link
#define FIRST_NO        "0.0.1.1"

#define SIM_START       00.0
#define SIM_STOP        40.0
#define DATA_MBYTES     500
#define PORT            50000

#define PROG_DIR	"local/exp04/data/"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("exp04-SimTcpFlowMonitor");

int 
main (int argc, char *argv[])
{
	std::string animFile = "flow-monitoring.xml" ;  // Name of file for animation output
        CommandLine cmd;
	cmd.Parse(argc, argv);
	
	uint32_t    nLeftLeaf  = 3;
  	uint32_t    nRightLeaf = 3;

	Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (1024));
        Config::SetDefault ("ns3::OnOffApplication::DataRate"  , StringValue ("500kb/s"));
	Config::SetDefault ("ns3::DropTailQueue::MaxPackets"   , UintegerValue (100));

	// set bottkeneck link
	PointToPointHelper p2pRouter;
	p2pRouter.SetDeviceAttribute  ("DataRate", StringValue ("1Mbps"));
  	p2pRouter.SetChannelAttribute ("Delay", StringValue ("5ms"));
	p2pRouter.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (10));

	PointToPointHelper p2pLeaf;
	p2pLeaf.SetDeviceAttribute    ("DataRate", StringValue ("10Mbps"));
  	p2pLeaf.SetChannelAttribute   ("Delay", StringValue ("1ms"));
	p2pLeaf.SetQueue ("ns3::DropTailQueue");

	// make dumbble network topology
  	PointToPointDumbbellHelper dumbbell_net (nLeftLeaf, p2pLeaf, nRightLeaf, p2pLeaf, p2pRouter);

  	// Install Stack
  	InternetStackHelper stack;
  	dumbbell_net.InstallStack (stack);

  	// Assign IP Addresses
  	dumbbell_net.AssignIpv4Addresses (
		Ipv4AddressHelper (NET_ADDR1, NET_MASK),
		Ipv4AddressHelper (NET_ADDR2, NET_MASK),
		Ipv4AddressHelper (NET_ADDR3, NET_MASK));

	// Install on/off app on all right side nodes
  	OnOffHelper tcp ("ns3::TcpSocketFactory", Address ());
  	tcp.SetAttribute ("OnTime" , StringValue ("ns3::UniformRandomVariable[Min=3.0|Max=5.00]"));
  	tcp.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.1|Max=0.50]"));
	tcp.SetAttribute ("DataRate"  , StringValue ("450Kbps"));
        tcp.SetAttribute ("PacketSize", UintegerValue (1024));

  	ApplicationContainer source;

	for (uint32_t i = 0; i < dumbbell_net.LeftCount (); i++) {
      		// Create an on/off app sending packets to the same leaf right side
      		AddressValue remoteAddress (InetSocketAddress (dumbbell_net.GetRightIpv4Address (i), PORT));
      		tcp.SetAttribute ("Remote", remoteAddress);
      		source.Add (tcp.Install (dumbbell_net.GetLeft (i)));
    	}
	source.Start (Seconds (SIM_START));
  	source.Stop  (Seconds (SIM_STOP));

	ApplicationContainer sink;
	for (uint32_t i = 0; i < dumbbell_net.RightCount (); i++) {
		Address sinkAddress (InetSocketAddress (dumbbell_net.GetRightIpv4Address (i), PORT));
       	 	PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkAddress);
      		sinkHelper.SetAttribute ("Local", AddressValue (sinkAddress));
		sink.Add (sinkHelper.Install (dumbbell_net.GetRight (i)));
	}
	sink.Start (Seconds (SIM_START));
	sink.Stop  (Seconds (SIM_STOP ));

	// Set the bounding box for animation
  	dumbbell_net.BoundingBox (1, 1, 100, 100);

	// Create the animation object and configure for specified output
	//AnimationInterface anim (animFile);
	
	// Set up the acutal simulation
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	// set Flowmonitor
        FlowMonitorHelper flowmon;
	flowmon.SetMonitorAttribute("JitterBinWidth", ns3::DoubleValue(0.001));
	flowmon.SetMonitorAttribute("DelayBinWidth", ns3::DoubleValue(0.001));
	flowmon.SetMonitorAttribute("PacketSizeBinWidth", ns3::DoubleValue(20));
        Ptr<FlowMonitor> monitor = flowmon.InstallAll();

	Simulator::Stop (Seconds(SIM_STOP));
	Simulator::Run ();

	// the following lines must setup after Run()
        monitor->CheckForLostPackets ();
        Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
        std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

        std::cout << "--------------------------------------" << std::endl;
        for(std::map<FlowId,FlowMonitor::FlowStats>::const_iterator i=stats.begin(); i!=stats.end(); ++i) {
                Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

                if((t.sourceAddress=="172.16.1.1" && t.destinationAddress == "172.18.1.1")
                 ||(t.sourceAddress=="172.16.2.1" && t.destinationAddress == "172.18.2.1")
                 ||(t.sourceAddress=="172.16.3.1" && t.destinationAddress == "172.18.3.1")) {

                        std::cout << "Flow " << i->first  << " (" << t.sourceAddress 
                                << " -> " << t.destinationAddress << ")\n";

			// The following flow stats can be find in src/flow-monitor/model/flow-monitor.h

			// Contains the sum of all end-to-end delays for all received packets of the flow.
                        std::cout << "      Delay sum:   " << i->second.delaySum << "\n";

			// Contains the sum of all end-to-end delay jitter (delay variation) values for 
			// all received packets of the flow. 
                        std::cout << "     Jitter sum:   " << i->second.jitterSum << "\n";

			// Total number of transmitted bytes for the flow.
                        std::cout << "       Tx Bytes:   " << i->second.txBytes << "\n";

			// Total number of received bytes for the flow.
                        std::cout << "       Rx Bytes:   " << i->second.rxBytes << "\n";

			// Total number of transmitted packets for the flow.
                        std::cout << "     Tx Packets:   " << i->second.txPackets << "\n";

			// Total number of received packets for the flow.
                        std::cout << "     Rx Packets:   " << i->second.rxPackets << "\n";

			// Total number of packets that are assumed to be lost, i.e. those that were 
			// transmitted but have not been reportedly received or forwarded for a long 
			// time. 
                        std::cout << "   lost Packets:   " << i->second.lostPackets << "\n";

			// Contains the number of times a packet has been reportedly forwarded, 
			// summed for all received packets in the flow
			std::cout << "Times Forwarded:   " << i->second.timesForwarded << "\n\n";

                        std::cout << "  Throughput: " << i->second.rxBytes * 8.0 
                                / (i->second.timeLastRxPacket.GetSeconds() 
                                - i->second.timeFirstTxPacket.GetSeconds())/1024  << " Kbps\n";
                        std::cout << "--------------------------------------" << std::endl;
                }
        }

	// make trace file's name
	AsciiTraceHelper ascii;
	std::string fname = std::string(PROG_DIR) + "exp04-TCP-flowmon.xml";
	monitor->SerializeToXmlFile(fname, true, true);

	//std::cout << "Animation Trace file created:" << animFile.c_str ()<< std::endl;
  	Simulator::Destroy ();

  	return 0;
}

