#include <stdio.h>
#include <string.h>
#include <server/pubsub_svc.h>
#include <common/udp_server.h>
#include <common/common.h>
#include <server/common.h>

// Officially declares it from extern
UdpServer * globalUdpServer;

int main(int argc, char * argv[]) {

  if(argc != 2) {
    fprintf(stderr, "Usage: ./server ipaddress\n");
    exit(0);
  }
 
    globalUdpServer = new UdpServer(10001);
    globalUdpServer->start();

    // Handle heartbeat
    globalUdpServer->setCallback([](const char * ip, int port, const char * data, size_t dataLength) {
        fprintf(stdout, "hearbeat\n");
	if(!strcmp(data, "heartbeat"))
	  globalUdpServer->send(ip, port, data, dataLength);
	else {
	  parse_and_print_list(data);
	}
    });

    //Registration
    register_server(argv[1]);

    get_list(argv[1]);

    startRPC();

    // Deregister
    deregister(argv[1]);
    
    globalUdpServer->stop();
    delete globalUdpServer;
}

void register_server(char *ip) {
  char msg[60];
  snprintf(msg, 60, "Register;RPC;%s;10001;ProgramID;Version", ip);
  globalUdpServer->send("128.101.35.147", 5105, msg, strlen(msg));
  return;
}

void deregister(char *ip) {
  char msg[60];
  snprintf(msg, 60, "Deregister;RPC;%s;10001", ip);
  globalUdpServer->send("128.101.35.147", 5105, msg, strlen(msg));
  return;
}   

void get_list(char *ip) {
  char msg[60];
  snprintf(msg, 60, "GetList;RPC;%s;10001", ip);
  globalUdpServer->send("128.101.35.147", 5105, msg, strlen(msg));
  return;
}

