/*  tracker.cc
 *  Motion Tracker Player Plugin
 *  Copyright (c) - Joseph Baxter {joseph.lee.baxter@gmail.com}
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
#if HAVE_CONFIG_H
  #include <config.h>
#endif
*/
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

#include <sys/socket.h>
#include <netdb.h>

#include <libplayercore/playercore.h>
#include <libplayerxdr/playerxdr.h>

#include "tracker.h"

using namespace std;

#define DEFAULT_GLOBAL_SERVER "haldir"
#define DEFAULT_GLOBAL_PORT "9000"

#define TCP_DEFAULT_PORT 6665

//  config
Driver* Tracker_Init(ConfigFile* cf, int section)
{
  return((Driver*)(new Tracker(cf, section)));
}

//  add driver
void Tracker_Register(DriverTable* table)
{
  table->AddDriver("tracker", Tracker_Init);
}
   

//  construtor
Tracker::Tracker(ConfigFile* cf, int section) : Driver(cf, section,true,PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{

  memset(&this->position2d_id, 0, sizeof(player_devaddr_t));
 
  this->position2d_subscriptions = 1;

  //  position2d
  if(cf->ReadDeviceAddr(&(this->position2d_id), section, "provides",
                      PLAYER_POSITION2D_CODE, -1, "tracker") == 0)
  {
    if(this->AddInterface(this->position2d_id) != 0)
    {
      this->SetError(-1);    
      return;
    }
    // Stop position messages in the queue from being overwritten
    this->InQueue->AddReplaceRule (this->position2d_id, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_POS, false);
  }else
    {
      PLAYER_WARN("Position2d interface not created for miabot driver");
    }


  this->global_server = cf->ReadString(section, "global_server", DEFAULT_GLOBAL_SERVER);
  this->global_port = cf->ReadString(section, "global_port", DEFAULT_GLOBAL_PORT);
  this->sk_wait = cf->ReadInt(section, "socket_wait", 0);
  this->x_conv = cf->ReadFloat(section, "x_conv", 0);
  this->y_conv = cf->ReadFloat(section, "y_conv", 0);
  this->x_res = cf->ReadInt(section, "x_res", 0);
  this->y_res = cf->ReadInt(section, "y_res", 0);
  
}
//  setup 
int Tracker::Setup(){

  /* now spawn reading thread */
  this->StartThread();
  
  return (0);
}
  
//  shutdown
int Tracker::Shutdown(){
  //disconnect
  close_servers();
  this->StopThread();
  return (0);
}

int Tracker::Subscribe(player_devaddr_t id){
  int setupResult;

  // do the subscription
  if((setupResult = Driver::Subscribe(id)) == 0)
    {
      // also increment the appropriate subscription counter

      if(Device::MatchDeviceAddress(id, this->position2d_id))
	this->position2d_subscriptions++;
     
    
    }

  return(setupResult);
  
}

int Tracker::Unsubscribe(player_devaddr_t id){
  int shutdownResult;
  
  // do the unsubscription
  if((shutdownResult = Driver::Unsubscribe(id)) == 0)
    {
      // also decrement the appropriate subscription counter
      if(Device::MatchDeviceAddress(id, this->position2d_id)){
	this->position2d_subscriptions--;
	assert(this->position2d_subscriptions >= 0);
      
      }
    }
  
  return(shutdownResult);
}


// main
void Tracker::Main(){
 
   int last_position2d_subscrcount = 0; 
  while(1){
    if(!last_position2d_subscrcount && this->position2d_subscriptions){
      //connect 
  
          
      global_socket = g_server(global_server,global_port);
  

    }else if(last_position2d_subscrcount && !(this->position2d_subscriptions)){
      //disconnect
      close_servers();
       
    }
    
    last_position2d_subscrcount = this->position2d_subscriptions;

   
    
    // handle pending messages
    if(!this->InQueue->Empty()){
      ProcessMessages();
    }
       
    // Get data from servers 
      
    memset(&(this->tracker_data),0,sizeof(player_tracker_data_t));
  
    update_everything(&tracker_data);
   
    pthread_testcancel();
    
    this->Publish(this->position2d_id,
		  PLAYER_MSGTYPE_DATA,
		  PLAYER_POSITION2D_DATA_STATE,
		  (void*)&(this->tracker_data.position2d),
		  sizeof(player_position2d_data_t),
		  NULL);
   
    pthread_testcancel();   
   
  }
  pthread_exit(NULL);    
   
}



void Tracker::update_everything(player_tracker_data_t* d){
 
  //parse blobserver data into position2d data

   
  d->position2d = readData(global_socket,sk_wait);

}

player_position2d_data_t Tracker::readData(int sk, int wait){ 
 
  int j; // based on client port number
  j = port - TCP_DEFAULT_PORT;
 
  string command;
  stringstream scommand;
  scommand << j;
  command = "[0,";
  command.append(scommand.str());
  command.append("]\n");

  if(send(sk,command.c_str(),command.length(),0) < 0){
    close(sk);
    printf("\nError: cannot send\n");
  } 

  scommand.str("");

  string data = "";

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = wait; //use set in config file
  
  char buffer[256];
  ostringstream sbuf; 
  
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(sk, &readfds); 
  
  // is there something to recv
  
  if(FD_ISSET(sk, &readfds)){
    
    
    while(select(sk+1, &readfds, NULL, NULL, &tv)){
      sbuf.clear();
      memset(buffer,0x0,sizeof(buffer)); 
      
      if(recv(sk,buffer,sizeof(buffer),0)<0){

	
      }else{
	
	sbuf << buffer;
	
      }
      
    }	
    
    data = sbuf.str();

  }
  
  
  string info;
  ostringstream os;
  int start, finish;
  
  os << "[" << 0 << ",";
  start = data.rfind(os.str());
  finish = data.find(']',start);
    
  if(start!=string::npos || finish!=string::npos){
    info = data.substr(start,finish-start);
   
  }
  
  os.str("");
  
  double tempx, tempy, tempa;
  int t;
  t = info.find(',');
  tempx = atoi(info.substr(t+1,info.find(',',t+1)).c_str());
  
  t = info.find(',',t+1);
  tempy = atoi(info.substr(t+1,info.find(',',t+1)).c_str());
  
  t = info.find(',',t+1);
  tempa = atoi(info.substr(t+1,info.find(',',t+1)).c_str());
    
  double xres, yres;
  double xconv, yconv;
  xres = this->x_res;
  yres = this->y_res;
  xconv = this->x_conv;
  yconv = this->y_conv;

  player_position2d_data_t temp;
  temp.pos.px = ((tempx/xres)*xconv) - (xconv/2); //conversion from px to other measurement
  temp.pos.py = -(((tempy/yres)*yconv) - (yconv/2)); //conversion from px to other measurement
  
 
  temp.pos.pa = -(tempa-90); // conversion from tracking system orientation to player orientation
  // if(temp.pos.pa < 0){
  //  temp.pos.pa = 360+temp.pos.pa;
  //}
  temp.pos.pa = DTOR(temp.pos.pa); //deg 2 rad

  temp.vel.px = 0;
  temp.vel.py = 0;
  temp.vel.pa = 0;
  temp.stall = 0;
  return temp;
}


int Tracker::g_server(string local_server,string local_port){
  struct sockaddr_in addr;
  struct hostent *hostInfo;
  int sock;
  unsigned short int serverPort;

  hostInfo = gethostbyname(local_server.c_str());
  if (hostInfo == NULL) {
    printf("problem interpreting host: %s\n", local_server.c_str());
    return -1;
  }

  
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    printf("\nsocket error\n");
        
    return -1;
  }
  
   
  addr.sin_family = hostInfo->h_addrtype;
  memcpy((char *) &addr.sin_addr.s_addr,hostInfo->h_addr_list[0], hostInfo->h_length);
  serverPort = atoi(local_port.c_str());
  addr.sin_port = htons(serverPort);

  if(connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    
    printf("\nconnection error\n");
    close(sock);
    return -1;
  }

 
  return sock;
}


