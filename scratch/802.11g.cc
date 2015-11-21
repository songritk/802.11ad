/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Daniel D. Costa <danieldouradocosta@gmail.com>

 Default Network Topology

  WIFI           LTE

  TX1            TX2
   |              |
   |              |
   AP            enb
   *              *
   -              -
   -              -
   - - - - ue - - -

*/

#include "ns3/lte-module.h"
#include "ns3/lte-helper.h"
#include "ns3/core-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/internet-module.h"
#include "ns3/config-store-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("debug");

void PrintLocations (NodeContainer,std::string);
void PrintAddresses(Ipv4InterfaceContainer,std::string);


int main (int argc, char *argv[])
{
	double simulationTime			= 3;

	bool verbose 					= true;

	double wifiServerStartTime		= 0.01;
	double wifiClientStartTime		= 1;

	uint16_t dlPort 				= 12345;
	uint16_t ulPort 				= 9;
	uint32_t maxTCPBytes 			= 0;

	double udpPacketInterval 		= 1;

	bool   useUdp					= true;

	bool   useDl					= true;
	bool   useUl					= false;

	bool   useWIFI					= true;

	CommandLine cmd;
	cmd.AddValue("simulationTime", "Simulation Time: ", simulationTime);
	cmd.Parse (argc, argv);
	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults ();
	cmd.Parse (argc, argv);

	// disable fragmentation for frames below 2200 bytes
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
	// turn off RTS/CTS for frames below 2200 bytes
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));

	if (verbose)
		LogComponentEnable("PacketSink", LOG_LEVEL_INFO);


	Ptr<ListPositionAllocator> 	positionAlloc 	= CreateObject<ListPositionAllocator> ();

	PointToPointHelper 			p2p;
	Ipv4AddressHelper 			ipv4;
	InternetStackHelper 		internet;
	MobilityHelper 				mobility;
	Ipv4StaticRoutingHelper 	ipv4RoutingHelper;


///////////////////////////////////////////////////////////
	NS_LOG_LOGIC ("->Initializing Remote Host WIFI (TX1)...");
///////////////////////////////////////////////////////////

	NS_LOG_LOGIC ("Creating p2pNodes WIFI...");
	NodeContainer remoteHostWIFIContainer;
	remoteHostWIFIContainer.Create (2);

	internet.Install(remoteHostWIFIContainer); //txNodeWifi and wifiApNode

	NS_LOG_LOGIC ("Creating pointToPoint WIFI...");
	p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1Gb/s")));
	p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
	p2p.SetDeviceAttribute ("Mtu", UintegerValue (1500));

	NS_LOG_LOGIC ("Creating p2pDevices WIFI...");
	NetDeviceContainer p2pDevicesWIFI;
	p2pDevicesWIFI = p2p.Install (remoteHostWIFIContainer);

	Ptr<Node> remoteHostWIFI 	= remoteHostWIFIContainer.Get (0);
	Ptr<Node> wifiApNode 		= remoteHostWIFIContainer.Get (1);

