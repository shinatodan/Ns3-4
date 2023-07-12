
#ifndef MOUNTAIN_PROPAGATION_LOSS_MODEL_REV_FORD_H
#define MOUNTAIN_PROPAGATION_LOSS_MODEL_REV_FORD_H


#include "ns3/nstime.h"
#include "ns3/vector.h"
#include "ns3/propagation-loss-model.h"

namespace ns3 {


class VegetationDataRevForD{
	public:
	Ptr<MobilityModel> np;		// ノードポインタ
	Vector3D 	position;		// 座標
	double 		elevation;		// 標高
	bool		vegetation;		// 植生判定
	bool		path;			// ルート判別(0=穂高, 1=槍)
};

class MountainPropagationLossModelRevForD : public PropagationLossModel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  MountainPropagationLossModelRevForD ();
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
	void SetVectorM1V0 (Vector3D m1v0);
	void SetVectorM1V1 (Vector3D m1v1);
	void SetVectorM1V2 (Vector3D m1v2);
	void SetVectorM2V0 (Vector3D m2v0);
	void SetVectorM2V1 (Vector3D m2v1);
	void SetVectorM2V2 (Vector3D m2v2);
  
  void SetAnntenaHeight(double aHeight);
 

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  //MountainPropagationLossModel (const MountainPropagationLossModel &);
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  //MountainPropagationLossModel & operator = (const MountainPropagationLossModel &);

  // フリスの自由空間損失の計算  
  double DoCalcFriisLoss (Ptr<MobilityModel> a,
						  Ptr<MobilityModel> b) const;
						  
  // 平面大地伝搬損失の計算
  double DoCalcTwoRayGroundLoss(Ptr<MobilityModel> a,
						  Ptr<MobilityModel> b) const;
						  
	double DoCalcTwoRayGroundLossRev( Ptr<MobilityModel> a,
                                                 Ptr<MobilityModel> b) const;
	Vector3D DoCalcVerticalTwoPoint(Vector3D n1, Vector3D n2) const;
								
  // 交差判定 
	Vector3D CrossingDetection01 (Vector3D A,Vector3D B) const;
	Vector3D CrossingDetection02 (Vector3D A,Vector3D B) const;
 
	
	

	double DoCalcDiffRev (Ptr<MobilityModel> n1, Ptr<MobilityModel> n2) const;
	
	
	double DoCalcDiffLossRev (Ptr<MobilityModel> n1, Ptr<MobilityModel> n2) const;
	
	//ナイフエッジの計算式
	double DoCalcKnifeEdgeLoss (double a, double b, double h)const;
	
	//ノードと回折点とが垂直点に交わる点を求める,
	Vector3D DoCalcVerticalPoint (Ptr<MobilityModel> a, Ptr<MobilityModel> b, Vector3D I) const;

	//2直線の交点を求める
	Vector3D DoCalcCrossingPoint (Ptr<MobilityModel> a,Vector3D L1, Vector3D X, Vector3D L2) const;

	//単位ベクトルを求める関数	
	Vector3D get_unit_vector (Vector3D v) const;

	//内積
	double dot_product( Vector3D vl, Vector3D vr) const;
	
	//２点間の距離
	double dist_vertex_np(Vector3D v1, Vector3D v2) const;

	// 山１の回折点の座標nを求めるメソッド
	Vector3D CalcNearPoint01 (Ptr<MobilityModel> a,Ptr<MobilityModel> b, Vector3D I) const;
	
	// 山2の回折点の座標nを求めるメソッド
	Vector3D CalcNearPoint02 (Ptr<MobilityModel> a,Ptr<MobilityModel> b, Vector3D I) const;
	
	
	//2点→ベクトル変換abベクトルの作成
	Vector3D get_vector(Vector3D a, Vector3D b) const;
	
	//3点の高さを求める
	//点bから辺acに向かって伸びる垂線
	double height(Vector3D a, Vector3D b, Vector3D c) const;
	
	//回折損用計算式
	//送信点と受信点のノードを引数とする
	double DoCalcDiffractionLoss (Ptr<MobilityModel> n1, Ptr<MobilityModel> n2) const;
	
	//回折点をもとめるメソッド
	//引数は山の辺を構成する２点と送信点ノード
	Vector3D directpoint(Vector3D a,Vector3D b,Vector3D p) const;
	
	//山一つとの回折の計算
	double DoCalcSingleDiffLoss(Vector3D n1, Vector3D n2, Vector3D d)const;
	
	//二重回折計算					
	//double DoCalcDoubleDiffLoss(Vector3D s,Vector3D d1,Vector3D f1,Vector3D r,Vector3D d2,Vector3D f2)const;
	double DoCalcDoubleDiffLoss(Vector3D s,Vector3D d1,Vector3D d2,Vector3D r)const;
	Vector3D watanabe1(Vector3D p) const;
	
	Vector3D watanabe2(Vector3D p) const;
	Vector3D watanabe(Vector3D a, Vector3D b, Vector3D c) const;
					
	double CalcVegetationalLoss (Ptr<MobilityModel> a, Ptr<MobilityModel> b
								, double distance) const;
							
									

  // 標高計算
  void DoCalcElevation(VegetationDataRevForD& vstruct)const;
  
  
  // ルート判定
  void DeterminePath(VegetationDataRevForD& vstrut)const;
  
  // 
  void DetermineArea(VegetationDataRevForD&  vstruct)const;
  
  // 
  double DoCalcVegetationalDistance(VegetationDataRevForD&  vstruct_a, VegetationDataRevForD&  vstruct_b)const;
  
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
  Vector3D M1V0;				// 交差判定を行う三角形の辺のベクトル0
  Vector3D M1V1;				// 交差判定を行う三角形の辺のベクトル1
  Vector3D M1V2;				// 交差判定を行う三角形の辺のベクトル2
  Vector3D M2V0;				// 交差判定を行う三角形の辺のベクトル0
  Vector3D M2V1;				// 交差判定を行う三角形の辺のベクトル1
  Vector3D M2V2;				// 交差判定を行う三角形の辺のベクトル2
  double AntennaHeight;

};

}

#endif /* Mountain_PROPAGATIONMODEL_H */
