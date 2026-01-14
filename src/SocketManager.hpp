#pragma once

#include <sdk.hpp>

// Include the pawn component information.
#include <Server/Components/Pawn/pawn.hpp>

#include <vector>

#include <thread>

#include "Socket.hpp"

#include <atomic>

using SocketHandle = uint32_t;




struct ProcessedMessage {
    size_t recvLen = -1;
    std::string fromIp;
    uint16_t fromPort = -1;
    char buffer[3072];

};

struct QueuedResponse {
    int pawn_socket_origin = -1;
    AMX* machine = nullptr;
    std::string callback;
    ProcessedMessage result;
    bool processed = false;
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
    
    std::atomic<bool> running_ = false;

    std::thread worker_thread;

    std::vector<QueuedResponse> Queue;
};

extern std::unique_ptr<SocketManager> socket_manager;