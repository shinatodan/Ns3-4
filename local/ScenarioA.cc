/*
 * 実験：ScenarioA
 * 
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/ipv4-header.h"
#include "ns3/packet.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-module.h"
#include "ns3/netanim-module.h"
#include "ns3/energy-module.h"          // エネルギー消費

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>

#define SIM_STOP 	8000.0
#define TH_INTERVAL     50.0
#define PDR_INTERVAL	50.0

#define NUM_FIXED_NODE 1
#define	NUM_MOVE_NODE	1

#define PROG_DIR        "mountain/data"


NS_LOG_COMPONENT_DEFINE ("test");

using namespace ns3;

class NetSim {
    public :
	NetSim();

	virtual void Configure(StringValue);
	virtual void CreateNetworkTopology ();
	virtual void ConfigureDataLinkLayer(bool, StringValue);
	virtual void ConfigureNetworkLayer ();
	virtual void ConfigureTransportLayerWithUDP (Ptr<Node>, uint32_t, Ptr<Node>, uint32_t);
	virtual void StartApplication(Ptr<Node>, DataRate, uint32_t);
	virtual void StartApplication(Ptr<Node>, uint32_t, uint32_t, double);

    // エネルギー関連
    EnergySourceContainer AttachEnergyModelToDevices(double, double, double);
    void TraceRemainingEnergy(uint32_t, EnergySourceContainer);
	void TraceTotalEnergy(uint32_t, EnergySourceContainer);

	Ptr<Node>    *n;
	Ptr<Node>    *mn;

	uint32_t totalReceived;
	uint32_t totalSent;
	
	double          elapse_time;
    double          last_time;
    double          last_totalReceived;
    double          this_Received;

    private :
	void GenerateTrafficWithDataRate (Ptr<Socket>, uint32_t, DataRate);
	void GenerateTrafficWithPktCount (Ptr<Socket>, uint32_t, uint32_t, Time);
	void SentPacketWithInterval		 (Ptr<Socket>, uint32_t, Time);
	void ReceivePacket   (Ptr<Socket>);
	InetSocketAddress setSocketAddress (Ptr<Node>, uint32_t);
	
    void GetRemainingEnergy (double, double);
    void TotalEnergy (double, double);			// 消費電力

	Ptr<Socket> source;
	Ptr<Socket> destination;

	uint32_t      nWifiNodes;
	NodeContainer  wlanNodes;
	NodeContainer   allNodes;
	NetDeviceContainer devices;	// 電力消費関係
	
	void traceThroughput ();
	void tracePDR ();
	
	void NotifyDrained();
	bool _energyDrained; // set it to true if energy is drained.
    double _remainingEnergy;
};

NetSim::NetSim() : totalReceived(0), totalSent(0)
{
	nWifiNodes = NUM_FIXED_NODE;
	
	//elapse_time = 0;
    //last_time = 0;
    last_totalReceived = 0.0;
    this_Received = 0.0;
}

// 電離残量が0になったときのメソッド
void
NetSim::NotifyDrained()
{
	std::cout <<"Energy was Drained. Stop send.\n";
	_energyDrained = true;
}

// エネルギーモデルを装着するメソッド
// エネルギーソースモデルを各ノードに装着した後、デバイスソースモデルを各ノードを装着する
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
        /* エネルギーソース側の電力が消費され尽くされた場合に呼び出されるコールバック関数を利用して
           NotifyDrainedメソッドを呼び出して、制御フラッグ_energyDrainedの設定を行う
           このフラグを利用して、送信やトレースの停止処理を行う */        
        radioEnergyHelper.SetDepletionCallback(
		MakeCallback(&NetSim::NotifyDrained, this));
        DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (devices, e);
	return e;
}


// 電力残量を出力するメソッド
// TraceRemaingEnergyメソッド中のコールバック関数より呼びだされる
void
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
// 電力残量をトレースするメソッド
// BasicEnergySourceのトレース機能のコールバック関数の呼び出しを行う
void
NetSim::TraceRemainingEnergy(uint32_t n, EnergySourceContainer e)
{
        // all energy sources are connected to resource node n>0
        Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (e.Get (n));
        basicSourcePtr->TraceConnectWithoutContext ("RemainingEnergy", 
               MakeCallback (&NetSim::GetRemainingEnergy, this));
}


