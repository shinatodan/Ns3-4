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
#include "ngpsr-helper.h"
#include "ns3/ngpsr.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/node-container.h"
#include "ns3/callback.h"
#include "ns3/udp-l4-protocol.h"
#include <openssl/dsa.h>


namespace ns3 {

NGpsrHelper::NGpsrHelper ()
  : Ipv4RoutingHelper ()
{
  m_agentFactory.SetTypeId ("ns3::ngpsr::RoutingProtocol");
}

NGpsrHelper*
NGpsrHelper::Copy (void) const
{
  return new NGpsrHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
NGpsrHelper::Create (Ptr<Node> node) const
{
  //Ptr<Ipv4L4Protocol> ipv4l4 = node->GetObject<Ipv4L4Protocol> ();
  Ptr<ngpsr::RoutingProtocol> ngpsr = m_agentFactory.Create<ngpsr::RoutingProtocol> ();
  //ngpsr->SetDownTarget (ipv4l4->GetDownTarget ());
  //ipv4l4->SetDownTarget (MakeCallback (&ngpsr::RoutingProtocol::AddHeaders, ngpsr));
  node->AggregateObject (ngpsr);
  //shinato
  ngpsr->SetDsaParameterIP(m_dsaParameter); // m_dsaParameterの設定
  ngpsr->SetDsaSignatureIP(m_dsaSignatureIP);
  ngpsr->SetDsaSignatureLengthIP(m_dsaSignatureLengthIP);
  ngpsr->SetDsaParameterPOS(m_dsaposParameter); // m_dsaParameterの設定
  ngpsr->SetDsaSignaturePOS(m_dsaposSignatureIP);
  ngpsr->SetDsaSignatureLengthPOS(m_dsaposSignatureLengthIP);
  ngpsr->Settracefile(m_tracefile);
  return ngpsr;
}

void
NGpsrHelper::Set (std::string name, const AttributeValue &value)
{
  m_agentFactory.Set (name, value);
}

//shinato
void NGpsrHelper::SetDsaParameterIP(DSA* parameter) {
    m_dsaParameter = parameter;
}
DSA* NGpsrHelper::GetDsaParameterIP() const {
  return m_dsaParameter;
}
void NGpsrHelper::SetDsaSignatureIP(const unsigned char* signature)
{
    memcpy(m_dsaSignatureIP, signature, 128);
}
const unsigned char* NGpsrHelper::GetDsaSignatureIP() const
{
    return m_dsaSignatureIP;
}
void NGpsrHelper::SetDsaSignatureLengthIP(unsigned int length)
{
    m_dsaSignatureLengthIP = length;
}
unsigned int NGpsrHelper::GetDsaSignatureLengthIP() const
{
    return m_dsaSignatureLengthIP;
}

void NGpsrHelper::SetDsaParameterPOS(DSA* posparameter) {
    m_dsaposParameter = posparameter;
}
DSA* NGpsrHelper::GetDsaParameterPOS() const {
  return m_dsaposParameter;
}
void NGpsrHelper::SetDsaSignaturePOS(const unsigned char* possignature)
{
    memcpy(m_dsaposSignatureIP, possignature, 128);
}
const unsigned char* NGpsrHelper::GetDsaSignaturePOS() const
{
    return m_dsaposSignatureIP;
}
void NGpsrHelper::SetDsaSignatureLengthPOS(unsigned int poslength)
{
    m_dsaposSignatureLengthIP = poslength;
}
unsigned int NGpsrHelper::GetDsaSignatureLengthPOS() const
{
    return m_dsaposSignatureLengthIP;
}

void NGpsrHelper::Settracefile(std::string tracefile) {
    m_tracefile = tracefile;
}
std::string NGpsrHelper::Gettracefile() const {
  return m_tracefile;
}


void
NGpsrHelper::Install (void) const
{
  NodeContainer c = NodeContainer::GetGlobal ();
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = (*i);
      Ptr<UdpL4Protocol> udp = node->GetObject<UdpL4Protocol> ();
      Ptr<ngpsr::RoutingProtocol> ngpsr = node->GetObject<ngpsr::RoutingProtocol> ();
      ngpsr->SetDownTarget (udp->GetDownTarget ());
      udp->SetDownTarget (MakeCallback(&ngpsr::RoutingProtocol::AddHeaders, ngpsr));
    }


}


}
