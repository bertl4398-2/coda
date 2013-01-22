/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/** 
 * Copyright (c) Francisco de Meneses Neves Ramos dos Santos
 * Email: francisco.santos@epfl.ch
 * Date: 7/9/2012
 */

//
// This program configures a grid (default 5x5) of nodes on an
// 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000
// (application) bytes to node 1.
//
// The default layout is like this, on a 2-D grid.
//
// n20  n21  n22  n23  n24
// n15  n16  n17  n18  n19
// n10  n11  n12  n13  n14
// n5   n6   n7   n8   n9
// n0   n1   n2   n3   n4
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
// When you are done tracing, you will notice many pcap trace files
// in your directory.  If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-grid-0-0.pcap -nn -tt
//
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/dce-module.h"
#include "ns3/netanim-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

NS_LOG_COMPONENT_DEFINE("WifiSimpleAdhocGrid");

using namespace ns3;

void CreateContent() {
	std::ofstream osf("/tmp/README", std::fstream::trunc);

	osf << "The wanted data is here :)";

	osf.close();
}

static std::string GetAddressFromNode(NodeContainer& nodes, uint32_t nodeId) {
	std::ostringstream oss("");
	if (nodeId < nodes.GetN()) {
		Ptr<Ipv4> ipv4 = nodes.Get(nodeId)->GetObject<Ipv4>();
		if (ipv4->GetNInterfaces() >= 1) {
			Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1, 0);
			Ipv4Address addri = iaddr.GetLocal();
			addri.Print(oss);
		}
	}
	return oss.str();
}

