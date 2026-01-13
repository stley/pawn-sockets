#include "Socket.hpp"


inline bool isValidSocket(SOCKET h){
    #ifdef _WIN32
        return (h != INVALID_SOCKET);
    #else
        return (h != -1);
    #endif
}

int GetSocketError(){
    #ifdef _WIN32
        return WSAGetLastError();
    #else
        return errno;
    #endif
}

Socket::Socket(int type) :
handle_(0),
protocol_(type),
state_(SocketState::Invalid)
{

    std::memset(&socketConfig, 0, sizeof(socketConfig));

    if(type == 0) return;

    int sockType = 0;
    int protocol = 0;


    switch(type){
        case 1:{
            sockType = SOCK_DGRAM;
            protocol = IPPROTO_UDP;                
            break;
        }
        case 2:{
            sockType = SOCK_STREAM;
            protocol = IPPROTO_TCP;            
            break;
        }
        default: return;
    }

    handle_ = socket(AF_INET, sockType, protocol);

    if(isValidSocket(handle_))
        state_ = SocketState::Created;
}


void Socket::Close(){ 
    socket_close(handle_);
    handle_ = INVALID_SOCKET;
    protocol_ = 0;
    state_ = SocketState::Invalid;
}

Socket::~Socket(){
    Socket::Close();
}


int Socket::Bind(uint16_t port){
    if(!isValidSocket(handle_)) return -1;

    socketConfig.sin_family = AF_INET;
    socketConfig.sin_port = htons(port);
    in_addr ip_addr;
    inet_pton(AF_INET, "0.0.0.0", &ip_addr);

    
    socketConfig.sin_addr = ip_addr;

    int rt = 0;
    rt = ::bind(handle_, (SOCKADDR*) &socketConfig, sizeof(socketConfig));

    if(rt == SOCKET_ERROR){
        std::memset(&socketConfig, 0, sizeof(socketConfig));
        state_ = SocketState::Error;
        getCore()->printLn("Socket::Bind failed: %d | OS HANDLE: %d", GetSocketError(), handle_);
    }

    else
        state_ = SocketState::Bound;

    return rt;
}

bool Socket::Connect(const std::string& ip, uint16_t port){
    if(!isValidSocket(handle_)) return false;

    int ret = 0;
    
    std::memset(&socketConfig, 0, sizeof(socketConfig));

    socketConfig.sin_family = AF_INET;
    socketConfig.sin_port = htons(port);
    in_addr ip_addr;
    inet_pton(AF_INET, ip.c_str(), &ip_addr);

    socketConfig.sin_addr = ip_addr;

    ret = connect(handle_, (SOCKADDR*) &socketConfig, sizeof(socketConfig));

    if(ret == SOCKET_ERROR){
        state_ = SocketState::Error;
        //error
        getCore()->printLn("Socket::Connect failed: %d | OS HANDLE: %d |", GetSocketError(), handle_);
        return false;
    }
    else{
        state_ = SocketState::Connected;
    }

    return true;
}

int Socket::Listen(int backlog){
    if(!isValidSocket(handle_)) return -1;
    int ret_val = listen(handle_, backlog);
    if(ret_val == SOCKET_ERROR){
        state_ = SocketState::Error;
        last_error = GetSocketError();
        getCore()->printLn("sendto() %d - SOCKET ERROR: %d", ret_val, GetSocketError());
    }
    else state_ = SocketState::Listening;
    return ret_val; 
}

int Socket::Send(const void* data, size_t size){
    if(!isValidSocket(handle_)) return -1;

    int ret_val = ::send(handle_, reinterpret_cast<const char*>(data), size, 0);
    if(ret_val == SOCKET_ERROR){
        state_ = SocketState::Error;
        last_error = GetSocketError();
    }

    return ret_val;

}
int Socket::Recv(void* buffer, size_t size){
    if(!isValidSocket(handle_)) return -1;
    int ret_val = ::recv(handle_, reinterpret_cast<char*>(buffer), size, 0);
    if(ret_val == SOCKET_ERROR){
        state_ = SocketState::Error;
        last_error = GetSocketError();
    }
    return ret_val;
}
int Socket::SendTo(const void* data, size_t size, const std::string& ip, uint16_t port){
    
    if(!isValidSocket(handle_)) return -1;
    sockaddr_in to;

    std::memset(&to, 0, sizeof(to));

    to.sin_family = AF_INET;

    to.sin_port = htons(port);

    in_addr ip_addr;
    inet_pton(AF_INET, ip.c_str(), &ip_addr);
    

    to.sin_addr = ip_addr;

    
    int ret_val = ::sendto(handle_, reinterpret_cast<const char*>(data), size, 0, reinterpret_cast<SOCKADDR*>(&to), sizeof(to));
    

    if(ret_val == SOCKET_ERROR){
        state_ = SocketState::Error;
        last_error = GetSocketError();
        getCore()->printLn("sendto() %d - SOCKET ERROR: %d", ret_val, GetSocketError());
    }

    return ret_val;
}

int Socket::RecvFrom(void* buffer, size_t size, std::string& outIp, uint16_t& outPort){
    if(!isValidSocket(handle_)) return -1;

    sockaddr_in Recipient;

    socklen_t theLen = sizeof(Recipient);

    int ret_val = ::recvfrom(handle_, reinterpret_cast<char*>(buffer), size, 0, reinterpret_cast<SOCKADDR*>(&Recipient), &theLen);
    
    if(ret_val == SOCKET_ERROR){
        state_ = SocketState::Error;
        last_error = GetSocketError();
    }
    
    char IpOutput[17];

    ::inet_ntop(AF_INET, &Recipient.sin_addr, IpOutput, sizeof(IpOutput));

    outIp = IpOutput;

    sockaddr_in* receivedFrom = reinterpret_cast<sockaddr_in*>(&Recipient);

    outPort = ntohs(receivedFrom->sin_port);

    return ret_val;
}

SOCKET Socket::getOSHandle()
{
    return handle_;
}

int Socket::protocol()
{
    return protocol_;
}

Socket::SocketState Socket::state()
{
    return state_;
}

int Socket::getLastError()
{
    return last_error;
}