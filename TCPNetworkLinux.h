#ifndef TCPNETWORKLINUX_H
#define TCPNETWORKLINUX_H

/**
 * @brief: This library is for TCP/IP netwok communication. It work just for linux based OS.
 * It used <sys/socket.h>, <netinet/in.h>, <arpa/inet.h> libraries for develop high level of TCP/IP communication.
 * The purpose is create a high level library for TCP/IP communication from basic libraries to work faster and easier programming.
 * The main classes is TCPServer and TCPClient for server and client devices.
 * 
 * Developed by: Mohammad Nikanjam
 */

// ###########################################################################################
// Include libraries:

#include <iostream>             // For standard input and output stream.
#include <cstring>              // For handling C-style strings.
#include <unistd.h>             // For close function.
#include <cerrno>               // For error handling.
#include <sys/socket.h>         // For socket programming.
#include <netinet/in.h>         // For sockaddr_in structure.
#include <arpa/inet.h>          // Include this header for inet_pton. For inet_pton function to convert IP addresses.
#include <fcntl.h>              // Include this header for fcntl
#include <poll.h>               // Used to monitor multiple file descriptors
#include <fstream>
#include <deque>                // Modern standard library for double-ended queue
#include <sys/ioctl.h>          // For ioctl
#include <ifaddrs.h>            // For getifaddrs
#include <net/if.h>             // For IFF_UP, IFF_RUNNING

// ############################################################################################
// Define Macros:

#define TCPNetworkLinux_DEBUG       0

// ############################################################################################
// General Functions:

// namespace for library functions protection.
namespace TCPNetworkLinuxNamespace
{

    // Return ethernet ip address by hardware interface port name(eg: eth0) 
    std::string getIPAddressByInterface(const std::string& interfaceName);

}

// ############################################################################################
// TCPServer class:

// TCP/IP Object for server
class TCPServer
{
    public:

        // Server Port number on which the server listens. eg: 8080
        int _port; 

        // Server IP address on which the server listens.. Replace with your interface's IP address. eg: "192.168.1.100"                         
        std::string _ip;  

        // Server interface port name on which the server listens. eg: eth0.
        std::string _portName; 

        // Last error accured for TCPServer object.
        std::string errorMessage;

        // Default constructor. init some variables.
        TCPServer();

        /** 
        * Configures and sets up the server.
        * Set port number and ip address.
        * Server start listening in non blocking mode.
        * @param port: port number for server listening.
        * @param ip: ip address for certain ethernet port.
        * @return true if successed.
        */
        bool startByIP(const uint16_t port, const char* ip);      

        /** 
        * Configures and sets up the server.
        * Set port number and interface name.
        * Server start listening in non blocking mode.
        * @param port: port number for server listening.
        * @param InterfaceName: ethernet port name. eg: eth0, en0.
        * @return true if successed.
        */
        bool startByName(const uint16_t port, const char* InterfaceName);             
        
        /// @brief Print last error accured for server.
        void printError(void);

        // Reteurn last error accured for server.
        std::string getError(void);

        /**
         * Return status of server listening.
         * @return true if server is listening.
         *  */ 
        bool isListening(void);

        /**
         * Return status of client connection.
         * @return true if client is connected.
         *  */ 
        bool isClientConnected(void);

        /**
         * Check ethernet physically cable connected to certain port.
         * @return true if ethernet port connected physically.
         *  */ 
        bool checkLinkStatus(const char* port_name);   

        /**
         * readWrite or send/recieve operation. 
         * Send txBuffer, receive and store in rxBuffer.
         * @return true if successed.
         * */ 
        bool readWrite(char *txBuffer = nullptr, size_t txSize = 0, char *rxBuffer = nullptr, size_t rxSize = 0);

        /**
         * read or recieve operation.
         * Receive and store in a rxBuffer char array.
         * @param rxSize: number of character for read.
         * @return number of bytes that read. return -1 if there is any error.
         *  */  
        int32_t read(char *rxBuffer, const size_t &rxSize);

        /**
         * read or recieve operation.
         * Receive and store all data(max size = rxBufferSize) in a rxBuffer string.
         * @return number of bytes that read. return -1 if there is any error.
         *  */ 
        int32_t read(std::string *rxBuffer);

