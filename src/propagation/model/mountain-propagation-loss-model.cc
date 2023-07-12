/* 山岳地帯での無線通信を再現するために作成した電波伝搬損失モデルです.
 * このモデルでは以下の３つの損失, 減衰を実装しています.
 * 1.Friisの自由空間損失
 * 2.回折損失
 * 3.植生による減衰(ITU-R attenuation in vegetation)
 * 
 * このプログラムには冗長な処理があります
 * 時間がなくて直してませんが, 処理速度を上げるには修正をする必要があります */

#include "ns3/propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "ns3/vector.h"
#include "mountain-propagation-loss-model.h"
#include <iomanip>			// std::setw()
#define _USE_MATH_DEFINES // for C++  
#include <cmath>  
#include <chrono>
#include <time.h> 
#include <iostream>
#include <fstream>
#include <sstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MountainPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED (MountainPropagationLossModel);


TypeId 
MountainPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MountainPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<MountainPropagationLossModel> ()
    .AddAttribute ("Frequency",
                   "The carrier frequency (in Hz) at which propagation occurs  (default is 900MHz).",
                   DoubleValue (142000000),
                   MakeDoubleAccessor (&MountainPropagationLossModel::SetFrequency,
                                       &MountainPropagationLossModel::GetFrequency),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("SystemLoss", "The system loss",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&MountainPropagationLossModel::m_systemLoss),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MinLoss", 
                   "The minimum value (dB) of the total loss, used at short ranges. Note: ",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&MountainPropagationLossModel::SetMinLoss,
                                       &MountainPropagationLossModel::GetMinLoss),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("V0", 
                   "Vector 0 of Obstacle",			// AD006
                   Vector3DValue (Vector3D(-3802056.926, 3470149.789, 3756099.022)),			// 初期値はVector3D(x, y, z)で指定する 
                   MakeVector3DAccessor (&MountainPropagationLossModel::SetVector0),
                   MakeVector3DChecker ())							// MakeDoubleChackerのように<>で型指定はしない（するとエラーになる)
    .AddAttribute ("V1", 
                   "Vector 1 of Obstacle",			// 蒲田富士
                   //Vector3DValue (Vector3D(-3794736.175, 3491774.157, 3745448.351)),
                   Vector3DValue (Vector3D(-3805222.186, 3468964.71, 3756626.002)),	
                   MakeVector3DAccessor (&MountainPropagationLossModel::SetVector1),
                   MakeVector3DChecker ())
    .AddAttribute ("V2", 
                   "Vector 2 of Obstacle",			// 計算から求めた
                   Vector3DValue (Vector3D(-3804293.663, 3468118.238, 3755709.337)),
                   MakeVector3DAccessor (&MountainPropagationLossModel::SetVector2),
                   MakeVector3DChecker ())
    .AddAttribute ("AntennaHeight",
                   "The height of the antenna (m) ",
                   DoubleValue (1.5),
                   MakeDoubleAccessor (&MountainPropagationLossModel::AntennaHeight),
                   MakeDoubleChecker<double> ())
    /*.AddAttribute ("V0",  
                   "Vector 0 of Obstacle",			// ap_h3
                   Vector3DValue (Vector3D(-3802056.926, 3470149.789, 3756099.022)),			// 初期値はVector3D(x, y, z)で指定する 
                   MakeVector3DAccessor (&MountainPropagationLossModel::SetVector0),
                   MakeVector3DChecker ())							// MakeDoubleChackerのように<>で型指定はしない（するとエラーになる)
    .AddAttribute ("V1", 
                   "Vector 1 of Obstacle",			// 涸沢岳
                   //Vector3DValue (Vector3D(-3794736.175, 3491774.157, 3745448.351)),
                   Vector3DValue (Vector3D(-3805222.186, 3468964.71, 3756626.002)),	
                   MakeVector3DAccessor (&MountainPropagationLossModel::SetVector1),
                   MakeVector3DChecker ())
    .AddAttribute ("V2", 
                   "Vector 2 of Obstacle",			// 計算から求めた
                   Vector3DValue (Vector3D(-3804293.663, 3468118.238, 3755709.337)),
                   MakeVector3DAccessor (&MountainPropagationLossModel::SetVector2),
                   MakeVector3DChecker ())
                   * */
  ;
  return tid;
}
// コンストラクタ
MountainPropagationLossModel::MountainPropagationLossModel ()
{
}

// システム損失のセット
void
MountainPropagationLossModel::SetSystemLoss (double systemLoss)
{					    
  m_systemLoss = systemLoss;
}

