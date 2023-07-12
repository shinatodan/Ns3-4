#include "simpleMeshLib.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

NS_LOG_COMPONENT_DEFINE ("exp06-SimpleMesh");

using namespace ns3;

int main (int argc, char *argv[])
{
	MeshDot11sSim sim;
	ns3::PacketMetadata::Enable ();

	sim.RunSim(argc, argv);

	return 0;
}

