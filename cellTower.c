#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>



// Handle client requests coming in through the server socket.  This code should run
// indefinitiely.  It should wait for a client to send a request, process it, and then
// close the client connection and wait for another client.  The requests that may be
// handles are SHUTDOWN, CONNECT and UPDATE.  A SHUTDOWN request causes the tower to
// go offline.   A CONNECT request contains 4 additional bytes which are the high and
// low bytes of the vehicle's X coordinate, followed by the high and low bytes of its
// Y coordinate.  If within range of this tower, the connection is accepted and a YES
// is returned, along with a char id for the vehicle and the tower id.   If UPDATE is
// received, the additional 4 byes for the (X,Y) coordinate are also received as well
// as the id of the vehicle.   Then YES is returned if the vehicle is still within
// the tower range, otherwise NO is returned.
void *handleIncomingRequests(void *ct) {

  // server variables
  CellTower       *tower = ct;
  int serverSocket, clientSocket;
  struct sockaddr_in serverAddress, clientAddr;
  int status, addrSize, bytesRcv;
  short buffer[50];
  short serverResponse[50];

  // Create the server socket
  serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (serverSocket < 0)
  {
    // printf("*** SERVER ERROR: Could not open socket.\n");
    exit(-1);
  }

  // Setup the server address
  memset(&serverAddress, 0, sizeof(serverAddress)); // zeros the struct
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddress.sin_port = htons((unsigned short)SERVER_PORT + tower->id);

  // Bind the server socket
  status = bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
  if (status < 0)
  {
    // printf("*** SERVER ERROR: Could not bind socket.\n");
    exit(-1);
  }

  // Set up the line-up to handle up to MAX_CONNECTIONS clients in line
  status = listen(serverSocket, MAX_CONNECTIONS);
  if (status < 0)
  {
    // printf("*** SERVER ERROR: Could not listen on socket.\n");
    exit(-1);
  }

  // wait for clients
  while(1) { // infinite loop for server

    addrSize = sizeof(clientAddr);
    // start waiting for new clients
    clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrSize); // wait for a new client
    if (clientSocket < 0) // check for error
    {
      // printf("*** SERVER ERROR: Could not accept incoming client connection.\n");
      exit(-1);
    }
    // printf("SERVER: Received client connection.\n");

    while(1) { // infinite loop for client connection
      bytesRcv = recv(clientSocket, buffer, sizeof(buffer), 0); // w
      // printf("SERVER: %d\n", buffer[0]);
      if(buffer[0] == SHUTDOWN) { // shutdown protocol
        close(clientSocket);
        break;
      }
      if(buffer[0] == CONNECT) { // connect protocol
        if ((((buffer[1] - tower->x) * (buffer[1] - tower->x)) + ((buffer[2] - tower->y) * (buffer[2] - tower->y))) <= (tower->radius) * (tower->radius)) { // pythagorean theorem to see if the car is too far
        if (tower->numConnectedVehicles >= MAX_CONNECTIONS) // check if we have max connections
        {
          // printf("SERVER: Sending NO response to client\n");
          serverResponse[0] = NO; // prepare our response
          send(clientSocket, serverResponse, sizeof(serverResponse), 0); // send off to the client
        } else { // if it passes, we loop through all the connected vehicles to find the first open spot
          for(int i = 0; i < MAX_CONNECTIONS; i++) { 
            if(tower->connectedVehicles[i].connected == 0) {
              tower->connectedVehicles[i].connected = 1;
              tower->connectedVehicles[i].x = buffer[1];
              tower->connectedVehicles[i].y = buffer[2];
              tower->numConnectedVehicles++;
              serverResponse[0] = YES; // prepare response
              serverResponse[1] = i; // connectionID
              serverResponse[2] = tower->id; // connectedtowerid
              // printf("SERVER: Sending YES response to client %d %d 92\n", i, tower->id);
              send(clientSocket, serverResponse, sizeof(serverResponse), 0);
              break; // break our loop once we find a spot
            }
          }
        }
      }
      }
      if(buffer[0] == UPDATE) { // update protocol
        if (((buffer[1] - tower->x) * (buffer[1] - tower->x)) + ((buffer[2] - tower->y) * (buffer[2] - tower->y)) > (tower->radius) * (tower->radius)) { // pythagorean theorem to see if the car is too far
          tower->connectedVehicles[buffer[3]].connected = 0; // if too far, reset all the variables
          tower->connectedVehicles[buffer[3]].x = 0;
          tower->connectedVehicles[buffer[3]].y = 0;
          tower->numConnectedVehicles--;
          serverResponse[0] = NO; // send a no to the client
          // send no response
          // printf("SERVER: Sending NO response UPDATE\n");
          send(clientSocket, serverResponse, sizeof(serverResponse), 0);
        } else { //if the car is in range, update coords, send yes
          tower->connectedVehicles[buffer[3]].x = buffer[1];
          tower->connectedVehicles[buffer[3]].y = buffer[2];
          serverResponse[0] = YES;
          send(clientSocket, serverResponse, sizeof(serverResponse), 0);
        }
      }
      // printf("SERVER: Closing Client Socket");
      close(clientSocket); //close the socket and begim accepting more
      break;
    }

  if(buffer[0] == SHUTDOWN) { // break the infinite loop
    break;
  }

  }
  close(serverSocket); // close the server gracefully
  // printf("SERVER: Shutting Down");
  
}
