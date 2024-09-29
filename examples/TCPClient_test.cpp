// For compile: g++ -o TCPClient_test TCPClient_test.cpp ../TCPNetwork.cpp
// For run: sudo ./TCPClient_test
// ##################################################
// Include libraries

#include <iostream>             // For standard input and output stream.
#include "../TCPNetwork.h"       // Custom TCP/IP network library for handel server and client

/*
to set the server socket to non-blocking mode before calling accept. 
This can be achieved using the fcntl function to modify the file descriptor flags.
*/
// ###################################################
// Global Variables

TCPClient client;

int serverPort = 9000;                       // Port number on which the server listens
const char *server_ip = "127.0.0.1";     // IP address on which the server listens.. Replace with your interface's IP address

int i;

// ###################################################
// Function declerations


// ###################################################
int main() 
{
    // Start the client and connect to the server
    if (client.start(serverPort, server_ip)) {
        std::cout << "Connected to the server successfully." << std::endl;
    } else {
        client.printError();
        return 1;
    }

    // Message to send to the server
    const char* message = "Hello, Server!";
    char buffer[1024];

    // Main loop for continuous communication
    while (client.isClientConnected()) {
        // Update the client: send and receive
        if (client.update(const_cast<char*>(message), strlen(message), buffer, sizeof(buffer) - 1)) {
            std::cout << "Message sent to the server." << std::endl;

            // Print the received message
            std::cout << "Received message from server: " << buffer << std::endl;
        } else {
            client.printError();
            client.clientClose();
            break;  // Exit the loop on error
        }

        // Sleep for a while before sending the next message
        usleep(1000000);  // Sleep for 1 second (1000000 microseconds)
    }

    // Close the client connection
    client.clientClose();
    std::cout << "Client connection closed." << std::endl;

    return 0;
}