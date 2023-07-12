/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* exp01-hidden-terminal.cc: classical hidden terminal problem and its RTS/CTS solution.
 * F.Qian, Dec., 2012 <fei@kanto-gakuin.ac.jp>
 * 
 *           -50dB      -50dB
 *       n0 ------- n1 ------- n2
 * 
 * This example illustrates the use of 
 *  - Wifi in ad-hoc mode
 *  - Matrix propagation loss model
 *  - Use of OnOffApplication to generate CBR stream 
 *  - IP flow monitor
 */
#include "ns3/core-module.h"
#include "ns3/propagation-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"

#include "ns3/netanim-module.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

using namespace ns3;

#define SIM_START 	1.0
#define SIM_STOP  	15.0
#define PORT      	9    // Discard port (RFC 863)

#define PROG_DIR	"wlan/exp01/data/"

NS_LOG_COMPONENT_DEFINE ("exp01-HiddenTerminal");

class NetSim {
    public :
        NetSim();
        ~NetSim(){};

	void ConfigureCtsRts();

	virtual void CreateNetworkTopology ();
	virtual void ConfigureDataLinkLayer();
	virtual void ConfigureNetworkLayer ();

	virtual void setUdpOnOffApp(Ptr<Node>, Ptr<Node>, std::string, std::string, double, double);
	virtual void setUdpEchoApp (Ptr<Node>, Ptr<Node>, std::string, std::string, double);

	virtual void ConfigureAnimation();

	void SetFlowMonitor ();
        void RunFlowMonitor ();


	void inline MacTxDrop(Ptr<const Packet> p){MacTxDropCount++;};
	void inline MacRx(Ptr<const Packet> p){MacRxCount++;};
	void inline PhyTxDrop(Ptr<const Packet> p){PhyTxDropCount++;};
	void inline PhyRxDrop(Ptr<const Packet> p){PhyRxDropCount++;};
	void inline PrintDrop(){
		std::cout << std::setw(6) << Simulator::Now().GetSeconds() 
			  << " MacTxDrop: " << MacTxDropCount 
			  << " PhyTxDrop: " << PhyTxDropCount 
			  << " PhyRxDrop: " << PhyRxDropCount 
			  << " MacRx:" << MacRxCount 
			  << std::endl;
		Simulator::Schedule(Seconds(1.0), &NetSim::PrintDrop, this);
	}
	void traceWifiDrop(uint32_t);

	Ptr <Node> *n;
	NetDeviceContainer devices;
	bool enableCtsRts;

    private :
	uint32_t 	   nNodes;
	NodeContainer 	   allNodes;

	FlowMonitorHelper  *flowmon;
        Ptr<FlowMonitor>   monitor;

	uint32_t MacTxDropCount, MacRxCount, PhyTxDropCount, PhyRxDropCount;
};

NetSim::NetSim() : nNodes(3)
{
	enableCtsRts = false;
	MacTxDropCount=0; 
	MacRxCount    =0; 
	PhyTxDropCount=0;
	PhyRxDropCount=0;
}

void
NetSim::CreateNetworkTopology()
{
	n = new Ptr<Node> [nNodes];
	for (size_t i = 0; i < nNodes; ++i) {
		n[i] = CreateObject<Node> ();
		// Place nodes somehow, this is required by every wireless simulation
		n[i]->AggregateObject (CreateObject<ConstantPositionMobilityModel> ());
		allNodes.Add (n[i]);
	}
}

