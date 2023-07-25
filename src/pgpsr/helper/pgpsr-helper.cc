/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 * Authors: António Fonseca <afonseca@tagus.inesc-id.pt>, written after OlsrHelper by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "pgpsr-helper.h"
#include "ns3/pgpsr.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/node-container.h"
#include "ns3/callback.h"
#include "ns3/udp-l4-protocol.h"
#include <openssl/ec.h>


namespace ns3 {

PGpsrHelper::PGpsrHelper ()
  : Ipv4RoutingHelper ()
{
  m_agentFactory.SetTypeId ("ns3::pgpsr::RoutingProtocol");
}

PGpsrHelper*
PGpsrHelper::Copy (void) const
{
  return new PGpsrHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
PGpsrHelper::Create (Ptr<Node> node) const
{
  //Ptr<Ipv4L4Protocol> ipv4l4 = node->GetObject<Ipv4L4Protocol> ();
  Ptr<pgpsr::RoutingProtocol> pgpsr = m_agentFactory.Create<pgpsr::RoutingProtocol> ();
  //pgpsr->SetDownTarget (ipv4l4->GetDownTarget ());
  //ipv4l4->SetDownTarget (MakeCallback (&pgpsr::RoutingProtocol::AddHeaders, pgpsr));
  node->AggregateObject (pgpsr);
  //shinato
  pgpsr->SetDsaParameterIP(m_dsaParameter); // m_dsaParameterの設定
  pgpsr->SetDsaSignatureIP(m_dsaSignatureIP);
  pgpsr->SetDsaParameterPOS(m_dsaposParameter); // m_dsaParameterの設定
  pgpsr->SetDsaSignaturePOS(m_dsaposSignatureIP);
  pgpsr->Settracefile(m_tracefile);
  return pgpsr;
}

void
PGpsrHelper::Set (std::string name, const AttributeValue &value)
{
  m_agentFactory.Set (name, value);
}

//shinato
void PGpsrHelper::SetDsaParameterIP(EC_KEY* parameter) {
    m_dsaParameter = parameter;
}
EC_KEY* PGpsrHelper::GetDsaParameterIP() const {
    return m_dsaParameter;
}
void PGpsrHelper::SetDsaSignatureIP(ECDSA_SIG* signature)
{
    m_dsaSignatureIP = signature;
}
ECDSA_SIG* PGpsrHelper::GetDsaSignatureIP() const{
    return m_dsaSignatureIP;
}
void PGpsrHelper::SetDsaParameterPOS(EC_KEY* posparameter) {
    m_dsaposParameter = posparameter;
}
EC_KEY* PGpsrHelper::GetDsaParameterPOS() const {
  return m_dsaposParameter;
}
void PGpsrHelper::SetDsaSignaturePOS(ECDSA_SIG* possignature)
{
    m_dsaposSignatureIP = possignature;
}
ECDSA_SIG* PGpsrHelper::GetDsaSignaturePOS() const
{
    return m_dsaposSignatureIP;
}
void PGpsrHelper::Settracefile(std::string tracefile) {
    m_tracefile = tracefile;
}
std::string PGpsrHelper::Gettracefile() const {
  return m_tracefile;
}


void
PGpsrHelper::Install (void) const
{
  NodeContainer c = NodeContainer::GetGlobal ();
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = (*i);
      Ptr<UdpL4Protocol> udp = node->GetObject<UdpL4Protocol> ();
      Ptr<pgpsr::RoutingProtocol> pgpsr = node->GetObject<pgpsr::RoutingProtocol> ();
      pgpsr->SetDownTarget (udp->GetDownTarget ());
      udp->SetDownTarget (MakeCallback(&pgpsr::RoutingProtocol::AddHeaders, pgpsr));
    }


}


}
