#include "TCPNetworkLinux.h"

using namespace TCPNetworkLinuxNamespace;

// ###############################################################################################
// Define Macros:

#define TCPNetworkLinux_version             "V1.1"          // Library version tag
#define TCPNetworkLinux_DEFAULT_RX_SIZE     1000            // Default RX buffer size
#define TCPNetworkLinux_DEFAULT_TX_SIZE     1000            // Default TX buffer size

// ###############################################################################################
// General functions:

std::string TCPNetworkLinuxNamespace::getIPAddressByInterface(const std::string& interfaceName) 
{
    struct ifaddrs *ifaddr, *ifa;
    std::string ipAddress;

    // Retrieve the list of network interfaces
    if (getifaddrs(&ifaddr) == -1) 
    {
        perror("getifaddrs");
        return "";
    }

    // Iterate through the linked list of interfaces
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) 
    {
        // Check if the interface is valid and is an IPv4 address
        if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET) 
        {
            continue;  // Skip if not an IPv4 address
        }

        // Check if the interface name matches and the interface is up and running
        if (interfaceName == ifa->ifa_name && (ifa->ifa_flags & IFF_UP) && (ifa->ifa_flags & IFF_RUNNING)) 
        {
            // Convert the IP address to a string
            char ip[INET_ADDRSTRLEN];
            struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
            if (inet_ntop(AF_INET, &addr->sin_addr, ip, INET_ADDRSTRLEN) != nullptr) 
            {
                ipAddress = ip;  // Store the IP address as a string
                break;  // Stop once the IP address is found
            }
        }
    }

    // Free the linked list of interfaces
    freeifaddrs(ifaddr);
    return ipAddress;  // Return the IP address or an empty string if not found
}

// ######################################################################
// TCPServer class:

TCPServer::TCPServer()
{
    _rxBufferSize = TCPNetworkLinux_DEFAULT_RX_SIZE;
    _txBufferSize = TCPNetworkLinux_DEFAULT_TX_SIZE;
    _serverSocket = -1;
    _clientSocket = -1;
}

bool TCPServer::startByIP(const uint16_t port, const char* ip)
{
    _port = port;
    _ip = ip;

    // Server configuration. socket() Returns a file descriptor for the new socket, or -1 for errors.
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket == -1) 
    {
        errorMessage = "TCPServer error: Error creating server socket: ";
        return false;
    }

    // Set SO_REUSEADDR option
    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) 
    {
        errorMessage = "TCPServer error: Error setting socket options.";
        _handleServerDisconnection();
        return false;
    }

    // Set the server socket to non-blocking mode
    int flags = fcntl(_serverSocket, F_GETFL, 0);
    if (flags == -1) 
    {
        errorMessage = "TCPServer error: Error getting socket flags for setting socket to non-blockings.";
        _handleServerDisconnection();
        return false;
    }

    if (fcntl(_serverSocket, F_SETFL, flags | O_NONBLOCK) == -1) 
    {
        errorMessage = "TCPServer error: Error setting socket to non-blocking.";
        _handleServerDisconnection();
        return false;
    }

    // Define the server address structure
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;     // This ensures that the socket will use the Internet Protocol (IPv4)
    serverAddress.sin_port = htons(port);   // Change port here

    // specify a particular IP address rather than binding to any available network interface.
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, ip, &serverAddress.sin_addr) <= 0) 
    {
        errorMessage = "TCPServer error: Invalid IP address/ Address not supported";
        _handleServerDisconnection();
        return false;
    }

    // Zero out the rest of the struct
    memset(&(serverAddress.sin_zero), 0, sizeof(serverAddress.sin_zero));

    // Bind the socket to the network address and port
    if (bind(_serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) 
    {
        errorMessage = "TCPServer error: Bind failed.";
        _handleServerDisconnection();
        return false;
    }

    /*
    MAX_CONNECTIONS defines the maximum length of the queue of pending connections. 
    When a client attempts to connect to the server, if the server is not ready to accept the connection immediately, 
    the connection request will be placed in a queue. MAX_CONNECTIONS specifies the maximum number of connections that 
    can be queued at any one time.
    */
    int MAX_CONNECTIONS = 10;
    // Listen for incoming connections
    if (listen(_serverSocket, MAX_CONNECTIONS) == -1) {
        errorMessage = "TCPServer error: Listen failed.";
        _handleServerDisconnection();
        return false;
    }

    return true;
}