///////////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Creating UE/Enb Node...");
///////////////////////////////////////////////////////////

	NodeContainer ueNodeContainer;
	ueNodeContainer.Create (1);

	internet.Install(ueNodeContainer);

	Ptr<Node> ueNode 		= ueNodeContainer.Get (0);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_LOGIC ("==> Initializing WifiHelper...");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

    NqosWifiMacHelper 		mac 	= NqosWifiMacHelper::Default ();
    YansWifiChannelHelper 	channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper 		phy 	= YansWifiPhyHelper::Default ();
	WifiHelper 				wifi 	= WifiHelper::Default ();

	Ssid ssid = Ssid ("ns3-80211g");

    wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
    phy.SetChannel (channel.Create ());

    mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
    NetDeviceContainer ueWifiDevice;
    ueWifiDevice = wifi.Install (phy, mac, ueNode);

    //mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
    mac.SetType ("ns3::ApWifiMac","Ssid", SsidValue (ssid),	"BeaconGeneration", BooleanValue (true),"BeaconInterval", TimeValue (Seconds (2.5)));
    NetDeviceContainer wifiApdevice;
    wifiApdevice = wifi.Install (phy, mac, wifiApNode);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_LOGIC ("==> Initializing WIFI Mobility...");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Installing mobility on remoteHostWIFI...");
	positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (0.0, 0.0, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (remoteHostWIFI);

	NS_LOG_UNCOND ("Installing mobility on wifiApNode...");
	positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (0.0, 20.0, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiApNode);

	NS_LOG_UNCOND ("Installing mobility on ueNode...");
	positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (15.0, 0.0, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    //mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel","Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)));
	mobility.Install (ueNode);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	   NS_LOG_LOGIC ("==> Initializing WIFI InternetStackHelper...");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	ipv4.SetBase ("60.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer wifiTxIPInterface = ipv4.Assign (p2pDevicesWIFI);

    ipv4.SetBase ("192.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer wifiApInterface 		= ipv4.Assign (wifiApdevice);
    Ipv4InterfaceContainer wifiUeIPInterface 	= ipv4.Assign (ueWifiDevice);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_LOGIC ("==> Initializing Applications...");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

    ApplicationContainer wifiClient;
    ApplicationContainer wifiServer;

    Ipv4Address remoteHostWIFIAddr 	= wifiTxIPInterface.GetAddress (0);
    Ipv4Address ueWIFIAddr 			= wifiUeIPInterface.GetAddress (0);

    if(useUdp)
    {
    	if(useWIFI)
    	{
			if(useDl)
			{
				NS_LOG_LOGIC ("Installing UDP DL app for UE WIFI ");
				PacketSinkHelper dlPacketSinkHelperWIFI ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
				wifiServer.Add (dlPacketSinkHelperWIFI.Install (ueNode));

				UdpClientHelper dlClientWIFI (ueWIFIAddr, dlPort);
				dlClientWIFI.SetAttribute ("Interval", TimeValue (MilliSeconds(udpPacketInterval)));
				dlClientWIFI.SetAttribute ("MaxPackets", UintegerValue(1000000));
				wifiClient.Add (dlClientWIFI.Install (remoteHostWIFI));
			}
			if(useUl)
			{
				NS_LOG_LOGIC ("Installing UDP UL app for UE WIFI ");
				PacketSinkHelper ulPacketSinkHelperWIFI ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
				wifiServer.Add (ulPacketSinkHelperWIFI.Install (remoteHostWIFI));

				UdpClientHelper ulClientWIFI (remoteHostWIFIAddr, ulPort);
				ulClientWIFI.SetAttribute ("Interval", TimeValue (MilliSeconds(udpPacketInterval)));
				ulClientWIFI.SetAttribute ("MaxPackets", UintegerValue(1000000));
				wifiClient.Add (ulClientWIFI.Install (ueNode));
			}
    	}

    	dlPort++;
    	ulPort++;
    }
    else
    {
    	if(useWIFI)
    	{
			if(useDl)
			{
			  PacketSinkHelper sinkDl ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
			  wifiServer.Add (sinkDl.Install (ueNode));

			  BulkSendHelper sourceDl ("ns3::TcpSocketFactory", InetSocketAddress (ueWIFIAddr, dlPort));
			  sourceDl.SetAttribute ("MaxBytes", UintegerValue (maxTCPBytes));
			  sourceDl.SetAttribute ("SendSize", UintegerValue (10000));
			  wifiClient.Add (sourceDl.Install (remoteHostWIFI));
			}
			if(useUl)
			{
			  PacketSinkHelper sinkUl ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
			  wifiServer.Add (sinkUl.Install (remoteHostWIFI));

			  BulkSendHelper sourceUl ("ns3::TcpSocketFactory", InetSocketAddress (remoteHostWIFIAddr, ulPort));
			  sourceUl.SetAttribute ("MaxBytes", UintegerValue (maxTCPBytes));
			  sourceUl.SetAttribute ("SendSize", UintegerValue (10000));
			  wifiClient.Add (sourceUl.Install (ueNode));
			}
    	}

    	dlPort++;
    	ulPort++;
    }

	wifiServer.Start (Seconds (wifiServerStartTime));
	wifiClient.Start (Seconds (wifiClientStartTime));

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
	NS_LOG_LOGIC ("==>Running Simulation...");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	Simulator::Stop (Seconds (simulationTime));

	//phy.EnablePcapAll ("temp", true);

	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();

	PrintAddresses(wifiTxIPInterface, "IP addresses of WIFI Remote Host (TX1)");
	PrintAddresses(wifiApInterface, "IP address of AP");
	PrintAddresses(wifiUeIPInterface, "IP addresses of wifi base stations(RX1)");
	PrintLocations(remoteHostWIFI, "Location of WIFI Remote Host");
	PrintLocations(wifiApNode, "Location of WIFI AP");
	PrintLocations(ueNode, "Location of StatNodes");

 	AnimationInterface anim ("debug_anim.xml");
 	anim.SetConstantPosition (remoteHostWIFI, 0.0, 0.0);
 	anim.SetConstantPosition (wifiApNode, 0.0, 20.0);
 	anim.SetConstantPosition (ueNode, 15.0, 30.0);

	Simulator::Run ();
	monitor->CheckForLostPackets ();
	monitor->SerializeToXmlFile ("debug_monitor.xml", true, true);
	Simulator::Destroy ();

	return 0;
}

void PrintLocations (NodeContainer nodes, std::string header)
{
	std::cout << header << std::endl;
	for(NodeContainer::Iterator iNode = nodes.Begin (); iNode != nodes.End (); ++iNode)
	{
		Ptr<Node> object = *iNode;
		Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		NS_ASSERT (position != 0);
		Vector pos = position->GetPosition ();
		std::cout << "(" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
	}

	std::cout << std::endl;
}

void PrintAddresses(Ipv4InterfaceContainer container, std::string header)
{
	std::cout << header << std::endl;
	uint32_t nNodes = container.GetN ();

	for (uint32_t i = 0; i < nNodes; ++i)
		std::cout << container.GetAddress(i, 0) << std::endl;

	std::cout << std::endl;
}
