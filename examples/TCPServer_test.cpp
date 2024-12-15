/*
For compile: 
mkdir -p ./bin && g++ -o ./bin/TCPServer_test TCPServer_test.cpp ../TCPNetworkLinux.cpp
For run: 
sudo ./bin/TCPServer_test
*/
// ##################################################
// Include libraries

#include <iostream>             // For standard input and output stream.
#include "../TCPNetworkLinux.h"       // Custom TCP/IP network library for handel server and client

using namespace std;

/*
to set the server socket to non-blocking mode before calling accept. 
This can be achieved using the fcntl function to modify the file descriptor flags.
*/
// ###################################################
// Global Variables

TCPServer server;

int serverPort = 8080;                       // Port number on which the server listens
const char *server_ip = "192.168.137.98";    // IP address on which the server listens.. Replace with your interface's IP address
const char *server_portName = "eno1";       

int i;

// ###################################################
// Function declerations


// ###################################################
int main() {

    if (server.startByName(serverPort, server_portName)) 
    {
        printf("Server is listening!\n");
    }
    else
    {
        server.printError();
    }

    while(true)
    {
        if(!server.isClientConnected())
        {
            // try to connect if any client exist.
            server.clientConnect();
        }

        std::string rxString;
        std::string txString = "hello mohammad.\n";

        server.pushBackTxBuffer(&txString);

        server.write();

        server.read();

        cout << "rxString: " << server.popAllRxBuffer() << endl;

        i++;
        printf("counter i: %d\n",i);
        usleep(1000000); // Sleep for 100ms to prevent busy waiting
    }

    server.serverClose();

    std::cout << "Server closed." << std::endl;

    return 0;
}