// システム損失の返却
double
MountainPropagationLossModel::GetSystemLoss (void) const
{
  return m_systemLoss;
}

// 最小損失のセット
void
MountainPropagationLossModel::SetMinLoss (double minLoss)
{
  m_minLoss = minLoss;
}

// 最小損失の返却
double
MountainPropagationLossModel::GetMinLoss (void) const
{
  return m_minLoss;
}

// 周波数と波長のセット
void
MountainPropagationLossModel::SetFrequency (double frequency)
{
  m_frequency = frequency;
  static const double C = 299792458.0; // speed of light in vacuum
  m_lambda = C / frequency;
}

// 周波数の返却
double
MountainPropagationLossModel::GetFrequency (void) const
{
  return m_frequency;
}

// 障害物のベクトルV0のセット
void
MountainPropagationLossModel::SetVector0 (Vector3D v0 )
{
	V0.x = v0.x;
	V0.y = v0.y;
	V0.z = v0.z;
}

// 障害物のベクトルV1のセット
void
MountainPropagationLossModel::SetVector1 (Vector3D v1 )
{
	V1.x = v1.x;
	V1.y = v1.y;
	V1.z = v1.z;
}

// 障害物のベクトルV2のセット
void
MountainPropagationLossModel::SetVector2 (Vector3D v2 )
{
	V2.x = v2.x;
	V2.y = v2.y;
	V2.z = v2.z;
}

void
MountainPropagationLossModel::SetAnntenaHeight (double aHeight)
{
	AntennaHeight = aHeight;
}

// dBm(デシベルミリ) → W(ワット)変換
double
MountainPropagationLossModel::DbmToW (double dbm) const
{
  double mw = std::pow (10.0,dbm/10.0);
  return mw / 1000.0;
}

// W(ワット) → dBm(デシベルミリ)変換
double
MountainPropagationLossModel::DbmFromW (double w) const
{
  double dbm = std::log10 (w * 1000.0) * 10.0;
  return dbm;
}

// Friisの自由空間損失の計算
double
MountainPropagationLossModel::DoCalcFriisLoss (Ptr<MobilityModel> a,
                                          Ptr<MobilityModel> b) const
{
  /*
   * Friis free space equation:
   * where Pt, Gr, Gr and P are in Watt units
   * L is in meter units.
   *
   *    P     Gt * Gr * (lambda^2)
   *   --- = ---------------------
   *    Pt     (4 * pi * d)^2 * L
   *
   * Gt: tx gain (unit-less)
   * Gr: rx gain (unit-less)
   * Pt: tx power (W)
   * d: distance (m)
   * L: system loss
   * lambda: wavelength (m)
   *
   * Here, we ignore tx and rx gain and the input and output values 
   * are in dB or dBm:
   *std::string tf = "watavege.txt";
	const char * st = tf.c_str();
	std::ofstream ofs(st,std::ios::out|std::ios::app);
	ofs<<"aの植生"<<vstructA.vegetation<<"\n";
	ofs<<"bの植生"<<vstructB.vegetation<<"\n";
	ofs.close();
   *                           lambda^2
   * rx = tx +  10 log10 (-------------------)
   *                       (4 * pi * d)^2 * L
   *
   * rx: rx power (dB)
   * tx: tx power (dB)
   * d: distance (m)
   * L: system loss (unit-less)
   * lambda: wavelength (m)
   */
   
  double distance = a->GetDistanceFrom (b);

  if (distance < 3*m_lambda)
    {
      NS_LOG_WARN ("distance not within the far field region => inaccurate propagation loss value");
    }
  if (distance <= 0)
    {
      return m_minLoss;
    }
  // 分子
  double numerator = m_lambda * m_lambda;
  // 分母
  double denominator = 16 * M_PI * M_PI * distance * distance * m_systemLoss;
  // 損失の計算
  double lossDb = 10 * log10 (numerator / denominator);
  
  NS_LOG_DEBUG ("distance=" << distance<< "m, loss=" << lossDb <<"dB");
  
  return lossDb;
	
}


  // 平面大地2波反射モデルによる損失
