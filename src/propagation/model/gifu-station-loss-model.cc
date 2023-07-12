
#include "ns3/propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "ns3/vector.h"
#include "gifu-station-loss-model.h"
#include <iomanip>			// std::setw()
#define _USE_MATH_DEFINES // for C++  
#include <cmath>  
#include <chrono>
#include <time.h> 
#include <iostream>
#include <fstream>
#include <sstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GifuStationPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED (GifuStationPropagationLossModel);


TypeId
GifuStationPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GifuStationPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<GifuStationPropagationLossModel> ()
    .AddAttribute ("Exponent",
                   "The exponent of the Path Loss propagation model",
                   DoubleValue (3.0),
                   MakeDoubleAccessor (&GifuStationPropagationLossModel::m_exponent),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ReferenceDistance",
                   "The distance at which the reference loss is calculated (m)",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&GifuStationPropagationLossModel::m_referenceDistance),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ReferenceLoss",
                   "The reference loss at reference distance (dB). (Default is Friis at 1m with 5.15 GHz)",
                   DoubleValue (46.6777),
                   MakeDoubleAccessor (&GifuStationPropagationLossModel::m_referenceLoss),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Frequency", 
                   "The carrier frequency (in Hz) at which propagation occurs  (default is 5.15 GHz).",
                   DoubleValue (5.150e9),
                   MakeDoubleAccessor (&GifuStationPropagationLossModel::SetFrequency,
                                       &GifuStationPropagationLossModel::GetFrequency), 
                   MakeDoubleChecker<double> ())                                 
  ;
  return tid;

}

GifuStationPropagationLossModel::GifuStationPropagationLossModel ()
{
}

void
GifuStationPropagationLossModel::SetPathLossExponent (double n)
{
  m_exponent = n;
}
void
GifuStationPropagationLossModel::SetReference (double referenceDistance, double referenceLoss)
{
  m_referenceDistance = referenceDistance;
  m_referenceLoss = referenceLoss;
}
double
GifuStationPropagationLossModel::GetPathLossExponent (void) const
{
  return m_exponent;
}

// 周波数と波長のセット
void
GifuStationPropagationLossModel::SetFrequency (double frequency)
{
  m_frequency = frequency;
  static const double C = 299792458.0; // speed of light in vacuum
  m_lambda = C / frequency;
}

// 周波数の返却
double
GifuStationPropagationLossModel::GetFrequency (void) const
{
  return m_frequency;
}

//ナイフエッジ回折損失の計算
double
GifuStationPropagationLossModel::DoCalcDiffractionLoss(Ptr<MobilityModel> a,
											     Ptr<MobilityModel> b) const
{
	Vector3D A = a->GetPosition();
	Vector3D B = b->GetPosition();
	Vector3D Cross = Intersection(a,b);
	
	if(Cross.x == 0 && Cross.y == 0){ //交差してないとき
		return 0;
	}
	
	else{
		double d1 = dist_vertex_np(A,Cross);
		double d2 = dist_vertex_np(B,Cross);
		
		double DiffLoss = 0.0;
		double temp1 = 2 / m_lambda;
		double temp2 = (d1 + d2) / (d1* d2);
		double temp3 = pow( (temp1 * temp2), 0.5);
		double v = 15 * temp3;
		double temp4 = pow( (v - 0.1), 2);
		double temp5 = pow( temp4+1 , 0.5);
		DiffLoss = 6.9 + 20 * log10(temp5 + v - 0.1);		
		return DiffLoss;
	}
}

//2点間の距離											     
double 
GifuStationPropagationLossModel::dist_vertex_np(Vector3D v1, Vector3D v2) const{
    
    double tmp1 = sqrt(pow(v1.x-v2.x,2) + pow(v1.y-v2.y,2) + pow(v1.z-v2.z,2));
    return tmp1;
}
											     
//交差判定の計算。交差している場合は交差点を、していない場合は(0,0,0)を返す
Vector3D
GifuStationPropagationLossModel::Intersection(Ptr<MobilityModel> a,
											     Ptr<MobilityModel> b) const
											     {
													 
	Vector3D A = a->GetPosition();
	Vector3D B = b->GetPosition();
	Vector3D C,D;
	
	C.x=622;C.y=1127;C.z=0; 
	D.x=1224;D.y=1067;D.z=0;
	
	double s,t,s_nume,s_deno,t_nume,t_deno;
	s_nume=((C.x-A.x)*(D.y-C.y))-((C.y-A.y)*(D.x-C.x));
	s_deno=((B.x-A.x)*(D.y-C.y))-((B.y-A.y)*(D.x-C.x));
	t_nume=((A.x-C.x)*(B.y-A.y))-((A.y-C.y)*(B.x-A.x));
	t_deno=((D.x-C.x)*(B.y-A.y))-((D.y-C.y)*(B.x-A.x));
	s=s_nume/s_deno;
	t=t_nume/t_deno;
	
	if(s < 0.0 || 1.0 < s || t < 0.0 || 1.0 < t){ //交差してないとき
		Vector3D Cross = Vector3D(0,0,0);
		return Cross;
	}	
	else{
		Vector3D Cross = Vector3D(A.x + s*(B.x-A.x),A.y + s*(B.y-A.y),15.0);							 
		return Cross;
	}			 
}											     

							     										    

double
GifuStationPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                                Ptr<MobilityModel> a,
                                                Ptr<MobilityModel> b) const
{
  double distance = a->GetDistanceFrom (b);
  if (distance <= m_referenceDistance)
    {
      return txPowerDbm;
    }
	    
    double DiffLoss = 0.0;
	DiffLoss = DoCalcDiffractionLoss(a, b);  /**
   * The formula is:
   * rx = 10 * log (Pr0(tx)) - n * 10 * log (d/d0)
   *
   * Pr0: rx power at reference distance d0 (W)
   * d0: reference distance: 1.0 (m)
   * d: distance (m)
   * tx: tx power (dB)
   * rx: dB
   *
   * Which, in our case is:
   *
   * rx = rx0(tx) - 10 * n * log (d/d0)
   */
   
  NS_LOG_DEBUG ("DiffLoss = " << DiffLoss); 
   
  double pathLossDb = 10 * m_exponent * std::log10 (distance / m_referenceDistance);
  double rxc = -m_referenceLoss - pathLossDb;
  
  return txPowerDbm + rxc - DiffLoss;

}

int64_t
GifuStationPropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

}
