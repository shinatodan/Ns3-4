/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Classical hidden terminal problem and its RTS/CTS solution.
 *
 *           -50dB      -50dB      -50dB
 *       n0 ------- n1 ------- n2 ------- n3
 *          <------               ------>
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

#include <fstream>

using namespace ns3;

#define SIM_START 	1.5
#define SIM_STOP  	10.0
#define PORT      	9    // Discard port (RFC 863)

#define PROG_DIR	"wlan/exp01/data/"

class SimClass {
    public :
        SimClass();
        ~SimClass(){};

	void ConfigureCtsRts();

	virtual void CreateNetworkTopology ();
	virtual void ConfigureDataLinkLayer();
	virtual void ConfigureNetworkLayer ();

	virtual void setUdpOnOffApp(Ptr<Node>, Ptr<Node>, std::string, std::string, double, double);
	virtual void setUdpEchoApp (Ptr<Node>, Ptr<Node>, std::string, std::string, double);

	virtual void ConfigureAnimation();

	void SetFlowMonitor ();
        void RunFlowMonitor ();

	Ptr <Node>     *n;
	bool enableCtsRts;

    private :
	uint32_t 	   nNodes;

	NodeContainer 	   nodes;
	NetDeviceContainer devices;

	FlowMonitorHelper  *flowmon;
        Ptr<FlowMonitor>   monitor;
};

SimClass::SimClass() : nNodes(4)
{
	enableCtsRts = false;
}

void
SimClass::CreateNetworkTopology()
{
	n = new Ptr<Node> [nNodes];
	for (size_t i = 0; i < nNodes; ++i) {
		n[i] = CreateObject<Node> ();
		// Place nodes somehow, this is required by every wireless simulation
		n[i]->AggregateObject (CreateObject<ConstantPositionMobilityModel> ());
		nodes.Add (n[i]);
	}
}

void // set UDP OnOff flow between n to m
SimClass::setUdpOnOffApp(Ptr <Node> n, Ptr <Node> m, std::string pktSize, std::string dataRate, double start, double stop)
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
SimClass::setUdpEchoApp(Ptr <Node> n, Ptr <Node> m, std::string pktSize, std::string maxPkts, double start)
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
SimClass::ConfigureDataLinkLayer()
{
	// Create propagation loss matrix
	Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
	lossModel->SetDefaultLoss (200); // set default loss to 200 dB (no link)
	lossModel->SetLoss (n[0]->GetObject<MobilityModel>(), 
		n[1]->GetObject<MobilityModel>(), 50); // set symmetric loss 0 <-> 1 to 50 dB
	lossModel->SetLoss (n[1]->GetObject<MobilityModel>(), 
		n[2]->GetObject<MobilityModel>(), 50); // set symmetric loss 1 <-> 2 to 50 dB
	lossModel->SetLoss (n[2]->GetObject<MobilityModel>(), 
		n[3]->GetObject<MobilityModel>(), 50); // set symmetric loss 2 <-> 3 to 50 dB

	// Create & setup wifi channel
	Ptr<YansWifiChannel> wifiChannel = CreateObject <YansWifiChannel> ();
	wifiChannel->SetPropagationLossModel (lossModel);
	wifiChannel->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());

	// Install wireless devices
	WifiHelper wifi;
	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
				      "DataMode"   ,StringValue ("DsssRate2Mbps"), 
				      "ControlMode",StringValue ("DsssRate1Mbps"));
	YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
	wifiPhy.SetChannel (wifiChannel);

	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	wifiMac.SetType ("ns3::AdhocWifiMac"); // use ad-hoc MAC
	devices = wifi.Install (wifiPhy, wifiMac, nodes);
}

void
SimClass::ConfigureNetworkLayer()
{
	InternetStackHelper internet;
	internet.Install (nodes);
	Ipv4AddressHelper ipv4;
	ipv4.SetBase ("10.0.0.0", "255.0.0.0");
	ipv4.Assign (devices);
}

