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

void createReadme(const std::string& filename) {
	std::ofstream osf(filename.c_str(), std::fstream::trunc);
	osf << "Here is the requested data :)" << std::endl;
	osf.close();
}

class Content {
	uint32_t contId;
	uint32_t nodeId;
	uint32_t contentLifetime;
	uint32_t publicationTime;
	uint32_t retrievalTime;
	uint32_t numberRequests;
	double requestInterval;
	std::string contentPrefix;
	std::string contentSuffix;
	std::string contentName;
	std::string filename;

public:
	Content(uint32_t nodeId, uint32_t contId, const NodeContainer& nodes,
			CcnClientHelper& dce, const std::string& filename) :
			nodeId(nodeId), // id of node producing content
			contId(contId), // id of content produced by nodeId
			contentLifetime(20), // lifetime of content in seconds
			publicationTime(50), // seconds
			retrievalTime(60), // seconds
			numberRequests(30), // number of requests
			requestInterval(25.0), // interval between content requests (seconds)
			filename(filename) {

		ostringstream oss;
		oss << "/node" << nodeId;
		this->contentPrefix = oss.str();

		oss.str("");
		oss << "/content" << contId;
		this->contentSuffix = oss.str();

		this->contentName = this->contentPrefix + this->contentSuffix;
	}

	uint32_t getContentLifetime() const {
		return contentLifetime;
	}

	void setContentLifetime(uint32_t contentLifetime) {
		this->contentLifetime = contentLifetime;
	}

	std::string getContentName() const {
		return contentName;
	}

	std::string getContentPrefix() const {
		return contentPrefix;
	}

	std::string getContentSuffix() const {
		return contentSuffix;
	}

	uint32_t getContId() const {
		return contId;
	}

	std::string getFilename() const {
		return filename;
	}

	uint32_t getNodeId() const {
		return nodeId;
	}

	uint32_t getNumberRequests() const {
		return numberRequests;
	}

	void setNumberRequests(uint32_t numberRequests) {
		this->numberRequests = numberRequests;
	}

	uint32_t getPublicationTime() const {
		return publicationTime;
	}

	void setPublicationTime(uint32_t publicationTime) {
		this->publicationTime = publicationTime;
	}

	double getRequestInterval() const {
		return requestInterval;
	}

	void setRequestInterval(double requestInterval) {
		this->requestInterval = requestInterval;
	}

	uint32_t getRetrievalTime() const {
		return retrievalTime;
	}

	void setRetrievalTime(uint32_t retrievalTime) {
		this->retrievalTime = retrievalTime;
	}

}
;

Ipv4Address getAddressFromNode(Ptr<Node> pNode) {
	Ipv4Address defaddr("255.255.255.255");
	Ptr<Ipv4> ipv4 = pNode->GetObject<Ipv4>();
	if (ipv4->GetNInterfaces() >= 1) {
		Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1, 0);
		Ipv4Address addri = iaddr.GetLocal();
		return addri;
	}
	return defaddr;
}

std::string getAddressStringFromNode(Ptr<Node> pNode) {
	std::ostringstream oss("");
	Ptr<Ipv4> ipv4 = pNode->GetObject<Ipv4>();
	if (ipv4->GetNInterfaces() >= 1) {
		Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1, 0);
		Ipv4Address addri = iaddr.GetLocal();
		addri.Print(oss);
	}
	return oss.str();
}

