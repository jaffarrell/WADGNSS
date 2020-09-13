#include "client.h"

int main(int argc, char* argv[])
//int main()
{
  // input check
  if(argc < 5){
    // UBX_TYPE: 1 for M8P, 2 for ZED-F9P
    std::cerr<<"eg: sudo ./client.o COM_PORT "
               "UBX_TYPE Sever_IP Server_port"<<std::endl;
    exit(EXIT_FAILURE);
  }
//  std::string com_port_name;
//  std::cout<<"Input COM PORT (for example: ttyACM0): "<<std::endl;
//  std::cin>>com_port_name;
//  std::string com_port = "/dev/" + com_port_name;
//  std::string UBX_TYPE_str;
//  std::cout<<"Input ublox type (1 for M8P, 2 for ZED-F9P): "<<std::endl;
//  std::cin>>UBX_TYPE_str;
//  int UBX_TYPE = stoi(UBX_TYPE_str);
//  std::string SERVER_IP_str;
//  std::cout<<"Input server IP: "<<std::endl;
//  std::cin>>SERVER_IP_str;
//  const char* SERVER_IP = SERVER_IP_str.c_str();
//  std::cout<<"Input server port: "<<std::endl;
//  std::string SERVER_PORT_str;
//  std::cin>>SERVER_PORT_str;
//  int SERVER_PORT = stoi(SERVER_PORT_str);
  const std::string com_port = argv[1];
  const std::string UBX_TYPE_ss = argv[2];
  int UBX_TYPE = stoi(UBX_TYPE_ss);
  const char* SERVER_IP = argv[3];
  const std::string SERVER_PORT_ss = argv[4];
  int SERVER_PORT = stoi(SERVER_PORT_ss);
  // 1. Build connection with rover
  int rover_fd;
  if (!opencom_port(com_port, rover_fd))
  {
    std::cerr<<"COM port open error: %s"<<com_port
             <<", pls check com number or permission"<<std::endl;
    exit(EXIT_FAILURE);
  }
  if (!init_ublox(rover_fd,UBX_TYPE)){
    std::cerr<<"Initialize u-blox failed. please check your input infor"
              <<std::endl;
    close(rover_fd);
    exit(EXIT_FAILURE);
  }
  // 2.create a socket
  int client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (client_fd==-1) {
    std::cerr<<"socket start error: "<<strerror(errno)<<std::endl;
  }
  // 3.connect server
  struct sockaddr_in server_addr{};
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  //server_addr.sin_addr.s_addr = htonl();
  inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr);

  // 4.connect
  int check = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if (check==-1) {
    std::cerr<<"connection start error: "<<strerror(errno)<<std::endl;
  }

//  int rc = setsockopt(client_thread->information.socket_descriptor, IPPROTO_TCP,
//                  TCP_NODELAY, (char *)&ON,
//                  sizeof(ON));  // configure client thread socket to send
//  // without internally buffering send requests
//  // so every send call will send a packet

  // Keep read position from the rover until success
  std::string pos_ecef;
  std::ofstream logto("../ubxlog.log");
  int count = 1;
  while (true){
    if (read_rover_pos(rover_fd, pos_ecef, logto)) {
      char pos_buff[RTCM_BUFSIZE];
      strcpy(pos_buff,pos_ecef.c_str());
      int send_ret = send(client_fd, pos_buff, RTCM_BUFSIZE,0);
      std::cout<<"send code: "<<send_ret<<std::endl;
      break;
    }
    usleep(UBX_wait);
    count++;
    if (count>200){
      std::cerr<<"Can't get position info from rover, "
                 "please check if the NMEA protocol PUBX 00 massage enabled."<<std::endl;
      logto.close();
      close(rover_fd);
      close(client_fd);
      exit(EXIT_FAILURE);
    }
  }
  // 5. Set timer for reading position from rover
  struct timezone tz({0, 0});
  timeval tv{};
  gettimeofday(&tv, &tz);
  uint64_t read_t = get_sec(tv);
  while(true){
    gettimeofday(&tv, &tz);
    uint64_t now = get_sec(tv);
    // Send user position by very period of time
    if (now / READ_PERIOD != read_t / READ_PERIOD){
      count = 0;
      while (count < 5) {
        if (read_rover_pos(rover_fd,pos_ecef,logto)){
          read_t = now;
          pos_ecef.resize(RTCM_BUFSIZE);
          char pos_buf[RTCM_BUFSIZE]={0};
          strcpy(pos_buf,pos_ecef.c_str());
          int send_ret = send(client_fd, pos_buf, RTCM_BUFSIZE,0);
          if (send_ret == -1){
            logto<<"Send error: "<<strerror(errno)<<std::endl;
            logto.close();
            exit(EXIT_FAILURE);
          } else if (send_ret == 0) {
            std::cout<<"server has closed connection."<<std::endl;
            logto.close();
            exit(EXIT_FAILURE);
          } else if (send_ret > 0) {
            break;
          }
        } else {
          count++;
          continue;
        }
      }
    } else {
      char pos_buff[POS_BUFSIZE]={0};
      int ret = read(rover_fd,pos_buff,POS_BUFSIZE);
      if (ret == -1){
        if(!(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)){
          std::cerr<<"Read u-blox failed"<<std::endl;
        }
      } else {
        std::string pos_str = pos_buff;
        std::string::size_type idxP=std::string::npos;
        std::string::size_type idxG=std::string::npos;
        if (pos_str.size()>10) {
          idxP = pos_str.find("PUBX,00");
          idxG = pos_str.find("GGA");
        }
        if ((idxP!=std::string::npos) | (idxG!=std::string::npos)) {
          logto << pos_buff << std::endl;
        }
      }
    }
    char RTCM_Buffer[RTCM_BUFSIZE]={0};
    int read_ret = recv(client_fd, RTCM_Buffer, sizeof(RTCM_Buffer),MSG_DONTWAIT);
    if(read_ret < 0)
    {
      if(!(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR))
      {
        std::cerr << "unexpected error: " << strerror(errno) << std::endl;
        logto.close();
        exit(EXIT_FAILURE);
      }
    }
    else if(read_ret == 0)
    {
      std::cout<<"server has closed connection."<<std::endl;
      break;
    }
    else if(read_ret > 0)
    {
      if (write(rover_fd,RTCM_Buffer,sizeof(RTCM_Buffer))==-1)
        std::cerr<<"Send to ublox failed"<<std::endl;
    }
    usleep(LOOP_WAIT);
  }
  logto.close();
  close(rover_fd);
  close(client_fd);
  return 0;
}

