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

static void createReadme(const std::string& filename) {
	std::ofstream osf(filename.c_str(), std::fstream::trunc);
	osf << "Here is the requested data :)" << std::endl;
	osf.close();
}

static void writeSimSummary(const std::string& filename,
		const std::ostringstream & oss) {
	std::ofstream osf(filename.c_str(), std::fstream::trunc);
	osf << oss.str();
	osf.close();
}

static double randZeroOne() {
	double rnd = rand() * 1.0 / ((double) (RAND_MAX + 0.001));
	return rnd;
}

class ZipfDist {
	uint32_t numClasses;
	double alpha;
	double normFactor;

public:
	ZipfDist(uint32_t numberClasses, double alpha) :
			numClasses(numberClasses), alpha(alpha) {
		normFactor = 0.0;
		for (int i = 0; i < numberClasses; ++i) {
			normFactor += 1.0 / pow((i + 1), alpha);
		}
	}

	double probability(uint32_t _class) {
		double ret = 0.0;
		if (_class >= 0 && _class < numClasses) {
			ret = (1.0 / pow((_class + 1), alpha)) / normFactor;
		}
		return ret;
	}
};

class Content {
	uint32_t contId;
	uint32_t nodeId;
	uint32_t rank;
	double contentLifetime;
	double probability;
	std::string contentName;
	std::string filename;

public:
	Content(uint32_t nodeId, uint32_t contId, const std::string& filename) :
			nodeId(nodeId), // id of node producing content
			contId(contId), // id of content produced by nodeId
			contentLifetime(20), // lifetime of content in seconds
			filename(filename), // system file
			probability(0.0),
			rank(0){

		ostringstream oss;
		oss << "/node" << nodeId << "/content" << contId;

		this->contentName = oss.str();
	}

	static void publishEvent(Content* ptr) {
		if (ptr != NULL) {
			ptr->publish();
		}
	}

	void publish() {
	}

	double getContentLifetime() const {
		return contentLifetime;
	}

	void setContentLifetime(double contentLifetime) {
		this->contentLifetime = contentLifetime;
	}

	std::string getContentName() const {
		return contentName;
	}

	std::string getRankingName() const {
		std::ostringstream oss;
		oss << "/rank" << this->rank;
		return oss.str();
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

	double getProbability() const {
		return probability;
	}

	void setProbability(double probability) {
		this->probability = probability;
	}

	uint32_t getRank() const {
		return rank;
	}

	void setRank(uint32_t rank) {
		this->rank = rank;
	}

}
;

class NodeWrapper {
	Ptr<Node> node;
	CcnClientHelper& dce;
	std::vector<Content *> publications;
	std::vector<Content *>& contents;
	const std::string& multicastAddr;
	const std::string& multicastPort;
	int publicationsPerNode;
	int requestsPerNode;
	int sizeBuffer;
	int ccnxMode;
	double contentLifetime;
	double requestLifetime;
	double interRequestPeriod;
	double slackTime;
	double windowSize;
	uint32_t helloPeriodicity;
	uint32_t pitDelay;
	uint32_t helloMaxBackoff;
	uint32_t pitMaxBackoff;

	Content* pickContent() {
		double rnd = randZeroOne();
		double probability = 0.0;
		std::ostringstream oss;
		if (contents.empty()) {
			return NULL;
		}
		vector<Content *>::iterator iter = contents.begin();
		for (; iter != contents.end(); ++iter) {
			probability = (*iter)->getProbability();
			if (rnd < probability) {
				return (*iter);
			}
			rnd -= probability;
		}
		/*
		 * should not reach this point
		 */
		return *(contents.end());
	}

