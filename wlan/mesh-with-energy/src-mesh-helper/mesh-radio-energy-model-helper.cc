/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
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
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#include "ns3/mesh-radio-energy-model-helper.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-net-device.h"
#include "ns3/mesh-point-device.h"
#include "ns3/config.h"
#include "ns3/names.h"

namespace ns3 {

MeshRadioEnergyModelHelper::MeshRadioEnergyModelHelper ()
{
  m_radioEnergy.SetTypeId ("ns3::WifiRadioEnergyModel");
  m_depletionCallback.Nullify ();
}

MeshRadioEnergyModelHelper::~MeshRadioEnergyModelHelper ()
{
}

void
MeshRadioEnergyModelHelper::Set (std::string name, const AttributeValue &v)
{
  m_radioEnergy.Set (name, v);
}

void
MeshRadioEnergyModelHelper::SetDepletionCallback (
  WifiRadioEnergyModel::WifiRadioEnergyDepletionCallback callback)
{
  m_depletionCallback = callback;
}

/*
 * Private function starts here.
 */

Ptr<DeviceEnergyModel>
MeshRadioEnergyModelHelper::DoInstall (Ptr<NetDevice> device,
                                       Ptr<EnergySource> source) const
{
  NS_ASSERT (device != NULL);
  NS_ASSERT (source != NULL);
  // check if device is MeshPointDevice
  std::string deviceName = device->GetInstanceTypeId ().GetName ();
  if (deviceName.compare ("ns3::MeshPointDevice") != 0)
    {
      NS_FATAL_ERROR ("NetDevice type is not MeshPointDevice!");
    }
  // Ptr<Node> node = device->GetNode ();
  Ptr<WifiRadioEnergyModel> model = m_radioEnergy.Create ()->GetObject<WifiRadioEnergyModel> ();
  NS_ASSERT (model != NULL);
  // set energy source pointer
  model->SetEnergySource (source);
  // set energy depletion callback
  model->SetEnergyDepletionCallback (m_depletionCallback);
  // add model to device model list in energy source
  source->AppendDeviceEnergyModel (model);
  // create and register energy model phy listener

// changed and appended by F.Qian, Jan. 2013
  Ptr<MeshPointDevice> mp = DynamicCast<MeshPointDevice> (device);
  std::vector<Ptr<NetDevice> > interfaces = mp->GetInterfaces ();
  
  for (std::vector<Ptr<NetDevice> >::const_iterator i = interfaces.begin (); i != interfaces.end (); i++)
  {
    Ptr<WifiNetDevice> wifiDevice = (*i)->GetObject<WifiNetDevice> ();
    Ptr<WifiPhy> wifiPhy = wifiDevice->GetPhy ();
    wifiPhy->RegisterListener (model->GetPhyListener ());
  }
// F.Qian


  return model;
}

} // namespace ns3