double
MountainPropagationLossModel::DoCalcTwoRayGroundLoss( Ptr<MobilityModel> a,
                                                 Ptr<MobilityModel> b) const
{
  double distance = a->GetDistanceFrom (b);
  if (distance <= 0.5)
    {
      return 0;
    }
	//std::cout << "Aのアンテナ高" <<AntennaHeight << " m" << std::endl;
    //std::cout << "Bのアンテナ高" << AntennaHeight<< " m" << std::endl;
  // ブレークポイント
  double dCross = (4 * M_PI * AntennaHeight * AntennaHeight) / m_lambda;

  if (distance <= dCross)
    {
      // Friisの自由空間損失モデル
      // 分子
	  double numerator = m_lambda * m_lambda;
	  // 分母
	  double denominator = 16 * M_PI * M_PI * distance * distance * m_systemLoss;
	  // 損失の計算
	  double lossDb = 10 * log10 (numerator / denominator);

      return lossDb;
    }
  else   // 平面大地２波反射モデル
    {
	  // 分子
	  double numerator = distance * distance;
	  // 分母
	  double denominator = AntennaHeight * AntennaHeight;
	  // 損失
	  double lossDb = 20 * log10 (numerator / denominator);
	  
	  return lossDb;

    }
}

 




// 回折損の計算
double
MountainPropagationLossModel::DoCalcDiffractionLoss (Ptr<MobilityModel> a,
											     Ptr<MobilityModel> b) const
{
	/* 回折損の計算
	 * 
	 * d1:送信点から回折点への距離(m)
	 * d2:回折点から受信点の距離(m)
	 * h:高さ(m)
	 * J(v):回折損
	 * 
	 * v = h * √(2/h * ((1/d1) + (1/d2)))
	 * J(v) = 6.9 + 20 * log10(√((v-0.1)^2) + v -1 )
	 */
	 
	double DiffLoss = 0.0;	// 回折損

	Vector3D I (CrossingDetection(a, b)); // 交差判定
	
	// 交差点の座標がすべて0(初期値のまま)なら交差していないと判定する
	if((I.x == 0.0) && (I.y == 0.0) && (I.z == 0.0)){
		return 0;
	}
	
	// ナイフエッジ回折損の計算に必要なデータ群をまとめた構造体
	calcdata_t D;
	D = CalcDistance(a, b, I);
	double temp1 = 2 / m_lambda;
	// std::cout << "2/λ : " << temp1 << std::endl;
	double temp2 = (D.d1 + D.d2) / (D.d1 * D.d2);
	double temp3 = pow( (temp1 * temp2), 0.5);
	double v = D.h * temp3;
	// std::cout << "v : " << v << std::endl;
	double temp4 = pow( (v - 0.1), 2);
	double temp5 = pow( temp4+1 , 0.5);
	DiffLoss = 6.9 + 20 * log10(temp5 + v - 0.1);	
	//std::cout << "DiffLossの返却 " << std::endl;
	return DiffLoss;
}