	void selectContentRequests(std::vector<Content *>& requests) {
		Content * content = NULL;
		uint32_t nodeId = node->GetId();
		if (contents.empty()) {
			return;
		}
		if (publicationsPerNode > contents.size()) {
			publicationsPerNode = contents.size();
		}
		/*
		 * pick content according to probability distribution
		 */
		for (int i = 0; i < publicationsPerNode; ++i) {
			do {
				/*
				 * pick content that we have not published!
				 * (cannot return null because contents is non-empty)
				 */
				content = pickContent();
			} while (content->getNodeId() == nodeId);
			requests.push_back(content);
		}

	}

public:
	NodeWrapper(Ptr<Node> node, std::vector<Content *>& contents,
			CcnClientHelper& dce, const std::string& filename,
			const std::string& multicastAddress,
			const std::string& multicastPort, double contentLifetime,
			double requestLifetime, double interRequestPeriod, double slackTime,
			double windowSize, uint32_t helloPeriodicity,
			uint32_t helloMaxBackoff, uint32_t pitDelay, uint32_t pitMaxBackoff,
			int publicationsPerNode, int requestsPerNode, int sizeBuffer,
			int ccnxMode) :
			node(node), contents(contents), dce(dce), multicastAddr(
					multicastAddress), multicastPort(multicastPort), contentLifetime(
					contentLifetime), requestLifetime(requestLifetime), interRequestPeriod(
					interRequestPeriod), slackTime(slackTime), windowSize(
					windowSize), helloPeriodicity(helloPeriodicity), helloMaxBackoff(
					helloMaxBackoff), pitDelay(pitDelay), pitMaxBackoff(
					pitMaxBackoff), publicationsPerNode(publicationsPerNode), requestsPerNode(
					requestsPerNode), sizeBuffer(sizeBuffer), ccnxMode(ccnxMode) {
		for (int i = 0; i < publicationsPerNode; ++i) {
			Content * content = new Content(node->GetId(), i, filename);
			content->setContentLifetime(contentLifetime);
			publications.push_back(content);
			contents.push_back(content);
		}
	}

	virtual ~NodeWrapper() {
		for (std::vector<Content *>::iterator iter = publications.begin();
				iter != publications.end(); ++iter) {
			Content * content = (*iter);
			delete (content);
		}
	}

	int getNumberContentItems() const {
		return publicationsPerNode;
	}

	void setNumberContentItems(int numberContentItems) {
		this->publicationsPerNode = numberContentItems;
	}

	static void setupEvent(NodeWrapper* nw, double endTime) {
		if (nw != NULL) {
			nw->setup(endTime);
		}
	}

	void setup(double endTime) {
		double startTime = slackTime * randZeroOne();
		std::ostringstream oss;
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

		oss.str("");
		oss << windowSize;
		dce.AddEnvironment("CCN_STATS_W_SECONDS", oss.str());

		oss.str("");
		oss << requestLifetime;
		dce.AddEnvironment("CCN_STATS_TTL_SECONDS", oss.str());

		oss.str("");
		oss << helloPeriodicity;
		dce.AddEnvironment("CCN_HELLO_PERIODICITY", oss.str());

		oss.str("");
		oss << helloMaxBackoff;
		dce.AddEnvironment("CCN_HELLO_MAX_BACKOFF", oss.str());

		oss.str("");
		oss << pitDelay;
		dce.AddEnvironment("CCN_PIT_DELAY", oss.str());

		oss.str("");
		oss << pitMaxBackoff;
		dce.AddEnvironment("CCN_PIT_MAX_BACKOFF", oss.str());

		dce.AddEnvironment("CCND_DEBUG", "-1");
		dce.AddEnvironment("CCN_LOCAL_PORT", "");
		dce.AddEnvironment("CCND_AUTOREG", "");
		dce.AddEnvironment("CCND_LISTEN_ON", "");
		dce.AddEnvironment("CCND_MTU", "");
		dce.AddEnvironment("CCND_LOCAL_SOCKNAME", "");
		dce.AddEnvironment("CCND_DATA_PAUSE_MICROSEC", "");
		dce.AddEnvironment("CCND_KEYSTORE_DIRECTORY", "");

		ApplicationContainer apps = dce.Install(node);
		Time install = Simulator::Now() + Seconds(startTime);
		//printf("Install ccnd time %f\n.", install.GetDouble());
		apps.Start(install);

		// default FIB entry
		startTime += slackTime * randZeroOne();

		dce.SetBinary("ccndc");
		dce.ResetArguments();
		dce.ResetEnvironment();
		dce.AddEnvironment("HOME", "/root");
		dce.AddArgument("-v");
		dce.AddArgument("add");
		dce.AddArgument("/");
		dce.AddArgument("udp");
		dce.AddArgument(multicastAddr);
		dce.AddArgument(multicastPort);

		apps = dce.Install(node);
		install = Simulator::Now() + Seconds(startTime);
		apps.Start(install);

		// Stop ccnd before end of simu.
		double stopTime = endTime + slackTime * randZeroOne();
		dce.ResetArguments();
		dce.ResetEnvironment();
		dce.SetBinary("ccndsmoketest");
		dce.SetStdinFile("");
		dce.AddArgument("kill");
		apps = dce.Install(node);
		install = Seconds(stopTime);
		//printf("Install ccndsmoketest time %f\n.", install.GetDouble());
		apps.Start(install);
	}