// トータル電力消費量を出力するメソッド
void // Trace function for total energy consumption at node.
NetSim::TotalEnergy (double oldValue, double totalEnergy)
{
        if((Simulator::Now ().GetSeconds () > 5500) && (Simulator::Now ().GetSeconds () < 9700))
			std::cout << Simulator::Now ().GetSeconds () <<"\t" << totalEnergy << std::endl;
}

// トータル電力消費量をトレースするメソッド
// BasicEnergySourceのトレース機能のコールバック関数の呼び出しを行う
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


/*
// 平均スループットをトレースするメソッド
void
NetSim::traceThroughput ()
{
	double now = (double)Simulator::Now ().GetSeconds ();
    double th = (totalReceived*8)/now;
	std::cout << std::setw(10) << now << "\t" << th << std::endl;
	Simulator::Schedule (Seconds (TH_INTERVAL), &NetSim::traceThroughput, this);
}
*/



void
NetSim::tracePDR ()
{
	double now = (double)Simulator::Now ().GetSeconds ();

	//double pdr = totalReceived / totalSent;
	std::cout << now << "\t" << totalSent << "\t" << totalReceived << std::endl;
	//std::cout << std::setw(10) << now << "\t" << totalReceived << "\t" << totalSent << "\t" << pdr << std::endl;
	Simulator::Schedule( Seconds (PDR_INTERVAL), &NetSim::tracePDR, this);
}


// 瞬間スループットの出力(bps)
void
NetSim::traceThroughput ()
{
	double now = (double)Simulator::Now ().GetSeconds ();
    //    elapse_time = now - last_time;
    //    last_time = now;
        this_Received = totalReceived - last_totalReceived;
        last_totalReceived = totalReceived;
        double th = ( this_Received * 8) / TH_INTERVAL;
	std::cout << std::setw(10) << now << "\t"  << th << std::endl;
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
	Time tNext (Seconds (pktSize * 8 / static_cast<double> (dataRate.GetBitRate ())));
	Ptr<Packet> newPacket = Create<Packet> (pktSize);
	socket->Send (newPacket);
	Simulator::Schedule (tNext, &NetSim::GenerateTrafficWithDataRate, this, socket, pktSize, dataRate);
	totalSent ++;
}

// パケットを一定間隔ごとに指定した数まで送信するメソッド
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

// 一定間隔ごとにパケットを送信するメソッド(pktCountはなし) ※まだ動作確認していない
void
NetSim::SentPacketWithInterval(Ptr<Socket> socket, uint32_t pktSize, Time pktInterval)
{
        socket->Send (Create<Packet> (pktSize));
        Simulator::Schedule (pktInterval, &NetSim::SentPacketWithInterval, this,
                           socket, pktSize, pktInterval);
		totalSent ++;
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
	// disable fragmentation for frames below 2200 bytes
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
	// turn off RTS/CTS for frames below 2200 bytes
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
	// Fix non-unicast data rate to be the same as that of unicast
	//Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));
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
	allNodes.Add(wlanNodes);
	
	
	// 移動ノード
	mn = new Ptr<Node> [NUM_MOVE_NODE];
	for (uint32_t i = 0; i < NUM_MOVE_NODE; ++i){
		mn[i] = CreateObject<Node> ();
		allNodes.Add(mn[i]);
	}
}