// 交差判定 戻り値は交点の３次元座標ベクトル
Vector3D
MountainPropagationLossModel::CrossingDetection (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
	// 交点の座標ベクトル
	Vector3D IntersectionPoint (0.0, 0.0, 0.0);
	// 交差判定フラグ
	bool C_Detection = false;
	
	// 通信ノードの座標ベクトルの取得
	Vector3D A = a->GetPosition();
	//std::cout << "A (" << std::setw(7) << A.x << ", " << std::setw(7) << A.y << ", " << std::setw(7) << A.z << ")" << std::endl;
	Vector3D B = b->GetPosition(); 
	//std::cout << "B (" << std::setw(7) << B.x << ", " <<  std::setw(7) << B.y << ", " <<  std::setw(7) << B.z << ")" << std::endl;
	
	/* 方向ベクトルの計算
	 * 浮動小数点の誤差対策のため-0.000001以上0.000001以下は0とみなす
	 */
	// ベクトルR(=AB)の計算
	Vector3D R (B.x - A.x, B.y - A.y, B.z - A.z);
	if((-0.000001 < R.x) && (R.x < 0.000001)) R.x = 0;
	if((-0.000001 < R.y) && (R.y < 0.000001)) R.y = 0;
	if((-0.000001 < R.z) && (R.z < 0.000001)) R.z = 0;
	// ベクトルE1(=V1-V0)の計算
	Vector3D E1 (V1.x - V0.x, V1.y - V0.y, V1.z - V0.z);
	if((-0.000001 < E1.x) && (E1.x < 0.000001)) E1.x = 0;
	if((-0.000001 < E1.y) && (E1.y < 0.000001)) E1.y = 0;
	if((-0.000001 < E1.z) && (E1.z < 0.000001)) E1.z = 0;
	// ベクトルE2(=V2-V0)の計算
	Vector3D E2 (V2.x - V0.x, V2.y - V0.y, V2.z - V0.z);
	if((-0.000001 < E2.x) && (E2.x < 0.000001)) E2.x = 0;
	if((-0.000001 < E2.y) && (E2.y < 0.000001)) E2.y = 0;
	if((-0.000001 < E2.z) && (E2.z < 0.000001)) E2.z = 0;
	// ベクトルT(=A-V0)の計算
	Vector3D T (A.x - V0.x, A.y - V0.y, A.z - V0.z);
	if((-0.000001 < T.x) && (T.x < 0.000001)) T.x = 0;
	if((-0.000001 < T.y) && (T.y < 0.000001)) T.y = 0;
	if((-0.000001 < T.z) && (T.z < 0.000001)) T.z = 0;

	
	/* クラメルの公式によりスカラー量t, u, vの値は
	 * t = det(T, E1, E2) / det(-R, E1, E2)
	 * u = det(-R,T, E2) / det(-R, E1, E2)
	 * v = det(-R, E1, T) / det(-R, E1, E2)
	 * となる. この計算すると計算時間が長くなるので式変形をする.
	 * 3次元の行列式において  det(a, b, c) = (a×b)・c  …① 
	 * 外積において  a × b = -b × a  …②
	 * a × b = -a × -b  …③
	 * ①〜③より先ほどの式は
	 * t = (T×E1)・E2 / (R×E2)・T
	 * u = (R×E2)・T / (R×E2)・T
	 * v = (T×E1)・R / (R×E2)・T
	 * となる. (T×E1), (R×E2)が頻出するのでこれらをまとめて計算すれば計算量を少なくすることができる.
	 */

	// 外積 T×E1 の計算
	Vector3D TE1 ( T.y*E1.z - T.z*E1.y, T.z*E1.x - T.x*E1.z, T.x*E1.y - T.y*E1.x);
	if((-0.000001 < TE1.x) && (TE1.x < 0.000001)) TE1.x = 0;
	if((-0.000001 < TE1.y) && (TE1.y < 0.000001)) TE1.y = 0;
	if((-0.000001 < TE1.z) && (TE1.z < 0.000001)) TE1.z = 0;
	// std::cout << "(T×E1) = (" << std::setw(7) << TE1.x << ", " <<  std::setw(7) << TE1.y << ", " <<  std::setw(7) << TE1.z << ")" << std::endl;
	// 外積 R×E2 の計算
	Vector3D RE2 ( R.y*E2.z - R.z*E2.y, R.z*E2.x - R.x*E2.z, R.x*E2.y - R.y*E2.x);
	if((-0.000001 < RE2.x) && (RE2.x < 0.000001)) RE2.x = 0;
	if((-0.000001 < RE2.y) && (RE2.y < 0.000001)) RE2.y = 0;
	if((-0.000001 < RE2.z) && (RE2.z < 0.000001)) RE2.z = 0;
	// std::cout << "(R×E2) = (" << std::setw(7) << RE2.x << ", " <<  std::setw(7) << RE2.y << ", " <<  std::setw(7) << RE2.z << ")" << std::endl;

	// 分母(R×E2・E1)
	double RE2E1 = RE2.x*E1.x + RE2.y*E1.y + RE2.z*E1.z;
	// std::cout << "(R×E2)・E1 = " << RE2E1 << std::endl;
	// uの分子
	double RE2T = RE2.x*T.x + RE2.y*T.y + RE2.z*T.z;
	// std::cout << "(R×E2)・T = " << RE2T << std::endl;
	// vの分子
	double TE1R = TE1.x*R.x + TE1.y*R.y + TE1.z*R.z;
	// std::cout << "(T×E1)・R = " << TE1R << std::endl;
	// tの分子
	double TE1E2 = TE1.x*E2.x + TE1.y*E2.y + TE1.z*E2.z;
	//std::cout << "(T×E1)・E2 = " << TE1E2 << std::endl;
	
	// スカラ量uの計算
	double u = RE2T / RE2E1;
	//std::cout << "u = " << u << std::endl;
	// スカラ量vの計算
	double v = TE1R / RE2E1;
	//std::cout << "v = " << v << std::endl;
	// スカラ量tの計算
	double t = TE1E2 / RE2E1;
	//std::cout << "t = " << t << std::endl;
	
	// 交差判定
	if((0 <= u) && (u <= 1) && (0 <= v) && (v <= 1) && (0 <= (u+v)) && ((u+v) <= 1) && (t <= 1) && (0 <= t)){
			//std::cout << "交差しました" << std::endl;
			C_Detection = true;		// 交差
	}
	
	// std::cout << "交差判定フラグ : " << C_Detection << std::endl;
	
	// 交差点の座標ベクトルの計算
	if(C_Detection == true){
		IntersectionPoint.x = V0.x + E1.x * u + E2.x * v;
		IntersectionPoint.y = V0.y + E1.y * u + E2.y * v;
		IntersectionPoint.z = V0.z + E1.z * u + E2.z * v;
		// std::cout << "交差点 (" << std::setw(7) << IntersectionPoint.x << ", " <<  std::setw(7) << IntersectionPoint.y << ", " <<  std::setw(7) << IntersectionPoint.z << ")" << std::endl;
	}
	 
	// 交点の座標ベクトルを返す
	return IntersectionPoint;
}

