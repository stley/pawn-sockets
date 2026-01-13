#pragma once
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#include <cstddef>

#include <memory>

#include <string>

#include <errno.h>


#ifdef _WIN32
    using ssize_t = std::ptrdiff_t;
#endif

#ifdef _WIN32
    #define socket_close(s) closesocket(s)
#else
    #define socket_close(s) close(s)
    using int = SOCKET;
    constexpr SOCKET INVALID_SOCKET = -1;
    constexpr int SOCKET_ERROR = -1;
#endif

#include "core.hpp"

inline bool isValidSocket(SOCKET h);


extern int GetSocketError();

class Socket{
public:
    enum class Type {
        NONE,
        UDP,
        TCP
    };

    enum class SocketState {
        Created,
        Closed,
        Bound,
        Listening,
        Connected,
        Invalid,
        Error
    };

    


    explicit Socket(int type = 0);
    ~Socket();

    int Bind(uint16_t port);
    bool Connect(const std::string& ip, uint16_t port);

    //TCP

    int Listen(int backlog = 16);
    std::unique_ptr<Socket> Accept();

    //I/O

    ssize_t Send(const void* data, size_t size);
    ssize_t Recv(void* buffer, size_t size);

    //UDP-only
    ssize_t SendTo(const void* data, size_t size, const std::string& ip, uint16_t port);

    ssize_t RecvFrom(void* buffer, size_t size, std::string& outIp, uint16_t& outPort);


    SOCKET getOSHandle();

    void Close();

    int protocol();

    SocketState state();

    int getLastError();
private:

    sockaddr_in socketConfig;


    SOCKET handle_ = INVALID_SOCKET;

    int protocol_ = 0;
    SocketState state_ = SocketState::Invalid;
    int last_error;
};