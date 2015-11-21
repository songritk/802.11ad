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

void ActivateDedicatedEpsBearer(uint16_t,uint16_t,NetDeviceContainer,Ptr<LteHelper>);
void PrintLocations (NodeContainer,std::string);
void PrintAddresses(Ipv4InterfaceContainer,std::string);


int main (int argc, char *argv[])
{
	bool verbose 					= true;
	bool tracing					= false;

	double simulationTime			= 2;

	double wifiServerStartTime		= 0.01;
	double wifiClientStartTime		= 1;

	double lteServerStartTime		= 0.01;
	double lteClientStartTime		= 1;

	uint32_t distanceUe				= 10;

	uint16_t dlPort 				= 12345;
	uint16_t ulPort 				= 9;

	uint32_t maxTCPBytes 			= 0;
	double udpPacketInterval 		= 100;

	bool   useUdp					= false;

	bool   useDl					= true;
	bool   useUl					= false;

	bool   useAppWIFI				= true;
	bool   useAppLTE				= false;

	std::string outFile ("debug");
	std::string p2pWifiRate ("1Gbps");
	std::string p2pLteRate ("1Gbps");

	CommandLine cmd;
	cmd.AddValue("simulationTime", "Simulation Time: ", simulationTime);
	cmd.AddValue("verbose", "verbose: ", verbose);
	cmd.Parse (argc, argv);
	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults ();
	cmd.Parse (argc, argv);

	// disable fragmentation for frames below 2200 bytes
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
	// turn off RTS/CTS for frames below 2200 bytes // // enable rts cts all the time.=0
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));

	if (useDl==false && useUl==false)
	{
	  std::cout << "===> Downlink or Uplink have to be enable!!! <===" << std::endl;
	  return 1;
	}

	if (useAppWIFI==false && useAppLTE==false)
	{
	  std::cout << "===> WIFI apps or LTE apps have to be enable!!! <===" << std::endl;
	  return 1;
	}

	if (verbose)
		LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

///////////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Variables");
///////////////////////////////////////////////////////////

	Ptr<ListPositionAllocator> 	positionAlloc 	= CreateObject<ListPositionAllocator> ();

	PointToPointHelper 			p2p;
	Ipv4AddressHelper 			ipv4;
	InternetStackHelper 		internet;
	MobilityHelper 				mobility;
	Ipv4StaticRoutingHelper 	ipv4RoutingHelper;


///////////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Remote Host WIFI (TX1)");
///////////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Creating p2pNodes WIFI");
	NodeContainer remoteHostWIFIContainer;
	remoteHostWIFIContainer.Create (2);

	internet.Install(remoteHostWIFIContainer); //txNodeWifi and wifiApNode

	NS_LOG_UNCOND ("Creating pointToPoint WIFI");
	p2p.SetDeviceAttribute ("DataRate", StringValue (p2pWifiRate));
	p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

	NS_LOG_UNCOND ("Creating p2pDevices WIFI");
	NetDeviceContainer p2pDevicesWIFI;
	p2pDevicesWIFI = p2p.Install (remoteHostWIFIContainer);

	Ptr<Node> remoteHostWIFI 	= remoteHostWIFIContainer.Get (0);
	Ptr<Node> wifiApNode 		= remoteHostWIFIContainer.Get (1);