void // set UDP OnOff flow between n to m
NetSim::setUdpOnOffApp(Ptr <Node> n, Ptr <Node> m, std::string pktSize, std::string dataRate, double start, double stop)
{
        Config::SetDefault ("ns3::OnOffApplication::PacketSize", StringValue(pktSize));
        Config::SetDefault ("ns3::OnOffApplication::DataRate"  , StringValue(dataRate));

        Ipv4InterfaceAddress madr = m->GetObject <Ipv4> ()->GetAddress(1, 0);
        AddressValue remoteAddr (InetSocketAddress (madr.GetLocal(), PORT));
        Ipv4InterfaceAddress nadr = n->GetObject <Ipv4> ()->GetAddress(1, 0);
        Address srcAddr (InetSocketAddress (nadr.GetLocal(), PORT));

        OnOffHelper onoff  ("ns3::UdpSocketFactory", srcAddr);
        onoff.SetAttribute ("OnTime"               , StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute ("OffTime"              , StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        onoff.SetAttribute ("Remote"               , remoteAddr);
        ApplicationContainer onoffapps = onoff.Install (n);
        onoffapps.Start (Seconds (start));
        onoffapps.Stop  (Seconds (stop ));
}

void // ping m from n
NetSim::setUdpEchoApp(Ptr <Node> n, Ptr <Node> m, std::string pktSize, std::string maxPkts, double start)
{
	uint16_t  echoPort = 7; // udp echo port
        Ipv4InterfaceAddress madr = m->GetObject <Ipv4> ()->GetAddress(1, 0);
        AddressValue remoteAddr (InetSocketAddress (madr.GetLocal(), echoPort));

	UdpEchoClientHelper echoClientHelper (madr.GetLocal(), echoPort);
	echoClientHelper.SetAttribute ("MaxPackets", StringValue (maxPkts));
	echoClientHelper.SetAttribute ("Interval"  , TimeValue   (Seconds (0.1)));
	echoClientHelper.SetAttribute ("PacketSize", StringValue (pktSize));
	echoClientHelper.SetAttribute ("StartTime" , TimeValue   (Seconds (start)));

	// again using different start times to workaround Bug 388 and Bug 912
	ApplicationContainer pingApps;
	pingApps.Add (echoClientHelper.Install (n)); 
}

void
NetSim::ConfigureDataLinkLayer()
{
	// Create propagation loss matrix
	Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
	lossModel->SetDefaultLoss (200); // set default loss to 200 dB (no link)
	lossModel->SetLoss (n[0]->GetObject<MobilityModel>(), 
		n[1]->GetObject<MobilityModel>(), 50); // set symmetric loss 0 <-> 1 to 50 dB
	lossModel->SetLoss (n[2]->GetObject<MobilityModel>(), 
		n[1]->GetObject<MobilityModel>(), 50); // set symmetric loss 2 <-> 1 to 50 dB

	// Create & setup wifi channel
	Ptr<YansWifiChannel> wifiChannel = CreateObject <YansWifiChannel> ();
	wifiChannel->SetPropagationLossModel (lossModel);
	wifiChannel->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());

	YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
	wifiPhy.SetChannel (wifiChannel);

	Ssid ssid = Ssid ("MySSID");
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	wifiMac.SetType ("ns3::AdhocWifiMac", "Ssid", SsidValue (ssid));

	// Install wireless devices
	WifiHelper wifi;
	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
				      "DataMode"   ,StringValue ("DsssRate2Mbps"), 
				      "ControlMode",StringValue ("DsssRate1Mbps"));
	devices = wifi.Install (wifiPhy, wifiMac, allNodes);
}

void
NetSim::ConfigureNetworkLayer()
{
	InternetStackHelper internet;
	internet.Install (allNodes);
	Ipv4AddressHelper ipv4;
	ipv4.SetBase ("192.168.1.0", "255.255.255.0");
	ipv4.Assign (devices);
}

void
NetSim::ConfigureAnimation()
{
	// set node name
/*
	AnimationInterface::SetNodeDescription (n[0], "node1"); 
	AnimationInterface::SetNodeDescription (n[1], "AP"); 
	AnimationInterface::SetNodeDescription (n[2], "node2"); 

	// set node color
	AnimationInterface::SetNodeColor       (n[0], 0  , 255, 0); // green
	AnimationInterface::SetNodeColor       (n[1], 255, 0  , 0); // red
	AnimationInterface::SetNodeColor       (n[2], 0  , 255, 0); // green
*/

	// set node position
	AnimationInterface::SetConstantPosition(n[0], 4.0 , 16.0);
	AnimationInterface::SetConstantPosition(n[1], 11.0, 6.0);
	AnimationInterface::SetConstantPosition(n[2], 18.0, 16.0);
}

void // Install FlowMonitor on all nodes
NetSim::SetFlowMonitor()
{
        // set Flowmonitor atributes and mount it to all nodes
        flowmon = new FlowMonitorHelper;
        flowmon->SetMonitorAttribute("PacketSizeBinWidth", ns3::DoubleValue(20));
        flowmon->SetMonitorAttribute("JitterBinWidth"    , ns3::DoubleValue(0.001));
        flowmon->SetMonitorAttribute("DelayBinWidth"     , ns3::DoubleValue(0.001));
        monitor = flowmon->InstallAll();
}