int main(int argc, char *argv[]) {
	std::string phyMode("DsssRate1Mbps");
	uint32_t numNodes = 25; // by default, 5x5
	uint32_t ccndStartTime = 0.2; // seconds
	uint32_t ccndcStartTime = 5; // seconds
	uint32_t simStopTime = 1000; // seconds
	uint32_t ccndStopTime = 999; // seconds
	uint32_t contentLifetime = 20; // seconds
	uint32_t pubTime = 50; // seconds
	uint32_t getTime = 60; // seconds
	uint32_t sinkNode = numNodes - 1; // node id
	uint32_t sourceNode = 0; // node id
	uint32_t routingTableUpdate = 10; // every 10 s
	double centerSquare = 1000.0; // center of square located at X=1000m and Y=1000m
	double reqInterval = 25.0; // seconds
	double timeChange = 2.0; // seconds before nodes change
	std::string speed = "Constant:200.0";
	bool verbose = false;
	bool tracing = true;
	std::string animFile = "mobility.xml";
	const int numberRequests = 30;

	CommandLine cmd;

	cmd.AddValue("phyMode", "Wifi Phy mode (default: DsssRate1Mbps).", phyMode);
	cmd.AddValue("verbose", "Turn on all WifiNetDevice log components (default: false).",
			verbose);
	cmd.AddValue("tracing", "Activate pcap tracing (default: true)", tracing);
	cmd.AddValue("numNodes", "number of nodes", numNodes);
	cmd.AddValue("centerSquare",
			"Nodes are spread out in a radius of X meters (default: 1000)",
			centerSquare);
	cmd.AddValue("timeChange",
			"Time in secs before a node changes speed and direction (default: 2)",
			timeChange);
	cmd.AddValue("speed", "Node speed in m/s (default: Constant:200.0)", speed);
	cmd.AddValue("sinkNode", "Receiver node number", sinkNode);
	cmd.AddValue("sourceNode", "Sender node number", sourceNode);

	cmd.Parse(argc, argv);

	// disable fragmentation for frames below 2200 bytes
	Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold",
			StringValue("2200"));
	// turn off RTS/CTS for frames below 2200 bytes
	Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
			StringValue("2200"));
	// Fix non-unicast data rate to be the same as that of unicast
	Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",
			StringValue(phyMode));

	NodeContainer nodes;
	nodes.Create(numNodes);

	// The below set of helpers will help us to put together the wifi NICs we want
	WifiHelper wifi;

	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	// set it to zero; otherwise, gain will be added
	wifiPhy.Set("RxGain", DoubleValue(-10));
	// ns-3 supports RadioTap and Prism tracing extensions for 802.11b
	wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");
	wifiPhy.SetChannel(wifiChannel.Create());

	// Add a non-QoS upper mac, and disable rate control
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
	wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode",
			StringValue(phyMode), "ControlMode", StringValue(phyMode));
	// Set it to adhoc mode
	wifiMac.SetType("ns3::AdhocWifiMac");
	NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

	ostringstream oss;

	oss << centerSquare;
	StringValue centerSV(oss.str());

	oss.str("");
	oss << "Uniform:0:" << centerSquare;
	StringValue uniformSV(oss.str());

	oss.str("");
	oss << "0|" << 2 * centerSquare << "|0|" << 2 * centerSquare;
	StringValue boundsSV(oss.str());

	oss.str("");
	oss << timeChange;
	StringValue timeSV(oss.str());

	StringValue speedSV(speed);

	MobilityHelper mobility;
	mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator", "X",
			centerSV, "Y", centerSV, "Rho", uniformSV);
	mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel", "Mode",
			StringValue("Time"), "Time", timeSV, "Speed", speedSV, "Bounds",
			boundsSV);
	mobility.InstallAll();

	// Enable OLSR
	OlsrHelper olsr;
	Ipv4StaticRoutingHelper staticRouting;

	Ipv4ListRoutingHelper list;
	list.Add(staticRouting, 0);
	list.Add(olsr, 10);

	InternetStackHelper internet;
	internet.SetRoutingHelper(list); // has effect on the next Install ()
	internet.Install(nodes);

	Ipv4AddressHelper ipv4;
	NS_LOG_INFO ("Assign IP Addresses.");
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer i = ipv4.Assign(devices);
	if (tracing) {
		wifiPhy.EnablePcapAll("wifi-simple-adhoc-grid");
	}

	if (verbose) {
		// Turn on all Wifi logging
		wifi.EnableLogComponents();

		// Trace routing tables
		Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>(
				"wifi-simple-adhoc-grid.routes", std::ios::out);
		olsr.PrintRoutingTableAllEvery(Seconds(routingTableUpdate),
				routingStream);
	}

	DceManagerHelper dceManager;
	dceManager.Install(nodes);

	CcnClientHelper dce;
	ApplicationContainer apps;

	std::string addrSource = GetAddressFromNode(nodes, sourceNode);

	for (int n = 0; n < numNodes; n++) {

		// Install ccnd on each node
		dce.SetStackSize(1 << 20);

		dce.SetBinary("ccnd");
		dce.ResetArguments();
		dce.ResetEnvironment();

		dce.AddEnvironment("CCND_CAP", "50000");
		dce.AddEnvironment("CCND_DEBUG", "-1");
		dce.AddEnvironment("CCN_LOCAL_PORT", "");

		dce.AddEnvironment("CCND_CAP", "");
		dce.AddEnvironment("CCND_AUTOREG", "");

		dce.AddEnvironment("CCND_LISTEN_ON", "");
		dce.AddEnvironment("CCND_MTU", "");
		dce.AddEnvironment("CCND_LOCAL_SOCKNAME", "");
		dce.AddEnvironment("CCND_DATA_PAUSE_MICROSEC", "");
		dce.AddEnvironment("CCND_KEYSTORE_DIRECTORY", "");

		apps = dce.Install(nodes.Get(n));
		apps.Start(Seconds(ccndStartTime));

		// Stop ccnd before end of simu.
		dce.ResetArguments();
		dce.ResetEnvironment();
		dce.SetBinary("ccndsmoketest");
		dce.SetStdinFile("");
		dce.AddArgument("kill");
		apps = dce.Install(nodes.Get(n));
		apps.Start(Seconds(ccndStopTime));

		// Set up each node to dynamically update its FIB

		if (n > 0) {
			dce.SetBinary("ccndc");
			dce.ResetArguments();
			dce.ResetEnvironment();
			dce.AddEnvironment("HOME", "/root");
			dce.AddArgument("-v");
			dce.AddArgument("add");
			dce.AddArgument("/NODE0");
			dce.AddArgument("udp");
			dce.AddArgument(addrSource);

			apps = dce.Install(nodes.Get(n));
			apps.Start(Seconds(ccndcStartTime));
		}

	}

	// Output what we are doing
	NS_LOG_UNCOND ("Publish content on node " << sourceNode << "; node " << sinkNode << " requests content.");
	// log
	NS_LOG_UNCOND ("Source publishes at regular intervals of '" << reqInterval << "s' , content expires after '" << contentLifetime << "' s.");
	// log
	NS_LOG_UNCOND ("Sink gets content at regular intervals of '" << reqInterval << "s' , '" << (getTime-pubTime) << "s' after content is published.");
	// log
	NS_LOG_UNCOND ("Nodes are initially spread in a circle with radius='" << centerSquare << "m'.");

	CreateContent();

	oss.str("");
	oss << contentLifetime;
	for (int i = 0; i < numberRequests; ++i) {

		// publish content on the source node
		dce.ResetArguments();
		dce.ResetEnvironment();
		dce.AddEnvironment("HOME", "/root");
		dce.SetBinary("ccnpoke");
		dce.SetStdinFile("/tmp/README");
		dce.AddFile("/tmp/README", "/tmp/README");
		dce.AddArgument("-x");

		dce.AddArgument(oss.str());
		dce.AddArgument("/NODE0/LeReadme");
		dce.AddEnvironment("HOME", "/root");

		apps = dce.Install(nodes.Get(sourceNode));
		apps.Start(Seconds(pubTime + i * reqInterval));
	}

	oss.str("");
	oss << 2 * contentLifetime;
	for (int i = 0; i < numberRequests; ++i) {
		// Retrieve the file using sink node, set linger time (time to wait for content) to twice the content's lifetime
		// ccnget -c /NODE0/LeReadme
		dce.ResetArguments();
		dce.ResetEnvironment();
		dce.AddEnvironment("HOME", "/root");
		dce.AddEnvironment("CCN_LINGER", oss.str()); // timeout for waiting for content
		dce.SetBinary("ccnpeek");
		dce.SetStdinFile("");
		dce.AddArgument("-c");
		//dce.AddArgument("-a"); // accept stale content NO!
		dce.AddArgument("/NODE0/LeReadme");
		apps = dce.Install(nodes.Get(sinkNode));
		apps.Start(Seconds(getTime + i * reqInterval));
	}

	// Create the animation object and configure for specified output
	AnimationInterface anim(animFile);
	// Create the animation object and configure for specified output
	Simulator::Stop(Seconds(simStopTime));
	Simulator::Run();
	Simulator::Destroy();

	anim.StopAnimation();

	return 0;
}
