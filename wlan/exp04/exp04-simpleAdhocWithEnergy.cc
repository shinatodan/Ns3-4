/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- *
 * F.Qian, Dec. 2012 <fei@kanto-gakuin.ac.jp>
 *
 *  n0 <----> n1 <----> n2 <----> n3
 *        192.168.1.0/24, AODV
 *  mn --------- move to --------->
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/energy-module.h"
#include "ns3/ipv4-header.h"
#include "ns3/packet.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-module.h"
#include "ns3/netanim-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>

#define SIM_STOP 20.0
#define TH_INTERVAL     0.1 

#define PROG_DIR	"wlan/exp04/data/"

NS_LOG_COMPONENT_DEFINE ("exp04-WifiSimpleAdhoc");

using namespace ns3;

class NetSim {
    public :
	NetSim();
	~NetSim();

	virtual void Configure(StringValue);
	virtual void CreateNetworkTopology();
	virtual void ConfigureDataLinkLayer(bool, StringValue, double);
	virtual void ConfigureNetworkLayer ();
	virtual void ConfigureTransportLayerWithUDP (Ptr<Node>, uint32_t, Ptr<Node>, uint32_t);
	virtual void ConfigureAnimation();
	virtual void StartApplication(DataRate, uint32_t);
	virtual void StartApplication(uint32_t, uint32_t, double);

	EnergySourceContainer AttachEnergyModelToDevices(double, double, double);
        void TraceRemainingEnergy(uint32_t, EnergySourceContainer);
	void TraceTotalEnergy(uint32_t, EnergySourceContainer);

	Ptr<Node>    *n;
	Ptr<Node>    mn;

	uint32_t totalReceived;
	uint32_t totalSent;

    private :
	InetSocketAddress setSocketAddress (Ptr<Node>, uint32_t);
	void GenerateTrafficWithDataRate (Ptr<Socket>, uint32_t, DataRate);
	void GenerateTrafficWithPktCount (Ptr<Socket>, uint32_t, uint32_t, Time);
	void ReceivePacket (Ptr<Socket>);

	void TotalEnergy (double, double);
        void GetRemainingEnergy (double, double);

	Ptr<Socket> source;
	Ptr<Socket> destination;

	uint32_t      nWifiNodes;
	NodeContainer  wlanNodes;
	NodeContainer   allNodes;
	NetDeviceContainer devices;

	void NotifyDrained();
	bool _energyDrained; // set it to true if energy is drained.

	void traceThroughput ();
	double _remainingEnergy;
};

NetSim::NetSim() : totalReceived(0), totalSent(0)
{
	nWifiNodes = 4;
	_energyDrained = false;
}

NetSim::~NetSim()
{
        for (uint32_t i = 0; i < nWifiNodes; ++i) n[i] = 0;
	n=0; mn=0;
}

void
NetSim::NotifyDrained()
{
	std::cout <<"Energy was Drained. Stop send.\n";
	_energyDrained = true;
}

EnergySourceContainer
NetSim::AttachEnergyModelToDevices(double initialEnergy, double txcurrent, double rxcurrent)
{
        // configure energy source
        BasicEnergySourceHelper basicSourceHelper;
        basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (initialEnergy));
	EnergySourceContainer e= basicSourceHelper.Install (allNodes);

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
        NS_LOG_UNCOND (Simulator::Now ().GetSeconds () <<"\t" << totalEnergy);
}

void
NetSim::TraceRemainingEnergy(uint32_t n, EnergySourceContainer e)
{
        // all energy sources are connected to resource node n>0
        Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (e.Get (n));
        basicSourcePtr->TraceConnectWithoutContext ("RemainingEnergy", 
               MakeCallback (&NetSim::GetRemainingEnergy, this));
}

void
NetSim::TraceTotalEnergy(uint32_t n, EnergySourceContainer e)
{
        Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (e.Get (n));
	Ptr<DeviceEnergyModel> basicRadioModelPtr =
		basicSourcePtr->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
		NS_ASSERT (basicRadioModelPtr != NULL);
	basicRadioModelPtr->TraceConnectWithoutContext ("TotalEnergyConsumption", 
		MakeCallback (&NetSim::TotalEnergy, this));
}

void
NetSim::traceThroughput ()
{
	double now = (double)Simulator::Now ().GetSeconds ();
        double th = (totalReceived*8)/now;
	std::cout << std::setw(10) << now << "\t" << th << std::endl;
	Simulator::Schedule (Seconds (TH_INTERVAL), &NetSim::traceThroughput, this);
}