///////////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Creating UE/Enb Node");
///////////////////////////////////////////////////////////

	NodeContainer ueNodeContainer;
	NodeContainer enbNodeContainer;
	ueNodeContainer.Create (1);
	enbNodeContainer.Create (1);

	internet.Install(ueNodeContainer);

	Ptr<Node> ueNode 		= ueNodeContainer.Get (0);
	Ptr<Node> enbNode 		= enbNodeContainer.Get (0);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Wifi");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

    //NqosWifiMacHelper 		mac 	= NqosWifiMacHelper::Default (); //802.11a,b,g
    //HtWifiMacHelper 		mac 	= HtWifiMacHelper::Default (); //802.11n
	VhtWifiMacHelper 		mac 	= VhtWifiMacHelper::Default (); //802.11ac/ad
    YansWifiChannelHelper 	channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper 		phy 	= YansWifiPhyHelper::Default ();
	WifiHelper 				wifi 	= WifiHelper::Default ();

	Ssid ssid = Ssid ("ns3-wifi");

	//wifi.SetStandard (WIFI_PHY_STANDARD_80211ac);
    wifi.SetStandard (WIFI_PHY_STANDARD_80211ad_OFDM);
    phy.SetChannel (channel.Create ());

    NetDeviceContainer wifiApdevice;
    mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
    wifiApdevice = wifi.Install (phy, mac, wifiApNode);

    NetDeviceContainer ueWifiDevice;
    mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
    ueWifiDevice = wifi.Install (phy, mac, ueNode);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing WIFI Mobility");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Installing mobility on remoteHostWIFI");
	positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (0.0, -20.0, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (remoteHostWIFI);

	NS_LOG_UNCOND ("Installing mobility on wifiApNode");
	positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (0.0, 0.0, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiApNode);

	NS_LOG_UNCOND ("Installing mobility on ueNode");
	positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (15.0, distanceUe, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (ueNode);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	   NS_LOG_UNCOND ("==> Initializing WIFI InternetStackHelper");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	ipv4.SetBase ("60.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer wifiTxIPInterface = ipv4.Assign (p2pDevicesWIFI);

    ipv4.SetBase ("192.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer wifiApInterface 		= ipv4.Assign (wifiApdevice);
    Ipv4InterfaceContainer wifiUeIPInterface 	= ipv4.Assign (ueWifiDevice);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing WIFI Routing Tables");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// LTE ////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Remote Host LTE");
///////////////////////////////////////////////////////////

	Ptr<LteHelper> 				lteHelper 		= CreateObject<LteHelper> ();
	Ptr<PointToPointEpcHelper>  epcHelper 		= CreateObject<PointToPointEpcHelper> ();
	Ptr<Node> 					pgw 			= epcHelper->GetPgwNode ();

	NS_LOG_UNCOND ("Enabling EPC");
	lteHelper->SetEpcHelper (epcHelper);

	NS_LOG_UNCOND ("Creating remote host LTE");
	NodeContainer remoteHostLteContainer;
	remoteHostLteContainer.Create (1);

	internet.Install(remoteHostLteContainer);

	NS_LOG_UNCOND ("Creating pointToPoint LTE");
	p2p.SetDeviceAttribute ("DataRate", StringValue (p2pLteRate));
	p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

	Ptr<Node> remoteHostLTE = remoteHostLteContainer.Get (0);

	NetDeviceContainer internetDevices;
	internetDevices = p2p.Install (pgw, remoteHostLTE);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	 NS_LOG_UNCOND ("==> Initializing LTE Mobility");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Installing mobility on remoteHostLTE");
	positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (30.0, -20.0, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (remoteHostLTE);

	NS_LOG_UNCOND ("Installing mobility on PGW");
	positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (40.0, -10.0, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (pgw);

	NS_LOG_UNCOND ("Installing mobility on enbNode");
	positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (30.0, 0.0, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (enbNode);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Install LTE Devices to the nodes");
/////////////////////////////////////////////////////
//////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Installing NetDeviceContainer LTE");
	NetDeviceContainer enbDevive = lteHelper->InstallEnbDevice (enbNode);
	NetDeviceContainer ueLteDevive = lteHelper->InstallUeDevice (ueNode);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	 NS_LOG_UNCOND ("==> Setting LTE IP Addresses");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Setting LTE Remote Host IP Address");
	ipv4.SetBase ("200.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer lteTxIPInterface = ipv4.Assign (internetDevices);

	NS_LOG_UNCOND ("Setting LTE UE IP Address");
	Ipv4InterfaceContainer lteUeIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevive));//7.0.0.0

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing LTE Routing Tables");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHostLTE->GetObject<Ipv4> ());
	remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

	Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
	ueStaticRouting->AddNetworkRouteTo (Ipv4Address ("200.0.0.0"), Ipv4Mask ("255.0.0.0"), 2);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Atacching Ue to Enb");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	lteHelper->Attach (ueLteDevive.Get (0), enbDevive.Get (0));

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Setting Up Applications");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

    ApplicationContainer wifiClient;
    ApplicationContainer wifiServer;
	ApplicationContainer lteClient;
	ApplicationContainer lteServer;

    Ipv4Address remoteHostWIFIAddr 	= wifiTxIPInterface.GetAddress (0);
    Ipv4Address ueWIFIAddr 			= wifiUeIPInterface.GetAddress (0);

    Ipv4Address remoteHostLTEAddr 	= lteTxIPInterface.GetAddress (1);
    Ipv4Address ueLTEAddr 			= lteUeIpIface.GetAddress (0);

    if(useUdp)
    {
    	if(useAppWIFI)
    	{
			if(useDl)
			{
				NS_LOG_UNCOND ("Installing UDP DL app for UE WIFI ");
				PacketSinkHelper dlPacketSinkHelperWIFI ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
				wifiServer.Add (dlPacketSinkHelperWIFI.Install (ueNode));

				UdpClientHelper dlClientWIFI (ueWIFIAddr, dlPort);
				dlClientWIFI.SetAttribute ("Interval", TimeValue (MilliSeconds(udpPacketInterval)));
				dlClientWIFI.SetAttribute ("MaxPackets", UintegerValue(1000000));
				wifiClient.Add (dlClientWIFI.Install (remoteHostWIFI));
			}
			if(useUl)
			{
				NS_LOG_UNCOND ("Installing UDP UL app for UE WIFI ");
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

    	if(useAppLTE)
    	{
    		if(useDl)
    		{
				NS_LOG_UNCOND ("Installing UDP DL app for UE LTE ");
				PacketSinkHelper dlPacketSinkHelperLTE ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
				lteServer.Add (dlPacketSinkHelperLTE.Install (ueNode));

				UdpClientHelper dlClientLTE (ueLTEAddr, dlPort);
				dlClientLTE.SetAttribute ("Interval", TimeValue (MilliSeconds(udpPacketInterval)));
				dlClientLTE.SetAttribute ("MaxPackets", UintegerValue(1000000));
				lteClient.Add (dlClientLTE.Install (remoteHostLTE));
    		}
    		if(useUl)
    		{
				NS_LOG_UNCOND ("Installing UDP UL app for UE LTE ");
				PacketSinkHelper ulPacketSinkHelperLTE ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
				lteServer.Add (ulPacketSinkHelperLTE.Install (remoteHostLTE));

				UdpClientHelper ulClientLTE (remoteHostLTEAddr, ulPort);
				ulClientLTE.SetAttribute ("Interval", TimeValue (MilliSeconds(udpPacketInterval)));
				ulClientLTE.SetAttribute ("MaxPackets", UintegerValue(1000000));
				lteClient.Add (ulClientLTE.Install (ueNode));
    		}

    		NS_LOG_UNCOND ("==> Activating a data radio bearer LTE UDP");
    		ActivateDedicatedEpsBearer(dlPort, ulPort, ueLteDevive, lteHelper);
    	}
    }
    else
    {
    	if(useAppWIFI)
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

    	if(useAppLTE)
    	{
    		if(useDl)
    		{
  			  NS_LOG_UNCOND ("Installing TCP DL app for UE LTE");
  			  PacketSinkHelper sinkDl ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
  			  wifiServer.Add (sinkDl.Install (ueNode));

  			  BulkSendHelper sourceDl ("ns3::TcpSocketFactory", InetSocketAddress (ueLTEAddr, dlPort));
  			  sourceDl.SetAttribute ("MaxBytes", UintegerValue (maxTCPBytes));
  			  sourceDl.SetAttribute ("SendSize", UintegerValue (10000));
  			  wifiClient.Add (sourceDl.Install (remoteHostLTE));
    		}
    		if(useUl)
    		{
  			  NS_LOG_UNCOND ("Installing TCP UL app for UE LTE");
  			  PacketSinkHelper sinkUl ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
  			  wifiServer.Add (sinkUl.Install (remoteHostLTE));

  			  BulkSendHelper sourceUl ("ns3::TcpSocketFactory", InetSocketAddress (remoteHostLTEAddr, ulPort));
  			  sourceUl.SetAttribute ("MaxBytes", UintegerValue (maxTCPBytes));
  			  sourceUl.SetAttribute ("SendSize", UintegerValue (10000));
  			  wifiClient.Add (sourceUl.Install (ueNode));
    		}

    		NS_LOG_UNCOND ("==> Activating a data radio bearer LTE TCP");
    		ActivateDedicatedEpsBearer(dlPort, ulPort, ueLteDevive, lteHelper);
    	}
    }

    NS_LOG_UNCOND ("Starting Applications");
	if(useAppWIFI)
	{
		wifiServer.Start (Seconds (wifiServerStartTime));
		wifiClient.Start (Seconds (wifiClientStartTime));
	}

	if(useAppLTE)
	{
		lteServer.Start (Seconds (lteServerStartTime));
		lteClient.Start (Seconds (lteClientStartTime));
	}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Running Simulation");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	Simulator::Stop (Seconds (simulationTime));

	if (tracing == true)
	{
	  p2p.EnablePcapAll (outFile);
	  phy.EnablePcap (outFile, wifiApdevice.Get (0));
	  phy.EnablePcapAll (outFile, true);
	  lteHelper->EnableTraces ();
	}

	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();

	PrintAddresses(wifiTxIPInterface, "IP addresses of WIFI Remote Host (TX1)");
	PrintAddresses(wifiApInterface, "IP address of AP");
	PrintAddresses(wifiUeIPInterface, "IP addresses of wifi base stations(RX1)");
	PrintAddresses(lteUeIpIface, "IP addresses of lte base station(RX2)");
	PrintAddresses(lteTxIPInterface, "IP addresses of LTE Remote HOst (TX2)");
	//PrintAddresses(enbIpIface, "IP addresses of Enb");
	PrintLocations(remoteHostWIFI, "Location of WIFI Remote Host");
	PrintLocations(wifiApNode, "Location of WIFI AP");
	PrintLocations(ueNode, "Location of StatNodes");
	PrintLocations(remoteHostLTE, "Location of LTE Remote Host");
	PrintLocations(enbNode, "Location of enb");

 	AnimationInterface anim (outFile+"_anim.xml");
 	//positions WIFI
 	anim.SetConstantPosition (remoteHostWIFI, 0.0, -20.0);
 	anim.SetConstantPosition (wifiApNode, 0.0, 0.0);
 	anim.SetConstantPosition (ueNode, 15.0, distanceUe);
 	//positions LTE
 	anim.SetConstantPosition (remoteHostLTE, 30.0, -20.0);
 	anim.SetConstantPosition (pgw, 40.0, -10.0);
 	anim.SetConstantPosition (enbNode, 30.0, 0.0);

	Simulator::Run ();
	monitor->CheckForLostPackets ();
	monitor->SerializeToXmlFile (outFile+"_monitor.xml", true, true);
	Simulator::Destroy ();

	return 0;
}

void ActivateDedicatedEpsBearer(uint16_t dlPort, uint16_t ulPort, NetDeviceContainer ueLteDevive, Ptr<LteHelper> lteHelper)
{
	Ptr<EpcTft> tft = Create<EpcTft> ();

	//download
	EpcTft::PacketFilter dlpf;
	dlpf.localPortStart = dlPort;
	dlpf.localPortEnd = dlPort;
	tft->Add (dlpf);

	//upload
	EpcTft::PacketFilter ulpf;
	ulpf.remotePortStart = ulPort;
	ulpf.remotePortEnd = ulPort;
	tft->Add (ulpf);

	EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
	lteHelper->ActivateDedicatedEpsBearer (ueLteDevive.Get (0), bearer, tft);
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
