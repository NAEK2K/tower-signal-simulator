#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "simulator.h"


void main() {
  int                 clientSocket;  // client socket id
  struct sockaddr_in  clientAddress; // client address
  int                 status, bytesRcv;
  unsigned char       command = SHUTDOWN; 

  // Contact all the cell towers and ask them to shut down
  // ...
  char                buffer[80];   // stores sent and received data

  

  for(int i = 0; i < 7; i++) { // loop through all the cell towers
    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket < 0) {
      // printf("*** CLIENT ERROR: Could open socket.\n");
      exit(-1);
    }
    
    // Setup address
    memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
    clientAddress.sin_port = htons((unsigned short) SERVER_PORT + i);

    // Connect to server
    status = connect(clientSocket, (struct sockaddr *) &clientAddress, sizeof(clientAddress));
    if (status < 0) {
      // printf("*** CLIENT ERROR: Could not connect.\n");
      exit(-1);
    }

    // Send SHUTDOWN to the cell tower
    buffer[0] = SHUTDOWN;
    buffer[1] = 0;

    send(clientSocket, buffer, strlen(buffer), 0); // send to server

    close(clientSocket);  // Don't forget to close the socket !
    // printf("CLIENT: Shutting down.\n");
  }

  
}