// 回折損の計算に必要な高さh(交点と辺の最近点の距離)を返すメソッド
MountainPropagationLossModel::calcdata_t
MountainPropagationLossModel::CalcDistance (Ptr<MobilityModel> a,
									  Ptr<MobilityModel> b, Vector3D I) const
{	
	// 通信ノードの座標ベクトルの取得
	Vector3D A = a->GetPosition();
	Vector3D B = b->GetPosition();
	// CalcData型構造体DiffDataの宣言
	calcdata_t DiffData;
	// 交差点とベクトルE1の最近点をメソッドCalcNearPointより求める
	//Vector3D n (1, 2, 3);
	Vector3D n ( CalcNearPoint(a, b, I));
	
	// 高さh(点nと点Iの距離)の計算
	DiffData.h = sqrt(pow( n.x - I.x, 2) + pow( n.y - I.y, 2) + pow( n.z - I.z, 2));
	
	// 通信ノードの座標と最近点の座標の距離d1, d2の計算
	double temp1 = pow((A.x - n.x),2) + pow((A.y - n.y),2) + pow((A.z - n.z),2);
	double temp2 = pow((B.x - n.x),2) + pow((B.y - n.y),2) + pow((B.z - n.z),2);
	DiffData.d1 = pow( temp1, 0.5);
	DiffData.d2 = pow( temp2, 0.5);
	
	// std::cout << "高さh " << DiffData.h << "(m)" << std::endl;
	// std::cout << "距離d1 " << DiffData.d1 << "(m)" << std::endl;
	// std::cout << "距離d2 " << DiffData.d2 << "(m)" << std::endl;
	 
	// 高さhと距離d1, d2が格納された構造体DiffDataを返す
	return DiffData;	
}

// 回折点の座標nを求めるメソッド
// 引数はノードポインタa, b と交点の座標ベクトルI
Vector3D
MountainPropagationLossModel::CalcNearPoint (Ptr<MobilityModel> a,
									  Ptr<MobilityModel> b, Vector3D I) const
{	
	Vector3D d(0, 0, 0);
	// ベクトルIの成分が全て0のとき交差はしていないと判断する
	if((I.x == 0.0) && (I.y == 0.0) && (I.z == 0.0)){
		std::cout << "交点エラー" <<std::endl;
	}
	else{
		// ベクトルE1 (= V1-V0 )の計算
		Vector3D E1 (V1.x - V0.x, V1.y - V0.y, V1.z - V0.z);
		// ベクトル I - V0 の計算
		Vector3D I_V0 (I.x - V0.x, I.y - V0.y, I.z - V0.z);
		// ベクトルE1の距離
		double len_E1 = pow( ( E1.x * E1.x ) + ( E1.y * E1.y ) + ( E1.z * E1.z ), 0.5 );
		// ベクトルE1の単位ベクトル
		Vector3D unitV_E1  (E1.x / len_E1, E1.y / len_E1, E1.z / len_E1);
		// 回折点dと点V0の距離
		double len_n_V0 = unitV_E1.x * I_V0.x + unitV_E1.y * I_V0.y + unitV_E1.z * I_V0.z;
		// 回折点dの計算
		d.x = V0.x + unitV_E1.x * len_n_V0;
		d.y = V0.y + unitV_E1.y * len_n_V0;
		d.z = V0.z + unitV_E1.z * len_n_V0;
	}
	// std::cout << "回折点n (" << std::setw(7) << d.x << ", " << std::setw(7) << d.y << ", "<< std::setw(7) << d.z << ")" << std::endl;
	return d;
}