bool TCPServer::startByName(const uint16_t port, const char* InterfaceName)
{
    _ip = TCPNetworkLinuxNamespace::getIPAddressByInterface(InterfaceName);

    if(_ip == "")
    {
        errorMessage = "TCPServer error: Invalid interface ethernet name.";
        return false;
    }

    return startByIP(port, _ip.c_str());
}

bool TCPServer::_clientConfig(void)
{
    // Accept incoming connection
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    
    _clientSocket = accept(_serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
    if (_clientSocket == -1) 
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN) 
        {
            // No pending connections; this is expected in non-blocking mode
        } 
        else 
        {
            errorMessage = "TCPServer error: Accept client failed";
        }
        return false;
    }

    return true;
}

void TCPServer::_handleClientDisconnection(void) 
{
    close(_clientSocket);
    _clientSocket = -1;
}

void TCPServer::_handleServerDisconnection(void) 
{
    _handleClientDisconnection();
    close(_serverSocket);  
    _serverSocket = -1;
}

bool TCPServer::readWrite(char *txBuffer, size_t txSize, char *rxBuffer, size_t rxSize)
{
    int bytesRead = read(rxBuffer, rxSize);
    if( bytesRead == -1)
    {
        return false;
    }

    if(!write(*txBuffer, txSize))
    {
        return false;
    }

    return true;     
}

int32_t TCPServer::read(char *rxBuffer, const size_t &rxSize)
{
    if(rxSize == 0)
    {
        errorMessage = "TCPServer error: rxSize is zero value.";
        #if(TCPNetworkLinux_DEBUG == 1)
                    std::cout << errorMessage << std::endl;
        #endif
        return -1;
    }
        
    // Number of bytes read from the socket.
    int32_t bytesRead = 0; 
       
    bytesRead = recv(_clientSocket, rxBuffer, rxSize, 0);
    
    if (bytesRead <= 0)
    {
        if (bytesRead == 0) 
        {
            // // Client disconnected
            // errorMessage = "TCPServer error: Error Client disconnected.";
            // _handleClientDisconnection();
            // return 0;
        } 
        else if (bytesRead == -1) 
        {
            if (errno != EWOULDBLOCK && errno != EAGAIN) 
            {
                errorMessage = "TCPServer error: Error receiving message.";
                _handleClientDisconnection();
                #if(TCPNetworkLinux_DEBUG == 1)
                    std::cout << errorMessage << std::endl;
                #endif
                return -1;
            } 
            return -1;
        }
    }

    if (bytesRead > (int64_t)rxSize)
    {
        // Receive proccess was not successed.
        errorMessage = "TCPServer error: Error receiving message. bytesRead is more than rxSize";
        #if(TCPNetworkLinux_DEBUG == 1)
                std::cout << errorMessage << std::endl;
        #endif
        return 0;
    }
        
    return bytesRead;     
}

int32_t TCPServer::read(std::string *data)
{
    int32_t bytesRead = available();

    if(bytesRead <= 0)
    {
        return bytesRead;
    }
    
    char buffer[bytesRead];

    if((size_t)bytesRead > _rxBufferSize)
    {
        bytesRead = _rxBufferSize;
    }
    bytesRead = read(buffer, bytesRead);

    if(bytesRead > 0)
    {
        data->assign(buffer, bytesRead);
    }
    
    return bytesRead;
}

int32_t TCPServer::read(void)
{
    std::string data;

    int32_t bytesRead = read(&data);    

    if(bytesRead > 0)
    {
        pushBackRxBuffer(data.c_str(), data.size());
    }
    
    return bytesRead;
}

