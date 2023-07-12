/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
//
// This program configures a grid (default 5x5) of nodes on an 
// 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 512 
// (application) bytes to node 1.
//
// The default layout is like this, on a 2-D grid.
//
// n00  n01  n02  n03  n04
// n05  n06  n07  n08  n09
// n10  n11  n12  n13  n14
// n15  n16  n17  n18  n19
// n20  n21  n22  n23  n24
//
// the layout is affected by the parameters given to GridPositionAllocator;
// by default, GridWidth is 5 and numNodes is 25..
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc-grid --help"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the ns-3 documentation.
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when distance increases beyond
// the default of 500m.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc --distance=500"
// ./waf --run "wifi-simple-adhoc --distance=1000"
// ./waf --run "wifi-simple-adhoc --distance=1500"
// 
// The source node and sink node can be changed like this:
// 
// ./waf --run "wifi-simple-adhoc --sourceNode=20 --sinkNode=10"
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
// 
// ./waf --run "wifi-simple-adhoc-grid --verbose=1"
//
// By default, trace file writing is off-- to enable it, try:
// ./waf --run "wifi-simple-adhoc-grid --tracing=1"
//

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/energy-module.h"

#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/aodv-module.h"
#include "ns3/dsdv-helper.h"
#include "ns3/dsr-module.h"

#include "ns3/netanim-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>

#define PROG_DIR	"wlan/exp05/data/"

NS_LOG_COMPONENT_DEFINE ("exp05-WifiSimpleAdhocGrid");

using namespace ns3;

//#define DEBUG 1

class NetSim {
    public :
	// initialize simulation parameters
	NetSim();
	~NetSim();

	// command line options processing
	void Configure(int, char **);

	// run simulation
	void RunSimulation ();

    private :
	// configure default attributes
	void ConfigureSetDefault ();

	// make network topology
	void MakeNetworkTopology();

	// configure data link layer entities
	void ConfigureDataLinkLayer();

	// configure network layer entities
	void ConfigureNetworkLayer ();

	// configure trasport layer entities and setup applications
	void ConfigureL4withDUPApp(uint32_t, uint32_t);

	// generate application traffics send it to socket
	void GenerateTraffic (Ptr<Socket>, Time);

	// receive packets from socket
	void ReceivePacket (Ptr<Socket>);

	// set node's peer address as (IP address, port number)
	InetSocketAddress setSocketAddress(Ptr<Node>, uint32_t);

	// attach energy mode to devices with (initial energy, Tx current, Rx current)
	EnergySourceContainer AttachEnergyModelToDevices(double, double, double);
	// trace remained energy for node n[traceNum]
	void TraceRemainingEnergy(EnergySourceContainer);
	// trace total consumed energy for node n[traceNum]
	void TraceTotalEnergy(EnergySourceContainer);
	// get total consumed energy for node n[traceNum] with (oldValue, new Value)
	void TotalEnergy (double, double);
	// get remained energy for node n[traceNum] with (oldValue, new Value)
	void GetRemainingEnergy (double, double);
	// notify when node n[traceNum]'s energy was drained
	void NotifyDrained();

	// record ASCII/PCAP traces to files
	void traceSimEvents();
	// trace routing tables to file
	void traceRoutingTables();

	// set up flow monitor
	void SetFlowMonitor ();
	// get statistics for flow between node1 and node2
	void RunFlowMonitor (uint32_t, uint32_t);

    private :
	uint32_t   numNodes;  // by default, 5x5

	double     distance;   // distance between two nodes in m
	uint32_t numPackets;   // number of nodes in the grid
	uint32_t packetSize;   // size of application packet sent in bytes
	uint32_t sourceNode;   // source node id
	uint32_t   sinkNode;   // destination node id
	double     interval;   // time interval between send packets
	bool        tracing;   // enable Routing Tables traces
	std::string phyMode;   // Wifi Physical layer mode
	std::string traceEng;  // enable remained energy consumption traces
	uint32_t    traceNum;  // trace energy consumption for node
	std::string rtproto;   // set default routing protocol(Aodv,Dsdv,Olsr,Dsr)

	uint32_t totalReceived;// total received packets
	uint32_t totalSent;    // total sent packets

