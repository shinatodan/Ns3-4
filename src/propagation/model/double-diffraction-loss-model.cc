#include "ns3/propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "ns3/vector.h"
#include "double-diffraction-loss-model.h"
#include <iomanip>			// std::setw()
#define _USE_MATH_DEFINES // for C++  
#include <cmath>  
#include <chrono>
#include <time.h> 
#include <iostream>
#include <fstream>
#include <sstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DoubleDiffractionLossModel");

NS_OBJECT_ENSURE_REGISTERED (DoubleDiffractionLossModel);


TypeId 
DoubleDiffractionLossModel::GetTypeId (void){
  static TypeId tid = TypeId ("ns3::DoubleDiffractionLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<DoubleDiffractionLossModel> ()
    .AddAttribute ("Frequency",
                   "The carrier frequency (in Hz) at which propagation occurs  (default is 900MHz).",
                   DoubleValue (142000000),
                   MakeDoubleAccessor (&DoubleDiffractionLossModel::SetFrequency,
                                       &DoubleDiffractionLossModel::GetFrequency),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("SystemLoss", "The system loss",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&DoubleDiffractionLossModel::m_systemLoss),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MinLoss", 
                   "The minimum value (dB) of the total loss, used at short ranges. Note: ",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&DoubleDiffractionLossModel::SetMinLoss,
                                       &DoubleDiffractionLossModel::GetMinLoss),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("M1V0", 
                   "Vector 0 of Obstacle",			// AD006
                   Vector3DValue (Vector3D(-3802056.926, 3470149.789, 3756099.022)),
                   //Vector3DValue (Vector3D(0, 0, 0)),
                   MakeVector3DAccessor (&DoubleDiffractionLossModel::SetVectorM1V0),
                   MakeVector3DChecker ())							// MakeDoubleChackerのように<>で型指定はしない（するとエラーになる)
    .AddAttribute ("M1V1", 
                   "Vector 1 of Obstacle",			// 蒲田富士
                   Vector3DValue (Vector3D(-3805222.186, 3468964.71, 3756626.002)),	
                   //Vector3DValue (Vector3D(0, 0, 0)),
                   MakeVector3DAccessor (&DoubleDiffractionLossModel::SetVectorM1V1),
                   MakeVector3DChecker ())
    .AddAttribute ("M1V2", 
                   "Vector 2 of Obstacle",			// 計算から求めた
                   Vector3DValue (Vector3D(-3803596.554,3468518.125,3755869.378)),
                   //Vector3DValue (Vector3D(0,0,0)),
                   MakeVector3DAccessor (&DoubleDiffractionLossModel::SetVectorM1V2),
                   MakeVector3DChecker ())
    .AddAttribute ("M2V0", 
                   "Vector 0 of Obstacle",			// AP_h2
                   Vector3DValue (Vector3D(-3803320.996,3469924.412,3755565.018)),
                   //Vector3DValue (Vector3D(-3803609.589,3469709.278, 3755669.815)),			// 初期値はVector3D(x, y, z)で指定する 
                   //Vector3DValue (Vector3D(-3801883.204, 3469896.97, 3756478.651)),
                   MakeVector3DAccessor (&DoubleDiffractionLossModel::SetVectorM2V0),
                   MakeVector3DChecker ())							// MakeDoubleChackerのように<>で型指定はしない（するとエラーになる)
    .AddAttribute ("M2V1", 
                   "Vector 1 of Obstacle",			// karasawa1(頂点)
                   Vector3DValue (Vector3D(-3805007.173,3469005.827,3756708.332)),
                   //Vector3DValue (Vector3D(-3801630.647, 3469592.819, 3758218.86)),		
                   MakeVector3DAccessor (&DoubleDiffractionLossModel::SetVectorM2V1),
                   MakeVector3DChecker ())
    .AddAttribute ("M2V2", 
                   "Vector 2 of Obstacle",			// 計算から求めた
                   Vector3DValue (Vector3D(-3804302.42,3468363.308,3756012.525)),
                   //Vector3DValue (Vector3D(-3801207.126392, 3469206.289061, 3757800.175681)),
                   MakeVector3DAccessor (&DoubleDiffractionLossModel::SetVectorM2V2),
                   MakeVector3DChecker ())
    .AddAttribute ("AntennaHeight",
                   "The height of the antenna (m) ",
                   DoubleValue (1.5),
                   MakeDoubleAccessor (&DoubleDiffractionLossModel::AntennaHeight),
                   MakeDoubleChecker<double> ())
   
                
  ;
  return tid;
}
// コンストラクタ
DoubleDiffractionLossModel::DoubleDiffractionLossModel (){
}




// システム損失のセット
void
DoubleDiffractionLossModel::SetSystemLoss (double systemLoss){					    
  m_systemLoss = systemLoss;
}

// システム損失の返却
double
DoubleDiffractionLossModel::GetSystemLoss (void) const{
  return m_systemLoss;
}

// 最小損失のセット
void
DoubleDiffractionLossModel::SetMinLoss (double minLoss){
  m_minLoss = minLoss;
}

// 最小損失の返却
double
DoubleDiffractionLossModel::GetMinLoss (void) const{
  return m_minLoss;
}

// 周波数と波長のセット
void
DoubleDiffractionLossModel::SetFrequency (double frequency){
  m_frequency = frequency;
  static const double C = 299792458.0; // speed of light in vacuum
  m_lambda = C / frequency;
}

// 周波数の返却
double
DoubleDiffractionLossModel::GetFrequency (void) const{
  return m_frequency;
}


// 障害物のベクトルM1VOのセット
void
DoubleDiffractionLossModel::SetVectorM1V0 (Vector3D m1v0 ){
	M1V0.x = m1v0.x;
	M1V0.y = m1v0.y;
	M1V0.z = m1v0.z;
}
// 障害物のベクトルM1V1のセット
void
DoubleDiffractionLossModel::SetVectorM1V1 (Vector3D m1v1 ){
	M1V1.x = m1v1.x;
	M1V1.y = m1v1.y;
	M1V1.z = m1v1.z;
}

// 障害物のベクトルM1V2のセット
void
DoubleDiffractionLossModel::SetVectorM1V2 (Vector3D m1v2 ){
	M1V2.x = m1v2.x;
	M1V2.y = m1v2.y;
	M1V2.z = m1v2.z;
}

// 障害物のベクトルM2V0のセット
void
DoubleDiffractionLossModel::SetVectorM2V0 (Vector3D m2v0 ){
	M2V0.x = m2v0.x;
	M2V0.y = m2v0.y;
	M2V0.z = m2v0.z;
}


// 障害物のベクトルM2V1のセット
void
DoubleDiffractionLossModel::SetVectorM2V1 (Vector3D m2v1 ){
	M2V1.x = m2v1.x;
	M2V1.y = m2v1.y;
	M2V1.z = m2v1.z;
}

// 障害物のベクトルM2V2のセット
void
DoubleDiffractionLossModel::SetVectorM2V2 (Vector3D m2v2 ){
	M2V2.x = m2v2.x;
	M2V2.y = m2v2.y;
	M2V2.z = m2v2.z;
}


void
DoubleDiffractionLossModel::SetAnntenaHeight (double aHeight){
	AntennaHeight = aHeight;
}

// dBm(デシベルミリ) → W(ワット)変換
double
DoubleDiffractionLossModel::DbmToW (double dbm) const{
  double mw = std::pow (10.0,dbm/10.0);
  return mw / 1000.0;
}

// W(ワット) → dBm(デシベルミリ)変換
double
DoubleDiffractionLossModel::DbmFromW (double w) const{
  double dbm = std::log10 (w * 1000.0) * 10.0;
  return dbm;
}

// Friisの自由空間損失の計算
double
DoubleDiffractionLossModel::DoCalcFriisLoss (Ptr<MobilityModel> a,
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
   *
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
DoubleDiffractionLossModel::DoCalcTwoRayGroundLoss( Ptr<MobilityModel> a,
                                                 Ptr<MobilityModel> b) const
{
  double distance = a->GetDistanceFrom (b);
  if (distance <= 0.5)
    {
      return 0;
    }

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

/*
  // 平面大地改定版
double
DoubleDiffractionLossModel::DoCalcTwoRayGroundLossRev( Ptr<MobilityModel> a,
                                                 Ptr<MobilityModel> b) const{
					
	int f;//交差判定フラグ								
	//座標取得				 
	Vector3D A = a->GetPosition();
	Vector3D B = b->GetPosition();		
	
	Vector3D f1 (CrossingDetection01(A,B)); // 山１との交差点
	Vector3D f2 (CrossingDetection02(A,B)); // 山2との交差点
	
	if(f1.x != 0.0 && f1.y != 0.0 && f1.z != 0.0)//山１で回折するなら
	{
		f=1;
		if(f2.x != 0.0 && f2.y != 0.0 && f2.z != 0.0)//二重回折なら
		f=3;
	}
	if(f2.x != 0.0 && f2.y != 0.0 && f2.z != 0.0 && f==0){
	f=2;//山２のみで回折
	}
	
	
	
	if(f==1){//山１のみの計算
		
		Vector3D d1 = watanabe1(f1);//回折点d1
		Vector3D VA = DoCalcVerticalTwoPoint(A,d1);//回折点から垂直におろした点
		Vector3D VB = DoCalcVerticalTwoPoint(B,d1);
		double AntennaA_d1 = dist_vertex_np(VA,d1);//アンテナ高(A-d1)
		double AntennaB_d1 = dist_vertex_np(VB,d1);
		double distanceA_d1 = dist_vertex_np(A,VA);//距離(A-VA)
		double distanceB_d1 = dist_vertex_np(B,VB);//距離(B-VB)
		
		// 分子
		double numeratorA_d1 = distanceA_d1 * distanceA_d1;
	  // 分母
		double denominatorA_d1 = AntennaA_d1 * AntennaHeight;
	  // 損失
		double lossDbA_d1 = 20 * log10 (numeratorA_d1 / denominatorA_d1);
		
		// 分子
		double numeratorB_d1 = distanceB_d1 * distanceB_d1;
	  // 分母
		double denominatorB_d1 = AntennaB_d1 * AntennaHeight;
	  // 損失
		double lossDbB_d1 = 20 * log10 (numeratorB_d1 / denominatorB_d1);
		
		return lossDbA_d1+lossDbB_d1; 
	}
	else if(f==3){//二重回折
		  
		Vector3D d1 = watanabe1(f1);
		Vector3D d2 = watanabe2(f2);
        Vector3D VA = DoCalcVerticalTwoPoint(A,d1);
	    Vector3D Vd1 = DoCalcVerticalTwoPoint(d2,d1);
		Vector3D Vd2 = DoCalcVerticalTwoPoint(B,d2);
		double Antennad1_VA = dist_vertex_np(d1,VA);
		double Antennad1_Vd1 = dist_vertex_np(d1,Vd1);
		double Antennad2_Vd2 = dist_vertex_np(d2,Vd2);
		double distanceA_VA = dist_vertex_np(A,VA);
		double distanceVd1_d2 = dist_vertex_np(Vd1,d2);
		double distanceVd2_B = dist_vertex_np(Vd2,B);
		  
		double numeratorA_VA = distanceA_VA * distanceA_VA;
	
	    double denominatord1_VA = Antennad1_VA * AntennaHeight;
	 
	    double lossDbA_d1 = 20 * log10 (numeratorA_VA / denominatord1_VA);
	    
	    double numeratorVd1_d2 = distanceVd1_d2 * distanceVd1_d2;
	
	    double denominatord1_Vd1 = Antennad1_Vd1 * Antennad2_Vd2;
	 
	    double lossDbd1_d2 = 20 * log10 (numeratorVd1_d2 / denominatord1_Vd1);
	    
	    double numeratorVd2_B = distanceVd2_B * distanceVd2_B;
	
	    double denominatord2_VB = Antennad2_Vd2 * AntennaHeight;
	 
	    double lossDbB_d2 = 20 * log10 (numeratorVd2_B / denominatord2_VB);
	    
	    return lossDbA_d1+lossDbd1_d2+lossDbB_d2;

	  }
	  else return 0;
	}
*/

/*
//回折点から垂直に下ろした点を求めるメソッド
//引数n1は送信点、n2は回折点
Vector3D
DoubleDiffiractionLossModel::DoCalcVerticalTwoPoint(Vector3D n1, Vector3D n2) const{
	
	Vector3D V;//n2から垂直に下ろした点
	double tmp1 = n1.x*n2.x+n1.y*n2.y+n1.z*n2.z;
	double t = tmp1/(n2.x*n2.x+n2.y*n2.y+n2.z*n2.z);
	V.x = t*n2.x;
	V.y = t*n2.y;
	V.z = t*n2.z;
	return V;
	}
*/
	
//改定版回折損失計算	
double
DoubleDiffractionLossModel::DoCalcDiffRev(Ptr<MobilityModel> n1,
											     Ptr<MobilityModel> n2)const{
	int f = 0;//交差判定用フラグ、0が回折しない、１が山１でのみ回折、２が山２のみで回折、３が二重回折
	int direction = 0;//正方向通信:0,逆方向通信:1
	// 通信ノードの座標ベクトルの取得
	Vector3D A = n1->GetPosition();
	//std::cout << "A (" << std::setw(7) << A.x << ", " << std::setw(7) << A.y << ", " << std::setw(7) << A.z << ")" << std::endl;
	Vector3D B = n2->GetPosition(); 
	//std::cout << "B (" << std::setw(7) << B.x << ", " <<  std::setw(7) << B.y << ", " <<  std::setw(7) << B.z << ")" << std::endl;
	double diffloss = 0.0;
	
	double distance = 0.0;
	
	Vector3D f1 (CrossingDetection01(A,B)); // 山１との交差点
	Vector3D f2 (CrossingDetection02(A,B)); // 山2との交差点
	double dist1 = dist_vertex_np(A,f1);
	double dist2 = dist_vertex_np(A,f2);
	
	if (dist1<dist2){//山１のほうが近い場合
	//二重回折か普通の回折か回折しないかを判定する
	if(f1.x != 0.0 && f1.y != 0.0 && f1.z != 0.0)
	{
		//山１との回折点d1を求める
		Vector3D d1 (watanabe1(f1));
		//d1とBで山２と交差するか判定
		Vector3D f3 (CrossingDetection02(d1,B));
		if(f3.x != 0.0 && f3.y != 0.0 && f3.z != 0.0)//山１と山２での回折	
		{	
			f=3;
			Vector3D d2 (watanabe2(f2));
			//std::cout << "回折点d2(" << std::setw(7) << d2.x << ", " << std::setw(7) << d2.y << ", " << std::setw(7) << d2.z << ")" << std::endl;
			diffloss = DoCalcDoubleDiffLoss(A,d1,d2,B);
			double tmp1 = dist_vertex_np(A,d1);
			double tmp2 = dist_vertex_np(d1,d2);
			double tmp3 = dist_vertex_np(d2,B);
			distance = tmp1+tmp2+tmp3;			
			std::cout << "2重回折損" << diffloss << " (dB)" << std::endl;
			std::cout << "経路長 " << distance << std::endl;
		}
		else{
		f = 1;
		diffloss = DoCalcSingleDiffLoss(A,B,d1);//山１のみと回折
		std::cout << "山１のみでの回折損" << diffloss << " (dB)" << std::endl;
		}
		
	}
	//山２でのみ交差する場合	
	if(f2.x != 0.0 && f2.y != 0.0 && f2.z != 0.0 && f == 0)
	{
		f = 2;
		//std::cout << "山２のみでの回折損" << diffloss << " (dB)" << std::endl;
		Vector3D d2 (watanabe2(f2));
		diffloss = DoCalcSingleDiffLoss(A,B,d2);
		}
	}
	
	else if(dist1>dist2){//山２のほうが近い場合
		//山２との回折点を求める
		//std::cout << "逆サイド通信" << std::endl;
		Vector3D d2 (watanabe2(f2));
		//d2と山１が交差するか判定
		Vector3D f3 (CrossingDetection01(d2,B));
		if(f3.x != 0.0 && f3.y != 0.0 && f3.z != 0.0)
		{
			f=3;
			Vector3D d1 (watanabe1(f1));
			diffloss = DoCalcDoubleDiffLoss(A,d2,d1,B);
			double tmp1 = dist_vertex_np(A,d2);
			double tmp2 = dist_vertex_np(d1,d2);
			double tmp3 = dist_vertex_np(d1,B);
			distance = tmp1+tmp2+tmp3;			
			std::cout << "2重回折損" << diffloss << " (dB)" << std::endl;
			std::cout << "経路長 " << distance << std::endl;
		}
		else{
		f = 2;//
		diffloss = DoCalcSingleDiffLoss(A,B,d2);//山２のみと回折
		//std::cout << "山２のみでの回折損" << diffloss << " (dB)" << std::endl;
		}
		//山１のみで回折
		if(f1.x != 0.0 && f1.y != 0.0 && f1.z != 0.0 && f == 0)
		{
		f = 1;
		Vector3D d1 (watanabe2(f1));
		diffloss = DoCalcSingleDiffLoss(A,B,d1);
		}
		
	}
	
	else 
		diffloss = 0.0;
		
	if(dist1>dist2)
	direction = 1;
	
		
	//山との交差判定フラグ
	//std::cout << "f = " << f << std::endl;
	double dist = dist_vertex_np(A,B);
	//std::cout << "二点間の距離 = " << dist << "m"<<std::endl;
	// 通信ノードの座標ベクトルの取得
	//std::cout << "A (" << std::setw(7) << A.x << ", " << std::setw(7) << A.y << ", " << std::setw(7) << A.z << ")" << std::endl;
	//std::cout << "B (" << std::setw(7) << B.x << ", " <<  std::setw(7) << B.y << ", " <<  std::setw(7) << B.z << ")" << std::endl;
	std::string tft = "dist.txt";
	const char * st = tft.c_str();
	std::ofstream ofs(st,std::ios::out|std::ios::app);
	ofs<<"f="<<f<<"\n";
	ofs<<"ノードAの座標"<< "A (" << std::setw(7) << A.x << ", " << std::setw(7) << A.y << ", " << std::setw(7) << A.z << ")" <<"\n";
	ofs<<"ノードBの座標"<< "B (" << std::setw(7) << B.x << ", " << std::setw(7) << B.y << ", " << std::setw(7) << B.z << ")" <<"\n";
	ofs<<"二点間の距離"<<dist<<"m"<<"\n";
	ofs<<"回折損失"<<diffloss<<"\n";
	ofs<<"方向フラグ"<<direction<<"\n";
	ofs<<"\n";
	ofs.close();
	
	return diffloss;

}


/*
double
DoubleDiffractionLossModel::DoCalcDiffLossRev(Ptr<MobilityModel> n1,
											     Ptr<MobilityModel> n2)const{
												 
												 
	//int f = 0;//交差判定用フラグ、0が回折しない、１が山１でのみ回折、２が山２のみで回折、３が二重回折
	// 通信ノードの座標ベクトルの取得
	Vector3D A = n1->GetPosition();
	//std::cout << "A (" << std::setw(7) << A.x << ", " << std::setw(7) << A.y << ", " << std::setw(7) << A.z << ")" << std::endl;
	Vector3D B = n2->GetPosition(); 
	//std::cout << "B (" << std::setw(7) << B.x << ", " <<  std::setw(7) << B.y << ", " <<  std::setw(7) << B.z << ")" << std::endl;
	double diffloss = 0.0;
	
	double distance = 0.0;
	
	Vector3D f1 (CrossingDetection01(A,B)); // 山１との交差点
	Vector3D f2 (CrossingDetection02(A,B)); // 山2との交差点
	
	Vector3D d1 (watanabe1(f1));
	Vector3D d2 (watanabe2(f2));
	
	double tmp1 = dist_vertex_np(A,d2);
	double tmp2 = dist_vertex_np(d1,d2);
	double tmp3 = dist_vertex_np(d1,B);
		
	distance = tmp1+tmp2+tmp3;
		
	diffloss = DoCalcDoubleDiffLoss(A,d1,d2,B);
	std::cout << "2重回折損" << diffloss << " (dB)" << std::endl;
	std::cout << "経路長 " << distance << std::endl;
	return diffloss;									 
												 
 }
*/
	
	
	
	
	

//山一つとの回折の計算
double
DoubleDiffractionLossModel::DoCalcSingleDiffLoss(Vector3D n1, Vector3D n2, Vector3D d)const{
	
	double h = height(n1,d,n2);
	double d1 = dist_vertex_np(n1,d);
	double d2 = dist_vertex_np(n2,d);
	double diffloss = DoCalcKnifeEdgeLoss(d1,d2,h);
	//std::cout << "回折損" << diffloss << " (dB)" << std::endl;
	return diffloss; 
}



//二重回折の計算
double
DoubleDiffractionLossModel::DoCalcDoubleDiffLoss(Vector3D s,Vector3D d1,Vector3D d2,Vector3D r)const{
  Vector3D h1,h2;
  h1.x = d1.x;
  h1.y = d1.y;
  h2.x = d2.x;
  h2.y = d2.y;

  double tmp_lsd1 = pow((s.x-d1.x),2)+pow((s.y-d1.y),2);
  double tmp_ld1d2 = pow((d1.x-d2.x),2)+pow((d1.y-d2.y),2);
  double tmp_ld2r = pow((d2.x-r.x),2)+pow((d2.y-r.y),2);

  double lsd1 = sqrt(tmp_lsd1);
  double ld1d2 = sqrt(tmp_ld1d2);
  double ld2r = sqrt(tmp_ld2r);
  double tmp = lsd1+ld1d2+ld2r;
  
  h1.z = s.z-(s.z-r.z)*lsd1/tmp;
  h2.z = r.z-(s.z-r.z)*(lsd1+ld1d2)/tmp;

  double a = dist_vertex_np(s,h1);
  double b = dist_vertex_np(h1,h2);
  double c = dist_vertex_np(h2,r);

  //n1,d2と山１の交点、d1,n2と山２の交点
  Vector3D IntersectionP1 = CrossingDetection01(s,d2);
  Vector3D IntersectionP2 = CrossingDetection02(d1,r);

  double l1 = dist_vertex_np(d1,IntersectionP1);
  double l2 = dist_vertex_np(d2,IntersectionP2);
  

  //ナイフエッジの計算
  double L1 = DoCalcKnifeEdgeLoss(a,b,l1);
  double L2 = DoCalcKnifeEdgeLoss(b,c,l2);

  //補正項
  double LC = 0.0;
  double tmp1 = (a+b)*(b+c);
  double tmp2 = b*(a+b+c);
  LC = 10*log10(tmp1/tmp2);

  return L1+L2+LC;
}


//ナイフエッジの減衰を計算する。
double 
DoubleDiffractionLossModel::DoCalcKnifeEdgeLoss (double d1, double d2, double h)const {

	double DiffLoss = 0.0;
	double temp1 = 2 / m_lambda;
	double temp2 = (d1 + d2) / (d1* d2);
	double temp3 = pow( (temp1 * temp2), 0.5);
	double v = h * temp3;
	double temp4 = pow( (v - 0.1), 2);
	double temp5 = pow( temp4+1 , 0.5);
	DiffLoss = 6.9 + 20 * log10(temp5 + v - 0.1);	
	//std::cout << "v = " << v << std::endl;
	//std::cout << "DiffLoss = " << DiffLoss << std::endl;
	return DiffLoss;

}


//山1との交差判定。戻り値は交点の３次元座標ベクトル
//引数Aは送信ノードの座標、引数Bは受信ノードの座標
Vector3D
DoubleDiffractionLossModel::CrossingDetection01 (Vector3D A,Vector3D B) const{
	// 交点の座標ベクトルVector3DValue (Vector3D(-3801207.126392, 3469206.289061, 3757800.175681)),
	Vector3D IntersectionPoint (0.0, 0.0, 0.0);
	// 交差判定フラグ
	bool C_Detection = false;
	
	// 通信ノードの座標ベクトルの取得
	//Vector3D A = a->GetPosition();
	//std::cout << "A (" << std::setw(7) << A.x << ", " << std::setw(7) << A.y << ", " << std::setw(7) << A.z << ")" << std::endl;
	//Vector3D B = b->GetPosition(); 
	//std::cout << "B (" << std::setw(7) << B.x << ", " <<  std::setw(7) << B.y << ", " <<  std::setw(7) << B.z << ")" << std::endl;
	
	Vector3D R (B.x - A.x, B.y - A.y, B.z - A.z);
	if((-0.000001 < R.x) && (R.x < 0.000001)) R.x = 0;
	if((-0.000001 < R.y) && (R.y < 0.000001)) R.y = 0;
	if((-0.000001 < R.z) && (R.z < 0.000001)) R.z = 0;
	// ベクトルE1(=M1V1-M1V0)の計算
	Vector3D E1 (M1V1.x - M1V0.x, M1V1.y - M1V0.y, M1V1.z - M1V0.z);
	//std::cout << "E1 (" << std::setw(7) << E1.x << ", " << std::setw(7) << E1.y << ", " << std::setw(7) << E1.z << ")" << std::endl;
	if((-0.000001 < E1.x) && (E1.x < 0.000001)) E1.x = 0;
	if((-0.000001 < E1.y) && (E1.y < 0.000001)) E1.y = 0;
	if((-0.000001 < E1.z) && (E1.z < 0.000001)) E1.z = 0;
	// ベクトルE2(=M1V2-M1V0)の計算
	Vector3D E2 (M1V2.x - M1V0.x, M1V2.y - M1V0.y, M1V2.z - M1V0.z);
	if((-0.000001 < E2.x) && (E2.x < 0.000001)) E2.x = 0;
	if((-0.000001 < E2.y) && (E2.y < 0.000001)) E2.y = 0;
	if((-0.000001 < E2.z) && (E2.z < 0.000001)) E2.z = 0;
	// ベクトルT(=A-V0)の計算
	Vector3D T (A.x - M1V0.x, A.y - M1V0.y, A.z - M1V0.z);
	if((-0.000001 < T.x) && (T.x < 0.000001)) T.x = 0;
	if((-0.000001 < T.y) && (T.y < 0.000001)) T.y = 0;
	if((-0.000001 < T.z) && (T.z < 0.000001)) T.z = 0;

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
	
	//std::cout << "山１との交差判定フラグ : " << C_Detection << std::endl;
	
	// 交差点の座標ベクトルの計算
	if(C_Detection == true){
		IntersectionPoint.x = M1V0.x + E1.x * u + E2.x * v;
		IntersectionPoint.y = M1V0.y + E1.y * u + E2.y * v;
		IntersectionPoint.z = M1V0.z + E1.z * u + E2.z * v;
		//std::cout << "交差点 (" << std::setw(7) << IntersectionPoint.x << ", " <<  std::setw(7) << IntersectionPoint.y << ", " <<  std::setw(7) << IntersectionPoint.z << ")" << std::endl;
	}
	 
	// 交点の座標ベクトルを返す
	return IntersectionPoint;												 
}
//山2との交差判定。戻り値は交点の３次元座標ベクトル
//引数Aは送信ノードの座標、引数Bは受信ノードの座標
Vector3D
DoubleDiffractionLossModel::CrossingDetection02 (Vector3D A,Vector3D B) const{
	Vector3D IntersectionPoint (0.0, 0.0, 0.0);
	// 交差判定フラグ
	bool C_Detection = false;
	
	// 通信ノードの座標ベクトルの取得
	//Vector3D A = a->GetPosition();
	//std::cout << "A (" << std::setw(7) << A.x << ", " << std::setw(7) << A.y << ", " << std::setw(7) << A.z << ")" << std::endl;
	//Vector3D B = b->GetPosition(); 
	//std::cout << "B (" << std::setw(7) << B.x << ", " <<  std::setw(7) << B.y << ", " <<  std::setw(7) << B.z << ")" << std::endl;
	
	Vector3D R (B.x - A.x, B.y - A.y, B.z - A.z);
	if((-0.000001 < R.x) && (R.x < 0.000001)) R.x = 0;
	if((-0.000001 < R.y) && (R.y < 0.000001)) R.y = 0;
	if((-0.000001 < R.z) && (R.z < 0.000001)) R.z = 0;
	// ベクトルE1(=M1V1-M1V0)の計算
	Vector3D E1 (M2V1.x - M2V0.x, M2V1.y - M2V0.y, M2V1.z - M2V0.z);
	//std::cout << "E1 (" << std::setw(7) << E1.x << ", " << std::setw(7) << E1.y << ", " << std::setw(7) << E1.z << ")" << std::endl;
	if((-0.000001 < E1.x) && (E1.x < 0.000001)) E1.x = 0;
	if((-0.000001 < E1.y) && (E1.y < 0.000001)) E1.y = 0;
	if((-0.000001 < E1.z) && (E1.z < 0.000001)) E1.z = 0;
	// ベクトルE2(=M1V2-M1V0)の計算
	Vector3D E2 (M2V2.x - M2V0.x, M2V2.y - M2V0.y, M2V2.z - M2V0.z);
	if((-0.000001 < E2.x) && (E2.x < 0.000001)) E2.x = 0;
	if((-0.000001 < E2.y) && (E2.y < 0.000001)) E2.y = 0;
	if((-0.000001 < E2.z) && (E2.z < 0.000001)) E2.z = 0;
	// ベクトルT(=A-V0)の計算
	Vector3D T (A.x - M2V0.x, A.y - M2V0.y, A.z - M2V0.z);
	if((-0.000001 < T.x) && (T.x < 0.000001)) T.x = 0;
	if((-0.000001 < T.y) && (T.y < 0.000001)) T.y = 0;
	if((-0.000001 < T.z) && (T.z < 0.000001)) T.z = 0;

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
	
	//std::cout << "山2との交差判定フラグ : " << C_Detection << std::endl;
	
	// 交差点の座標ベクトルの計算
	if(C_Detection == true){
		IntersectionPoint.x = M2V0.x + E1.x * u + E2.x * v;
		IntersectionPoint.y = M2V0.y + E1.y * u + E2.y * v;
		IntersectionPoint.z = M2V0.z + E1.z * u + E2.z * v;
		//std::cout << "交差点 (" << std::setw(7) << IntersectionPoint.x << ", " <<  std::setw(7) << IntersectionPoint.y << ", " <<  std::setw(7) << IntersectionPoint.z << ")" << std::endl;
	}
	 
	// 交点の座標ベクトルを返す
	return IntersectionPoint;													 
}
										 

//単位ベクトルを求める関数
Vector3D
DoubleDiffractionLossModel::get_unit_vector (Vector3D v) const{
	 //ベクトルの長さ
    double length = pow( ( v.x * v.x ) + ( v.y * v.y ) + ( v.z * v.z ), 0.5 );

    //XY各成分を長さで割る
    Vector3D unit;
    unit.x = v.x / length;
    unit.y = v.y / length;
    unit.z = v.z / length;

    return unit;
}
//内積
double 
DoubleDiffractionLossModel::dot_product( Vector3D vl,  Vector3D vr) const {
    return vl.x * vr.x + vl.y * vr.y + vl.z * vr.z;
}

//２点間の距離
double 
DoubleDiffractionLossModel::dist_vertex_np(Vector3D v1, Vector3D v2) const{
    
    double tmp1 = sqrt(pow(v1.x-v2.x,2) + pow(v1.y-v2.y,2) + pow(v1.z-v2.z,2));
    return tmp1;
}

//2点→ベクトル変換abベクトルの作成
Vector3D
DoubleDiffractionLossModel::get_vector(Vector3D a, Vector3D b) const{
	Vector3D New;
	New.x = b.x-a.x;
	New.y = b.y-a.y;
	New.z = b.z-a.z;
	return New;
}

//3点の高さを求める
//点bから辺acに向かって伸びる垂線
double DoubleDiffractionLossModel::height(Vector3D a, Vector3D b, Vector3D c) const{
	//三角形の面積の公式
	//S=b*c*sinA/2=(√a*a*b*b-(a・b)*(a・b))/2
	/*double A = dist_vertex_np(b,c);
	double B = dist_vertex_np(a,b);
	double C = dist_vertex_np(a,c);
	Vector3D AC = get_vector(a,c);
	Vector3D AB = get_vector(a,b);
	double pro = dot_product(AB,AC);
	double tmp = sqrt((B*B*A*A)-(pro*pro));
	double sin = tmp/(B*C);
	return B*sin;
	*/
	//AB,ACの長さの作成
	double AB = dist_vertex_np(a,b);
	double AC = dist_vertex_np(a,c);
	
	//ab,acベクトルの作成
	Vector3D ac = get_vector(a,c);
	Vector3D ab = get_vector(a,b);
	
	//内積ab・acの計算
	double pro = dot_product(ab,ac);
	
	//ab,acのなす角のsinを求める
	double tmp1 = sqrt((AB*AB*AC*AC)-(pro*pro));
	double tmp2 = AB*AC;
	double sin = tmp1/tmp2;
	
	//高さを求める
	double h = sin*AB;
	return h;

}



//回折点をもとめるメソッド
//山１,渡辺方式,引数は交差点I、戻り値は回折点
Vector3D
DoubleDiffractionLossModel::watanabe1(Vector3D p) const{
	
	//山稜ベクトルE1と山端から交差点E2の作成
	Vector3D E1 = get_vector(M1V0,M1V1);
	Vector3D E2 = get_vector(M1V0,p);
	//E1の単位ベクトルunitE1を作成
	Vector3D unitE1 = get_unit_vector(E1);
	double tmp = dot_product(unitE1,E2);
	
	Vector3D d;
	d.x = M1V0.x + unitE1.x*tmp;
	d.y = M1V0.y + unitE1.y*tmp;
	d.z = M1V0.z + unitE1.z*tmp;
	
	return d;
	
}

//回折点をもとめるメソッド
//山2,渡辺方式,引数は交差点I、戻り値は回折点
Vector3D
DoubleDiffractionLossModel::watanabe2(Vector3D p) const{
	
	//山稜ベクトルE1と山端から交差点E2の作成
	Vector3D E1 = get_vector(M2V0,M2V1);
	Vector3D E2 = get_vector(M2V0,p);
	//E1の単位ベクトルunitE1を作成
	Vector3D unitE1 = get_unit_vector(E1);
	double tmp = dot_product(unitE1,E2);
	
	Vector3D d;
	d.x = M2V0.x + unitE1.x*tmp;
	d.y = M2V0.y + unitE1.y*tmp;
	d.z = M2V0.z + unitE1.z*tmp;
	
	return d;
	
}

// V-1 標高計算メソッド(近似式ver)
void
DoubleDiffractionLossModel::DoCalcElevation(VegeData& vstruct)const
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
	vstruct.elevation = h - Ng;;
}


// V-2 ルート判定メソッド
void
DoubleDiffractionLossModel::DeterminePath(VegeData& vstruct)const
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
DoubleDiffractionLossModel::DetermineArea(VegeData& vstruct)const
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
DoubleDiffractionLossModel::DoCalcVegetationalDistance(VegeData& vstruct_a, VegeData& vstruct_b)const
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
		std::cout << "植生外" << std::endl;
	}
	// 両方のノードが植生の中にいる
	else if((vstruct_a.vegetation == true) && (vstruct_b.vegetation == true)){
		Vdistance = DistanceAtoB;
		std::cout << "共に植生内" << std::endl;
	}
	// 一方のノードのみが植生内にいる
	else{
		std::cout << "片方が植生内" << std::endl;
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
	std::string tf = "vegedismy.txt";
	const char * st = tf.c_str();
	std::ofstream ofs(st,std::ios::out|std::ios::app);
	ofs<<"植生距離"<<Vdistance<<"m"<<"\n";
	ofs.close();
	return Vdistance;
}								  

// 植生減衰を計算するメソッド
double
DoubleDiffractionLossModel::DoCalcVegetationalLoss(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
	
	VegeData vstructA;
	VegeData vstructB;
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
	std::cout << "植生距離" << distance << std::endl; 
		   
	double Aev = 0.0;	// Attenuation in vegetation
	double Am = 0.0;	// maximum attenuation for one terminal within a specific type and depth of vegetation
	double A_gamma = 0.0; // specific attenuation for very short vegetative paths
	
	std::string tf = "myvege.txt";
	const char * st = tf.c_str();
	std::ofstream ofs(st,std::ios::out|std::ios::app);
	ofs<<"aの植生"<<vstructA.vegetation<<"\n";
	ofs<<"bの植生"<<vstructB.vegetation<<"\n";
	ofs.close();
	
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
DoubleDiffractionLossModel::DoCalcRxPower (double txPowerDbm,
                                          Ptr<MobilityModel> a,
                                          Ptr<MobilityModel> b) const{
	// 最終的な損失
	double SumLoss = 0.0;
	// 回折損
	//double DiffLoss = 0.0;
	
	double diffrev = 0.0;
	
	
	// 自由空間損失 & 平面大地
	double TwoRayLoss = 0.0;
	
	// 植生による減衰
	double vAttenuation = 0.0;
	
	//二点間の距離
	double distance = a->GetDistanceFrom (b);
	
	//DiffLoss = DoCalcDiffractionLoss(a, b);		// 回折損
	//std::cout << "回折損" << DiffLoss << " (dB)" << std::endl;
	
	
	diffrev = DoCalcDiffRev(a,b);
	//DiffLoss = DoCalcDiffractionLoss(a,b);  //二重回折損
	TwoRayLoss = DoCalcTwoRayGroundLoss(a,b);	//平面大地による損失
	vAttenuation = DoCalcVegetationalLoss(a,b); //植生による減衰

	
	//std::cout << "回折損1" << DiffLoss << " (dB)" << std::endl;
	//std::cout << "平面大地による損失" << TwoRayLoss << " (dB)" << std::endl;
	//std::cout << "植生による損失" << vAttenuation << " (dB)" << std::endl;
	//std::cout << "回折損2" << diffrev << " (dB)" << std::endl;
	
	SumLoss = TwoRayLoss+vAttenuation+diffrev;
	
/*	std::string tf = "prop.txt";
	const char * st = tf.c_str();
	std::ofstream ofs(st,std::ios::out|std::ios::app);
	ofs<<"平面大地"<<TwoRayLoss<<"\n";
	ofs<<"植生減衰"<<vAttenuation<<"\n";
	ofs<<"回折損失"<<diffrev<<"\n";
	ofs<<"\n";
	ofs.close();
*/	
	std::string tf = "propmy.txt";
	const char * st = tf.c_str();
	std::ofstream ofs(st,std::ios::out|std::ios::app);
	ofs<<TwoRayLoss<<","<<vAttenuation<<","<<diffrev<<","<<SumLoss<<"\n";
	ofs<<"ノード間距離"<<distance<<"m"<<"\n";
	ofs.close();

	
	return txPowerDbm-SumLoss;
   
}

// PropagationLossModelを親クラスにもつ場合は必ず実装するメソッド
int64_t
DoubleDiffractionLossModel::DoAssignStreams (int64_t stream){
  return 0;
}

}