bool TCPServer::write(const char &txBuffer, const size_t &txSize)
{
    // Number of bytes write to the socket.
    int bytesWrite; 

    if (_clientSocket == -1)  // No client connected
    { 
        if (_clientConfig()) 
        {
            // Set the client socket to non-blocking mode
            int clientFlags = fcntl(_clientSocket, F_GETFL, 0);
            if (clientFlags == -1) 
            {
                errorMessage = "TCPServer error: Error getting client socket flags.";
                _handleClientDisconnection();
                return false;
            }

            if (fcntl(_clientSocket, F_SETFL, clientFlags | O_NONBLOCK) == -1) 
            {
                errorMessage = "TCPServer error: Error setting client socket to non-blocking.";
                _handleClientDisconnection();
                return false;
            }
        }
    }
    else   // Client is connected
    { 
        struct pollfd pfd;
        pfd.fd = _clientSocket;
        pfd.events = POLLOUT | POLLHUP;

        int pollRes = poll(&pfd, 1, 0);

        if (pollRes == -1) 
        {
            errorMessage = "TCPServer error: Poll error.";
            _handleClientDisconnection();
            return false;
        }

        if (pfd.revents & POLLHUP) 
        {
            errorMessage = "TCPServer error: Client disconnected.";
            _handleClientDisconnection();
            return false;
        }

        if (pfd.revents & POLLOUT) 
        {
            bytesWrite = send(_clientSocket, &txBuffer, txSize, 0);

            if (bytesWrite == -1) 
            {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    errorMessage = "TCPServer error: Error sending message.";
                    _handleClientDisconnection();
                    return false;
                }
            } 
            else if (bytesWrite < (int)txSize) 
            {
                errorMessage = "TCPServer error: Partial write. Not all data was sent.";
                return false;
                // Handle partial send if needed
                // Optionally, implement logic to retry sending the remaining data
            }
        }
    } 

    return true;     
}

bool TCPServer::write(const std::string &txBuffer)
{
    const char *data = txBuffer.c_str();

    return write(*data, txBuffer.size());
}

bool TCPServer::write(void)
{
    std::string data(_txBuffer.begin(), _txBuffer.begin() + _txBuffer.size());
    if(!write(data))
    {
        return false;
    }
    _txBuffer.clear();

    return true;
}

void TCPServer::printError(void)
{
    printf("%s\n",errorMessage.c_str());
}

std::string TCPServer::getError(void)
{
    return errorMessage;
}

bool TCPServer::isListening(void)
{
    if(_serverSocket == -1)
        return false;

    return true;
}

bool TCPServer::isClientConnected(void)
{
    // Check if the socket is valid
    if (_clientSocket == -1) 
    {
        return false; // No client connected
    }

    // recv() with MSG_PEEK:
    // Allows you to check if the client has disconnected without actually consuming any data from the socket buffer.
    // If recv() returns 0, it means the client has disconnected.
    // If recv() returns a negative value and errno is set to ECONNRESET or EPIPE, it means there’s been an error like the connection being reset by the client.

    // Use recv to check if the client has disconnected
    char buffer[1];
    ssize_t recvRes = recv(_clientSocket, buffer, sizeof(buffer), MSG_PEEK);
    if (recvRes == 0) 
    {
        // Client disconnected
        errorMessage = "Client disconnected.";
        _handleClientDisconnection();
        #if (TCPNetworkLinux_DEBUG == 1)
            std::cout << errorMessage << std::endl;
        #endif
        return false;
    }
    else if (recvRes < 0) 
    {
        // Error occurred during recv
        if (errno == ECONNRESET || errno == EPIPE) 
        {
            errorMessage = "TCPServer error: Connection reset by peer.";
            _handleClientDisconnection();
            #if (TCPNetworkLinux_DEBUG == 1)
                std::cout << errorMessage << std::endl;
            #endif
            return false;
        }
    }

    // Connection is still alive
    return true;  
}