static void generateTraffic(NodeContainer& nodes, CcnClientHelper& dce,
		Content c) {
	uint32_t numberRequests = c.getNumberRequests();
	uint32_t contentLifetime = c.getContentLifetime();
	std::string filename = c.getFilename();
	std::string contentname = c.getContentName();
	Time intReqInterval = Seconds(c.getRequestInterval());
	Time diffPubRet = Seconds(c.getRetrievalTime() - c.getPublicationTime());

	if( numberRequests == 0 ){
		return;
	}
	c.setNumberRequests(numberRequests-1);

	std::ostringstream oss;
	oss << contentLifetime;

	// publish content on the source node
	dce.ResetArguments();
	dce.ResetEnvironment();
	dce.AddEnvironment("HOME", "/root");
	dce.SetBinary("ccnpoke");
	dce.SetStdinFile(filename);
	dce.AddFile(filename, filename);
	dce.AddArgument("-x");

	dce.AddArgument(oss.str());
	dce.AddArgument(c.getContentName());
	dce.AddEnvironment("HOME", "/root");

	ApplicationContainer apps = dce.Install(nodes.Get(0));
	apps.Start(Simulator::Now() + Seconds(1));

	oss.str("");
	oss << 2 * contentLifetime;

	int delta = 0; // don't generate concurrent events if we don't have to!
	NodeContainer::Iterator iter = nodes.Begin();
	for (; iter != nodes.End(); iter++) {
		Ptr<Node> pNode = (*iter);
		int n = pNode->GetId();
		if (n != c.getNodeId()) {

			// Retrieve the file using all nodes but source,
			// set linger time (time to wait for content) to twice the content's lifetime
			// ccnget -c /node-id/content-id
			dce.ResetArguments();
			dce.ResetEnvironment();
			dce.AddEnvironment("HOME", "/root");
			dce.AddEnvironment("CCN_LINGER", oss.str()); // timeout for waiting for content = twice its lifetime
			dce.SetBinary("ccnpeek");
			dce.SetStdinFile("");
			dce.AddArgument("-c");
			dce.AddArgument(contentname);
			ApplicationContainer apps = dce.Install(pNode);
			apps.Start(
					Simulator::Now() + diffPubRet
							+ Seconds(delta * 0.001 + 1.0));
			delta++;
		}
	}

	Simulator::Schedule(intReqInterval, &generateTraffic, nodes, dce, c);

}