    private :
	// node container for save all nodes
	NodeContainer allNodes;
	// individual nodes, pressented as n[i], for i=0,1,...,numNodes-1
	Ptr<Node> *n;

	// network device container for all nodes
	NetDeviceContainer devices;
	// helper used to assign positions and mobility models to nodes
	MobilityHelper mobility;

	// helper for create and manage tans model PHY objects
	YansWifiPhyHelper wifiPhy;
	// helper for store IPv4 address information on an interface
	Ipv4InterfaceContainer ifs;

	OlsrHelper olsr; // OLSR's helper
	AodvHelper aodv; // Aodv's helper
	DsdvHelper dsdv; // Dsdv's helper

	FlowMonitorHelper  *flowmon;
        Ptr<FlowMonitor>   monitor;

	double _remainingEnergy;// save current remained energy for the node
	bool   _energyDrained;  // set it to true if energy is drained.
};

NetSim::NetSim() : totalReceived(0), totalSent(0)
{
	numNodes   = 25; // 5x5 Grid

	distance   = 500; 
	packetSize = 512;
	numPackets = 1;
	sourceNode = 0;
	sinkNode   = 24;
	interval   = 0.05; 
	tracing    = true;      // turn on routing table traces
	traceNum   = 12;        // trace energy consumption for center node
	phyMode    = std::string("DsssRate1Mbps");
	traceEng   = std::string("remained"); // trace remained energy consumption
	rtproto    = std::string("Olsr");     // set default routing protocol
}

NetSim::~NetSim()
{
        for (uint32_t i = 0; i < numNodes; ++i) n[i] = 0;
        n=0; 
}

void
NetSim::SetFlowMonitor()
{
	NS_LOG_FUNCTION(this);

        // set Flowmonitor atributes and mount it to all nodes
        flowmon = new FlowMonitorHelper;
        flowmon->SetMonitorAttribute("PacketSizeBinWidth", ns3::DoubleValue(20));
        flowmon->SetMonitorAttribute("JitterBinWidth"    , ns3::DoubleValue(0.001));
        flowmon->SetMonitorAttribute("DelayBinWidth"     , ns3::DoubleValue(0.001));
        monitor = flowmon->InstallAll();
}

void // Print per flow statistics
NetSim::RunFlowMonitor(uint32_t src, uint32_t dst)
{
	NS_LOG_FUNCTION(this);

        double sumThroughput = 0, th;
        uint64_t sumTx = 0, tx, sumRx = 0, rx, sumLost = 0, lost;

	// get src and dst's ip addresses
	Ipv4InterfaceAddress src_adr = n[src]->GetObject <Ipv4> ()->GetAddress(1, 0);
	Ipv4Address src_ip = src_adr.GetLocal();
	Ipv4InterfaceAddress dst_adr = n[dst]->GetObject <Ipv4> ()->GetAddress(1, 0);
	Ipv4Address dst_ip = dst_adr.GetLocal();

	NS_LOG_UNCOND("Flow Monitor ----------------------------------");
	NS_LOG_UNCOND(src_ip << "->" << dst_ip);

        monitor->CheckForLostPackets ();
        Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon->GetClassifier ());
        std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
        for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i=stats.begin (); 
                i != stats.end (); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

                if((t.sourceAddress == src_ip && t.destinationAddress == dst_ip)) {
			// first 2 FlowIds are for ECHO apps, we don't want to display them
			if (i->first > 2) {
                            th = i->second.rxBytes *8.0/10.0/1024; sumThroughput += th;
                            tx = i->second.txBytes; sumTx += tx;
                            rx = i->second.rxBytes; sumRx += rx;
                            lost = i->second.lostPackets; sumLost += lost;

                            NS_LOG_UNCOND("-----------------------------------------------");
                            Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
                            NS_LOG_UNCOND("Flow " << i->first - 2 << " (" << t.sourceAddress 
                                  << " -> " << t.destinationAddress << ")");
                            NS_LOG_UNCOND("             Tx/Rx(B)/lost(p): " << tx << "/" << rx << "/" << lost);
                            NS_LOG_UNCOND("                   Throughput: " << th << " Kbps");
                            NS_LOG_UNCOND("Packet Delivery Fraction(PDF): " 
                                  << (double)i->second.rxBytes/(double)i->second.txBytes);
                	}
		}
        }
        NS_LOG_UNCOND("-----------------------------------------------");
        NS_LOG_UNCOND("    Total Sent Bytes: " << sumTx << " bytes");
        NS_LOG_UNCOND("Total Received Bytes: " << sumRx << " bytes");
        NS_LOG_UNCOND("  Total Lost Packets: " << sumLost << " pkts");
        NS_LOG_UNCOND("    Total Throughput: " << sumThroughput << " Kbps");
        NS_LOG_UNCOND("-----------------------------------------------");
}