void // Print per flow statistics
NetSim::RunFlowMonitor()
{
	double sumThroughput = 0, th;
	uint64_t sumTx = 0, tx, sumRx = 0, rx, sumLost = 0, lost;

	monitor->CheckForLostPackets ();
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon->GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i=stats.begin (); 
		i != stats.end (); ++i) {
		// first 2 FlowIds are for ECHO apps, we don't want to display them
		if (i->first > 2) {
			th = i->second.rxBytes *8.0/10.0/1024/1024; sumThroughput += th;
			tx = i->second.txBytes; sumTx += tx;
			rx = i->second.rxBytes; sumRx += rx;
			lost = i->second.lostPackets; sumLost += lost;

      			NS_LOG_UNCOND("-----------------------------------------------");
			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
			NS_LOG_UNCOND("Flow " << i->first - 2 << " (" << t.sourceAddress 
				  << " -> " << t.destinationAddress << ")");
			NS_LOG_UNCOND("Tx/Rx(B)/lost(p): " << tx << "/" << rx << "/" << lost);
			NS_LOG_UNCOND("      Throughput: " << th << " Mbps");
      			NS_LOG_UNCOND("       R/S Ratio: " 
				  << (double)i->second.rxBytes/(double)i->second.txBytes);
		}
	}
      	NS_LOG_UNCOND("-----------------------------------------------");
	NS_LOG_UNCOND("    Total Sent Bytes: " << sumTx);
	NS_LOG_UNCOND("Total Received Bytes: " << sumRx);
	NS_LOG_UNCOND("  Total Lost Packets: " << sumLost);
	NS_LOG_UNCOND("    Total Throughput: " << sumThroughput);
}

void
NetSim::ConfigureCtsRts()
{
	if(enableCtsRts)
		NS_LOG_UNCOND("Hidden station experiment with RTS/CTS enabled: ");
	else
		NS_LOG_UNCOND("Hidden station experiment with RTS/CTS disabled:");

	// Enable or disable CTS/RTS
	UintegerValue ctsThr = (enableCtsRts ? UintegerValue (10) : UintegerValue (2200));
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThr);
}

void
NetSim::traceWifiDrop(uint32_t number)
{
	// Trace Collisions
	std::stringstream ss;
	ss << number;
	std::string prefix = "/NodeList/" + ss.str();
	std::string _mactxdrop = prefix+ "/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop";
	std::string _macrx     = prefix+ "/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx";
	std::string _phyrxdrop = prefix+ "/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop";
	std::string _phytxdrop = prefix+ "/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop";

	Config::ConnectWithoutContext(_mactxdrop, MakeCallback(&NetSim::MacTxDrop, this));
	Config::ConnectWithoutContext(_macrx    , MakeCallback(&NetSim::MacRx    , this));
	Config::ConnectWithoutContext(_phyrxdrop, MakeCallback(&NetSim::PhyRxDrop, this));
	Config::ConnectWithoutContext(_phytxdrop, MakeCallback(&NetSim::PhyTxDrop, this));

	Ptr<WifiNetDevice> wifidev = DynamicCast<WifiNetDevice> (devices.Get(number));
	Ptr<WifiPhy> wifiphy = wifidev->GetPhy();
	Ptr<WifiMac> wifimac = wifidev->GetMac();
        Ipv4InterfaceAddress n1_adr = n[number]->GetObject <Ipv4> ()->GetAddress(1, 0);
	std::cout << "MAC: "     << wifimac->GetAddress() 
		  << ", IP: "    << n1_adr.GetLocal()
		  << ", BSSID: " << wifimac->GetBssid() 
		  << ", SSID: "  << wifimac->GetSsid() 
		  << std::endl;
	Simulator::Schedule(Seconds(SIM_STOP), &NetSim::PrintDrop, this);
}

// Run single 10 seconds experiment with enabled or disabled RTS/CTS mechanism
int main (int argc, char **argv)
{
	NetSim sim;

	CommandLine cmd;
	cmd.AddValue("CtsRts", "experiment with RTS/CTS enabled(1)/disable(0).", sim.enableCtsRts);
        cmd.Parse(argc, argv);

	sim.ConfigureCtsRts();
	sim.CreateNetworkTopology ();
	sim.ConfigureDataLinkLayer();
	sim.ConfigureNetworkLayer ();

	// Install applications: two CBR streams each saturating the channel 
	sim.setUdpOnOffApp(sim.n[0], sim.n[1], "200", "1Mbps", SIM_START, SIM_STOP);
	sim.setUdpOnOffApp(sim.n[2], sim.n[1], "200", "1Mbps", SIM_START+0.001, SIM_STOP);

	// we also use separate UDP applications that will send a single
	// packet before the CBR flows start. 
	sim.setUdpEchoApp(sim.n[0], sim.n[1], "10", "1", 0.01);
	sim.setUdpEchoApp(sim.n[2], sim.n[1], "10", "1", 0.03);

	sim.SetFlowMonitor();
	sim.traceWifiDrop(1);

	sim.ConfigureAnimation();
	std::string xf = std::string(PROG_DIR) + "hiden-terminal.xml";
	AnimationInterface anim (xf); 

	Simulator::Stop (Seconds (SIM_STOP));
	Simulator::Run ();
	sim.RunFlowMonitor();

	Simulator::Destroy ();
	return 0;
}

