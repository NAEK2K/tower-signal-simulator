#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "simulator.h"


// GPS Data for this client as well as the connected tower ID
short  x;
short  y;
short  direction;
char  connectionID;
char  connectedTowerID;

void updateCoords() // updates the coords
{
  x += VEHICLE_SPEED * cos(direction);
  y += VEHICLE_SPEED * sin(direction);
}

void updateDirection(int random) // update direction, with 1/3 chance of each option
{
  if (random == 0)
  {
    direction += VEHICLE_TURN_ANGLE;
  }
  else if (random == 1)
  {
    direction -= VEHICLE_TURN_ANGLE;
  }
  else if (random == 2)
  {
    direction = direction;
  }
}


int main(int argc, char * argv[]) {
  // server variables
  int clientSocket;                 // client socket id
  struct sockaddr_in clientAddress; // client address
  int status, bytesRcv;
  short clientResponse[50];
  short buffer[50];
  // Set up the random seed
  srand(time(NULL));

  // Get the starting coordinate and direction from the command line arguments
  x = atoi(argv[1]);
  y = atoi(argv[2]);
  direction = atoi(argv[3]);

  // To start, this vehicle is not connected to any cell towers
  connectionID = -1;
  connectedTowerID = -1;

  // Go into an infinite loop to keep sending updates to cell towers
  while(1) {
    usleep(50000);  // A delay to slow things down a little
    updateCoords(); // update our variables
    updateDirection(rand() % 3);
    if(connectionID == -1) { // if we arent connected, try and connect
      for(int i = 0; i < 7; i++) { // loop through the towers to find one in range
        clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (clientSocket < 0)
        {
          // printf("*** CLIENT ERROR: Could open socket.\n");
          exit(-1);
        }

        // Setup address
        memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddress.sin_family = AF_INET;
        clientAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
        clientAddress.sin_port = htons((unsigned short)SERVER_PORT + i);

        // Connect to server
        status = connect(clientSocket, (struct sockaddr *)&clientAddress, sizeof(clientAddress));
        if (status < 0)
        {
          // printf("*** CLIENT ERROR: Could not connect.\n");
          exit(-1);
        }

        //setup response
        clientResponse[0] = CONNECT;
        clientResponse[1] = x;
        clientResponse[2] = y;

        // printf("CLIENT: Sending CONNECT response.\n");
        send(clientSocket, clientResponse, sizeof(clientResponse), 0);

        bytesRcv = recv(clientSocket, buffer, 80, 0);

        if(buffer[0] == YES) {
          // printf("CLIENT: Recevied a YES response. 103 %d %d\n", buffer[1], buffer[2]);
          connectionID = buffer[1];
          connectedTowerID = buffer[2];
          close(clientSocket);
          break;
        }
        // printf("CLIENT: Closing Socket 110");
        close(clientSocket);
      }
    } else {

        clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // make a new socket

        if (clientSocket < 0) // error check
        {
          // printf("*** CLIENT ERROR: Could open socket.\n");
          exit(-1);
        }

        // Setup address
        memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddress.sin_family = AF_INET;
        clientAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
        clientAddress.sin_port = htons((unsigned short)SERVER_PORT + connectedTowerID);

        // Connect to server
        status = connect(clientSocket, (struct sockaddr *)&clientAddress, sizeof(clientAddress));
        if (status < 0)
        {
          // printf("*** CLIENT ERROR: Could not connect.\n");
          exit(-1);
        }

      clientResponse[0] = UPDATE; // prepare our response
      clientResponse[1] = x;
      clientResponse[2] = y;
      clientResponse[3] = connectionID;

      // printf("CLIENT: Sending UPDATE response. x: %d y: %d\n", x, y);
      send(clientSocket, clientResponse, sizeof(clientResponse), 0); // send and wait for a response

      bytesRcv = recv(clientSocket, buffer, 80, 0); // wait on response
      // printf("CLIENT RECEIVED: %d\n", buffer[0]);
      if(buffer[0] == NO) { // if no, reset connectionID and start search again
        // printf("CLIENT: NO repsonse received\n");
        connectionID = -1;
        connectedTowerID = -1;
      }
      // printf("CLIENT: Closing Socket 152\n");
      close(clientSocket); // close socket
    }
  }
}

