/*  tracker.h
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

#ifndef _TRACKERDEVICE_H
#define _TRACKERDEVICE_H

#include <libplayercore/playercore.h>

#include <string>

using namespace std;

typedef struct player_tracker_data{

  player_position2d_data_t position2d;

}__attribute__((packed)) player_tracker_data_t;
     
class Tracker : public Driver
{

 public:
   
  Tracker(ConfigFile* cf, int section);
  ~Tracker(void);
  virtual int Setup();
  virtual int Shutdown();
  virtual int Subscribe(player_devaddr_t id);
  virtual int Unsubscribe(player_devaddr_t id); 
  virtual void Main(); 
  virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, 
		     void * data);
  
 private:

  player_tracker_data_t tracker_data;
 
  player_devaddr_t position2d_id;
 
  int sk_wait;
  int position2d_subscriptions;
 
  int global_socket;
  string global_server;
  string global_port;

  int port;
  int x_res;
  int y_res;
  double x_conv;
  double y_conv;



  //methods
  
  void update_everything(player_tracker_data_t* d);
  int g_server(string global_server,string global_port);
  
  void close_servers();
  player_position2d_data_t readData(int sk, int wait);
  int HandleConfig(QueuePointer & resp_queue, player_msghdr * hdr, 
		   void * data);
 };

#endif
