/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/config-store-module.h>
#include <ns3/buildings-module.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/applications-module.h>
#include <ns3/log.h>
#include <iomanip>
#include <ios>
#include <string>
#include <vector>
#include <ns3/flow-monitor.h>
#include <ns3/flow-monitor-module.h>
#include <ns3/flow-monitor-helper.h>
#include <ns3/flow-classifier.h>

// The topology of this simulation program is inspired from 
// 3GPP R4-092042, Section 4.2.1 Dual Stripe Model
// note that the term "apartments" used in that document matches with
// the term "room" used in the BuildingsMobilityModel 

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LteGrid");

bool ifTraceTcp = true;
std::string cwnd_tr_file_name = "cwndTr.txt";
std::string ssthresh_tr_file_name = "sshThTr.txt";
std::string rtt_tr_file_name = "rttTr.txt";
std::string rto_tr_file_name = "rtoTr.txt";
std::string rwnd_tr_file_name = "rwndTr.txt";
std::string hrack_tr_file_name = "hrackTr.txt";
std::string trFileDir;////////////!!!!!!!!!!!!!!!!!!!not complet3

std::string thrputStream_tr_file_name = "throughput.txt";

std::string basicTracePath = "/NodeList/59/$ns3::TcpL4Protocol/SocketList/";

bool firstCwnd = true;
bool firstSshThr = true;
bool firstRtt = true;
bool firstRto = true;
bool firstRwnd = true;
bool firstHrack = true;
Ptr<OutputStreamWrapper> cWndStream;
Ptr<OutputStreamWrapper> ssThreshStream;
Ptr<OutputStreamWrapper> rttStream;
Ptr<OutputStreamWrapper> rtoStream;
Ptr<OutputStreamWrapper> thrputStream;
Ptr<OutputStreamWrapper> rwndStream;
Ptr<OutputStreamWrapper> hrackStream;

uint32_t cWndValue;
uint32_t ssThreshValue;
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>.
ApplicationContainer sourceApps;
ApplicationContainer sinkApps;
Time *lastReportTime;
std::vector <double> lastRxMbits;

uint32_t nMacroUes = 1;
std::vector <int32_t>  uePosX (3, 0);
std::vector <int32_t>  uePosY (3, 0);

///////////////////////////////////////////////Tracers////////////////////////////////////////////////////////
	static void
CwndTracer (uint32_t oldval, uint32_t newval)
{
	if (firstCwnd)
	{
		firstCwnd = false;
	}
	*cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
	cWndValue = newval;

	if (!firstSshThr)
	{
		*ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << ssThreshValue << std::endl;
	}
}

	static void
SsThreshTracer (uint32_t oldval, uint32_t newval)
{
	if (firstSshThr)
	{
		firstSshThr = false;
	}
	*ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
	ssThreshValue = newval;

	if (!firstCwnd)
	{
		*cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << cWndValue << std::endl;
	}
}

	static void
