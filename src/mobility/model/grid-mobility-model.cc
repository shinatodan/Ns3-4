/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* Copyright (c) 2007 INRIA
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
* Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
*/
#include <cmath>
#include "ns3/simulator.h"
#include "ns3/random-variable-stream.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "grid-mobility-model.h"
#include "position-allocator.h"
#include "ns3/uinteger.h"

#include <vector>

namespace ns3 {

  NS_OBJECT_ENSURE_REGISTERED (GridMobilityModel);

  TypeId
  GridMobilityModel::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::GridMobilityModel")
    .SetParent<MobilityModel> ()
    .SetGroupName ("Mobility")
    .AddConstructor<GridMobilityModel> ()
    .AddAttribute ("Speed",
    "A random variable used to pick the speed of a random waypoint model.",
    StringValue ("ns3::UniformRandomVariable[Min=0.3|Max=0.7]"),
    MakePointerAccessor (&GridMobilityModel::m_speed),
    MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("Pause",
    "A random variable used to pick the pause of a random waypoint model.",
    StringValue ("ns3::ConstantRandomVariable[Constant=2.0]"),
    MakePointerAccessor (&GridMobilityModel::m_pause),
    MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("AreaSize",
    "size of the area",
    UintegerValue (490),
    MakeUintegerAccessor (&GridMobilityModel::m_areaSizeXY),
    MakeUintegerChecker<uint32_t> ());

    return tid;
  }

  void
  GridMobilityModel::BeginWalk (void)
  {//次の位置を取得し，その位置に向かって進み始め，将来の一時停止イベントをスケジュールする
    m_helper.Update ();//最後の位置と最後の更新時刻から位置を更新する

    Vector m_current = m_helper.GetCurrentPosition ();//現在の位置Vectorを取得する

    std::vector< std::vector<int> > coordinate;//交差点座標
    //int areaSizeXY=500;
    int divNumXY=5;//分割数
    //int error=(areaSizeXY/divNumXY);
    coordinate.resize(2);
    for(int i=0;i<2;i++)
    coordinate[i].resize(divNumXY+1);//交差点座標の要素数を決める

    switch(m_areaSizeXY){
      case 245: coordinate[0][0]=0;coordinate[1][0]=0;
                coordinate[0][1]=49;coordinate[1][1]=49;
                coordinate[0][2]=98;coordinate[1][2]=98;
                coordinate[0][3]=147;coordinate[1][3]=147;
                coordinate[0][4]=196;coordinate[1][4]=196;
                coordinate[0][5]=245;coordinate[1][5]=245;
                break;
      case 268: coordinate[0][0]=0;coordinate[1][0]=0;
                coordinate[0][1]=53;coordinate[1][1]=53;
                coordinate[0][2]=106;coordinate[1][2]=106;
                coordinate[0][3]=159;coordinate[1][3]=159;
                coordinate[0][4]=212;coordinate[1][4]=212;
                coordinate[0][5]=268;coordinate[1][5]=268;
                break;
      case 300: coordinate[0][0]=0;coordinate[1][0]=0;
                coordinate[0][1]=60;coordinate[1][1]=60;
                coordinate[0][2]=120;coordinate[1][2]=120;
                coordinate[0][3]=180;coordinate[1][3]=180;
                coordinate[0][4]=240;coordinate[1][4]=240;
                coordinate[0][5]=300;coordinate[1][5]=300;
                break;
      case 346: coordinate[0][0]=0;coordinate[1][0]=0;
                coordinate[0][1]=69;coordinate[1][1]=69;
                coordinate[0][2]=138;coordinate[1][2]=138;
                coordinate[0][3]=207;coordinate[1][3]=207;
                coordinate[0][4]=276;coordinate[1][4]=276;
                coordinate[0][5]=346;coordinate[1][5]=346;
                break;
      case 424: coordinate[0][0]=0;coordinate[1][0]=0;
                coordinate[0][1]=84;coordinate[1][1]=84;
                coordinate[0][2]=168;coordinate[1][2]=168;
                coordinate[0][3]=252;coordinate[1][3]=252;
                coordinate[0][4]=336;coordinate[1][4]=336;
                coordinate[0][5]=424;coordinate[1][5]=424;
                break;
      case 600: coordinate[0][0]=0;coordinate[1][0]=0;
                coordinate[0][1]=120;coordinate[1][1]=120;
                coordinate[0][2]=240;coordinate[1][2]=240;
                coordinate[0][3]=360;coordinate[1][3]=360;
                coordinate[0][4]=480;coordinate[1][4]=480;
                coordinate[0][5]=600;coordinate[1][5]=600;
                break;
    }

int error=5;

int elementX=0,elementY=0;//要素番号
Vector destination;//次の宛先
for(int i=0;i<=divNumXY;i++){
  if(-error<m_current.x-coordinate[0][i] && m_current.x-coordinate[0][i]<error )
  elementX=i;
  if(-error<m_current.y-coordinate[1][i] && m_current.y-coordinate[1][i]<error)
  elementY=i;
}

int seed=time(NULL);
srand(seed);
double dx,dy;
m_flag=4;

if(elementX==0 && elementY==0 ){//(0.0)
  if(m_flag==2){
    destination.x=coordinate[0][0];
    destination.y=coordinate[1][1];
    dx = 0;
    dy = (destination.y - m_current.y);
    m_flag=1;
  }else if(m_flag==3){
    destination.x=coordinate[0][1];
    destination.y=coordinate[1][0];
    dx = (destination.x - m_current.x);
    dy = 0;
    m_flag=0;
  }else{
    switch(rand()%2){
      case 0:destination.x=coordinate[0][1];
      destination.y=coordinate[1][0];
      dx = (destination.x - m_current.x);
      dy = 0;
      m_flag=0;
      break;
      case 1:destination.x=coordinate[0][0];
      destination.y=coordinate[1][1];
      dx = 0;
      dy = (destination.y - m_current.y);
      m_flag=1;
      break;
    }
  }
}else if(elementX==0 && elementY==divNumXY ){//(0,5)
    if(m_flag==1){
      destination.x=coordinate[0][1];
      destination.y=coordinate[1][divNumXY];
      dx = (destination.x - m_current.x);
      dy = 0;
      m_flag=0;
    }else if(m_flag==2){
      destination.x=coordinate[0][0];
      destination.y=coordinate[1][divNumXY-1];
      dx = 0;
      dy = (destination.y - m_current.y);
      m_flag=3;
    }else{
      switch(rand()%2){
        case 0:destination.x=coordinate[0][0];
        destination.y=coordinate[1][divNumXY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
        case 1:destination.x=coordinate[0][1];
        destination.y=coordinate[1][divNumXY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
      }
    }
  }else if(elementX==divNumXY && elementY==divNumXY ){//(5,5)
    if(m_flag==0){
      destination.x=coordinate[0][divNumXY];
      destination.y=coordinate[1][divNumXY-1];
      dx = 0;
      dy = (destination.y - m_current.y);
      m_flag=3;
    }else if(m_flag==1){
      destination.x=coordinate[0][divNumXY-1];
      destination.y=coordinate[1][divNumXY];
      dx = (destination.x - m_current.x);
      dy = 0;
      m_flag=2;
    }else{
      switch(rand()%2){
        case 0:destination.x=coordinate[0][divNumXY-1];
        destination.y=coordinate[1][divNumXY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 1:destination.x=coordinate[0][divNumXY];
        destination.y=coordinate[1][divNumXY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
      }
    }
  }else if(elementX==divNumXY && elementY==0 ){//(5,0)
    if(m_flag==0){
      destination.x=coordinate[0][divNumXY];
      destination.y=coordinate[1][1];
      dx = 0;
      dy = (destination.y - m_current.y);
      m_flag=1;
    }else if(m_flag==3){
      destination.x=coordinate[0][divNumXY-1];
      destination.y=coordinate[1][0];
      dx = (destination.x - m_current.x);
      dy = 0;
      m_flag=2;
    }else{
      switch(rand()%2){
        case 0:destination.x=coordinate[0][divNumXY-1];
        destination.y=coordinate[1][0];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 1:destination.x=coordinate[0][divNumXY];
        destination.y=coordinate[1][1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
      }
    }
  }else if(elementY==0 && elementX!=0 && elementX!=divNumXY){//頂点を除くY=0
    if(m_flag==0){
      switch(rand()%2){
        case 0:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
        case 1:destination.x=coordinate[0][elementX+1];
        destination.y=coordinate[1][0];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
      }
    }else if(m_flag==3){
      switch(rand()%2){
        case 0:destination.x=coordinate[0][elementX-1];
        destination.y=coordinate[1][0];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 1:destination.x=coordinate[0][elementX+1];
        destination.y=coordinate[1][0];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
      }
    }else if(m_flag==2){
      switch(rand()%2){
        case 0:destination.x=coordinate[0][elementX-1];
        destination.y=coordinate[1][0];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 1:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
      }
    }else{
      switch(rand()%3){
        case 0:destination.x=coordinate[0][elementX-1];
        destination.y=coordinate[1][0];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 1:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
        case 2:destination.x=coordinate[0][elementX+1];
        destination.y=coordinate[1][0];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
      }
    }
  }else if(elementX==0 && elementY!=0 && elementY!=divNumXY){//頂点を除くX=0
    if(m_flag==1){
      switch(rand()%2){
        case 0:destination.x=coordinate[0][elementX+1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
        case 1:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY+1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
      }
    }else if(m_flag==2){
      switch(rand()%2){
        case 0:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
        case 1:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY+1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
      }
    }else if(m_flag==3){
      switch(rand()%2){
        case 0:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
        case 1:destination.x=coordinate[0][elementX+1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
      }
    }else{
      switch(rand()%3){
        case 0:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
        case 1:destination.x=coordinate[0][elementX+1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
        case 2:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY+1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
      }
    }
  }else if(elementY==divNumXY && elementX!=0 && elementX!=divNumXY){//頂点を除くY=5
    if(m_flag==0){
      switch(rand()%2){
        case 0:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
        case 1:destination.x=coordinate[0][elementX+1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
      }
    }else if(m_flag==1){
      switch(rand()%2){
        case 0:destination.x=coordinate[0][elementX-1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 1:destination.x=coordinate[0][elementX+1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
      }
    }else if(m_flag==2){
      switch(rand()%2){
        case 0:destination.x=coordinate[0][elementX-1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 1:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
      }
    }else{
      switch(rand()%3){
        case 0:destination.x=coordinate[0][elementX-1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 1:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
        case 2:destination.x=coordinate[0][elementX+1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
      }
    }
  }else if(elementX==divNumXY && elementY!=0 && elementY!=divNumXY){//頂点を除くX=5
    if(m_flag==1){
      switch(rand()%2){
        case 0:destination.x=coordinate[0][elementX-1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 1:destination.x=coordinate[0][elementX];
        destination.y=coordinate[0][elementY+1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
      }
    }else if(m_flag==0){
      switch(rand()%2){
        case 0:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
        case 1:destination.x=coordinate[0][elementX];
        destination.y=coordinate[0][elementY+1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
      }
    }else if(m_flag==3){
      switch(rand()%2){
        case 0:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
        case 1:destination.x=coordinate[0][elementX-1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
      }
    }else{
      switch(rand()%3){
        case 0:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
        case 1:destination.x=coordinate[0][elementX-1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 2:destination.x=coordinate[0][elementX];
        destination.y=coordinate[0][elementY+1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
      }
    }
  }else if(elementX!=0 && elementX!=divNumXY && elementY!=0 && elementY!=divNumXY){//頂点を除くX=5
    if(m_flag==0){
      switch(rand()%3){
        case 0:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY+1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
        case 1:destination.x=coordinate[0][elementX+1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
        case 2:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
      }
    }else if(m_flag==3){
      switch(rand()%3){
        case 0:destination.x=coordinate[0][elementX-1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 1:destination.x=coordinate[0][elementX+1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
        case 2:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
      }
    }else if(m_flag==2){
      switch(rand()%3){
        case 0:destination.x=coordinate[0][elementX-1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 1:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY+1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
        case 2:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
      }
    }else if(m_flag==1){
      switch(rand()%3){
        case 0:destination.x=coordinate[0][elementX-1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 1:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY+1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
        case 2:destination.x=coordinate[0][elementX+1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
      }
    }else{
      switch(rand()%4){
        case 0:destination.x=coordinate[0][elementX-1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=2;
        break;
        case 1:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY+1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=1;
        break;
        case 2:destination.x=coordinate[0][elementX+1];
        destination.y=coordinate[1][elementY];
        dx = (destination.x - m_current.x);
        dy = 0;
        m_flag=0;
        break;
        case 3:destination.x=coordinate[0][elementX];
        destination.y=coordinate[1][elementY-1];
        dx = 0;
        dy = (destination.y - m_current.y);
        m_flag=3;
        break;
      }
    }
  }
  double speed = m_speed->GetValue ();//速度を返す
  //double dx = (destination.x - m_current.x);
  //double dy = (destination.y - m_current.y);
  dx = (destination.x - m_current.x);
  dy = (destination.y - m_current.y);
  double dz = 0;
  double k = speed / std::sqrt (dx*dx + dy*dy + dz*dz);

  m_helper.SetVelocity (Vector (k*dx, k*dy, k*dz));//新しい速度ベクトルを設定する
  m_helper.Unpause ();//現在の速度で現在の位置から移動を開始する
  Time travelDelay = Seconds (CalculateDistance (destination, m_current) / speed);
  //距離を速度で割って，現在の位置から宛先までにかかる時間を計算する
  m_event.Cancel ();
  m_event = Simulator::Schedule (travelDelay,
    &GridMobilityModel::DoInitializePrivate, this);//
    NotifyCourseChange ();
  }

  void
  GridMobilityModel::DoInitialize (void)
  {//Initialize()によって一度呼び出される
    DoInitializePrivate ();
    MobilityModel::DoInitialize ();
  }

  void
  GridMobilityModel::DoInitializePrivate (void)
  {//現在の一時停止イベントを開始し，将来のイベントをスケジュールする
    m_helper.Pause ();//現在の位置で移動性を停止する
    m_helper.Update ();//最後の位置と最後の更新時刻から位置を更新する
    m_helper.Pause ();//現在の位置で移動性を停止する
    Time pause = Seconds (m_pause->GetValue ());//一時停止時間を取得す
    
    if(Now() <= Seconds(10.0)){
    //std::cout << Now() << std::endl;
    m_event = Simulator::Schedule (Seconds(10), &GridMobilityModel::BeginWalk, this);
}
    else{
	//std::cout << "over 10 seconds" << std::endl;
    m_event = Simulator::Schedule (pause, &GridMobilityModel::BeginWalk, this);
    NotifyCourseChange ();
}
  }

  Vector
  GridMobilityModel::DoGetPosition (void) const
  {//現在の位置
    m_helper.Update ();
    return m_helper.GetCurrentPosition ();
  }
  void
  GridMobilityModel::DoSetPosition (const Vector &position)
  {//位置を設定する
    m_helper.SetPosition (position);
    Simulator::Remove (m_event);
    m_event = Simulator::ScheduleNow (&GridMobilityModel::DoInitializePrivate, this);
  }
  Vector
  GridMobilityModel::DoGetVelocity (void) const
  {//現在の速度
    return m_helper.GetVelocity ();
  }
  /*int64_t
  GridMobilityModel::DoAssignStreams (int64_t stream)
  {
  int64_t positionStreamsAllocated;
  m_speed->SetStream (stream);
  m_pause->SetStream (stream + 1);
  positionStreamsAllocated = m_position->AssignStreams (stream + 2);
  return (2 + positionStreamsAllocated);
}*/


} // namespace ns3