/*
// ノードの標高を計算するメソッド(収束計算ver)
double
MountainPropagationLossModel::DoCalcElevation(Ptr<MobilityModel> n) const
{
	// 通信ノードの座標ベクトルの取得
	Vector3D position = n->GetPosition();
	std::cout << "X : " << std::setprecision(11) << position.x << std::endl;
	std::cout << "Y : " << std::setprecision(11) << position.y << std::endl;
	std::cout << "Z : " << std::setprecision(11) << position.z << std::endl;
	// φiとφi-1を格納する配列
	double phi[2];
	
	double long_r = 6378137;
	double eccentricity2 = 0.00669438;
	double Ng = 42.7329;

	// P
	double P = std::sqrt((std::pow(position.x, 2) + std::pow(position.y, 2)));
	std::cout << "P : " << P << std::endl;
	// 緯度φ0の計算
	std::cout<< M_PI << std::endl;
	phi[0] = std::atan2(position.z, P);
	std::cout << "φ0 = " << phi[0] << std::endl; 
	
	// 経度λの計算(変数名lamdbaは使えないのでv_lamdbaとおく)
	// 標高の計算には使わないのでコメントアウトしておく
	// double v_lambda = 1std::atan2(position.y, position.x);
	
	// 緯度φが収束条件|φi-φ(i-1)|≪10^(-12) を満たすまで繰り返し計算
	double W = std::sqrt((1 - eccentricity2 * std::pow(std::sin(phi[0]), 2)));
	std::cout << "W0 = " << W << std::endl;
	// 卯酉線曲率半径N
	double N = long_r / W;
	std::cout << "N0 = " << N << std::endl;
	
	// φiの計算
	phi[1] = std::atan2(position.z, (P - eccentricity2 * N * std::cos(phi[0])));
	std::cout << "φ1 = " << phi[1] << std::endl;
	double h = P/std::cos(phi[1]) - N;
	std::cout << "h : " << h << std::endl;

	do{
		std::cout << "収束判定" << phi[1]-phi[0] << std::endl;
		// long double temp = phi[0];
		phi[0] = phi[1];
		W = std::sqrt((1 - eccentricity2 * std::pow(std::sin(phi[0]), 2)));
		std::cout << "Wi = " << W << std::endl;
		N = long_r / W;
		std::cout << "Ni = " << N << std::endl;
		phi[1] = std::atan2(position.z, (P - eccentricity2 * N * std::cos(phi[1])));
		std::cout << "φi = " << phi[1] << std::endl; 
		std::cout << P/std::cos(phi[1]) - N << std::endl;
	}while((phi[1] - phi[0]) >= 10e-12);
	
	// 楕円体高
	h = P/std::cos(phi[1]) - N;
	std::cout << "h : " << h << std::endl;
	// 標高
	double H = h - Ng;
	
	// 標高の値を返却する
	return H;
}
*/

// V-1 標高計算メソッド(近似式ver)
void
MountainPropagationLossModel::DoCalcElevation(VegetationData& vstruct)const
{
	//std::cout << "npのアドレス（メソッド内) " << std::hex << vstruct->np << std::endl;
	vstruct.position = vstruct.np->GetPosition();
	//std::cout << "X : " << std::setprecision(10) << vstruct.position.x << std::endl;
	//std::cout << "Y : " << std::setprecision(10) << vstruct.position.y << std::endl;
	//std::cout << "Z : " << std::setprecision(10) << vstruct.position.z << std::endl;
	
	double long_r = 6378137;
	double short_r = 6356752.314;
	double eccentricity2 = 0.00669438;	
	double eccentricity2d = 0.006739497;
	double Ng = 42.7329;
	
	// P
	double p = std::sqrt((std::pow(vstruct.position.x, 2) + std::pow(vstruct.position.y, 2)));
	//std::cout << "P : " << p << std::endl;
	
	// θ(rad)
	double sita = atan2(vstruct.position.z*long_r, p * short_r);

	// φ
	double phi = atan2((vstruct.position.z + eccentricity2d * short_r * std::pow(std::sin(sita), 3)),
												( p-eccentricity2 * long_r * std::pow(std::cos(sita),3)));
	
	// W
	double W = std::sqrt((1 - eccentricity2 * std::pow(std::sin(phi), 2)));
	//std::cout << "W0 = " << W << std::endl;
	
	// 卯酉線曲率半径N
	double N = long_r / W;
	//std::cout << "N0 = " << N << std::endl;
	
	// 楕円体高(m)
	double h = p/std::cos(phi) - N;

	// 標高(m)
	vstruct.elevation = h - Ng;
}


