#pragma once

#include <sdk.hpp>

// Include the pawn component information.
#include <Server/Components/Pawn/pawn.hpp>
#include "Socket.hpp"

#include <vector>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

using SocketHandle = uint32_t;




struct ProcessedMessage 
{
    size_t recvLen = -1;
    std::string fromIp;
    uint16_t fromPort = -1;
    char buffer[3072];

};

/*struct ClientInfo
{
    std::string fromIp;
    uint16_t fromPort = -1;
};*/

struct ConnectionResponse {
    int pawn_socket_origin = -1;
    //AMX* machine = nullptr;
    bool success = false;
};

struct IncomingData {
    int pawn_socket_origin = -1;
    //AMX* machine = nullptr;
    std::string callback;
    ProcessedMessage result;
    //int timeout_ms = 500;
};


class SocketManager  {

public:
    void start(ICore* core);
    void stop();

    
    int create(int type);

    void destroy(SocketHandle h);


    Socket* get(SocketHandle h);
    
    void dispatch();

    bool getState() { return running_; }
    
private:
    void run();

    ICore* the_core;

    std::vector<std::unique_ptr<Socket>> socketList;
    std::mutex socketList_mutex;

    
    std::atomic<bool> running_ = false;

    std::thread worker_thread;

    std::queue<SocketHandle> DropQueue;

    std::queue<ConnectionResponse> ConnectQueue;

    std::queue<IncomingData> IncomingQueue;
};

extern std::unique_ptr<SocketManager> socket_manager;