
using namespace ns3;

class CustomizedApp : public Application 
{
  public:
	CustomizedApp ();
	virtual ~CustomizedApp();
	void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate,
		    Ipv4Address src, Ipv4Address dst);

  private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);
	void ScheduleTx (void);
	void SendPacket (void);

	Ptr<Socket>     m_socket;
	Address         m_peer;
	uint32_t        m_packetSize;
	DataRate        m_dataRate;
	EventId         m_sendEvent;
	bool            m_running;
	uint32_t        m_packetsSent;
	Ipv4Address	m_src, m_dst;

};

CustomizedApp::CustomizedApp ()
	: m_socket (0), 
	  m_peer (), 
	  m_packetSize (0), 
	  m_dataRate (0), 
	  m_sendEvent (), 
	  m_running (false), 
	  m_packetsSent (0)
{
}

CustomizedApp::~CustomizedApp()
{
	m_socket = 0;
}

void
CustomizedApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate,
        Ipv4Address src, Ipv4Address dst)
{
	m_socket = socket;
	m_peer = address;
	m_packetSize = packetSize;
	m_dataRate = dataRate;
  	m_src = src;
	m_dst = dst;
}

void
CustomizedApp::StartApplication (void)
{
	m_running = true;
	m_packetsSent = 0;
	m_socket->Bind ();
	m_socket->Connect (m_peer);
	SendPacket ();
}

void 
CustomizedApp::StopApplication (void)
{
	m_running = false;

	if (m_sendEvent.IsRunning ())
		Simulator::Cancel (m_sendEvent);
	if (m_socket)
		m_socket->Close ();
}

void 
CustomizedApp::SendPacket (void)
{
	Ptr<Packet> packet = Create<Packet> (m_packetSize);
//std::cout << "set IP header" << std::endl;
	Ipv4Header iph;
  	iph.SetSource (m_src);
	iph.SetDestination (m_dst);
	iph.SetTtl (255);
	packet->AddHeader (iph);
	
	m_socket->Send (packet);
	++m_packetsSent;
	ScheduleTx ();
}

void 
CustomizedApp::ScheduleTx (void)
{
	if (m_running) {
		Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
		m_sendEvent = Simulator::Schedule (tNext, &CustomizedApp::SendPacket, this);
	}
}