void
SimClass::ConfigureAnimation()
{
	// set node name
	AnimationInterface::SetNodeDescription (n[0], "node1"); 
	AnimationInterface::SetNodeDescription (n[1], "node2"); 
	AnimationInterface::SetNodeDescription (n[2], "node3"); 
	AnimationInterface::SetNodeDescription (n[3], "node4"); 

	// set node color
	AnimationInterface::SetNodeColor (n[0], 0  , 255, 0); // green
	AnimationInterface::SetNodeColor (n[1], 255, 0  , 0); // red
	AnimationInterface::SetNodeColor (n[2], 255, 0  , 0); // red
	AnimationInterface::SetNodeColor (n[3], 0  , 255, 0); // green

	// set node position
	AnimationInterface::SetConstantPosition(n[0], 0.0 , 5.0);
	AnimationInterface::SetConstantPosition(n[1], 5.0 , 5.0);
	AnimationInterface::SetConstantPosition(n[2], 10.0, 5.0);
	//AnimationInterface::SetConstantPosition(n[3], 15.0, 5.0);

	AnimationInterface anim ("exposed-terminal.xml"); 
}

void // Install FlowMonitor on all nodes
SimClass::SetFlowMonitor()
{
        // set Flowmonitor atributes and mount it to all nodes
        flowmon = new FlowMonitorHelper;
        flowmon->SetMonitorAttribute("PacketSizeBinWidth", ns3::DoubleValue(20));
        flowmon->SetMonitorAttribute("JitterBinWidth"    , ns3::DoubleValue(0.001));
        flowmon->SetMonitorAttribute("DelayBinWidth"     , ns3::DoubleValue(0.001));
        monitor = flowmon->InstallAll();
}

void // Print per flow statistics
SimClass::RunFlowMonitor()
{
	monitor->CheckForLostPackets ();
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon->GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i=stats.begin (); 
		i != stats.end (); ++i) {
		// first 2 FlowIds are for ECHO apps, we don't want to display them
		if (i->first > 2) {
      			std::cout << "-----------------------------------------------\n";
			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
			std::cout << "Flow " << i->first - 2 << " (" << t.sourceAddress 
				  << " -> " << t.destinationAddress << ")\n";
			std::cout << "    Tx Bytes: " << i->second.txBytes << "\n";
      			std::cout << "    Rx Bytes: " << i->second.rxBytes << "\n";
			std::cout << "  Throughput: " 
				  << i->second.rxBytes *8.0/10.0/1024/1024 << " Mbps\n";
      			std::cout << "   R/S Ratio: " 
				  << (double)i->second.rxBytes/(double)i->second.txBytes << "\n";
		}
	}
}

void
SimClass::ConfigureCtsRts()
{
	if(enableCtsRts)
		NS_LOG_UNCOND("Hidden station experiment with RTS/CTS enabled: ");
	else
		NS_LOG_UNCOND("Hidden station experiment with RTS/CTS disabled:");

	// 0. Enable or disable CTS/RTS
	// if (data packet + LLC header + MAC header + FCS trailer) > RtsCtsThreshold, 
	// we use an RTS/CTS handshake before sending the data
	UintegerValue ctsThr = (enableCtsRts ? UintegerValue (100) : UintegerValue (2200));
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThr);
}

// Run single 10 seconds experiment with enabled or disabled RTS/CTS mechanism
int main (int argc, char **argv)
{
	SimClass sim;

	CommandLine cmd;
	cmd.AddValue("CtsRts", "experiment with RTS/CTS enabled(1)/disable(0).", sim.enableCtsRts);
        cmd.Parse(argc, argv);

	sim.ConfigureCtsRts();
	sim.CreateNetworkTopology ();
	sim.ConfigureDataLinkLayer();
	sim.ConfigureNetworkLayer ();

	// Install applications: two CBR streams each saturating the channel 
	sim.setUdpOnOffApp(sim.n[1], sim.n[0], "200", "3Mbps", SIM_START      , SIM_STOP);
	sim.setUdpOnOffApp(sim.n[2], sim.n[3], "200", "3Mbps", SIM_START+0.001, SIM_STOP);

	// we also use separate UDP applications that will send a single
	// packet before the CBR flows start. 
	sim.setUdpEchoApp(sim.n[1], sim.n[0], "10", "1", 0.001);
	sim.setUdpEchoApp(sim.n[2], sim.n[3], "10", "1", 0.005);

	sim.SetFlowMonitor();

	sim.ConfigureAnimation();
	std::string xf = std::string(PROG_DIR) + "hiden-xposed.xml";
        AnimationInterface anim (xf); 

	Simulator::Stop (Seconds (SIM_STOP));
	Simulator::Run ();
	sim.RunFlowMonitor();

	Simulator::Destroy ();
	return 0;
}

