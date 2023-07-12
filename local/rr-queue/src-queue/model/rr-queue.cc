/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// F.Qian, Jan. 2013, fei@kanto-gakuin.ac.jp
// Packet Scheduller with Round Robin Method 

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/callback.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-header.h"
#include "ns3/ppp-header.h"
#include "rr-queue.h"

NS_LOG_COMPONENT_DEFINE ("RRQueue");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RRQueue);

TypeId RRQueue::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::RRQueue")
    .SetParent<Queue> ()
    .AddConstructor<RRQueue> ()
    .AddAttribute ("Mode", 
                   "Whether to use bytes (see MaxBytes) or packets (see MaxPackets) as the maximum queue size metric.",
                   EnumValue (QUEUE_MODE_PACKETS),
                   MakeEnumAccessor (&RRQueue::SetMode),
                   MakeEnumChecker (QUEUE_MODE_BYTES, "QUEUE_MODE_BYTES",
                                    QUEUE_MODE_PACKETS, "QUEUE_MODE_PACKETS"))
    .AddAttribute ("MaxPackets", 
                   "The maximum number of packets accepted by this RRQueue.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&RRQueue::m_maxPackets),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxBytes", 
                   "The maximum number of bytes accepted by this RRQueue.",
                   UintegerValue (100 * 65535),
                   MakeUintegerAccessor (&RRQueue::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NumFlows", 
                   "The maximum number of flows(1-6).",
                   UintegerValue (3),
                   MakeUintegerAccessor (&RRQueue::m_numflows),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Flow1_Source", 
                   "The source of traffic flow1.(192.168.1.1)",
                   StringValue ("192.168.1.1"),
                   MakeStringAccessor (&RRQueue::m_src1),
                   MakeStringChecker ())
    .AddAttribute ("Flow1_Destination", 
                   "The source of traffic flow1.(192.168.10.1)",
                   StringValue ("192.168.10.1"),
                   MakeStringAccessor (&RRQueue::m_dst1),
                   MakeStringChecker ())
    .AddAttribute ("Flow2_Source", 
                   "The source of traffic flow1.(192.168.1.2)",
                   StringValue ("192.168.1.2"),
                   MakeStringAccessor (&RRQueue::m_src2),
                   MakeStringChecker ())
    .AddAttribute ("Flow2_Destination", 
                   "The source of traffic flow1.(192.168.10.2)",
                   StringValue ("192.168.10.2"),
                   MakeStringAccessor (&RRQueue::m_dst2),
                   MakeStringChecker ())
    .AddAttribute ("Flow3_Source", 
                   "The source of traffic flow1.(192.168.1.3)",
                   StringValue ("192.168.1.3"),
                   MakeStringAccessor (&RRQueue::m_src3),
                   MakeStringChecker ())
    .AddAttribute ("Flow3_Destination", 
                   "The source of traffic flow3.(192.168.10.3)",
                   StringValue ("192.168.10.3"),
                   MakeStringAccessor (&RRQueue::m_dst3),
                   MakeStringChecker ())
    .AddAttribute ("Flow4_Source", 
                   "The source of traffic flow1.(192.168.1.4)",
                   StringValue ("192.168.1.4"),
                   MakeStringAccessor (&RRQueue::m_src4),
                   MakeStringChecker ())
    .AddAttribute ("Flow4_Destination", 
                   "The source of traffic flow3.(192.168.10.4)",
                   StringValue ("192.168.10.4"),
                   MakeStringAccessor (&RRQueue::m_dst4),
                   MakeStringChecker ())
    .AddAttribute ("Flow5_Source", 
                   "The source of traffic flow1.(192.168.1.5)",
                   StringValue ("192.168.1.5"),
                   MakeStringAccessor (&RRQueue::m_src5),
                   MakeStringChecker ())
    .AddAttribute ("Flow5_Destination", 
                   "The source of traffic flow3.(192.168.10.5)",
                   StringValue ("192.168.10.5"),
                   MakeStringAccessor (&RRQueue::m_dst5),
                   MakeStringChecker ())
    .AddAttribute ("Flow6_Source", 
                   "The source of traffic flow1.(192.168.1.6)",
                   StringValue ("192.168.1.6"),
                   MakeStringAccessor (&RRQueue::m_src6),
                   MakeStringChecker ())
    .AddAttribute ("Flow6_Destination", 
                   "The source of traffic flow3.(192.168.10.6)",
                   StringValue ("192.168.10.6"),
                   MakeStringAccessor (&RRQueue::m_dst6),
                   MakeStringChecker ())
  ;

  return tid;
}

