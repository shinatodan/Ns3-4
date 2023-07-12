#ifndef GIFU_STATION_PROPAGATIONI_LOSS_MODEL_H
#define GIFU_STATION_PROPAGATIONI_LOSS_MODEL_H


#include "ns3/nstime.h"
#include "ns3/vector.h"
#include "ns3/propagation-loss-model.h"
namespace ns3 {
	class GifuStationPropagationLossModel : public PropagationLossModel
	{
		public:
		
		static TypeId GetTypeId (void);
		GifuStationPropagationLossModel ();
		
		void SetFrequency (double frequency);
		
		double GetFrequency (void) const;
		
		void SetPathLossExponent (double n);
		
		double GetPathLossExponent (void) const;
		
		void SetReference (double referenceDistance, double referenceLoss);
		
		double DoCalcDiffractionLoss(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
		
		double dist_vertex_np(Vector3D v1, Vector3D v2) const;
		
		Vector3D Intersection(Ptr<MobilityModel> a,Ptr<MobilityModel> b) const;
		
		virtual double DoCalcRxPower (double txPowerDbm, Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
		
		virtual int64_t DoAssignStreams (int64_t stream);
		

		double m_exponent; //!< model exponent
		double m_referenceDistance; //!< reference distance
		double m_referenceLoss; //!< reference loss
		double m_lambda;        //!< the carrier wavelength
		double m_frequency;     //!< the carrier frequency
	};
	
	
	}
#endif
