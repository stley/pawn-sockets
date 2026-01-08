#include "Socket.hpp"


inline bool isValidSocket(SOCKET h){
    #ifdef _WIN32
        return (h != INVALID_SOCKET);
    #else
        return (h != -1);
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

    if(!isValidSocket(handle_))
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
    #ifdef _WIN32

    if(rt == SOCKET_ERROR){
        std::memset(&socketConfig, 0, sizeof(socketConfig));
        state_ = SocketState::Error;
        getCore()->printLn("socket_bind failed: %d | OS HANDLE: %d", WSAGetLastError(), handle_);
    }

    #else

    if(rt == -1){
        std::memset(&socketConfig, 0, sizeof(socketConfig));
        state_ = SocketState::Error;
        //error

    }

    #endif
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
        getCore()->printLn("socket_connect failed: %d | OS HANDLE: %d", WSAGetLastError(), handle_);
        return false;
    }
    else{
        state_ = SocketState::Connected;
    }

    return true;
}

int Socket::Listen(int backlog){
    if(!isValidSocket(handle_)) return -1;
    return listen(handle_, backlog);
}

int Socket::Send(const void* data, size_t size){
    if(!isValidSocket(handle_)) return -1;

    return ::send(handle_, reinterpret_cast<const char*>(data), size, 0);

}
int Socket::Recv(void* buffer, size_t size){
    if(!isValidSocket(handle_)) return -1;
    return ::recv(handle_, reinterpret_cast<char*>(buffer), size, 0);
}
int Socket::SendTo(const void* data, size_t size, const std::string& ip, uint16_t port){
    
    if(!isValidSocket(handle_)) return -1;
    sockaddr_in to;

    to.sin_family = AF_INET;

    to.sin_port = htons(port);

    in_addr ip_addr;
    inet_pton(AF_INET, ip.c_str(), &ip_addr);
    //ip_addr.s_addr = ::inet_addr(ip.c_str());

    to.sin_addr = ip_addr;

    
    int ret_value = ::sendto(handle_, reinterpret_cast<const char*>(data), size, 0, reinterpret_cast<SOCKADDR*>(&to), sizeof(to));

    //if(ret_value == SOCKET_ERROR) getCore()->printLn("%d", WSAGetLastError());
    getCore()->printLn("sendto() %d - WSAERROR: %d", ret_value, WSAGetLastError());
    return ret_value;
}

int Socket::RecvFrom(void* buffer, size_t size, std::string& outIp, uint16_t& outPort){
    if(!isValidSocket(handle_)) return -1;

    SOCKADDR Recipient;

    socklen_t theLen = sizeof(Recipient);

    int ret_val = ::recvfrom(handle_, reinterpret_cast<char*>(buffer), size, 0, &Recipient, &theLen);
    
    
    //if(ret_val == SOCKET_ERROR) 

    //sockaddr_in receivedFrom = reinterpret_cast<sockaddr_in>(Recipient);
    
    char IpOutput[17];

    ::inet_ntop(AF_INET, &Recipient, IpOutput, sizeof(IpOutput));

    outIp = IpOutput;

    sockaddr_in* receivedFrom = reinterpret_cast<sockaddr_in*>(&Recipient);

    outPort = ntohs(receivedFrom->sin_port);

    return ret_val;
}

SOCKET Socket::getOSHandle(){
    return handle_;
}

int Socket::protocol(){
    return protocol_;
}

Socket::SocketState Socket::state(){
    return state_;
}