	static void requestContentEvent(NodeWrapper* nw) {
		if (nw != NULL) {
			nw->requestContent();
		}
	}

	void requestContent() {
		std::ostringstream oss;
		oss << requestLifetime;

		std::vector<Content *> requests;
		selectContentRequests(requests);
		double startTime = slackTime * randZeroOne();
		Time install;
		for (std::vector<Content *>::iterator iter = requests.begin();
				iter != requests.end(); ++iter) {
			// ccnget -c /node-id/content-id
			dce.ResetArguments();
			dce.ResetEnvironment();
			dce.AddEnvironment("HOME", "/root");
			//dce.AddEnvironment("CCN_LINGER", oss.str()); // timeout for waiting for content
			dce.SetBinary("ccnpeek");
			dce.SetStdinFile("");
			dce.AddArgument("-c");
			dce.AddArgument("-l"); // information request lifetime
			dce.AddArgument(oss.str());
			dce.AddArgument("-w"); // process dies after information request dies
			dce.AddArgument(oss.str());
			dce.AddArgument((*iter)->getRankingName());
			ApplicationContainer apps = dce.Install(node);
			install = Simulator::Now() + Seconds(startTime);
			//printf("Install ccnpeek time %f.\n", install.GetDouble());
			apps.Start(install);
			startTime += slackTime * randZeroOne();
		}
		Simulator::Schedule(
				Seconds(startTime + requestLifetime + interRequestPeriod),
				&NodeWrapper::requestContentEvent, this);
	}

	static void publishContentEvent(NodeWrapper* nw) {
		if (nw != NULL) {
			nw->publishContent();
		}
	}

	void publishContent() {
		std::ostringstream oss;
		oss << contentLifetime;

		double startTime = slackTime * randZeroOne();
		Time install;
		for (std::vector<Content *>::iterator iter = publications.begin();
				iter != publications.end(); ++iter) {
			Content * publication = (*iter);

			// publish content
			dce.ResetArguments();
			dce.ResetEnvironment();
			dce.AddEnvironment("HOME", "/root");
			dce.SetBinary("ccnpoke");
			dce.SetStdinFile(publication->getFilename());
			dce.AddFile(publication->getFilename(), publication->getFilename());
			dce.AddArgument("-x");
			dce.AddArgument(oss.str());
			//dce.AddArgument("-w");
			//dce.AddArgument(oss.str());
			dce.AddArgument("-f");
			dce.AddArgument(publication->getRankingName());
			dce.AddEnvironment("HOME", "/root");

			ApplicationContainer apps = dce.Install(node);
			install = Simulator::Now() + Seconds(startTime);
			//printf("Install ccnpoke time %f.\n", install.GetDouble());
			apps.Start(install);

			startTime += slackTime * randZeroOne();
		}
		Simulator::Schedule(Seconds(contentLifetime + startTime),
				&NodeWrapper::publishContentEvent, this);
	}

	int getCcnxMode() const {
		return ccnxMode;
	}

	void setCcnxMode(int ccnxMode) {
		this->ccnxMode = ccnxMode;
	}

	double getRequestLifetime() const {
		return requestLifetime;
	}

	void setRequestLifetime(double requestLifetime) {
		this->requestLifetime = requestLifetime;
	}

	int getSizeBuffer() const {
		return sizeBuffer;
	}

	void setSizeBuffer(int sizeBuffer) {
		this->sizeBuffer = sizeBuffer;
	}

	double getSlackTime() const {
		return slackTime;
	}

	void setSlackTime(double slackTime) {
		this->slackTime = slackTime;
	}

	Ptr<Node> getNode() const {
		return node;
	}

	double getContentLifetime() const {
		return contentLifetime;
	}

	std::vector<Content*> getPublications() const {
		return publications;
	}

	int getPublicationsPerNode() const {
		return publicationsPerNode;
	}

	int getRequestsPerNode() const {
		return requestsPerNode;
	}