void
NetSim::ConfigureSetDefault ()
{
	NS_LOG_FUNCTION(this);

        Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
        Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
        Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));
}

void 
NetSim::ReceivePacket (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION(this);

	Ptr<Packet> packet;
	Address from;

	while ((packet = socket->RecvFrom (from))) {
		if (packet->GetSize () > 0) {
#ifdef DEBUG
			InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (from);
			std::cout << setw(10) << Simulator::Now ().GetSeconds ()
       				<< " received "<< packet->GetSize ()
       				<< " bytes from: (" << iaddr.GetIpv4 ()
       				<< ", " << iaddr.GetPort () << ")"
       				<< std::endl;
#endif
                        totalReceived++;
                }
        }
}

void 
NetSim::GenerateTraffic (Ptr<Socket> socket, Time pktInterval)
{
	NS_LOG_FUNCTION(this);

	if (numPackets > 0) {
		socket->Send (Create<Packet> (packetSize));
		Simulator::Schedule (pktInterval, &NetSim::GenerateTraffic, this, socket, pktInterval);
		totalSent ++;
		numPackets --;
    	} else {
		socket->Close ();
	}
}

void 
NetSim::MakeNetworkTopology()
{
	NS_LOG_FUNCTION(this);

        n = new Ptr<Node> [numNodes];
        for (uint32_t i = 0; i < numNodes; ++i) {
                n[i] = CreateObject<Node> ();
                n[i]->AggregateObject (CreateObject<ConstantPositionMobilityModel> ());
                allNodes.Add  (n[i]);
        }

	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
		"MinX", DoubleValue (0.0),
		"MinY", DoubleValue (0.0),
		"DeltaX", DoubleValue (distance),
		"DeltaY", DoubleValue (distance),
		"GridWidth", UintegerValue (5),
		"LayoutType", StringValue ("RowFirst"));
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (allNodes);
}

void 
NetSim::ConfigureDataLinkLayer()
{
	NS_LOG_FUNCTION(this);

	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss  ("ns3::FriisPropagationLossModel");

	wifiPhy =  YansWifiPhyHelper::Default ();
	// set it to zero; otherwise, gain will be added
	wifiPhy.Set ("RxGain", DoubleValue (-10) ); 
	// ns-3 supports RadioTap and Prism tracing extensions for 802.11b
	wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 
	wifiPhy.SetChannel (wifiChannel.Create ());

	// Add a non-QoS upper mac, and disable rate control
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	wifiMac.SetType ("ns3::AdhocWifiMac");

	WifiHelper wifi;
	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
		"DataMode",StringValue (phyMode),
		"ControlMode",StringValue (phyMode));
	// Set it to adhoc mode
	devices = wifi.Install (wifiPhy, wifiMac, allNodes);
}

void 
NetSim::ConfigureNetworkLayer ()
{
	NS_LOG_FUNCTION(this);

	InternetStackHelper internet;

	if(rtproto == "Olsr" || rtproto == "OLSR") {
		Ipv4StaticRoutingHelper staticRouting;
		Ipv4ListRoutingHelper list;
		list.Add (staticRouting, 0);
		list.Add (olsr, 10);
		internet.SetRoutingHelper (list); // has effect on the next Install ()
	} else if(rtproto == "Aodv" || rtproto == "AODV") {
		internet.SetRoutingHelper(aodv);
	} else if(rtproto == "Dsdv" || rtproto == "DSDV") {
		// Periodic Interval Time
		dsdv.Set ("PeriodicUpdateInterval", TimeValue (Seconds (5.0)));
		// Settling Time before sending out an update for changed metric
		dsdv.Set ("SettlingTime", TimeValue (Seconds (5.0)));
		internet.SetRoutingHelper(dsdv);
	} else if(rtproto == "Dsr" || rtproto == "DSR") {
		DsrMainHelper dsrMain;
		DsrHelper dsr;
		internet.Install (allNodes);
		dsrMain.Install (dsr, allNodes);
	} else {
		NS_LOG_INFO("No such routing protocol!");
		exit(1);
	}

	if(rtproto != "Dsr" && rtproto != "DSR") 
		internet.Install (allNodes);

	Ipv4AddressHelper ipv4;
	ipv4.SetBase ("10.1.1.0", "255.255.255.0");
	ifs = ipv4.Assign (devices);
}