// V-2 ルート判定メソッド
void
MountainPropagationLossModel::DeterminePath(VegetationData& vstruct)const
{
	// 内側法線ベクトルN
	// あとで属性に登録しておく
	Vector3D N = Vector3D(1766468.316, -2286212.248, 3900466.542);
//	Vector3D N = Vector3D(1020776.021, -1003559.514, 1969814.385);

	// 超平面上の点
	Vector3D Point = Vector3D(-3802057.301	, 	3470280.218	, 	3755963.738);
	// ３次元平面の頂点からノードの座標へ伸びるベクトルV
	Vector3D V;
	V.x = vstruct.position.x - Point.x;
	V.y = vstruct.position.y - Point.y;
	V.z = vstruct.position.z - Point.z;
	
	// ベクトルの内積
	double DotProduct = N.x * V.x + N.y * V.y + N.z * V.z;
	
	// 内積が正なら内側、負なら外側
	if(DotProduct > 0)
	{
		vstruct.path = true;	// 内側
		//std::cout << "内側" << std::endl;
	}else
	{
		vstruct.path = false;	// 外側
		//std::cout << "外側" << std::endl;
	}
}


// V-3 植生判定メソッド
void
MountainPropagationLossModel::DetermineArea(VegetationData& vstruct)const
{
	// 森林限界
	double h_timberline = 1810;
	double y_timberline = 1752;
	
	// ルートと, そのルートの森林限界との比較を同時に行う
	if((vstruct.path == true) && (vstruct.elevation > h_timberline)){
		vstruct.vegetation = false;
	}
	else if((vstruct.path == false) && (vstruct.elevation > y_timberline))
	{
		vstruct.vegetation = false;
	}else
	{
		vstruct.vegetation = true;
	}
}

// V-4 植生減衰の距離を計算するメソッド
double
MountainPropagationLossModel::DoCalcVegetationalDistance(VegetationData& vstruct_a, VegetationData& vstruct_b)const
{
	// 植生による減衰をうける距離
	double Vdistance = 0;
	// 森林限界
	double timberline = 0;
	// 森林限界（これもあとで属性にする)
	double h_timberline = 1810;
	double y_timberline = 1752;
	// ノード間距離
	double DistanceAtoB = CalculateDistance(vstruct_a.position, vstruct_b.position);
	
	
	// どちらも植生内にいない
	if((vstruct_a.vegetation == false) && (vstruct_b.vegetation == false)){
		Vdistance = 0;
		//std::cout << "植生外" << std::endl;
	}
	// 両方のノードが植生の中にいる
	else if((vstruct_a.vegetation == true) && (vstruct_b.vegetation == true)){
		Vdistance = DistanceAtoB;
		//std::cout << "共に植生内" << std::endl;
	}
	// 一方のノードのみが植生内にいる
	else{
		//std::cout << "片方が植生内" << std::endl;
		// ノードaのみが植生内にいる
		if((vstruct_a.vegetation == true) && (vstruct_b.vegetation == false)){
					if(vstruct_a.path == true)
						timberline = h_timberline;
					if(vstruct_a.path == false)
						timberline = y_timberline;
					// ノードaと森林限界の標高差
					double A_Timb = vstruct_a.elevation - timberline;
					// ノードbと森林限界の標高差
					double Timb_B = timberline - vstruct_b.elevation;
					// 植生距離
					Vdistance = DistanceAtoB * A_Timb / Timb_B;
					
		}
		// ノードbのみが植生内にいる
		else if((vstruct_a.vegetation == false) && (vstruct_b.vegetation == true)){
					if(vstruct_b.path == true)
						timberline = h_timberline;
					if(vstruct_b.path == false)
						timberline = y_timberline;
					// ノードaと森林限界の標高差
					double A_Timb = vstruct_a.elevation - timberline;
					// ノードbと森林限界の標高差
					double Timb_B = timberline - vstruct_b.elevation;
					// 植生距離
					Vdistance = DistanceAtoB * Timb_B / A_Timb;
		}
	}
	
	return Vdistance;
}								  