RRQueue::RRQueue () :
	Queue (),
	//m_packets (),
	//m_bytesInQueue (0),
	m_hasRRStarted(false),
	m_turn (0)
{
	NS_LOG_FUNCTION (this);
	// initialize virtual queue(maximum number of queues is 16)
	for(uint16_t i=0;i<16;i++)
		m_bytesInVQueue[i] = 0;
}

RRQueue::~RRQueue ()
{
	NS_LOG_FUNCTION (this);
}

void
RRQueue::SetMode (RRQueue::QueueMode mode)
{
	NS_LOG_FUNCTION (this << mode);
	m_mode = mode;
}

RRQueue::QueueMode
RRQueue::GetMode (void)
{
	NS_LOG_FUNCTION (this);
	return m_mode;
}

void
RRQueue::SetParameters ()
{
        NS_LOG_FUNCTION (this);

        std::string key;
        switch (m_numflows) {
        case 6:
                key = m_src6 + "->" + m_dst6;
                m_flowMap.insert(std::make_pair(key, 5));
                NS_LOG_DEBUG(key << " : 5");
        case 5:
                key = m_src5 + "->" + m_dst5;
                m_flowMap.insert(std::make_pair(key, 4));
                NS_LOG_DEBUG(key << " : 4");
        case 4:
                key = m_src4 + "->" + m_dst4;
                m_flowMap.insert(std::make_pair(key, 3));
                NS_LOG_DEBUG(key << " : 3");
        case 3:
                key = m_src3 + "->" + m_dst3;
                m_flowMap.insert(std::make_pair(key, 2));
                NS_LOG_DEBUG(key << " : 2");

        case 2:
                key = m_src2 + "->" + m_dst2;
                m_flowMap.insert(std::make_pair(key, 1));
                NS_LOG_DEBUG(key << " : 1");
        case 1:
                key = m_src1 + "->" + m_dst1;
                m_flowMap.insert(std::make_pair(key, 0));
                NS_LOG_DEBUG(key << " : 0");
        }
}

#include <stdio.h>
void 
RRQueue::dumpPacketBuffer (Ptr<Packet> p)
{
        // get flowID(src, dst)
        uint8_t *buffer = new uint8_t [p->GetSize()];
        p->CopyData(buffer, p->GetSize());

        char   ver[3]; sprintf(  ver, "%d", (buffer[2]&0xf0)>>4);
        char hsize[3]; sprintf(hsize, "%d", buffer[2]&0x0f);
        char   tos[5]; sprintf(  tos, "0x%02x", buffer[3]);
        char  size[8]; sprintf( size, "%d", ((buffer[4]&0x1f)<<8)|buffer[5]);
        char    id[8]; sprintf(   id, "%d",(buffer[6]<<8)|buffer[7]);
        char  flag[3]; sprintf( flag, "0x%02x",(buffer[8]&0xe0)>>5);
        char   seg[8]; sprintf(  seg, "%d",((buffer[8]&0x1f)<<8)|buffer[9]);
        char   ttl[6]; sprintf(  ttl, "%d", buffer[10]);
        char proto[6]; sprintf(proto, "%d", buffer[11]);
        char  csum[8]; sprintf( csum, "0x%04x", ((buffer[12]&0x1f)<<8)|buffer[13]);
        char  src[16]; sprintf(  src, "%d.%d.%d.%d", buffer[14],buffer[15],buffer[16],buffer[17]);
        char  dst[16]; sprintf(  dst, "%d.%d.%d.%d", buffer[18],buffer[19],buffer[20],buffer[21]);

        NS_LOG_UNCOND("version:" << ver 
                << " hsize:" << hsize 
                << " Tos:" << tos 
                << " psize:" << size
                << " id:" << id
                << " flag:" << flag
                << " seg:" << seg
                << " ttl:" << ttl
                << " proto:" << proto
                << " csum:" << csum
        );
        //NS_LOG_UNCOND("src address:" << src << " des address:" << dst);
}