InetSocketAddress
NetSim::setSocketAddress(Ptr<Node> node, uint32_t port)
{
	NS_LOG_FUNCTION(this);

        Ipv4InterfaceAddress adr = node->GetObject <Ipv4> ()->GetAddress(1, 0);
        return InetSocketAddress (Ipv4Address(adr.GetLocal()), port);
}

void 
NetSim::ConfigureL4withDUPApp(uint32_t src, uint32_t dst)
{
	NS_LOG_FUNCTION(this);

	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

	Ptr<Socket> destination = Socket::CreateSocket (n[dst], tid);
	InetSocketAddress dstSocketAddr = setSocketAddress (n[dst], 8888);
	destination->Bind (dstSocketAddr);
	destination->SetRecvCallback (MakeCallback (&NetSim::ReceivePacket, this));

	// set sender(n0)'s socket
	Ptr<Socket> source = Socket::CreateSocket (n[src], tid);
	InetSocketAddress srcSocketAddr = setSocketAddress (n[src], 8888);
	source->Bind (srcSocketAddr);
	source->SetAllowBroadcast (true);
	source->Connect (dstSocketAddr);

	Time interPacketInterval = Seconds (interval);
	// Give Proactive algorithms some time to converge -- 20 seconds perhaps
	Simulator::Schedule (Seconds (20.0), &NetSim::GenerateTraffic, this, source, interPacketInterval);
}

void
NetSim::traceSimEvents()
{
	NS_LOG_FUNCTION(this);

	AsciiTraceHelper ascii;
	wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("exp05-RoutingOnadhocGrid.tr"));
	wifiPhy.EnablePcap ("exp05-RoutingOnadhocGrid", devices);
}

void // Trace routing tables
NetSim::traceRoutingTables()
{
	NS_LOG_FUNCTION(this);

	std::string tf = std::string(PROG_DIR) + "exp05-routingTables.tr";
	Ptr<OutputStreamWrapper> routingStream = 
		Create<OutputStreamWrapper> (tf, std::ios::out);
	// PrintRoutingTableAllEvery() : defined at Ipv4RoutingHelper class
	// and inheritanced by each rouring helper. ref. Ipv4RoutingHelper.
	if(rtproto == "Aodv" || rtproto == "AODV")
		aodv.PrintRoutingTableAllEvery (Seconds (2), routingStream);
	else if(rtproto == "Dsdv" || rtproto == "DSDV")
		dsdv.PrintRoutingTableAllEvery (Seconds (2), routingStream);
	else if(rtproto == "Olsr" || rtproto == "OLSR")
		olsr.PrintRoutingTableAllEvery (Seconds (2), routingStream);
}

EnergySourceContainer
NetSim::AttachEnergyModelToDevices(double initialEnergy, double txcurrent, double rxcurrent)
{
	NS_LOG_FUNCTION(this);

	// configure energy source
	BasicEnergySourceHelper basicSourceHelper;
	basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (initialEnergy));
	EnergySourceContainer e = basicSourceHelper.Install (allNodes);

	// transmit at 0dBm = 17.4mA, receive mode = 19.7mA
	WifiRadioEnergyModelHelper radioEnergyHelper;
	radioEnergyHelper.Set ("TxCurrentA", DoubleValue (txcurrent));
	radioEnergyHelper.Set ("RxCurrentA", DoubleValue (rxcurrent));
	radioEnergyHelper.SetDepletionCallback(
	MakeCallback(&NetSim::NotifyDrained, this));
	DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (devices, e);
	return e;
}