int main(int argc, char *argv[]) {
	std::string phyMode("DsssRate1Mbps");
	uint32_t sizeBuffer = 5; // number of content items to store in the buffer
	uint32_t ccnxMode = 2; // utility = 2, LRUMRU = 1, base = 0
	uint32_t numNodes = 25; // by default, 5x5 (25)
	uint32_t ccndStartTime = 0; // seconds
	uint32_t simStopTime = 1000; // seconds
	uint32_t ccndStopTime = 999; // seconds
	uint32_t contentPerNode = 5; // content items per node (5)
	uint32_t numberRequests = 30; // total number of content requests
	uint32_t publicationTime = 50; // seconds time to start publishing content
	uint32_t retrievalTime = 60; // seconds time to start retrieving content
	uint32_t contentLifetime = 20; // lifetime of content in seconds
	uint32_t reqInterval = 25; // interval between content requests (seconds)
	uint32_t numPublishers = 5; // number of content producing nodes
	uint32_t ccndcStartTime = 5; // seconds time to write FIB entries

	double radius = 1000.0; // place nodes in a circle of radius 1000m
	double timeChange = 2.0; // seconds before nodes change direction and speed
	std::string speed = "Constant:13.89";
	bool verbose = false;
	bool tracing = false;
	std::string animFile = "mobility.xml";

	CommandLine cmd;

	cmd.AddValue("phyMode", "Wifi Phy mode (default: DsssRate1Mbps).", phyMode);
	cmd.AddValue("verbose",
			"Turn on all WifiNetDevice log components (default: false).",
			verbose);
	cmd.AddValue("tracing", "Activate pcap tracing (default: true)", tracing);
	cmd.AddValue("numNodes", "Number of nodes (default: 25).", numNodes);
	cmd.AddValue("radius",
			"Nodes are spread out in a radius of X meters (default: 1000)",
			radius);
	cmd.AddValue("timeChange",
			"Time in secs before a node changes speed and direction (default: 2)",
			timeChange);
	cmd.AddValue("speed", "Node speed in m/s (default: Constant:13.89)", speed); // approximately 50km/h
	cmd.AddValue("contentPerNode",
			"Content items produced per node (default: 5).", contentPerNode);
	cmd.AddValue("numberRequests",
			"Total number of requests per content issued (default: 30).",
			numberRequests);
	cmd.AddValue("reqInterval",
			"Interval in seconds between content requests (default: 25.0).",
			reqInterval);
	cmd.AddValue("contentLifetime",
			"Content lifetime in seconds (default: 20).", contentLifetime);
	cmd.AddValue("publicationTime",
			"Time in seconds to start publishing content (default: 50).",
			publicationTime);
	cmd.AddValue("retrievalTime",
			"Time to start retrieving content (default: 60).", retrievalTime);
	cmd.AddValue("numPublishers",
			"Number of content producing nodes (default: 5).", numPublishers);
	cmd.AddValue("sizeBuffer",
			"Number of content items to store in the buffer (default: 5).",
			sizeBuffer);
	cmd.AddValue("ccnxMode",
			"CCNx operation mode: 2 - DR utility, 1 - removal LRU/ scheduling MRU, 0 - base.\n",
			ccnxMode);

	cmd.Parse(argc, argv);

	ostringstream oss;

	oss << "===================================" << std::endl;
	oss << "Simulation summary:" << std::endl;
	oss << "===================================" << std::endl;
	oss << "phyMode: " << phyMode << std::endl;
	oss << "verbose: " << verbose << std::endl;
	oss << "tracing: " << tracing << std::endl;
	oss << "numNodes: " << numNodes << std::endl;
	oss << "radius: " << radius << std::endl;
	oss << "timeChange: " << timeChange << std::endl;
	oss << "speed: " << speed << std::endl;
	oss << "contentPerNode: " << contentPerNode << std::endl;
	oss << "numberRequests: " << numberRequests << std::endl;
	oss << "reqInterval: " << reqInterval << std::endl;
	oss << "contentLifetime: " << contentLifetime << std::endl;
	oss << "publicationTime: " << publicationTime << std::endl;
	oss << "retrievalTime: " << retrievalTime << std::endl;
	oss << "numPublishers: " << numPublishers << std::endl;
	oss << "sizeBuffer: " << sizeBuffer << std::endl;
	oss << "ccnxMode: " << ccnxMode << std::endl;
	oss << "===================================" << std::endl;

	// log what we are doing.
	NS_LOG_UNCOND(oss.str());

	// disable fragmentation for frames below 2200 bytes
	Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold",
			StringValue("2200"));
	// turn off RTS/CTS for frames below 2200 bytes
	Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
			StringValue("2200"));
	// Fix non-unicast data rate to be the same as that of unicast
	Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",
			StringValue(phyMode));

	NS_LOG_INFO ("Create nodes.");
	// one container for the whole LAN
	NodeContainer nodes;
	nodes.Create(numNodes);

	NS_LOG_INFO ("Build Topology.");
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

	// We will use these NetDevice containers later, for IP addressing
	NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes); // First LAN

	NS_LOG_INFO("Add IP Stack.");
	InternetStackHelper internet;
	internet.Install(nodes);

	NS_LOG_INFO ("Assign IP Addresses.");
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaceContainer = ipv4.Assign(devices);

	NS_LOG_INFO("Configure Mobility.");

	oss.str("");

	oss << radius;
	StringValue centerSV(oss.str());

	oss.str("");
	oss << "Uniform:0:" << radius;
	StringValue uniformSV(oss.str());

	oss.str("");
	oss << "0|" << 2 * radius << "|0|" << 2 * radius;
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

	NS_LOG_INFO("Configure tracing.");

	if (tracing) {
		AsciiTraceHelper ascii;
		wifiPhy.EnableAsciiAll(
				ascii.CreateFileStream("wifi-simple-adhoc-grid.tr"));
		wifiPhy.EnablePcapAll("wifi-simple-adhoc-grid");
	}

	if (verbose) {
		// Turn on all Wifi logging
		wifi.EnableLogComponents();
	}

	NS_LOG_INFO("Configure multicasting and create applications.");

	DceManagerHelper dceManager;
	dceManager.Install(nodes);

	CcnClientHelper dce;
	ApplicationContainer apps;

	Ipv4StaticRoutingHelper multicast;

	std::string multicastGroupStr("225.1.2.4");
	std::string portStr("9695");
	Ipv4Address multicastGroup(multicastGroupStr.c_str());

	int delta = 0;
	for (int i = 0; i < nodes.GetN(); ++i) {

		Ptr<Node> pnode = nodes.Get(i);
		Ptr<NetDevice> pnodeIf = devices.Get(i);

		// Set up a default multicast route for pnode.
		// All nodes in the LAN hear pnode's transmissions.
		multicast.SetDefaultMulticastRoute(pnode, pnodeIf);

		// Install ccnd on each node
		dce.SetStackSize(1 << 20);

		dce.SetBinary("ccnd");
		dce.ResetArguments();
		dce.ResetEnvironment();

		oss.str("");
		oss << sizeBuffer;
		dce.AddEnvironment("CCND_CAP", oss.str());

		oss.str("");
		oss << ccnxMode;
		dce.AddEnvironment("CCN_MODE", oss.str());

		dce.AddEnvironment("CCND_DEBUG", "-1");
		dce.AddEnvironment("CCN_LOCAL_PORT", "");
		dce.AddEnvironment("CCND_AUTOREG", "");
		dce.AddEnvironment("CCND_LISTEN_ON", "");
		dce.AddEnvironment("CCND_MTU", "");
		dce.AddEnvironment("CCND_LOCAL_SOCKNAME", "");
		dce.AddEnvironment("CCND_DATA_PAUSE_MICROSEC", "");
		dce.AddEnvironment("CCND_KEYSTORE_DIRECTORY", "");

		apps = dce.Install(pnode);
		apps.Start(Seconds(ccndStartTime + delta * 0.001));

		// Stop ccnd before end of simu.
		dce.ResetArguments();
		dce.ResetEnvironment();
		dce.SetBinary("ccndsmoketest");
		dce.SetStdinFile("");
		dce.AddArgument("kill");
		apps = dce.Install(pnode);
		apps.Start(Seconds(ccndStopTime + delta * 0.001));
		delta++;
	}

	// Generate content events
	std::string filename = "/tmp/README";
	createReadme(filename);

	// Create FIB entries :-> default route for mcast group
	delta = 0;
	for (NodeContainer::Iterator iter = nodes.Begin(); iter != nodes.End();
			++iter) {
		Ptr<Node> pnode = (*iter);
		dce.SetBinary("ccndc");
		dce.ResetArguments();
		dce.ResetEnvironment();
		dce.AddEnvironment("HOME", "/root");
		dce.AddArgument("-v");
		dce.AddArgument("add");
		dce.AddArgument("/");
		dce.AddArgument("udp");
		dce.AddArgument(multicastGroupStr);
		dce.AddArgument(portStr);

		apps = dce.Install(pnode);
		apps.Start(Seconds(ccndcStartTime + delta * 0.001));
		delta++;
	}

	// Generate content; only numPublishers produce something, all other nodes retrieve this content.
	const int limitN = min(nodes.GetN(), numPublishers);

	for (int n = 0; n < limitN; ++n) {
		for (int c = 0; c < contentPerNode; ++c) {
			int startPub = publicationTime + c + limitN * c;
			int startRet = retrievalTime + c + limitN * c;

			Content cnt(n, c, nodes, dce, filename);
			cnt.setContentLifetime(contentLifetime); // lifetime of content in seconds
			cnt.setPublicationTime(startPub); // seconds
			cnt.setRetrievalTime(startRet); // seconds
			cnt.setNumberRequests(numberRequests); // number of requests
			cnt.setRequestInterval(reqInterval); // interval between content requests (seconds)

			Simulator::Schedule(Seconds(startPub), &generateTraffic, nodes, dce,
					cnt);
		}
	}

	// Create the animation object and configure for specified output
	AnimationInterface anim(animFile);

	// Create the animation object and configure for specified output
	Simulator::Stop(Seconds(simStopTime));
	time_t start = 0, end = 0;
	double minElapsed = 0.0;
	start = time(NULL);
	Simulator::Run();
	end = time(NULL);
	minElapsed = (end - start) * 1.0 / 60.0;
	std::cout << "Time elapsed: " << minElapsed << " min." << std::endl;
	Simulator::Destroy();

	anim.StopAnimation();

	// delete content readme file
	remove(filename.c_str());

	return 0;
}