std::string
RRQueue::getIPAddressFromPacket(Ptr<Packet> p)
{
	uint8_t *buffer = new uint8_t [p->GetSize()];
	p->CopyData(buffer, p->GetSize());
	char src[16]; sprintf(src, "%d.%d.%d.%d", buffer[14],buffer[15],buffer[16],buffer[17]);
	char dst[16]; sprintf(dst, "%d.%d.%d.%d", buffer[18],buffer[19],buffer[20],buffer[21]);
	std::string key = std::string(src) + "->" + std::string(dst);
	return key;
}

bool 
RRQueue::DoEnqueue (Ptr<Packet> p)
{
	uint16_t idx = 0; // queue index;

	NS_LOG_FUNCTION (this << p);

	if (!m_hasRRStarted ) {
		NS_LOG_INFO ("Initializing DRR params.");
		SetParameters ();
		m_hasRRStarted = true;
	}

	Ptr<Packet> copy = p->Copy ();
	//copy->Print(std::cout);
	//std::cout << std::endl;

	//TcpHeader tcpheader;
	//Ipv4Header ipv4header;
	//copy->PeekHeader(tcpheader);
	//copy->PeekHeader(ipv4header);
	//std::cout << tcpheader.GetFlags () << "\t" 
	// 	  << tcpheader.GetSourcePort() << "\t" 
	//	  << tcpheader.GetDestinationPort () << std::endl;

	//dumpPacketBuffer (p);

	// get flowID according to (src->dst)
	std::map<std::string, uint16_t>::iterator it;
	std::string key = getIPAddressFromPacket(p);
	//NS_LOG_UNCOND(key);
	it = m_flowMap.find(key);
	if(it != m_flowMap.end()) {
                //NS_LOG_UNCOND("flow:" << it->first << " \t flow ID: " << it->second);
		idx = it->second;
	} else {
		//NS_LOG_UNCOND(key << " \t (best effort flow)");
		idx = m_numflows;// for Best Effort queue
	}
	
	if (m_mode == QUEUE_MODE_PACKETS && (m_vqueue[idx].size () >= m_maxPackets/m_numflows)) {
		NS_LOG_LOGIC ("Queue full (at max packets) -- droppping pkt");
		Drop (p);
		return false;
	}

	if (m_mode == QUEUE_MODE_BYTES && (m_bytesInVQueue[idx] + p->GetSize () >= m_maxBytes/m_numflows)) {
		NS_LOG_LOGIC ("Queue full (packet would exceed max bytes) -- droppping pkt");
		Drop (p);
		return false;
	}

	m_bytesInVQueue[idx] += p->GetSize ();
	m_vqueue[idx].push (p);

	NS_LOG_DEBUG(key << " enqueue:" << idx << " vqueue size:" << m_vqueue[idx].size ());

	NS_LOG_LOGIC ("Number packets " << m_vqueue[idx].size ());
	NS_LOG_LOGIC ("Number bytes " << m_bytesInVQueue[idx]);

	return true;
}

uint16_t
RRQueue::getPhyQueueSize (void)
{
        int qsize=0;
        for(uint16_t i=0;i<=m_numflows;i++)
                qsize += m_vqueue[i].size();
        return qsize;
}

Ptr<Packet>
RRQueue::DoDequeue (void)
{
	NS_LOG_FUNCTION (this);

	NS_LOG_DEBUG("phyQueue size:" << getPhyQueueSize() << " turn:" << m_turn);

	if (getPhyQueueSize () == 0) {
		NS_LOG_LOGIC ("Queue empty");
		return 0;
	}

	Ptr<Packet> p = m_vqueue[m_turn].front ();
	if(p != 0) {
		m_vqueue[m_turn].pop ();
		m_bytesInVQueue[m_turn] -= p->GetSize ();
	}
	while (p==0 && getPhyQueueSize() > 0) {
                m_turn = (m_turn + 1) % (m_numflows+1);
                p = m_vqueue[m_turn].front ();
		if(p != 0) {
                	m_vqueue[m_turn].pop ();
                	m_bytesInVQueue[m_turn] -= p->GetSize ();
		}
        } 

	if(p != 0) {
                m_turn = (m_turn + 1) % (m_numflows+1);
        }

	NS_LOG_LOGIC ("Popped " << p);

	return p;
}

Ptr<const Packet>
RRQueue::DoPeek (void) const
{
	NS_LOG_FUNCTION (this);

	if (m_vqueue[m_turn].empty ()) {
		NS_LOG_LOGIC ("Queue empty");
		return 0;
	}

	Ptr<Packet> p = m_vqueue[m_turn].front ();

	return p;
}

} // namespace ns3