void 
NetSim::ConfigureDataLinkLayer(bool verbose, StringValue phyMode)
{
	// The below set of helpers will help us to put together the wifi NICs we want
	WifiHelper wifi;
	if (verbose)
		wifi.EnableLogComponents ();  // Turn on all Wifi logging
	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
				      "DataMode"   , phyMode,
				      "ControlMode", phyMode);

	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss  ("ns3::DoubleDiffractionLossModel");

	YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
	wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 
	wifiPhy.SetChannel (wifiChannel.Create ());
	wifiPhy.Set("TxPowerStart", DoubleValue (20.0));	// 送信電力の最小値
	wifiPhy.Set("TxPowerEnd", DoubleValue (20.0));		// 送信電力の最大値
	wifiPhy.Set("EnergyDetectionThreshold", DoubleValue(-117.0)); // 電力検出閾値
	wifiPhy.Set("CcaMode1Threshold", DoubleValue(-117.0));

	// Add a non-QoS upper mac, and disable rate control
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	Ssid ssid = Ssid ("MySSID");
	wifiMac.SetType  ("ns3::AdhocWifiMac", "Ssid", SsidValue (ssid));
	devices = wifi.Install (wifiPhy, wifiMac, allNodes);

	
	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
        //for (uint32_t i = 0; i < nWifiNodes; ++i) 
        //positionAlloc->Add (Vector (-3802126.083, 3469869.295, 3756290.303)); // ni's position
        //positionAlloc->Add (Vector (-3802057.301	, 	3470280.218	, 	3755963.738));	// 分岐点
        //positionAlloc->Add (Vector (-3792415.714, 3492952.431, 3744356.91));
		//positionAlloc->Add (Vector (-3793119.517, 3491026.289, 3746501.736)); // 槍平小屋
		//positionAlloc->Add (Vector (-3792839.822, 3491917.531, 3745212.654));	// 中継
		//positionAlloc->Add (Vector (-3805346.638, 3468971.755, 3756298.344));		// 穂高岳山荘
		positionAlloc->Add (Vector (-3802395.335, 3469288.991, 3756665.149));	// y3
		
		
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wlanNodes);
	

		Ptr<WaypointMobilityModel> m1;
        m1 = CreateObject<WaypointMobilityModel> ();
        mn[0]->AggregateObject (m1);
        Waypoint m1_start  (Seconds (0.0), Vector (-3800244.615	, 	3473088.05	, 	3754476.076));		// 登山口
        m1->AddWaypoint  (m1_start);
        Waypoint m1_junc   (Seconds (7200.0), Vector (-3802057.301	, 	3470280.218	, 	3755963.738));	// 分岐点
        m1->AddWaypoint  (m1_junc);
        Waypoint m1_cp1   (Seconds (14400.0), Vector (-3803467.661	, 	3469793.668	, 	3755601.681));	// cp1
        m1->AddWaypoint  (m1_cp1);
        Waypoint m1_cp2   (Seconds (19800.0), Vector (-3803967.904	, 	3469542.857	, 	3755815.148));	// cp2
        m1->AddWaypoint  (m1_cp2);
        Waypoint m1_goal   (Seconds (32400.0), Vector (-3805346.638	, 	3468971.755	, 	3756298.344));	// 穂高山荘
        m1->AddWaypoint  (m1_goal);


	


	//wifiPhy.EnablePcap ("simpleAdhoc", devices);
}

void 
NetSim::ConfigureNetworkLayer ()
{
	InternetStackHelper internet;
	AodvHelper aodv; // use AODV routing protocol
	aodv.Set("HelloInterval",TimeValue(Seconds(150)));
	internet.SetRoutingHelper(aodv);
	internet.Install (allNodes);

	Ipv4AddressHelper ipv4;
	ipv4.SetBase ("192.168.1.0", "255.255.255.0");
	Ipv4InterfaceContainer ifs = ipv4.Assign (devices);
}

void 
NetSim::ConfigureTransportLayerWithUDP (Ptr<Node> src, uint32_t src_port, Ptr<Node> dst, uint32_t dst_port)
{
	// set receiver(mn)'s socket
	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

	Ptr<Socket> destination = Socket::CreateSocket (dst, tid);
        InetSocketAddress dstSocketAddr = setSocketAddress (dst, dst_port);
	destination->Bind (dstSocketAddr);
	destination->SetRecvCallback (MakeCallback (&NetSim::ReceivePacket, this));

	// set sender(n0)'s socket
	//Ptr<Socket> source = Socket::CreateSocket (src, tid);
	source = Socket::CreateSocket (src, tid);
        InetSocketAddress srcSocketAddr = setSocketAddress (src, src_port);
	source->Bind (srcSocketAddr);
	source->SetAllowBroadcast (true);
	source->Connect (dstSocketAddr);
}