void Tracker::close_servers(){
  close(global_socket);
  
}

// process messages
int Tracker::ProcessMessage(QueuePointer & resp_queue,player_msghdr * hdr,void * data)
{
  // Look for configuration requests
  if(hdr->type == PLAYER_MSGTYPE_REQ){
    this->port = hdr->addr.robot;
    
    return(this->HandleConfig(resp_queue,hdr,data));
  }else{
    return(-1);
  }
}

// Config handler functions

int Tracker::HandleConfig(QueuePointer & resp_queue, player_msghdr * hdr,void * data)
{



 if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_GET_GEOM,this->position2d_id)){
    if(hdr->size != 0)
      {
	PLAYER_WARN("Arg get robot (compass) geom is wrong size; ignoring");
	return(-1);
      }
     
    player_position2d_geom_t tracker_geom;
    tracker_geom.pose.px = 0;
    tracker_geom.pose.py = 0;
    tracker_geom.pose.pyaw = 0;
      
    //tracker_geom.size.sl = 0;
    //tracker_geom.size.sw = 0;
    
    Publish(this->position2d_id, resp_queue, PLAYER_MSGTYPE_RESP_ACK, hdr->subtype, &tracker_geom,sizeof(tracker_geom));
    return 0;

  }else{
    PLAYER_WARN("unknown config request to miabot driver");
    return(-1);
  }
}

Tracker::~Tracker (void)
{
  player_position2d_data_t_cleanup(&tracker_data.position2d);
   

 
}

// Extra stuff for building a shared object.x

/* need the extern to avoid C++ name-mangling  */
extern "C" {
  int player_driver_init(DriverTable* table)
  {
    puts("tracker driver initializing");
    Tracker_Register(table);
    puts("tracker driver done");
    return(0);
  }
}