// 植生減衰を計算するメソッド
double
MountainPropagationLossModel::DoCalcVegetationalLoss(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
	
	VegetationData vstructA;
	VegetationData vstructB;
	//std::cout << "構造体aのアドレス" << std::hex << vstruct_a << std::endl;
	vstructA.np = a;
	vstructB.np = b;
		
	DoCalcElevation(vstructA);
	// std::cout << "aの標高 : " << vstructA.elevation << std::endl;
	DeterminePath(vstructA);
	//std::cout << "X : " << std::setprecision(10) << vstructA.position.x << std::endl;
	//std::cout << "Y : " << std::setprecision(10) << vstructA.position.y << std::endl;
	//std::cout << "Z : " << std::setprecision(10) << vstructA.position.z << std::endl;
	//std::cout << "aのルート" << vstructA.path << std::endl << std::endl;
	DetermineArea(vstructA);
	//std::cout << "aの植生" << vstructA.vegetation << std::endl;
		
	DoCalcElevation(vstructB);
	// std::cout << "bの標高 : " << vstructB.elevation << std::endl;
	DeterminePath(vstructB);
	// std::cout << "X : " << std::setprecision(10) << vstructB.position.x << std::endl;
	// std::cout << "Y : " << std::setprecision(10) << vstructB.position.y << std::endl;
	// std::cout << "Z : " << std::setprecision(10) << vstructB.position.z << std::endl;
	 //std::cout << "bのルート" << vstructB.path << std::endl;
	// std::cout << std::endl;
	DetermineArea(vstructB);
	//std::cout << "bの植生" << vstructB.vegetation << std::endl;
		
	double distance = DoCalcVegetationalDistance(vstructA, vstructB);
	//std::cout << "植生距離" << distance << std::endl; 
		   
	double Aev = 0.0;	// Attenuation in vegetation
	double Am = 0.0;	// maximum attenuation for one terminal within a specific type and depth of vegetation
	double A_gamma = 0.0; // specific attenuation for very short vegetative paths
	
	
	if(m_frequency == 142000000){
		Am = 9.4;
		A_gamma = 0.04;
		Aev = Am * (1 - exp((-distance * A_gamma)/Am));
	}
	
	else if(m_frequency == 900000000){
			Am = 26.5;
			A_gamma = 0.17;
			Aev = Am * (1 - exp((-distance * A_gamma)/Am));
	}
	
	return Aev;
}

// 受信電力の計算
double
MountainPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                          Ptr<MobilityModel> a,
                                          Ptr<MobilityModel> b) const
{
	// 最終的な損失
	double SumLoss = 0.0;
	// 回折損
	double DiffLoss = 0.0;
	// 自由空間損失 & 平面大地
	double TwoRayLoss = 0.0;
	// 植生による減衰
	double vAttenuation = 0.0;
	//二点間の距離
	//double distance = a->GetDistanceFrom (b);	
	
	DiffLoss = DoCalcDiffractionLoss(a, b);		// 回折損
	 //std::cout << "回折損" << DiffLoss << " (dB)" << std::endl;
//	std::cout << DiffLoss << std::endl;
	SumLoss += DiffLoss;
	/*
	if(DiffLoss == 0.0){
		vAttenuation = DoCalcVegetationalLoss(a, b);
		//std::cout << "植生減衰" << vAttenuation << " (dB)" << std::endl;
		SumLoss += vAttenuation;
	}
	*/
//	std::cout <<  vAttenuation << std::endl;
		vAttenuation = DoCalcVegetationalLoss(a, b);
		//std::cout << "植生減衰" << vAttenuation << " (dB)" << std::endl;
		SumLoss += vAttenuation;

    TwoRayLoss = DoCalcTwoRayGroundLoss(a, b);
    //std::cout << "平面大地伝搬損失" << TwoRayLoss << " (dB)" << std::endl;
//    std::cout << TwoRayLoss << std::endl;
    SumLoss += TwoRayLoss;
    // Friisの自由空間損失
		
//	std::cout << "最終的な損失 : " << SumLoss << " (dB)" << std::endl;
//	std::cout << SumLoss << std::endl;
//	std::cout << std::endl;
/*	std::string tf = "prop.txt";
	const char * st = tf.c_str();
	std::ofstream ofs(st,std::ios::out|std::ios::app);
	ofs<<"平面大地"<<TwoRayLoss<<"\n";
	ofs<<"植生減衰"<<vAttenuation<<"\n";
	ofs<<"回折損失"<<DiffLoss<<"\n";
	ofs<<"\n";
	ofs.close();
*/	

/*	std::string tf = "ClimberProp.txt";
	const char * st = tf.c_str();
	std::ofstream ofs(st,std::ios::out|std::ios::app);
	ofs<<distance<<","<<TwoRayLoss<<"\n";
	ofs.close();
*/
std::cout <<  DiffLoss << "," << vAttenuation<<","<<TwoRayLoss<<","<<SumLoss<<std::endl;


	
	// 受信電力の返却
	//std::cout << "受信電力の返却" << std::endl;
//	std::cout << txPowerDbm - SumLoss << std::endl;
	return txPowerDbm - SumLoss;
   
}

// PropagationLossModelを親クラスにもつ場合は必ず実装するメソッド
int64_t
MountainPropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

}