void
NetSim::StartApplication(Ptr<Node> src, DataRate dataRate, uint32_t packetSize)
{
	Simulator::ScheduleWithContext (src->GetId (),
		Seconds (1.0), &NetSim::GenerateTrafficWithDataRate, this, 
		NetSim::source, packetSize, dataRate);
	//Simulator::Schedule (Seconds(PDR_INTERVAL), &NetSim::tracePDR, this);
}

void
NetSim::StartApplication(Ptr<Node> src, uint32_t packetSize, uint32_t numPkt, double interval)
{
	Time interPacketInterval = Seconds (interval);
	Simulator::ScheduleWithContext (src->GetId (),
		Seconds (1.0), &NetSim::GenerateTrafficWithPktCount, this, 
		NetSim::source, packetSize, numPkt, interPacketInterval);
}

int 
main (int argc, char *argv[])
{
	NetSim sim;

	std::string phyMode ("DsssRate1Mbps");
	bool        verbose    = false;
	bool        sendFlag   = false; // send pkts with dataRate	
    std::string traceFlag ("total"); 				// trace total energy consumption
	DataRate    dataRate   = DataRate ("2.4Kbps");  // sendFlag = true(1)のときに送信するパケットのデータレート
	uint32_t    packetSize = 1024;                  // bytes
	uint32_t    numPackets = 1000;                  // senfFlag = false(0)のときに送信するパケット数
	double      interval   = 10.0;                  // seconds
    uint32_t    traceNum   = 1;						// 消費電力をトレースするノードの配列番号
    Ptr<Node>	srcNode;							// パケット送信ノード

	CommandLine cmd;
	cmd.AddValue ("phyMode"   , "Wifi Phy mode", phyMode);
	cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
	cmd.AddValue ("dataRate"  , "dataRate of application packet sent", dataRate);
	cmd.AddValue ("numPackets", "number of packets generated", numPackets);
	cmd.AddValue ("interval"  , "interval (seconds) between packets", interval);
	cmd.AddValue ("sendFlag"  , "send flag (1: use pkt data rate, 0: use number of pkts)", sendFlag);
	cmd.AddValue ("verbose"   , "turn on all WifiNetDevice log components", verbose);
    cmd.AddValue ("traceNode" , "trace Node's number", traceNum);
	cmd.Parse (argc, argv);
	
	sim.Configure(phyMode);
	sim.CreateNetworkTopology();
	sim.ConfigureDataLinkLayer(verbose, phyMode);

    // エネルギーモデルの装着部とトレース部を呼び出す
	EnergySourceContainer e = sim.AttachEnergyModelToDevices(10000, 0.0174, 0.0197);
	if(traceFlag == "remained")
		sim.TraceRemainingEnergy(traceNum, e);
	else
		sim.TraceTotalEnergy(traceNum, e);
	
	sim.ConfigureNetworkLayer ();
	sim.ConfigureTransportLayerWithUDP(sim.mn[0], 8888, sim.n[0], 8888);


	if(sendFlag)
		sim.StartApplication (sim.mn[0], dataRate, packetSize);
	else
		sim.StartApplication (sim.mn[0], packetSize, numPackets, interval);


	std::string xf = std::string(PROG_DIR) + "simpleAdhoc.xml";
	AnimationInterface anim (xf);
	anim.EnablePacketMetadata (true);
	
	// 時間の計測
	//auto start = std::chrono::system_clock::now();  

	Simulator::Stop (Seconds (SIM_STOP));
	Simulator::Run ();
	Simulator::Destroy ();

/*
	auto end = std::chrono::system_clock::now();
	auto dur = end - start;
	auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();	
	std::cout << msec << " milli sec \n";
*/

	 NS_LOG_UNCOND ("    Total packets sent: " << sim.totalSent);
	 NS_LOG_UNCOND ("Total packets received: " << sim.totalReceived);

	return 0;
}

