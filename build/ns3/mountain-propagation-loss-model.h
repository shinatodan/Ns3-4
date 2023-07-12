
#ifndef MOUNTAIN_PROPAGATION_LOSS_MODEL_H
#define MOUNTAIN_PROPAGATION_LOSS_MODEL_H

#include "ns3/nstime.h"
#include "ns3/vector.h"
#include "ns3/propagation-loss-model.h"

namespace ns3 {

class VegetationData{
	public:
	Ptr<MobilityModel> np;		// ノードポインタ
	Vector3D 	position;		// 座標
	double 		elevation;		// 標高
	bool		vegetation;		// 植生判定
	bool		path;			// ルート判別(0=穂高, 1=槍)
};

class MountainPropagationLossModel : public PropagationLossModel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  MountainPropagationLossModel ();
  /**
   * \param frequency (Hz)
   *
   * Set the carrier frequency used in the Friis model 
   * calculation.
   */
   
  void SetFrequency (double frequency);
  /**
   * \param systemLoss (dimension-less)
   *
   * Set the system loss used by the Friis propagation model.
   */
  void SetSystemLoss (double systemLoss);

  /**
   * \param minLoss the minimum loss (dB)
   *
   * no matter how short the distance, the total propagation loss (in
   * dB) will always be greater or equal than this value 
   */
  void SetMinLoss (double minLoss);

  /**
   * \return the minimum loss.
   */
  double GetMinLoss (void) const;

  /**
   * \returns the current frequency (Hz)
   */
  double GetFrequency (void) const;
  /**
   * \returns the current system loss (dimension-less)
   */
  double GetSystemLoss (void) const;
  
  // 交差判定に使う三角形の辺のベクトルを格納するメソッド
  void SetVector0 (Vector3D v0);
  void SetVector1 (Vector3D v1);
  void SetVector2 (Vector3D v2);
  
  void SetAnntenaHeight(double aHeight);
  
  // 回折の計算に使うデータ群(高さh, 距離d1, d2)を格納する構造体のtypedef宣言
  typedef struct data{
	  double h;
	  double d1;
	  double d2;
  }calcdata_t;
  

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  MountainPropagationLossModel (const MountainPropagationLossModel &);
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  MountainPropagationLossModel & operator = (const MountainPropagationLossModel &);

  // フリスの自由空間損失の計算  
  double DoCalcFriisLoss (Ptr<MobilityModel> a,
						  Ptr<MobilityModel> b) const;
						  
  // 平面大地伝搬損失の計算
  double DoCalcTwoRayGroundLoss(Ptr<MobilityModel> a,
						  Ptr<MobilityModel> b) const;
						  
  // 回折損の計算					   
  double DoCalcDiffractionLoss (Ptr<MobilityModel> a,
								Ptr<MobilityModel> b) const;
								
  // 交差判定
  Vector3D CrossingDetection (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
  
  // 回折損の計算に必要な距離の計算
  calcdata_t CalcDistance (Ptr<MobilityModel> a,
							Ptr<MobilityModel> b,
							Vector3D I) const;
							
  // 回折損の計算に必要な回折点の計算
  Vector3D CalcNearPoint (Ptr<MobilityModel> a,
						Ptr<MobilityModel> b,
						Vector3D I) const;
	/*					
  double CalcVegetationalLoss (Ptr<MobilityModel> a, Ptr<MobilityModel> b
								, double distance) const;
		*/						

  // 標高計算
  void DoCalcElevation(VegetationData& vstruct)const;
  
  
  // ルート判定
  void DeterminePath(VegetationData& vstrut)const;
  
  // 
  void DetermineArea(VegetationData&  vstruct)const;
  
  // 
  double DoCalcVegetationalDistance(VegetationData&  vstruct_a, VegetationData&  vstruct_b)const;
  
  //
  double DoCalcVegetationalLoss(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
  

  // 受信電力の計算
  virtual double DoCalcRxPower (double txPowerDbm,
                                Ptr<MobilityModel> a,
                                Ptr<MobilityModel> b) const;
  
  // PropagationLossModelを親クラスにもつなら必ず実装する                             
  virtual int64_t DoAssignStreams (int64_t stream);

  /**
   * Transforms a Dbm value to Watt
   * \param dbm the Dbm value
   * \return the Watts
   */
  double DbmToW (double dbm) const;

  /**
   * Transforms a Watt value to Dbm
   * \param w the Watt value
   * \return the Dbm
   */
  double DbmFromW (double w) const;

  double m_lambda;        //!< the carrier wavelength
  double m_frequency;     //!< the carrier frequency
  double m_systemLoss;    //!< the system loss
  double m_minLoss;       //!< the minimum loss
  Vector3D V0;				// 交差判定を行う三角形の辺のベクトル0
  Vector3D V1;				// 交差判定を行う三角形の辺のベクトル1
  Vector3D V2;				// 交差判定を行う三角形の辺のベクトル2
  double AntennaHeight;

};

}

#endif /* Mountain_PROPAGATIONMODEL_H */