void
NetSim::ReceivePacket (Ptr<Socket> socket)
{
	Ptr<Packet> packet;
	Address from;

	while ((packet = socket->RecvFrom (from))) {
		if (packet->GetSize () > 0) {
#ifdef DEBUG
			InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (from);
			std::cout << std::setw(10) << Simulator::Now ().GetSeconds ()
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
NetSim::GenerateTrafficWithDataRate (Ptr<Socket> socket, uint32_t pktSize, DataRate dataRate )
{
	if(_energyDrained==false) {
		Time tNext (Seconds (pktSize * 8 / static_cast<double> (dataRate.GetBitRate ())));
		Ptr<Packet> newPacket = Create<Packet> (pktSize);
		socket->Send (newPacket);
		Simulator::Schedule (tNext, &NetSim::GenerateTrafficWithDataRate, this, 
			socket, pktSize, dataRate);
		totalSent ++;
	} else {
                socket->Close ();
        }
}

void 
NetSim::GenerateTrafficWithPktCount (Ptr<Socket> socket, uint32_t pktSize, 
	uint32_t pktCount, Time pktInterval)
{
        if (pktCount > 0) {
                socket->Send (Create<Packet> (pktSize));
                Simulator::Schedule (pktInterval, &NetSim::GenerateTrafficWithPktCount, this,
                           socket, pktSize, pktCount-1, pktInterval);
		totalSent ++;
        } else {
                socket->Close ();
        }
}

InetSocketAddress
NetSim::setSocketAddress(Ptr<Node> node, uint32_t port)
{
	Ipv4InterfaceAddress adr = node->GetObject <Ipv4> ()->GetAddress(1, 0);
        return InetSocketAddress (Ipv4Address(adr.GetLocal()), port);
}

void
NetSim::Configure(StringValue phyMode)
{
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
	Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", phyMode);
}

void
NetSim::CreateNetworkTopology()
{
	n = new Ptr<Node> [nWifiNodes];
        for (uint32_t i = 0; i < nWifiNodes; ++i) {
                n[i] = CreateObject<Node> ();
                n[i]->AggregateObject (CreateObject<ConstantPositionMobilityModel> ());
                wlanNodes.Add (n[i]);
        }
	mn = CreateObject<Node> (); // create 1 mobile node

	allNodes.Add(wlanNodes);
	allNodes.Add(mn);
}

void 
NetSim::ConfigureDataLinkLayer(bool verbose, StringValue phyMode, double dist)
{
	WifiHelper wifi;
	if (verbose) wifi.EnableLogComponents ();  // Turn on all Wifi logging
	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
	//wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
				      "DataMode"   , phyMode,
				      "ControlMode", phyMode);

	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss  ("ns3::LogDistancePropagationLossModel",
		"Exponent", DoubleValue(3.0),
		"ReferenceDistance", DoubleValue(1.0),
		"ReferenceLoss", DoubleValue(46.6777));

	YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
	wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 
	wifiPhy.SetChannel (wifiChannel.Create ());

	// Add a non-QoS upper mac, and disable rate control
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	Ssid ssid = Ssid ("MySSID");
	wifiMac.SetType  ("ns3::AdhocWifiMac", "Ssid", SsidValue (ssid));
	devices = wifi.Install (wifiPhy, wifiMac, allNodes);

	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
        for (uint32_t i = 0; i < nWifiNodes; ++i) 
		positionAlloc->Add (Vector (i*80.0, 100.0, 0.0)); // ni's position
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wlanNodes);

	Ptr<WaypointMobilityModel> m_mob;
        m_mob = CreateObject<WaypointMobilityModel> ();
        mn->AggregateObject (m_mob);
        Waypoint wpt_start  (Seconds (0.0), Vector (0.0, dist, 0.0));
        m_mob->AddWaypoint  (wpt_start);
        Waypoint wpt_stop   (Seconds (SIM_STOP), Vector (nWifiNodes*100.0, dist, 0.0));
        m_mob->AddWaypoint  (wpt_stop);

	//wifiPhy.EnablePcap ("simpleEnergy", devices);
}

void 
NetSim::ConfigureNetworkLayer ()
{
	InternetStackHelper internet;
	AodvHelper aodv; // use AODV routing protocol
	internet.SetRoutingHelper(aodv);
	internet.Install (allNodes);

	Ipv4AddressHelper ipv4;
	ipv4.SetBase ("192.168.0.0", "255.255.0.0", "0.0.1.1");
	Ipv4InterfaceContainer ifs = ipv4.Assign (devices);
}

void 
NetSim::ConfigureTransportLayerWithUDP (Ptr<Node> src, uint32_t src_port, Ptr<Node> dst, uint32_t dst_port)
{
	// set receiver(mn)'s socket
	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

	destination = Socket::CreateSocket (dst, tid);
	//Ptr<Socket> destination = Socket::CreateSocket (dst, tid);
        InetSocketAddress dstSocketAddr = setSocketAddress (dst, dst_port);
	destination->Bind (dstSocketAddr);
	destination->SetRecvCallback (MakeCallback (&NetSim::ReceivePacket, this));

	// set sender(n0)'s socket
	source = Socket::CreateSocket (src, tid);
        InetSocketAddress srcSocketAddr = setSocketAddress (src, src_port);
	source->Bind (srcSocketAddr);
	source->SetAllowBroadcast (true);
	source->Connect (dstSocketAddr);
}

void
NetSim::StartApplication(DataRate dataRate, uint32_t packetSize)
{
	Simulator::Schedule( Seconds (0.1), &NetSim::GenerateTrafficWithDataRate, this, 
		source, packetSize, dataRate);
	//Simulator::Schedule (Seconds(TH_INTERVAL), &NetSim::traceThroughput, this);
}

void
NetSim::StartApplication(uint32_t packetSize, uint32_t numPkt, double interval)
{
	Time interPacketInterval = Seconds (interval);
	Simulator::Schedule( Seconds (0.1), &NetSim::GenerateTrafficWithPktCount, this, 
		source, packetSize, numPkt, interPacketInterval);
}

void
NetSim::ConfigureAnimation()
{
/*
	AnimationInterface::SetNodeDescription (n[0], "n0");
	AnimationInterface::SetNodeDescription (n[1], "n1");
	AnimationInterface::SetNodeDescription (n[2], "n2");
	AnimationInterface::SetNodeDescription (n[3], "n3");
	AnimationInterface::SetNodeDescription (mn  , "mn");
*/
}

int 
main (int argc, char *argv[])
{
	NetSim sim;

	std::string phyMode ("DsssRate1Mbps");
	bool        verbose    = false;
	bool        sendFlag   = false; // send pkts with dataRate
	std::string traceFlag ("total"); // trace total energy consumption
	DataRate    dataRate   = DataRate ("512Kbps");
	uint32_t    packetSize = 1024; // bytes
	uint32_t    numPackets = 10;
	uint32_t    traceNum   = 0;
	double      distance   = 200.0; // distance from mobile node
	double      interval   = 0.1; // seconds

	CommandLine cmd;
	cmd.AddValue ("phyMode"   , "Wifi Phy mode", phyMode);
	cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
	cmd.AddValue ("dataRate"  , "dataRate of application packet sent", dataRate);
	cmd.AddValue ("distance"  , "distance between STA and MN", distance);
	cmd.AddValue ("numPackets", "number of packets generated", numPackets);
	cmd.AddValue ("interval"  , "interval (seconds) between packets", interval);
	cmd.AddValue ("sendFlag"  , "send flag (1: use pkt data rate, 0: use number of pkts)", sendFlag);
	cmd.AddValue ("traceFlag" , "trace flag (remained|total)", traceFlag);
	cmd.AddValue ("traceNode" , "trace Node's number", traceNum);
	cmd.AddValue ("verbose"   , "turn on all WifiNetDevice log components", verbose);
	cmd.Parse (argc, argv);

	sim.Configure(phyMode);
	sim.CreateNetworkTopology();
	sim.ConfigureDataLinkLayer(verbose, phyMode, distance);

	EnergySourceContainer e = sim.AttachEnergyModelToDevices(1.5, 0.0174, 0.0197);
	if(traceFlag == "remained")
		sim.TraceRemainingEnergy(traceNum, e);
	else
		sim.TraceTotalEnergy(traceNum, e);

	sim.ConfigureNetworkLayer ();
	//sim.ConfigureTransportLayerWithUDP(sim.n[0], 8888, sim.mn, 8888);
	sim.ConfigureTransportLayerWithUDP(sim.mn, 8888, sim.n[0], 8888);

	if(sendFlag)
		sim.StartApplication (dataRate, packetSize);
	else
		sim.StartApplication (packetSize, numPackets, interval);

	sim.ConfigureAnimation();
	std::string xf = std::string(PROG_DIR) + "simple-energy.xml";
	AnimationInterface anim (xf);
	anim.EnablePacketMetadata (true);

	Simulator::Stop (Seconds (SIM_STOP));
	Simulator::Run ();
	Simulator::Destroy ();

	NS_LOG_UNCOND ("    Total packets sent: " << sim.totalSent);
	NS_LOG_UNCOND ("Total packets received: " << sim.totalReceived);

	return 0;
}