void // Trace function for remaining energy at node.
NetSim::GetRemainingEnergy (double oldValue, double remainingEnergy)
{
	NS_LOG_FUNCTION(this);

	if(_energyDrained==false) {
		// show current remaining energy(J)
		std::cout << std::setw(10) << Simulator::Now ().GetSeconds () 
	 	<< "\t " 
		<< remainingEnergy <<std::endl;
		_remainingEnergy = remainingEnergy;
	}
}

void // Trace function for total energy consumption at node.
NetSim::TotalEnergy (double oldValue, double totalEnergy)
{
	NS_LOG_FUNCTION(this);

	NS_LOG_UNCOND (Simulator::Now ().GetSeconds () <<"\t" << totalEnergy);
}

void
NetSim::TraceRemainingEnergy(EnergySourceContainer e)
{
	NS_LOG_FUNCTION(this);

	// all energy sources are connected to resource node n>0
	Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (e.Get (traceNum));
	basicSourcePtr->TraceConnectWithoutContext ("RemainingEnergy", 
		MakeCallback (&NetSim::GetRemainingEnergy, this));
}

void
NetSim::TraceTotalEnergy(EnergySourceContainer e)
{
	NS_LOG_FUNCTION(this);

	Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (e.Get (traceNum));
	Ptr<DeviceEnergyModel> basicRadioModelPtr =
		basicSourcePtr->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
	NS_ASSERT (basicRadioModelPtr != NULL);
	basicRadioModelPtr->TraceConnectWithoutContext ("TotalEnergyConsumption", 
	MakeCallback (&NetSim::TotalEnergy, this));
}

void
NetSim::NotifyDrained()
{
	NS_LOG_FUNCTION(this);

        std::cout <<"Energy was Drained. Stop send.\n";
        _energyDrained = true;
}

void 
NetSim::Configure (int argc, char *argv[])
{
	NS_LOG_FUNCTION(this);

	CommandLine cmd;
	cmd.AddValue ("phyMode"   , "Wifi Physical layer mode", phyMode);
	cmd.AddValue ("distance"  , "distance (m)", distance);
	cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
	cmd.AddValue ("numPackets", "total number of packets generated", numPackets);
	cmd.AddValue ("interval"  , "interval (seconds) between send packets", interval);
	cmd.AddValue ("numNodes"  , "total number of nodes on the Grid", numNodes);
	cmd.AddValue ("sourceNode", "Sender's node number", sourceNode);
	cmd.AddValue ("sinkNode"  , "Receiver's node number", sinkNode);
	cmd.AddValue ("traceEng"  , "trace energy flag (remained|total)", traceEng);
	cmd.AddValue ("traceNode" , "trace energy for node", traceNum);
	cmd.AddValue ("rtProto"   , "set routing protocol(Olsr,Aodv,Dsdv)", rtproto);
	cmd.Parse (argc, argv);
}

void 
NetSim::RunSimulation ()
{
	NS_LOG_FUNCTION(this);

	ConfigureSetDefault();
	MakeNetworkTopology();
	ConfigureDataLinkLayer();

	EnergySourceContainer e = AttachEnergyModelToDevices(1.5, 0.0857, 0.0528);
        if (traceEng == "remained")
                TraceRemainingEnergy(e);
        else
                TraceTotalEnergy(e);

	ConfigureNetworkLayer ();
	ConfigureL4withDUPApp(sourceNode, sinkNode);

	if (tracing == true) {
		traceRoutingTables();
		// To do-- enable an IP-level trace that shows forwarding events only
	}

	std::string xf = std::string(PROG_DIR) + "RoutingProtocols.xml";
	AnimationInterface anim (xf);
        anim.EnablePacketMetadata (true);

	// Output what we are doing
	NS_LOG_UNCOND ("Testing from node " << sourceNode 
		<< " to " << sinkNode 
		<< " with grid distance " << distance);

	SetFlowMonitor();

	Simulator::Stop (Seconds (40.0));
	Simulator::Run ();

	RunFlowMonitor(sourceNode, sinkNode);

	Simulator::Destroy ();

	NS_LOG_UNCOND ("    Total packets sent: " << totalSent);
        NS_LOG_UNCOND ("Total packets received: " << totalReceived);
}

int main (int argc, char *argv[])
{
	NetSim sim;

	sim.Configure (argc, argv);
	sim.RunSimulation ();

	return 0;
}