bool TCPServer::checkLinkStatus(const char* port_name)
{
    std::string carrierPath = std::string("/sys/class/net/") + port_name + "/carrier";
    std::ifstream carrierFile(carrierPath);
    if (!carrierFile.is_open()) {
        errorMessage = "Error opening carrier file for interface: eno1";
        return false;
    }

    int status;
    carrierFile >> status;
    carrierFile.close();
    
    if (status == 1) {
        return true; // Link is up
    } else {
        return false; // Link is down
    }
}

void TCPServer::serverClose(void)
{
    _handleServerDisconnection();
}

void TCPServer::clientClose(void)
{
    _handleClientDisconnection();
}

bool TCPServer::clientConnect(void)
{
    if (!isClientConnected())  
    { 
        if (_clientConfig()) 
        {
            // Set the client socket to non-blocking mode
            int clientFlags = fcntl(_clientSocket, F_GETFL, 0);
            if (clientFlags == -1) 
            {
                errorMessage = "TCPServer error: Error getting client socket flags.";
                _handleClientDisconnection();
                return false;
            }

            if (fcntl(_clientSocket, F_SETFL, clientFlags | O_NONBLOCK) == -1) 
            {
                errorMessage = "TCPServer error: Error setting client socket to non-blocking.";
                _handleClientDisconnection();
                return false;
            }
        }
    }

    return true;
}

std::string TCPServer::getVersion(void)
{
    return TCPNetworkLinux_version;
}

void TCPServer::setTxBufferSize(size_t size)
{
    _txBufferSize = size;
}

void TCPServer::setRxBufferSize(size_t size)
{
    _rxBufferSize = size;
}

int32_t TCPServer::available(void)
{
    // Use ioctl to find out how many bytes are available in the receive buffer
    int32_t bytesAvailable = 0;
    
    if( (_serverSocket > 0 ) && (_clientSocket > 0))
    {   
        if (ioctl(_clientSocket, FIONREAD, &bytesAvailable) < 0) 
        {
            errorMessage = "ioctl failed";
            #if(TCPNetworkLinux_DEBUG == 1)
                std::cout << errorMessage << std::endl;
            #endif
            return -1;
        }
    }

    return bytesAvailable;
}

void TCPServer::removeFrontRxBuffer(size_t num)
{
    for(size_t i = 0; i < num ; i++)
    {
        if(_rxBuffer.empty())
            return;

        _rxBuffer.pop_front();
    }
}

void TCPServer::removeFrontTxBuffer(size_t size)
{
    for(size_t i = 0; i < size ; i++)
    {
        if(_txBuffer.empty())
            return;

        _txBuffer.pop_front();
    }
}

void TCPServer::removeAllRxBuffer(void)
{
    _rxBuffer.clear();
}

void TCPServer::removeAllTxBuffer(void)
{
    _txBuffer.clear();
}

std::string TCPServer::popFrontRxBuffer(size_t size)
{
    std::string data;

    for(size_t i = 0; i < size ; i++)
    {
        if(_rxBuffer.empty())
            return data;

        data.push_back(_rxBuffer.front());
        _rxBuffer.pop_front();
    }

    return data;
}

std::string TCPServer::popAllRxBuffer(void)
{
    std::string data(_rxBuffer.begin(), _rxBuffer.end());
    _rxBuffer.clear();

    return data;
}

void TCPServer::pushBackRxBuffer(const char* data, size_t size)
{
    // empty space size of rx buffer.
    int64_t emptySize;

    emptySize = (int64_t)(_rxBuffer.size() + size) - (int64_t)_rxBufferSize;

    if(emptySize > 0)
    {
        removeFrontRxBuffer(emptySize);
    }

    // Append the char array to the deque
    _rxBuffer.insert(_rxBuffer.end(), data, data + size);
}

void TCPServer::pushBackRxBuffer(const std::string* data)
{
    pushBackRxBuffer(data->c_str(), data->size());
}