	double getWindowSize() const {
		return windowSize;
	}

	double getInterRequestPeriod() const {
		return interRequestPeriod;
	}

	uint32_t getHelloPeriodicity() const {
		return helloPeriodicity;
	}

	uint32_t getPitDelay() const {
		return pitDelay;
	}

};

class NodeContainerWrapper {
	std::vector<NodeWrapper *> nodes;
public:

	NodeContainerWrapper(const NodeContainer& nodeContainer,
			std::vector<Content *>& contents, CcnClientHelper& dce,
			const std::string& filename, const std::string& multicastAddr,
			const std::string& multicastPort, double contentLifetime,
			double requestLifetime, double interRequestPeriod, double slackTime,
			double windowSize, uint32_t helloPeriodicity,
			uint32_t helloMaxBackoff, uint32_t pitDelay, uint32_t pitMaxBackoff,
			int publicationsPerNode, int requestsPerNode, int sizeBuffer,
			int ccnxMode) {
		NodeContainer::Iterator iter = nodeContainer.Begin();
		while (iter != nodeContainer.End()) {
			Ptr<Node> node = (*iter);
			NodeWrapper * nw = new NodeWrapper(node, contents, dce, filename,
					multicastAddr, multicastPort, contentLifetime,
					requestLifetime, interRequestPeriod, slackTime, windowSize,
					helloPeriodicity, helloMaxBackoff, pitDelay, pitMaxBackoff,
					publicationsPerNode, requestsPerNode, sizeBuffer, ccnxMode);
			nodes.push_back(nw);
			++iter;
		}
	}

	virtual ~NodeContainerWrapper() {
		for (std::vector<NodeWrapper *>::iterator iter = nodes.begin();
				iter != nodes.end(); ++iter) {
			NodeWrapper * nw = (*iter);
			delete (nw);
		}
	}

	void setupPhase(double startTime, double endTime) {
		int delta = 0;
		for (std::vector<NodeWrapper *>::iterator iter = nodes.begin();
				iter != nodes.end(); ++iter) {
			NodeWrapper * nw = (*iter);
			Simulator::Schedule(Seconds(startTime + delta),
					&NodeWrapper::setupEvent, nw, endTime);
			++delta;
		}
	}

	void publishPhase(double startTime) {
		int delta = 0;
		for (std::vector<NodeWrapper *>::iterator iter = nodes.begin();
				iter != nodes.end(); ++iter) {
			NodeWrapper * nw = (*iter);
			Simulator::Schedule(Seconds(startTime + delta),
					&NodeWrapper::publishContentEvent, nw);
			++delta;
		}
	}

