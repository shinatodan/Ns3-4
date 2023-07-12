/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/waypoint-mobility-model.h"
#include "ns3/netanim-module.h"

#define	SIM_STOP 50

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Topology-test");

void 
TestGridPositionAllocator (std::string mode)
{
  	uint32_t nWifi = 9;
  	NodeContainer wifiStaNodes;
  	wifiStaNodes.Create (nWifi);

  	MobilityHelper mobility;
	if (mode == "RowFirst")
  		mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
		"MinX"      , DoubleValue (2.0),
		"MinY"      , DoubleValue (2.0),
		"DeltaX"    , DoubleValue (5.0),
		"DeltaY"    , DoubleValue (5.0),
		"GridWidth" , UintegerValue (3),
		//"LayoutType", StringValue ("RowFirst"));
		"LayoutType", EnumValue (ns3::GridPositionAllocator::ROW_FIRST));
	else
  		mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
		"MinX"      , DoubleValue (2.0),
		"MinY"      , DoubleValue (2.0),
		"DeltaX"    , DoubleValue (5.0),
		"DeltaY"    , DoubleValue (5.0),
		"GridWidth" , UintegerValue (3),
		//"LayoutType", StringValue ("ColumnFirst"));
		"LayoutType", EnumValue (ns3::GridPositionAllocator::COLUMN_FIRST));

  	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
 	mobility.Install (wifiStaNodes);

	char nodeId[10];
	for(uint32_t i=0;i <wifiStaNodes.GetN();i++) {
		sprintf(nodeId, "STA%d", i);
  		//AnimationInterface::SetNodeDescription (wifiStaNodes.Get(i), nodeId); // Optional
	}
  	//AnimationInterface::SetNodeColor (wifiStaNodes, 255, 0, 0); // Optional
}

void 
TestWaypointMove ()
{
  	uint32_t nSta = 3;
  	NodeContainer StaNodes;
  	StaNodes.Create (nSta);

  	NodeContainer mobileNodes;
	Ptr<Node> mn = CreateObject<Node> ();
	mobileNodes.Add(mn);

	for (size_t i = 0; i < StaNodes.GetN(); ++i) {
                StaNodes.Get (i)->AggregateObject (CreateObject<ConstantPositionMobilityModel> ());
        }

  	MobilityHelper mobility;
  	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
		"MinX"      , DoubleValue (2.0),
		"MinY"      , DoubleValue (2.0),
		"DeltaX"    , DoubleValue (5.0),
		"DeltaY"    , DoubleValue (0.0),
		"GridWidth" , UintegerValue (3),
		"LayoutType", StringValue ("RowFirst"));
  	//mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
 	mobility.Install (StaNodes);

	Ptr<WaypointMobilityModel> m_mob;
	m_mob = CreateObject<WaypointMobilityModel> ();
	mn->AggregateObject (m_mob);
	Waypoint wpt_start (Seconds (0.0), Vector (2.0, 10.0, 0.0));
	m_mob->AddWaypoint (wpt_start);
	Waypoint wpt_stop  (Seconds (SIM_STOP), Vector (12.0, 10.0, 0.0));
	m_mob->AddWaypoint (wpt_stop);

	char nodeId[10];
	for(uint32_t i=0;i <StaNodes.GetN();i++) {
		sprintf(nodeId, "STA%d", i);
  		//AnimationInterface::SetNodeDescription (StaNodes.Get(i), nodeId);
	}
  	//AnimationInterface::SetNodeColor (StaNodes, 0, 255, 0); // Optional
  	//AnimationInterface::SetNodeDescription (mn, "MN");
  	//AnimationInterface::SetNodeColor (mn, 0, 255, 0);
}

int
main (int argc, char *argv[])
{
  	CommandLine cmd;
  	cmd.Parse (argc,argv);

	std::string mode="RowFirst";
	//TestGridPositionAllocator (mode);
	TestWaypointMove ();

  	AnimationInterface anim ("topology-test.xml"); // Mandatory

  	Simulator::Stop (Seconds (SIM_STOP));
  	Simulator::Run ();
  	Simulator::Destroy ();
  	return 0;
}