RttTracer (Time oldval, Time newval)
{
	if (firstRtt)
	{
		firstRtt = false;
	}
	*rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

	static void
RtoTracer (Time oldval, Time newval)
{
	if (firstRto)
	{
		firstRto = false;
	}
	*rtoStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}



	static void
RwndTracer (uint32_t oldval, uint32_t newval)
{
	if (firstRwnd)
	{
		firstRwnd = false;
	}
	*rwndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
}


	static void
HrackTracer (const SequenceNumber32 oldval, const SequenceNumber32 newval)
{
	if (firstHrack)
	{
		firstHrack = false;
	}
	*hrackStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	static void
TraceCwnd (std::string cwnd_tr_file_name)
{
	AsciiTraceHelper ascii;
	cWndStream = ascii.CreateFileStream ((trFileDir + cwnd_tr_file_name).c_str ());
	Config::ConnectWithoutContext ("/NodeList/59/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));

}

	static void
TraceSsThresh (std::string ssthresh_tr_file_name)
{
	AsciiTraceHelper ascii;
	ssThreshStream = ascii.CreateFileStream ((trFileDir + ssthresh_tr_file_name).c_str ());
	Config::ConnectWithoutContext ("/NodeList/59/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold", MakeCallback (&SsThreshTracer));
}

	static void
TraceRtt (std::string rtt_tr_file_name)
{
	AsciiTraceHelper ascii;
	rttStream = ascii.CreateFileStream ((trFileDir + rtt_tr_file_name).c_str ());
	Config::ConnectWithoutContext ("/NodeList/59/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeCallback (&RttTracer));
}

	static void
TraceRto (std::string rto_tr_file_name)
{
	AsciiTraceHelper ascii;
	rtoStream = ascii.CreateFileStream ((trFileDir + rto_tr_file_name).c_str ());
	Config::ConnectWithoutContext ("/NodeList/59/$ns3::TcpL4Protocol/SocketList/0/RTO", MakeCallback (&RtoTracer));
}

	static void
TraceRwnd (std::string rwnd_tr_file_name)
{
	AsciiTraceHelper ascii;
	rwndStream = ascii.CreateFileStream ((trFileDir + rwnd_tr_file_name).c_str ());
	Config::ConnectWithoutContext ("/NodeList/59/$ns3::TcpL4Protocol/SocketList/0/RWND", MakeCallback (&RwndTracer));
}

// Highest ack received from peer
	static void
TraceHighestRxAck (std::string hrack_tr_file_name)
{
	AsciiTraceHelper ascii;
	hrackStream = ascii.CreateFileStream ((trFileDir + hrack_tr_file_name).c_str ());
	Config::ConnectWithoutContext ("/NodeList/59/$ns3::TcpL4Protocol/SocketList/0/HighestRxAck", MakeCallback (&HrackTracer));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PrintProgress(Ptr<OutputStreamWrapper> stream_throughput)
{
	Time now_t = Simulator::Now ();
	/* The first time the function is called we schedule the next call after
	 * biDurationNs ns and then return*/
	if (*(lastReportTime) == MicroSeconds(0)) {
		AsciiTraceHelper ascii;
		stream_throughput = ascii.CreateFileStream ((trFileDir+ thrputStream_tr_file_name).c_str ());

		Simulator::Schedule(Seconds(0.25), PrintProgress, stream_throughput);
		lastRxMbits.resize(sinkApps.GetN(), 0.0);
		*(lastReportTime) = now_t;
		return;
	} 

	*stream_throughput->GetStream() << now_t;
	*stream_throughput->GetStream() << std::fixed << std::setprecision(6);

	for (uint32_t appIdx = 0; appIdx < sinkApps.GetN(); appIdx++ ){
		uint64_t rx_bytes;
		double cur_rx_Mbits;

		rx_bytes = DynamicCast<PacketSink>(sinkApps.Get(appIdx))->GetTotalRx ();//the total bytes received in this sink app 
		cur_rx_Mbits = (rx_bytes * 8.0) / 1e6;

		*stream_throughput->GetStream() << " " << (cur_rx_Mbits - lastRxMbits[appIdx]) / ((now_t - *(lastReportTime)).GetNanoSeconds() / 1e9) << " ";

		lastRxMbits[appIdx] = cur_rx_Mbits;
	}

	*stream_throughput->GetStream() << std::endl;

	Simulator::Schedule(Seconds(0.25), PrintProgress, stream_throughput);

	*(lastReportTime) = now_t;

}


bool AreOverlapping (Box a, Box b)
{
	return !((a.xMin > b.xMax) || (b.xMin > a.xMax) || (a.yMin > b.yMax) || (b.yMin > a.yMax));
}

class FemtocellBlockAllocator
{
	public:
		FemtocellBlockAllocator (Box area, uint32_t nApartmentsX, uint32_t nFloors);
		void Create (uint32_t n);
		void Create ();

	private:
		bool OverlapsWithAnyPrevious (Box);
		Box m_area;
		uint32_t m_nApartmentsX;
		uint32_t m_nFloors;
		std::list<Box> m_previousBlocks;
		double m_xSize;
		double m_ySize;
		Ptr<UniformRandomVariable> m_xMinVar;
		Ptr<UniformRandomVariable> m_yMinVar;

};

	FemtocellBlockAllocator::FemtocellBlockAllocator (Box area, uint32_t nApartmentsX, uint32_t nFloors)
: m_area (area),
	m_nApartmentsX (nApartmentsX),
	m_nFloors (nFloors),
	m_xSize (nApartmentsX*10 + 20),
	m_ySize (70)
{
	m_xMinVar = CreateObject<UniformRandomVariable> ();
	m_xMinVar->SetAttribute ("Min", DoubleValue (area.xMin));
	m_xMinVar->SetAttribute ("Max", DoubleValue (area.xMax - m_xSize));
	m_yMinVar = CreateObject<UniformRandomVariable> ();
	m_yMinVar->SetAttribute ("Min", DoubleValue (area.yMin));
	m_yMinVar->SetAttribute ("Max", DoubleValue (area.yMax - m_ySize));
}

	void 
FemtocellBlockAllocator::Create (uint32_t n)
{
	for (uint32_t i = 0; i < n; ++i)
	{
		Create ();
	}
}

	void
FemtocellBlockAllocator::Create ()
{
	Box box;
	uint32_t attempt = 0;
	do 
	{
		NS_ASSERT_MSG (attempt < 100, "Too many failed attemtps to position apartment block. Too many blocks? Too small area?");
		box.xMin = m_xMinVar->GetValue ();
		box.xMax = box.xMin + m_xSize;
		box.yMin = m_yMinVar->GetValue ();
		box.yMax = box.yMin + m_ySize;
		++attempt;
	}
	while (OverlapsWithAnyPrevious (box));

	NS_LOG_LOGIC ("allocated non overlapping block " << box);
	m_previousBlocks.push_back (box);
	Ptr<GridBuildingAllocator>  gridBuildingAllocator;
	gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
	gridBuildingAllocator->SetAttribute ("GridWidth", UintegerValue (1));
	gridBuildingAllocator->SetAttribute ("LengthX", DoubleValue (10*m_nApartmentsX)); 
	gridBuildingAllocator->SetAttribute ("LengthY", DoubleValue (10*2));
	gridBuildingAllocator->SetAttribute ("DeltaX", DoubleValue (10));
	gridBuildingAllocator->SetAttribute ("DeltaY", DoubleValue (10));
	gridBuildingAllocator->SetAttribute ("Height", DoubleValue (3*m_nFloors));
	gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (m_nApartmentsX));
	gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (2));
	gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (m_nFloors));
	gridBuildingAllocator->SetAttribute ("MinX", DoubleValue (box.xMin + 10));
	gridBuildingAllocator->SetAttribute ("MinY", DoubleValue (box.yMin + 10));
	gridBuildingAllocator->Create (2);
}

	bool 
FemtocellBlockAllocator::OverlapsWithAnyPrevious (Box box)
{
	for (std::list<Box>::iterator it = m_previousBlocks.begin (); it != m_previousBlocks.end (); ++it)
	{
		if (AreOverlapping (*it, box))
		{
			return true;
		}
	}
	return false;
}

	void 
PrintGnuplottableBuildingListToFile (std::string filename)
{
	std::ofstream outFile;
	outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
	if (!outFile.is_open ())
	{
		NS_LOG_ERROR ("Can't open file " << filename);
		return;
	}
	uint32_t index = 0;
	for (BuildingList::Iterator it = BuildingList::Begin (); it != BuildingList::End (); ++it)
	{
		++index;
		Box box = (*it)->GetBoundaries ();
		outFile << "set object " << index
			<< " rect from " << box.xMin  << "," << box.yMin
			<< " to "   << box.xMax  << "," << box.yMax
			<< " front fs empty "
			<< std::endl;
	}
}

	void 
PrintGnuplottableUeListToFile (std::string filename)
{
	std::ofstream outFile;
	outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
	if (!outFile.is_open ())
	{
		NS_LOG_ERROR ("Can't open file " << filename);
		return;
	}
	for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
	{
		Ptr<Node> node = *it;
		int nDevs = node->GetNDevices ();
		for (int j = 0; j < nDevs; j++)
		{
			Ptr<LteUeNetDevice> uedev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
			if (uedev)
			{
				Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
				outFile << "set label \"" << uedev->GetImsi ()
					<< "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,4\" textcolor rgb \"grey\" front point pt 1 ps 0.3 lc rgb \"grey\" offset 0,0"
					<< std::endl;
			}
		}
	}
}

	void 
PrintGnuplottableEnbListToFile (std::string filename)
{
	std::ofstream outFile;
	outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
	if (!outFile.is_open ())
	{
		NS_LOG_ERROR ("Can't open file " << filename);
		return;
	}
	for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
	{
		Ptr<Node> node = *it;
		int nDevs = node->GetNDevices ();
		for (int j = 0; j < nDevs; j++)
		{
			Ptr<LteEnbNetDevice> enbdev = node->GetDevice (j)->GetObject <LteEnbNetDevice> ();
			if (enbdev)
			{
				Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
				outFile << "set label \"" << enbdev->GetCellId ()
					<< "\" at "<< pos.x << "," << pos.y
					<< " left font \"Helvetica,4\" textcolor rgb \"white\" front  point pt 2 ps 0.3 lc rgb \"white\" offset 0,0"
					<< std::endl;
			}
		}
	}
}


static ns3::GlobalValue g_nBlocks ("nBlocks",
		"Number of femtocell blocks",
		ns3::UintegerValue (1),
		ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_nApartmentsX ("nApartmentsX",
		"Number of apartments along the X axis in a femtocell block",
		ns3::UintegerValue (10),
		ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_nFloors ("nFloors",
		"Number of floors",
		ns3::UintegerValue (1),
		ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_nMacroEnbSites ("nMacroEnbSites",
		"How many macro sites there are",
		ns3::UintegerValue (19),
		ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_nMacroEnbSitesX ("nMacroEnbSitesX",
		"(minimum) number of sites along the X-axis of the hex grid",
		ns3::UintegerValue (3),
		ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_interSiteDistance ("interSiteDistance",
		"min distance between two nearby macro cell sites",
		ns3::DoubleValue (500),
		ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_areaMarginFactor ("areaMarginFactor",
		"how much the UE area extends outside the macrocell grid, "
		"expressed as fraction of the interSiteDistance",
		ns3::DoubleValue (0.5),
		ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_macroUeDensity ("macroUeDensity",
		"How many macrocell UEs there are per square meter",
		ns3::DoubleValue (0.00002),
		ns3::MakeDoubleChecker<double> ());                                     
static ns3::GlobalValue g_macroEnbTxPowerDbm ("macroEnbTxPowerDbm",
		"TX power [dBm] used by macro eNBs",
		ns3::DoubleValue (46.0),
		ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_macroEnbDlEarfcn ("macroEnbDlEarfcn",
		"DL EARFCN used by macro eNBs",
		ns3::UintegerValue (100),
		ns3::MakeUintegerChecker<uint16_t> ());
static ns3::GlobalValue g_macroEnbBandwidth ("macroEnbBandwidth",
		"bandwidth [num RBs] used by macro eNBs",
		ns3::UintegerValue (50),
		ns3::MakeUintegerChecker<uint16_t> ());
static ns3::GlobalValue g_simTime ("simTime",
		"Total duration of the simulation [s]",
		ns3::DoubleValue (50.0),
		ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_positionIndex ("positionIndex",
		"node position Index",
		ns3::UintegerValue (1),
		MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_generateRem ("generateRem",
		"if true, will generate a REM and then abort the simulation;"
		"if false, will run the simulation normally (without generating any REM)",
		ns3::BooleanValue (false),
		ns3::MakeBooleanChecker ());
static ns3::GlobalValue g_remRbId ("remRbId",
		"Resource Block Id of Data Channel, for which REM will be generated;"
		"default value is -1, what means REM will be averaged from all RBs of "
		"Control Channel",
		ns3::IntegerValue (-1),
		MakeIntegerChecker<int32_t> ());
static ns3::GlobalValue g_epc ("epc",
		"If true, will setup the EPC to simulate an end-to-end topology, "
		"with real IP applications over PDCP and RLC UM (or RLC AM by changing "
		"the default value of EpsBearerToRlcMapping e.g. to RLC_AM_ALWAYS). "
		"If false, only the LTE radio access will be simulated with RLC SM. ",
		ns3::BooleanValue (true),
		ns3::MakeBooleanChecker ());
static ns3::GlobalValue g_epcDl ("epcDl",
		"if true, will activate data flows in the downlink when EPC is being used. "
		"If false, downlink flows won't be activated. "
		"If EPC is not used, this parameter will be ignored.",
		ns3::BooleanValue (true),
		ns3::MakeBooleanChecker ());
static ns3::GlobalValue g_epcUl ("epcUl",
		"if true, will activate data flows in the uplink when EPC is being used. "
		"If false, uplink flows won't be activated. "
		"If EPC is not used, this parameter will be ignored.",
		ns3::BooleanValue (false),
		ns3::MakeBooleanChecker ());
static ns3::GlobalValue g_useUdp ("useUdp",
		"if true, the UdpClient application will be used. "
		"Otherwise, the BulkSend application will be used over a TCP connection. "
		"If EPC is not used, this parameter will be ignored.",
		ns3::BooleanValue (false),
		ns3::MakeBooleanChecker ());
static ns3::GlobalValue g_fadingTrace ("fadingTrace",
		"The path of the fading trace (by default no fading trace "
		"is loaded, i.e., fading is not considered)",
		ns3::StringValue ("src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad"),
		ns3::MakeStringChecker ());
static ns3::GlobalValue g_numBearersPerUe ("numBearersPerUe",
		"How many bearers per UE there are in the simulation",
		ns3::UintegerValue (1),
		ns3::MakeUintegerChecker<uint16_t> ());
static ns3::GlobalValue g_srsPeriodicity ("srsPeriodicity",
		"SRS Periodicity (has to be at least "
		"greater than the number of UEs per eNB)",
		ns3::UintegerValue (80),
		ns3::MakeUintegerChecker<uint16_t> ());
static ns3::GlobalValue g_outdoorUeMinSpeed ("outdoorUeMinSpeed",
		"Minimum speed value of macor UE with random waypoint model [m/s].",
		ns3::DoubleValue (3/3.6),
		ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_outdoorUeMaxSpeed ("outdoorUeMaxSpeed",
		"Maximum speed value of macor UE with random waypoint model [m/s].",
		ns3::DoubleValue (3/3.6),
		ns3::MakeDoubleChecker<double> ());


//
//ns3::StringValue ("src/lte/model/fading-traces/fading_trace_ETU_0kmph.fad"),
	int
main (int argc, char *argv[])
{
	// change some default attributes so that they are reasonable for
	// this scenario, but do this before processing command line
	// arguments, so that the user is allowed to override these settings 
	Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MicroSeconds (1024*8*(1/7.5))));
	Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue (4294967295));
	Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (1000 * 1024));
	Config::SetDefault ("ns3::LteHelper::UsePdschForCqiGeneration", BooleanValue (false));
	Config::SetDefault ("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (true));
	//Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue (ns3::LteEnbRrc::RLC_AM_ALWAYS));//RLC_UM_ALWAYS

	CommandLine cmd;
	cmd.Parse (argc, argv);
	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults ();
	// parse again so you can override input file default values via command line
	cmd.Parse (argc, argv); 

	// the scenario parameters get their values from the global attributes defined above
	UintegerValue uintegerValue;
	IntegerValue integerValue;
	DoubleValue doubleValue;
	BooleanValue booleanValue;
	StringValue stringValue;

	/*GlobalValue::GetValueByName ("nBlocks", uintegerValue);
	  uint32_t nBlocks = uintegerValue.Get ();
	  GlobalValue::GetValueByName ("nApartmentsX", uintegerValue);
	  uint32_t nApartmentsX = uintegerValue.Get ();
	  GlobalValue::GetValueByName ("nFloors", uintegerValue);
	  uint32_t nFloors = uintegerValue.Get ();
	  */
	GlobalValue::GetValueByName ("nMacroEnbSites", uintegerValue);
	uint32_t nMacroEnbSites = uintegerValue.Get ();
	GlobalValue::GetValueByName ("nMacroEnbSitesX", uintegerValue);
	uint32_t nMacroEnbSitesX = uintegerValue.Get ();
	GlobalValue::GetValueByName ("interSiteDistance", doubleValue);
	double interSiteDistance = doubleValue.Get ();
	GlobalValue::GetValueByName ("areaMarginFactor", doubleValue);
	double areaMarginFactor = doubleValue.Get ();
	// GlobalValue::GetValueByName ("macroUeDensity", doubleValue);
	// double macroUeDensity = doubleValue.Get ();
	GlobalValue::GetValueByName ("macroEnbTxPowerDbm", doubleValue);
	double macroEnbTxPowerDbm = doubleValue.Get ();
	GlobalValue::GetValueByName ("macroEnbDlEarfcn", uintegerValue);
	uint16_t macroEnbDlEarfcn = uintegerValue.Get ();
	GlobalValue::GetValueByName ("macroEnbBandwidth", uintegerValue);
	uint16_t macroEnbBandwidth = uintegerValue.Get ();
	GlobalValue::GetValueByName ("simTime", doubleValue);
	double simTime = doubleValue.Get ();
	GlobalValue::GetValueByName ("positionIndex", uintegerValue);
	uint32_t positionIndex = uintegerValue.Get ();
	GlobalValue::GetValueByName ("epc", booleanValue);
	bool epc = booleanValue.Get ();
	GlobalValue::GetValueByName ("epcDl", booleanValue);
	bool epcDl = booleanValue.Get ();
	GlobalValue::GetValueByName ("epcUl", booleanValue);
	bool epcUl = booleanValue.Get ();
	GlobalValue::GetValueByName ("useUdp", booleanValue);
	bool useUdp = booleanValue.Get ();
	GlobalValue::GetValueByName ("generateRem", booleanValue);
	bool generateRem = booleanValue.Get ();
	GlobalValue::GetValueByName ("remRbId", integerValue);
	int32_t remRbId = integerValue.Get ();
	GlobalValue::GetValueByName ("fadingTrace", stringValue);
	std::string fadingTrace = stringValue.Get ();
	GlobalValue::GetValueByName ("numBearersPerUe", uintegerValue);
	uint16_t numBearersPerUe = uintegerValue.Get ();
	GlobalValue::GetValueByName ("srsPeriodicity", uintegerValue);
	uint16_t srsPeriodicity = uintegerValue.Get ();
	GlobalValue::GetValueByName ("outdoorUeMinSpeed", doubleValue);
	double outdoorUeMinSpeed = doubleValue.Get ();
	GlobalValue::GetValueByName ("outdoorUeMaxSpeed", doubleValue);
	double outdoorUeMaxSpeed = doubleValue.Get ();

	Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (srsPeriodicity));

	std::ostringstream oss;
	if(!useUdp){
		oss << "bashoutput/single/bulksend/ctrlErrorOn/tcp/location"
			<< positionIndex
			<<"/";}
	else{
		oss << "bashoutput/single/bulksend/ctrlErrorOn/udp/location"
			<< positionIndex
			<<"/";}

	trFileDir = oss.str ();////

	Box macroUeBox;
	double ueZ = 1.5;
	if (nMacroEnbSites > 0)
	{
		uint32_t currentSite = nMacroEnbSites -1;
		uint32_t biRowIndex = (currentSite / (nMacroEnbSitesX + nMacroEnbSitesX + 1));
		uint32_t biRowRemainder = currentSite % (nMacroEnbSitesX + nMacroEnbSitesX + 1);
		uint32_t rowIndex = biRowIndex*2 + 1;
		if (biRowRemainder >= nMacroEnbSitesX)
		{
			++rowIndex;
		}
		uint32_t nMacroEnbSitesY = rowIndex;
		NS_LOG_LOGIC ("nMacroEnbSitesY = " << nMacroEnbSitesY);

		macroUeBox = Box (-areaMarginFactor*interSiteDistance, 
				(nMacroEnbSitesX + areaMarginFactor)*interSiteDistance, 
				-areaMarginFactor*interSiteDistance, 
				(nMacroEnbSitesY -1)*interSiteDistance*sqrt (0.75) + areaMarginFactor*interSiteDistance,
				ueZ, ueZ);
	}
	else
	{
		// still need the box to place femtocell blocks
		macroUeBox = Box (0, 150, 0, 150, ueZ, ueZ);
	}

	//FemtocellBlockAllocator blockAllocator (macroUeBox, nApartmentsX, nFloors);
	//blockAllocator.Create (nBlocks);


	//double macroUeAreaSize = (macroUeBox.xMax - macroUeBox.xMin) * (macroUeBox.yMax - macroUeBox.yMin);
	//uint32_t nMacroUes = round (macroUeAreaSize * macroUeDensity);
	//NS_LOG_LOGIC ("nMacroUes = " << nMacroUes << " (density=" << macroUeDensity << ")");
	NS_LOG_LOGIC ("nMacroUes = " << nMacroUes);
	NodeContainer macroEnbs;
	macroEnbs.Create (3 * nMacroEnbSites);
	NodeContainer macroUes;
	macroUes.Create (nMacroUes);

	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

	Ptr <LteHelper> lteHelper = CreateObject<LteHelper> ();
	lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::HybridBuildingsPropagationLossModel"));
	lteHelper->SetPathlossModelAttribute ("ShadowSigmaExtWalls", DoubleValue (0));
	lteHelper->SetPathlossModelAttribute ("ShadowSigmaOutdoor", DoubleValue (1));
	lteHelper->SetPathlossModelAttribute ("ShadowSigmaIndoor", DoubleValue (1.5));
	// use always LOS model
	lteHelper->SetPathlossModelAttribute ("Los2NlosThr", DoubleValue (1e6));
	lteHelper->SetSpectrumChannelType ("ns3::MultiModelSpectrumChannel");

	//   lteHelper->EnableLogComponents ();
	//   LogComponentEnable ("PfFfMacScheduler", LOG_LEVEL_ALL);

	if (!fadingTrace.empty ())
	{
		lteHelper->SetAttribute ("FadingModel", StringValue ("ns3::TraceFadingLossModel"));
		lteHelper->SetFadingModelAttribute ("TraceFilename", StringValue (fadingTrace));
	}

	Ptr<PointToPointEpcHelper> epcHelper;
	if (epc)
	{
		NS_LOG_LOGIC ("enabling EPC");
		epcHelper = CreateObject<PointToPointEpcHelper> ();
		lteHelper->SetEpcHelper (epcHelper);
	}

	// Macro eNBs in 3-sector hex grid

	mobility.Install (macroEnbs);
	BuildingsHelper::Install (macroEnbs);
	Ptr<LteHexGridEnbTopologyHelper> lteHexGridEnbTopologyHelper = CreateObject<LteHexGridEnbTopologyHelper> ();
	lteHexGridEnbTopologyHelper->SetLteHelper (lteHelper);
	lteHexGridEnbTopologyHelper->SetAttribute ("InterSiteDistance", DoubleValue (interSiteDistance));
	lteHexGridEnbTopologyHelper->SetAttribute ("MinX", DoubleValue (-500));
	lteHexGridEnbTopologyHelper->SetAttribute ("MinY", DoubleValue (-500*sqrt(3)));
	lteHexGridEnbTopologyHelper->SetAttribute ("GridWidth", UintegerValue (nMacroEnbSitesX));
	Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (macroEnbTxPowerDbm));
	lteHelper->SetEnbAntennaModelType ("ns3::ParabolicAntennaModel");
	lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (70));
	lteHelper->SetEnbAntennaModelAttribute ("MaxAttenuation",     DoubleValue (20.0));
	lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (macroEnbDlEarfcn));
	lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (macroEnbDlEarfcn + 18000));
	lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (macroEnbBandwidth));
	lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (macroEnbBandwidth));
	NetDeviceContainer macroEnbDevs = lteHexGridEnbTopologyHelper->Set2LayerHexPositionAndInstallEnbDevice (macroEnbs);

	if (epc)
	{
		// this enables handover for macro eNBs
		lteHelper->AddX2Interface (macroEnbs);
	}


	Ptr<PositionAllocator> positionAlloc = CreateObject<RandomRoomPositionAllocator> ();

	// macro Ues
	NS_LOG_LOGIC ("UEs speedMin " << outdoorUeMinSpeed << " speedMax " << outdoorUeMaxSpeed);


	if (outdoorUeMaxSpeed!=0.0)
	{
        mobility.SetMobilityModel ("ns3::SteadyStateRandomWaypointMobilityModel");
		float deltaX = 20;

		int32_t fromX = (300 - (positionIndex)*deltaX);

		uint32_t i =0;
		for (NodeContainer::Iterator it = macroUes.Begin ();it != macroUes.End ();++it){

			Box ueBox = Box(int32_t(fromX - 10 ),  
					int32_t(fromX + 10 ), 
					-10, 10, ueZ, ueZ); 

			uePosX.at(i) = ueBox.xMin + 10;
			uePosY.at(i) = ueBox.yMin + 10;


			Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinX", DoubleValue (ueBox.xMin));
			Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinY", DoubleValue (ueBox.yMin));
			Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxX", DoubleValue (ueBox.xMax));
			Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxY", DoubleValue (ueBox.yMax));
			Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::Z", DoubleValue (ueZ));
			Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxSpeed", DoubleValue (outdoorUeMaxSpeed));
			Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinSpeed", DoubleValue (outdoorUeMinSpeed));

			NS_LOG_INFO("UE"<< i <<" in box "<< ueBox);
			// this is not used since SteadyStateRandomWaypointMobilityModel
			// takes care of initializing the positions;  however we need to
			// reset it since the previously used PositionAllocator
			positionAlloc = CreateObject<RandomBoxPositionAllocator> ();
			mobility.SetPositionAllocator (positionAlloc);
			mobility.Install ((*it));

			i++;
		}
		// forcing initialization so we don't have to wait for Nodes to
		// start before positions are assigned (which is needed to
		// output node positions to file and to make AttachToClosestEnb work)
		for (NodeContainer::Iterator it = macroUes.Begin ();
				it != macroUes.End ();
				++it)
		{
			(*it)->Initialize ();
		}
	}
	else
	{
		MobilityHelper macroUeMob;
		Ptr<ListPositionAllocator> uePosAlloc = CreateObject<ListPositionAllocator> ();

		for (uint32_t i = 0; i < nMacroUes; i++)
		{

			Vector posNode (int32_t(80*(i+1)), 0, 1.5);

			//   std::cout << int32_t(currentX * (i+1)/3) <<"\t"<< int32_t(currentY*(i+1)/3) << std::endl;
			//Vector posNode (int32_t(currentX * (i+1)/3), int32_t(currentY*(i+1)/3), 1.5);
			uePosAlloc->Add(posNode);
			//                uePosX.at(i) = int32_t(refOrigin + (currentX - refOrigin)*(i+1)/3);
			//                uePosY.at(i) = int32_t(currentY*(i+1)/3);
		}
		macroUeMob.SetPositionAllocator (uePosAlloc);
		macroUeMob.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); 
		macroUeMob.Install (macroUes);
	}
	BuildingsHelper::Install (macroUes);

	NetDeviceContainer macroUeDevs = lteHelper->InstallUeDevice (macroUes);

	Ipv4Address remoteHostAddr;
	NodeContainer ues;
	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	Ipv4InterfaceContainer ueIpIfaces;
	Ptr<Node> remoteHost;
	NetDeviceContainer ueDevs;

	NS_LOG_LOGIC ("epc? "<<epc);
	if (epc)
	{
		NS_LOG_LOGIC ("setting up internet and remote host");

		// Create a single RemoteHost
		NodeContainer remoteHostContainer;
		remoteHostContainer.Create (1);
		remoteHost = remoteHostContainer.Get (0);
		InternetStackHelper internet;
		internet.Install (remoteHostContainer);

		// Create the Internet
		PointToPointHelper p2ph;
		p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
		p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
		p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));//100ms
		Ptr<Node> pgw = epcHelper->GetPgwNode ();
		NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
		Ipv4AddressHelper ipv4h;
		ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
		Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
		// in this container, interface 0 is the pgw, 1 is the remoteHost
		remoteHostAddr = internetIpIfaces.GetAddress (1);

		Ipv4StaticRoutingHelper ipv4RoutingHelper;
		Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
		remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

		ues.Add (macroUes);
		ueDevs.Add (macroUeDevs);

		// Install the IP stack on the UEs
		internet.Install (ues);
		ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

		// attachment (needs to be done after IP stack configuration)
		// using initial cell selection
		lteHelper->Attach (macroUeDevs);//,macroEnbDevs.Get(0));
	}
	else
	{
		// macro UEs attached to the closest macro eNB
		lteHelper->AttachToClosestEnb (macroUeDevs, macroEnbDevs);

		NetDeviceContainer::Iterator ueDevIt;
		NetDeviceContainer::Iterator enbDevIt;
		{
			// this because of the order in which SameRoomPositionAllocator
			// will place the UEs
			{
			}
			lteHelper->Attach (*ueDevIt, *enbDevIt);
		}
	}

	if (epc)
	{
		NS_LOG_LOGIC ("setting up applications");

		// Install and start applications on UEs and remote host
		uint16_t dlPort = 10000;
		uint16_t ulPort = 20000;

		// randomize a bit start times to avoid simulation artifacts
		// (e.g., buffer overflows due to packet transmissions happening
		// exactly at the same time) 
		Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
		if (useUdp)
		{
			startTimeSeconds->SetAttribute ("Min", DoubleValue (0));
			startTimeSeconds->SetAttribute ("Max", DoubleValue (0.010));
		}
		else
		{
			// TCP needs to be started late enough so that all UEs are connected
			// otherwise TCP SYN packets will get lost
			startTimeSeconds->SetAttribute ("Min", DoubleValue (0.100));
			startTimeSeconds->SetAttribute ("Max", DoubleValue (0.110));
		}

		for (uint32_t u = 0; u < ues.GetN (); ++u)
		{
			Ptr<Node> ue = ues.Get (u);
			// Set the default gateway for the UE
			Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
			ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

			for (uint32_t b = 0; b < numBearersPerUe; ++b)
			{
				++dlPort;
				++ulPort;

				if (useUdp)
				{
					if (epcDl)
					{
						NS_LOG_LOGIC ("installing UDP DL app for UE " << u);
						UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);
						sourceApps.Add (dlClientHelper.Install (remoteHost));
						PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", 
								InetSocketAddress (Ipv4Address::GetAny (), dlPort));
						sinkApps.Add (dlPacketSinkHelper.Install (ue));
					}
					if (epcUl)
					{
						NS_LOG_LOGIC ("installing UDP UL app for UE " << u);
						UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
						sourceApps.Add (ulClientHelper.Install (ue));
						PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", 
								InetSocketAddress (Ipv4Address::GetAny (), ulPort));
						sinkApps.Add (ulPacketSinkHelper.Install (remoteHost));
					}
				}
				else // use TCP
				{
					if (epcDl)
					{
						NS_LOG_LOGIC ("installing TCP DL app for UE " << u);
						BulkSendHelper dlClientHelper ("ns3::TcpSocketFactory",
						                              InetSocketAddress (ueIpIfaces.GetAddress (u), dlPort));
						dlClientHelper.SetAttribute ("SendSize", UintegerValue (1024));//in bytes						
                                                //dlClientHelper.SetAttribute ("MaxBytes", UintegerValue (1000));
						//OnOffHelper dlClientHelper ("ns3::TcpSocketFactory", InetSocketAddress (ueIpIfaces.GetAddress (u), dlPort));
						//dlClientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
						//dlClientHelper.SetAttribute ("DataRate", ns3::DataRateValue(7.5*1000000));//in bps
						//dlClientHelper.SetAttribute ("PacketSize", UintegerValue (1024));//in bytes
						sourceApps.Add (dlClientHelper.Install (remoteHost));
						PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory", 
								InetSocketAddress (Ipv4Address::GetAny (), dlPort));
						sinkApps.Add (dlPacketSinkHelper.Install (ue));
					}
					if (epcUl)
					{
						NS_LOG_LOGIC ("installing TCP UL app for UE " << u);
						BulkSendHelper ulClientHelper ("ns3::TcpSocketFactory",
								InetSocketAddress (remoteHostAddr, ulPort));
						//ulClientHelper.SetAttribute ("MaxBytes", UintegerValue (1000));
						sourceApps.Add (ulClientHelper.Install (ue));
						PacketSinkHelper ulPacketSinkHelper ("ns3::TcpSocketFactory", 
								InetSocketAddress (Ipv4Address::GetAny (), ulPort));
						sinkApps.Add (ulPacketSinkHelper.Install (remoteHost));
					}
				} // end if (useUdp)

				Ptr<EpcTft> tft = Create<EpcTft> ();
				if (epcDl)
				{
					EpcTft::PacketFilter dlpf;
					dlpf.localPortStart = dlPort;
					dlpf.localPortEnd = dlPort;
					tft->Add (dlpf); 
				}
				if (epcUl)
				{
					EpcTft::PacketFilter ulpf;
					ulpf.remotePortStart = ulPort;
					ulpf.remotePortEnd = ulPort;
					tft->Add (ulpf);
				}

				if (epcDl || epcUl)
				{
					EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
					lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (u), bearer, tft);
				}
				Time startTime = Seconds (startTimeSeconds->GetValue ());
				sinkApps.Start (startTime);
				sourceApps.Start (startTime);

			} // end for b
		}

	} 
	else // (epc == false)
	{
		NetDeviceContainer ueDevs;
		ueDevs.Add (macroUeDevs);
		for (uint32_t u = 0; u < ueDevs.GetN (); ++u)
		{
			Ptr<NetDevice> ueDev = ueDevs.Get (u);
			for (uint32_t b = 0; b < numBearersPerUe; ++b)
			{
				enum EpsBearer::Qci q = EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
				EpsBearer bearer (q);
				lteHelper->ActivateDataRadioBearer (ueDev, bearer);
			}
		}
	}

	BuildingsHelper::MakeMobilityModelConsistent ();

	Ptr<RadioEnvironmentMapHelper> remHelper;
	if (generateRem)
	{
		PrintGnuplottableBuildingListToFile ("buildings.txt");
		PrintGnuplottableEnbListToFile ("enbs.txt");
		PrintGnuplottableUeListToFile ("ues.txt");

		remHelper = CreateObject<RadioEnvironmentMapHelper> ();
		remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/0"));
		remHelper->SetAttribute ("OutputFile", StringValue ("lte-grid.rem"));
		remHelper->SetAttribute ("Earfcn", UintegerValue (macroEnbDlEarfcn));
		//      remHelper->SetAttribute ("XMin", DoubleValue (macroUeBox.xMin));
		//      remHelper->SetAttribute ("XMax", DoubleValue (macroUeBox.xMax));
		remHelper->SetAttribute ("XMin", DoubleValue (-1300));
		remHelper->SetAttribute ("XMax", DoubleValue (1300));
		remHelper->SetAttribute ("XRes", UintegerValue (4000));
		remHelper->SetAttribute ("YMin", DoubleValue (-1300));
		remHelper->SetAttribute ("YMax", DoubleValue (1300));
		//      remHelper->SetAttribute ("YMin", DoubleValue (macroUeBox.yMin));
		//      remHelper->SetAttribute ("YMax", DoubleValue (macroUeBox.yMax));
		remHelper->SetAttribute ("XRes", UintegerValue (4000));
		remHelper->SetAttribute ("Z", DoubleValue (1.5));

		if (remRbId >= 0)
		{
			//    remHelper->SetAttribute ("UseDataChannel", BooleanValue (true));
			//   remHelper->SetAttribute ("RbId", IntegerValue (remRbId));
		}

		remHelper->Install ();
		// simulation will stop right after the REM has been generated
	}
	else
	{
		Simulator::Stop (Seconds (simTime));
	}

	lteHelper->EnableTraces ();
	/******************************************
	 *Setting up Flow Monitor
	 *******************************************/
	Ptr<FlowMonitor> monitor;
	FlowMonitorHelper flowHelper;
	NodeContainer flowEnd2End = macroUes;
	flowEnd2End.Add(remoteHost);
	monitor = flowHelper.Install(flowEnd2End);

	lastReportTime=new Time(MicroSeconds(0));

	if(!useUdp&&ifTraceTcp){
		Simulator::Schedule (Seconds (0.11001), &TraceCwnd, cwnd_tr_file_name);
		Simulator::Schedule (Seconds (0.11001), &TraceSsThresh, ssthresh_tr_file_name);
		Simulator::Schedule (Seconds (0.11001), &TraceRtt, rtt_tr_file_name);
		Simulator::Schedule (Seconds (0.11001), &TraceRto, rto_tr_file_name);
		Simulator::Schedule (Seconds (0.11001), &TraceRwnd, rwnd_tr_file_name);
		Simulator::Schedule (Seconds (0.11001), &TraceHighestRxAck, hrack_tr_file_name);
		Simulator::Schedule (Seconds (3), PrintProgress, thrputStream);
	}
	if(useUdp)
		Simulator::Schedule (Seconds (0.25), PrintProgress, thrputStream);


	Simulator::Run ();

	//GtkConfigStore config;
	//config.ConfigureAttributes ();
	//After run

	//	uint32_t numSinkApps = (cfg.downLink==true)?(cfg.dlSinkApps.GetN()):(cfg.ulSinkApps.GetN());
	for (uint32_t appIdx = 0; appIdx < sinkApps.GetN(); appIdx++ ){

		Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (appIdx));
		//std::cout << " Packet sink " << appIdx << " Rx " << sink1->GetTotalRx () << " bytes" << std::endl;
		//std::cout << std::endl;
	}


	monitor->CheckForLostPackets ();
	double throughput = 0.0;

	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
	uint32_t ueId = 0;
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
	{
		std::cout << " " << uePosX.at(ueId) << "\t" << uePosY.at(ueId) << "\t";
		ueId++;
		std::cout << " " << i->second.txBytes << "\t";
		std::cout << " " << i->second.rxBytes << "\t";
		std::cout << " " << i->second.lostPackets << "\t";
		//			std::cout << " bytesDropped " << i->second.bytesDropped	 << "\t";

		throughput = i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024;
		std::cout << " " <<  throughput << " \t";			
		
if (ueId == 3)
			break;
	}
	std::cout << "\n";

	monitor->SerializeToXmlFile("lte-grid.flowmon", true, true);


	lteHelper = 0;
	Simulator::Destroy ();
	return 0;
}
