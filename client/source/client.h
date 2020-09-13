
#ifndef WADGNSS_CLIENT_CLIENT_H
#define WADGNSS_CLIENT_CLIENT_H
#pragma once
#include "sys/time.h"
#include <arpa/inet.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fstream>
#include "ubxcfg.h"
//#define SERVER_IP "192.168.1.118"
//#define SERVER_PORT 3636
#define RTCM_BUFSIZE   1200
#define POS_BUFSIZE    10000
#define READ_PERIOD    600 // read period of rover position in seconds
#define RE_WGS84       6378137.0      /* earth semimajor axis (WGS84) (m) */
#define FE_WGS84       (1.0/298.257223563) /* earth flattening (WGS84) */
#define PI             3.141592653589793
#define LOOP_WAIT      1000 //0.001s
bool opencom_port(const std::string &com_port,int &fd){
  char const *rover = com_port.c_str();
  fd = open(rover,O_RDWR|O_NONBLOCK);
  return fd != -1;
}

uint64_t get_sec(timeval tv) {
  return ((double)tv.tv_sec) + ((double)tv.tv_usec) / 1000000.;
}

// Get local time string
std::string get_time() {
  time_t rawtime;
  struct tm *ltm;
  time(&rawtime);
  ltm = localtime(&rawtime);
  char buf[28];
  sprintf(buf, "[%04d-%02d-%02d %02d:%02d:%02d]: ", ltm->tm_year + 1900,
          ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min,
          ltm->tm_sec);
  return std::string(buf);
}

double D2R(const double D)
{
  return D*PI/180;
}
/* transform geodetic to ecef position -----------------------------------------
* transform geodetic position to ecef position
* args   : double *pos      I   geodetic position {lat,lon,h} (rad,m)
*          double *r        O   ecef position {x,y,z} (m)
* return : none
* notes  : WGS84, ellipsoidal height
*-----------------------------------------------------------------------------*/
void pos2ecef(const double *pos, double *r)
{
  double sinp=sin(pos[0]),cosp=cos(pos[0]),sinl=sin(pos[1]),cosl=cos(pos[1]);
  double e2=FE_WGS84*(2.0-FE_WGS84),v=RE_WGS84/sqrt(1.0-e2*sinp*sinp);

  r[0]=(v+pos[2])*cosp*cosl;
  r[1]=(v+pos[2])*cosp*sinl;
  r[2]=(v*(1.0-e2)+pos[2])*sinp;
}

std::string to_string_with_precision(const double val,int precison)
{
  std::ostringstream out;
  out << std::fixed<<std::setprecision(precison) << val;
  return out.str();
}

bool read_rover_pos(int fd,std::string &pos_ecef,std::ofstream &logto){
  char pos_buff[POS_BUFSIZE]={0};
  int ret = read(fd,pos_buff,POS_BUFSIZE);
  if (ret == -1){
    if(!(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)){
      std::cerr<<"Read u-blox failed"<<std::endl;
      return false;
    }
    else return false;
  }
  else{
    std::string pos_str = pos_buff;
    std::string::size_type idx=std::string::npos;
    if (pos_str.size()>10) idx = pos_str.find("PUBX,00");
    if (idx!=std::string::npos){
      logto<<pos_str<<std::endl;
      std::string Lat_d_str = pos_str.substr(idx+18,2);
      std::string Lat_m_str = pos_str.substr(idx+20,8);
      std::string Lat_sig_str = pos_str.substr(idx+29,1);
      std::string Lon_d_str = pos_str.substr(idx+31,3);
      std::string Lon_m_str = pos_str.substr(idx+34,8);
      std::string Lon_sig_str = pos_str.substr(idx+43,1);
      std::string Alt_str = pos_str.substr(idx+45,7);
      int Lat_sig=0,Lon_sig=0;
      if (Lat_sig_str=="N") Lat_sig = 1;
      else if(Lat_sig_str=="S") Lat_sig = -1;
      if (Lon_sig_str=="E") Lon_sig = 1;
      else if (Lon_sig_str=="W") Lon_sig = -1;
      if (stoi(Lat_d_str)==0&&stoi(Lon_d_str)==0) {
        return false;
      }
      double Lat = D2R(Lat_sig*(stod(Lat_d_str) + stod(Lat_m_str)/60)+0.1);
      double Lon = D2R(Lon_sig*(stod(Lon_d_str) + stod(Lon_m_str)/60)-0.1);
      double Alt = stod(Alt_str);
      std::cout<<"Get pos info from rover: "
              <<Lat<<" "<<Lon<<" "<<Alt
              <<std::endl;
      double lla_pos[3] = {Lat,Lon,Alt};
      double ecef_p[3];
      pos2ecef(lla_pos, ecef_p);
      pos_ecef = "$POSECEF "+ to_string_with_precision(ecef_p[0],4)
                 +" "+to_string_with_precision(ecef_p[1],4)
                 +" "+to_string_with_precision(ecef_p[2],4)
                 +" "+"\r\n";
      std::cout<<pos_ecef<<std::endl;
      return true;
    }
    else return false;
  }
}
#endif //WADGNSS_CLIENT_CLIENT_H