	void requestPhase(double startTime) {
		int delta = 0;
		for (std::vector<NodeWrapper *>::iterator iter = nodes.begin();
				iter != nodes.end(); ++iter) {
			NodeWrapper * nw = (*iter);
			Simulator::Schedule(Seconds(startTime + delta),
					&NodeWrapper::requestContentEvent, nw);
			++delta;
		}
	}
};

class DecreasingProbability {
public:
	bool operator()(Content* c1, Content* c2) {
		double p1 = c1->getProbability();
		double p2 = c2->getProbability();
		return p2 < p1;
	}
};

int main(int argc, char *argv[]) {
	std::string phyMode("DsssRate1Mbps");
	uint32_t ccnxMode = 2; // utility = 2, LRUMRU = 1, base = 0
	double simulationTime = 1000.0; // seconds
	double windowSize = 300.0; // window size in seconds
	double slackTime = 5.0; // seconds
	uint32_t publicationsPerNode = 5; // content items published per node (5)
	uint32_t requestsPerNode = 5; // content items requested per node
	double startPublication = 50; // time to start publishing content
	double startRequest = 60; // time to start requesting content
	double contentLifetime = simulationTime; // lifetime of content in seconds
	double requestLifetime = 30.0; // lifetime of an information request
	double interRequestPeriod = 0.0; // time to wait before issuing next request
	uint32_t helloPeriodicity = 1000000; // approx time between hello messages (us)
	uint32_t pitDelay = 500000; // approx delay before sending a pit after receiving a hello message (us)
	uint32_t helloMaxBackoff = 3; // largest factor by which we multiply the helloPeriodicity to delay the sending of a hello message
	uint32_t pitMaxBackoff = 3; // largest factor by which we
	uint32_t sizeBuffer = 15; // number of content items to store in the buffer
	bool verbose = false;
	bool tracing = false;
	double powerLaw = 0.6667;
	std::string animFile = "mobility.xml";
	std::vector<Content*> contents;
	const uint32_t numNodes = 2; // by default, 5x5 (25)

	/*
	 * sizeBuffer = (int) floor(
	 numNodes * publicationsPerNode * 1.0 * log(2) / log(numNodes));
	 */
	CommandLine cmd;

	cmd.AddValue("simulationTime",
			"Duration of the simulation in seconds (default: 1000).",
			simulationTime);
	cmd.AddValue("phyMode", "Wifi Phy mode (default: DsssRate1Mbps).", phyMode);
	cmd.AddValue("verbose",
			"Turn on all WifiNetDevice log components (default: 0).", verbose);
	cmd.AddValue("tracing", "Activate pcap tracing (default: 0)", tracing);
	cmd.AddValue("publicationsPerNode",
			"Content items produced per node (default: 5).",
			publicationsPerNode);
	cmd.AddValue("requestsPerNode",
			"Content items requested per node (default: 5).", requestsPerNode);
	cmd.AddValue("startPublication",
			"Time in seconds to start publishing content (default: 50).",
			startPublication);
	cmd.AddValue("startRequest",
			"Time in seconds to start requesting content (default: 60).",
			startRequest);
	cmd.AddValue("contentLifetime",
			"Content lifetime in seconds (default: 1000).", contentLifetime);
	cmd.AddValue("requestLifetime",
			"Lifetime of an information request in seconds (default: 30).",
			requestLifetime);
	cmd.AddValue("windowSize",
			"Size of the window (in seconds) used for measuring the utility of requests (default: 300.0).",
			windowSize);
	cmd.AddValue("sizeBuffer",
			"Maximum number of content items that can be stored in the buffer (default: 15) ",
			sizeBuffer);
	cmd.AddValue("interRequestPeriod",
			"Time to wait before issuing the next request (default: 0s).",
			interRequestPeriod);
	cmd.AddValue("helloPeriodicity",
			"Approximate time between hello messages (us) (default: 1000000).\n",
			helloPeriodicity);
	cmd.AddValue("helloMaxBackoff",
			"Maximum backoff for delaying hello messages (default: 3).\n",
			helloMaxBackoff);
	cmd.AddValue("pitDelay",
			"Approximate delay before sending a pit message after receiving a hello message (us) (default: 500000).\n",
			pitDelay);
	cmd.AddValue("pitMaxBackoff",
			"Maximum backoff for delaying pit messages (default: 3).\n",
			pitMaxBackoff);
	cmd.AddValue("powerLaw",
			"Parameter of the Zipf distribution that describes the popularity of content (default: 0.6667).\n",
			powerLaw);
	cmd.AddValue("ccnxMode",
			"CCNx operation mode: 2 - DR utility, 1 - removal LRU/ scheduling MRU, 0 - base.\n",
			ccnxMode);

	cmd.Parse(argc, argv);

	/*
	 * check simulation parameters
	 */
	if (numNodes <= 0) {
		return 1;
	}

	/*
	 * print simulation parameters
	 */

	ostringstream oss;

	oss << "===================================" << std::endl;
	oss << "Simulation summary:" << std::endl;
	oss << "===================================" << std::endl;
	oss << "simulationTime: " << simulationTime << std::endl;
	oss << "phyMode: " << phyMode << std::endl;
	oss << "verbose: " << verbose << std::endl;
	oss << "tracing: " << tracing << std::endl;
	oss << "numNodes: " << numNodes << std::endl;
	oss << "publicationsPerNode: " << publicationsPerNode << std::endl;
	oss << "requestsPerNode: " << requestsPerNode << std::endl;
	oss << "startPublication: " << startPublication << std::endl;
	oss << "startRequest: " << startRequest << std::endl;
	oss << "contentLifetime: " << contentLifetime << std::endl;
	oss << "requestLifetime: " << requestLifetime << std::endl;
	oss << "windowSize: " << windowSize << std::endl;
	oss << "sizeBuffer: " << sizeBuffer << std::endl;
	oss << "interRequestPeriod: " << interRequestPeriod << std::endl;
	oss << "helloPeriodicity: " << helloPeriodicity << std::endl;
	oss << "helloMaxBackoff : " << helloMaxBackoff << std::endl;
	oss << "pitDelay: " << pitDelay << std::endl;
	oss << "pitMaxBackoff: " << pitMaxBackoff << std::endl;
	oss << "powerLaw: " << powerLaw << std::endl;
	oss << "ccnxMode: " << ccnxMode << std::endl;
	oss << "===================================" << std::endl;

	// log what we are doing.
	NS_LOG_UNCOND(oss.str());
	writeSimSummary("simulation.txt", oss);

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
	/*
	 * wireless range set to slightly more than 100m (approx)
	 * as expected for IEEE802.11b radios.
	 */
	wifiPhy.Set("TxGain", DoubleValue(-14));
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

	MobilityHelper mobility0, mobility1;

	// Place node 0 at (0,0,0)
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<
			ListPositionAllocator>();
	positionAlloc->Add(Vector(0.0, 0.0, 0.0));
	mobility0.SetPositionAllocator(positionAlloc);
	mobility0.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility0.Install(nodes.Get(0));

	// Set node 1 on a path from 500,0,0 to 50,0,0
	mobility1.SetMobilityModel("ns3::WaypointMobilityModel");
	mobility1.Install(nodes.Get(1));

	Waypoint wpt1(Seconds(0.0), Vector(500.0, 0.0, 0.0));
	Waypoint wpt2(Seconds(300.0), Vector(50.0, 0.0, 0.0));
	Ptr<WaypointMobilityModel> mob = nodes.Get(1)->GetObject<
			WaypointMobilityModel>();
	mob->AddWaypoint(wpt1);
	mob->AddWaypoint(wpt2);

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

	NS_LOG_INFO("Configure multicasting.");

	DceManagerHelper dceManager;
	dceManager.Install(nodes);

	CcnClientHelper dce;
	ApplicationContainer apps;

	Ipv4StaticRoutingHelper multicast;

	std::string multicastGroupStr("225.1.2.4");
	std::string portStr("9695");

	int delta = 0;
	for (int i = 0; i < nodes.GetN(); ++i) {

		Ptr<Node> pnode = nodes.Get(i);
		Ptr<NetDevice> pnodeIf = devices.Get(i);

		// Set up a default multicast route for pnode.
		// All nodes in the LAN hear pnode's transmissions.
		multicast.SetDefaultMulticastRoute(pnode, pnodeIf);
	}

	NS_LOG_INFO("Generate content filename");

	// Generate content events
	std::string filename = "/tmp/README";
	createReadme(filename);

	NS_LOG_INFO("Generate node wrappers.");

	NodeContainerWrapper wnodes(nodes, contents, dce, filename,
			multicastGroupStr, portStr, contentLifetime, requestLifetime,
			interRequestPeriod, slackTime, windowSize, helloPeriodicity,
			helloMaxBackoff, pitDelay, pitMaxBackoff, publicationsPerNode,
			requestsPerNode, sizeBuffer, ccnxMode);

	if (contents.empty()) {
		/*
		 * cannot continue without content
		 */
		return 1;
	}

	NS_LOG_INFO("Assign content popularity according to Zipf's Law.");

	const int totalItems = contents.size();
	/*
	 * Assign request probability (popularity) to each content
	 * and sort by decreasing probability
	 * classes go from 0 to totalItems - 1
	 */
	ZipfDist dist(totalItems, powerLaw);
	int _class = 0;
	std::random_shuffle(contents.begin(), contents.end());
	for (std::vector<Content *>::iterator iter = contents.begin();
			iter != contents.end(); ++iter, ++_class) {
		Content* content = (*iter);
		double probability = dist.probability(_class);
		content->setProbability(probability);
		content->setRank(_class);
	}
	//std::sort(contents.begin(), contents.end(), DecreasingProbability());

	NS_LOG_INFO("Schedule DCE events.");
	wnodes.setupPhase(0, simulationTime - 20 * slackTime);
	wnodes.publishPhase(startPublication);
	wnodes.requestPhase(startRequest);

	NS_LOG_INFO("Start the simulation.");
	// Create the animation object and configure for specified output
	AnimationInterface anim(animFile);

	// Create the animation object and configure for specified output
	Simulator::Stop(Seconds(simulationTime));
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