        /**
         * read or recieve operation.
         * Receive and append all data  in to the deque rxbuffer of server. *Hint: max size of deque of rxBuffer is limited to rxBufferSize.
         * @return number of bytes that read. return -1 if there is any error.
         *  */ 
        int32_t read(void);

        /**
         * Write or send operation.
         * @param txBuffer: pointer to the char array.
         * @param txSize: number of char that want to send.
         * @return true if successed.
         *  */  
        bool write(const char &txBuffer, const size_t &txSize);

        /**
         * Write or send operation.
         * @param txBuffer: pointer to the string that want to send.
         * @return true if successed.
         *  */  
        bool write(const std::string &txBuffer);

        /**
         * Write or send operation.
         * Write all data on TX buffer then remove tx elements.
         * @return true if successed.
         *  */  
        bool write(void);

        // Close server socket.
        void serverClose(void);

        // Close client socket.
        void clientClose(void);

        /**
         * Try to connect to any client socket.
         * @return true if client connected.
         */
        bool clientConnect(void);

        // Return library version.
        std::string getVersion(void);

        // Set transmit buffer size. 
        void setTxBufferSize(size_t size = 1000);

        // Set receive buffer size.
        void setRxBufferSize(size_t size = 1000);

        // Return number of character available for read on the server socket.
        int32_t available(void);

        // Remove certain number character from front of RX deque buffer.
        void removeFrontRxBuffer(size_t num);

        // Remove certain number character from front of TX deque buffer.
        void removeFrontTxBuffer(size_t size);

        // Remove all data from RX deque buffer.
        void removeAllRxBuffer(void);

        // Remove all data from TX deque buffer.
        void removeAllTxBuffer(void);

        /**
         * Pop front certain number elements from RX buffer and remove them.
         * @return string that pop front.
         *  */
        std::string popFrontRxBuffer(size_t size);

        /**
         * Pop front all elements from RX buffer and remove them.
         * @return string that pop front.
         *  */
        std::string popAllRxBuffer(void);

        /**
         * Push back certain number character from char array to RX buffer.
         */
        void pushBackRxBuffer(const char* data, size_t size);

        /**
         * Push back certain string to RX buffer.
         */
        void pushBackRxBuffer(const std::string* data);

        /**
         * Push back certain number character from char array to TX buffer.
         */
        void pushBackTxBuffer(const char* data, size_t size);

        /**
         * Push back certain string to TX buffer.
         */
        void pushBackTxBuffer(const std::string* data);

    
    private:

        std::deque<char> _txBuffer;        // TX deque buffer.
        std::deque<char> _rxBuffer;        // RX deque buffer.

        size_t _txBufferSize;              // Max size for tx deque buffer.
        size_t _rxBufferSize;              // Max size for rx deque buffer.
        
        // Integer representing the server's socket descriptor.
        int _serverSocket;             

        // Integer representing the client's socket descriptor. 
        int _clientSocket;                            

        /**
         * Accepts a client connection. [ Non blocking mode.]
         * @return true if successfully connected to a client
         *  */ 
        bool _clientConfig(void);                
        
        // Handles client disconnection
        void _handleClientDisconnection(void);  

        // Handles server disconnection
        void _handleServerDisconnection(void);

};

// ############################################################################################
// TCPClient class:

class TCPClient
{
    public:

        // Destructor
        ~TCPClient() {
            clientClose();
        }

        /*
        Configures and sets up the client.
        Set port and ip address.
        Client connection is in non blocking mode.
        */
        bool start(int port, const char* ip);                 
        
        // Update send/recieve operation. 
        // Send txBuffer, receive and store in rxBuffer.
        bool update(char *txBuffer, int txSize, char *rxBuffer, int rxSize);

        /// @brief Print last error accured in methods of server.
        void printError(void);

        bool isClientConnected(void);

        // Close client socket.
        void clientClose(void);

        std::string getVersion(void);

    private:

        ssize_t bytesRead, bytesSent;       

        // Integer representing the client's socket descriptor. 
        int clientSocket = -1;              

        // Server Port number on which the server listens. eg: 8080
        int _port; 

        // Server IP address on which the server listens.. Replace with your interface's IP address. eg: "192.168.1.100"                         
        std::string _ip;                    

        // Last error accured in methods of server.
        std::string errorMessage;

        // Accepts a client connection. [ No blocking mode.]
        bool clientConfig(void);                
        
        // Handles client disconnection
        void handleClientDisconnection(void);  

};

#endif
