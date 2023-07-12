
    //根据右手准则找邻居节点,没有邻居就返回address.getzero（）

    Ipv4Address
    PositionTable::BestAngle (Vector previousHop, Vector nodePos)
    {
      Purge ();

      if (m_table.empty ())
      {
        NS_LOG_DEBUG (" Recovery-mode but neighbours table empty; Position: " << nodePos);
        return Ipv4Address::GetZero ();
      } //if table is empty (no neighbours)

      NS_LOG_DEBUG (" Recovery-mode start bestangle " << nodePos);
      double tmpAngle;
      Ipv4Address bestFoundID = Ipv4Address::GetZero ();
      double bestFoundAngle = 360;
      std::map<Ipv4Address, Metrix >::iterator i;

      for (i = m_table.begin (); !(i == m_table.end ()); i++)
      {
        tmpAngle = GetAngle(nodePos, previousHop, i->second.position);
        if (bestFoundAngle > tmpAngle && tmpAngle != 0)
        {
          bestFoundID = i->first;
          bestFoundAngle = tmpAngle;
        }
      }
      if(bestFoundID == Ipv4Address::GetZero ()) //only if the only neighbour is who sent the packet
      {
        bestFoundID = m_table.begin ()->first;
      }
      return bestFoundID;
    }


    //Gives angle between the vector CentrePos-Refpos to the vector CentrePos-node counterclockwise
    double
    PositionTable::GetAngle (Vector centrePos, Vector refPos, Vector node){
      double const PI = 4*atan(1);

      std::complex<double> A = std::complex<double>(centrePos.x,centrePos.y);
      std::complex<double> B = std::complex<double>(node.x,node.y);
      std::complex<double> C = std::complex<double>(refPos.x,refPos.y); //Change B with C if you want angles clockwise

      std::complex<double> AB; //reference edge
      std::complex<double> AC;
      std::complex<double> tmp;
      std::complex<double> tmpCplx;

      std::complex<double> Angle;

      AB = B - A;
      AB = (real(AB)/norm(AB)) + (std::complex<double>(0.0,1.0)*(imag(AB)/norm(AB)));

      AC = C - A;
      AC = (real(AC)/norm(AC)) + (std::complex<double>(0.0,1.0)*(imag(AC)/norm(AC)));

      tmp = log(AC/AB);
      tmpCplx = std::complex<double>(0.0,-1.0);
      Angle = tmp*tmpCplx;
      Angle *= (180/PI);
      if (real(Angle)<0)
      Angle = 360+real(Angle);

      return real(Angle);

    }

