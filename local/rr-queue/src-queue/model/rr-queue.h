/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef RR_QUEUE_H
#define RR_QUEUE_H

#include <queue>
#include "ns3/packet.h"
#include "ns3/queue.h"

#include "ns3/string.h"
#include "ns3/queue.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/ipv4-header.h"

namespace ns3 {

class TraceContainer;

/**
 * \ingroup queue
 *
 * \brief A DropTail/Round-Robin packet queue
 *
 * Drops tail-end packets on overflow and send out a packet with Round-Robin method.
 */
class RRQueue : public Queue {
    public:
	static TypeId GetTypeId (void);
	/**
	 * \brief RRQueue Constructor
	 *
	 * Creates a droptail queue with a maximum size of 100 packets by default
	 */
	RRQueue ();

	virtual ~RRQueue();

	/**
	 * Set the operating mode of this device.
	 *
	 * \param mode The operating mode of this device.
	 *
	 */
	void SetMode (RRQueue::QueueMode mode);

	/**
	 * Get the encapsulation mode of this device.
	 *
	 * \returns The encapsulation mode of this device.
	 */
	RRQueue::QueueMode GetMode (void);

    private:
	/**
	 * Do enqueue, when a packet arrived
	 *
	 * \param arrived packet
	 */
	virtual bool DoEnqueue (Ptr<Packet> p);

	/**
	 * Do dequeue, send out a packet with round robin
	 */
	virtual Ptr<Packet> DoDequeue (void);

	virtual Ptr<const Packet> DoPeek (void) const;

	/**
	 * Dump packet buffer for debug (decapsulation processing)
	 *
	 * \param arrived packet
	 */
	void dumpPacketBuffer (Ptr<Packet> p);

	/**
	 * Set RR Queue parameters
	 */
	void SetParameters ();

	/**
	 * Get peer IP address (used for set flow Id)
	 *
	 * \param arrived packet
	 *
	 * \returns The string like as "192.168.1.1->192.168.2.1"
	 */
	std::string getIPAddressFromPacket(Ptr<Packet> p);

	/**
	 * Get physical queue size
	 */
	uint16_t getPhyQueueSize();

	std::map<std::string, uint16_t> m_flowMap;
	std::queue< Ptr<Packet> > m_vqueue[16]; // virtual queue
	uint16_t  m_bytesInVQueue[16];
	uint32_t  m_maxPackets;
	uint32_t  m_maxBytes;
	bool      m_hasRRStarted;
	uint16_t  m_turn;
	QueueMode m_mode;

	uint32_t m_numflows; // number of flows(max=4)
	std::string m_src1;  // source's ip address 
	std::string m_src2;  // source's ip address 
	std::string m_src3;  // source's ip address 
	std::string m_src4;  // source's ip address 
	std::string m_src5;  // source's ip address 
	std::string m_src6;  // source's ip address 

	std::string m_dst1;  // destination's ip address 
	std::string m_dst2;  // destination's ip address 
	std::string m_dst3;  // destination's ip address 
	std::string m_dst4;  // destination's ip address 
	std::string m_dst5;  // destination's ip address 
	std::string m_dst6;  // destination's ip address 
};

} // namespace ns3

#endif /* RR_QUEUE_H */