void TCPServer::pushBackTxBuffer(const char* data, size_t size)
{
    // empty space size of rx buffer.
    int64_t emptySize;

    emptySize = (int64_t)(_txBuffer.size() + size) - (int64_t)_txBufferSize;

    if(emptySize > 0)
    {
        removeFrontTxBuffer(emptySize);
    }

    // Append the char array to the deque
    _txBuffer.insert(_txBuffer.end(), data, data + size);

}

void TCPServer::pushBackTxBuffer(const std::string* data)
{
    pushBackTxBuffer(data->c_str(), data->size());
}

// ##########################################################################################
// TCPClient class:

bool TCPClient::start(int port, const char* ip)
{
    _port = port;
    _ip = ip;

    // Create a socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) 
    {
        errorMessage = "Error creating socket.";
        return false;
    }

    // Set socket to non-blocking
    int flags = fcntl(clientSocket, F_GETFL, 0);
    if (flags == -1) 
    {
        errorMessage = "Error getting socket flags.";
        handleClientDisconnection();
        return false;
    }

    if (fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK) == -1) 
    {
        errorMessage = "Error setting socket to non-blocking.";
        handleClientDisconnection();
        return false;
    }

    // Define the server address
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(ip);

    if (inet_pton(AF_INET, ip, &serverAddress.sin_addr) <= 0) {
        errorMessage = "Invalid address or address not supported";
        handleClientDisconnection();
        return false;
    }

    // Connect to the server (non-blocking)
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) 
    {
        if (errno != EINPROGRESS) 
        {
            errorMessage = "Connect failed.";
            handleClientDisconnection();
            return false;
        }
    }

    // // Wait for the connection to be established (using select or epoll can be more efficient)
    // fd_set writeSet;
    // FD_ZERO(&writeSet);
    // FD_SET(clientSocket, &writeSet);

    // struct timeval timeout;
    // timeout.tv_sec = 10; // 10 seconds timeout
    // timeout.tv_usec = 0;

    // int selectResult = select(clientSocket + 1, nullptr, &writeSet, nullptr, &timeout);
    // if (selectResult == -1) 
    // {
    //     errorMessage = "Select error.";
    //     handleClientDisconnection();
    //     return false;
    // } 
    // else if (selectResult == 0) 
    // {
    //     errorMessage = "Connection timeout.";
    //     handleClientDisconnection();
    //     return false;
    // }

    return true;
}

bool TCPClient::update(char *txBuffer, int txSize, char *rxBuffer, int rxSize)
{
    if (txBuffer && txSize > 0) 
    {
        bytesSent = send(clientSocket, txBuffer, txSize, 0);
        if (bytesSent == -1) 
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK) 
            {
                errorMessage = "Send failed.";
                handleClientDisconnection();
                return false;
            }
        } 
        else 
        {
            // std::cout << "Message sent to server." << std::endl;
        }
    }

    if (rxBuffer && rxSize > 0) 
    {
        // Receive the echoed message (non-blocking)
        bytesRead = recv(clientSocket, rxBuffer, rxSize, 0);
        if (bytesRead == -1) 
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK) 
            {
                errorMessage = "Receive failed.";
                handleClientDisconnection();
                return false;
            }
        } 
        else if (bytesRead == 0) 
        {
            errorMessage = "Server disconnected.";
            handleClientDisconnection();
            return false;
        } 
        else 
        {
            // receive successed.
        }
    }
    
    return true;
}

void TCPClient::handleClientDisconnection(void) 
{
    close(clientSocket);
    clientSocket = -1;
}

void TCPClient::clientClose(void)
{
    handleClientDisconnection();
}

void TCPClient::printError(void)
{
    printf("%s\n",errorMessage.c_str());
}

bool TCPClient::isClientConnected(void)
{
    if(clientSocket == -1)
        return false;

    return true;
}

std::string TCPClient::getVersion(void)
{
    return TCPNetworkLinux_version;
}






