#include "core.hpp"
#include "SocketManager.hpp"



#include <chrono>

void SocketManager::start(ICore* core){
    the_core = core;
    the_core->printLn("Socket Manager started.");
    
    #ifdef _WIN32 //windows

    WSADATA wsaData;

    int iResult = 0;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if(iResult != 0){
        return;
    }

    #else //linux


    #endif

    running_ = true;
    worker_thread = std::thread(&SocketManager::run, this);
}

void SocketManager::stop(){
    if(the_core != nullptr) the_core->printLn("Socket Manager stopped.");    
    running_ = false;

    for(SocketHandle i = 0; i < socketList.size(); i++){
        this->destroy(i);
    }

    #ifdef _WIN32
    WSACleanup();
    #endif
    if(worker_thread.joinable())
        worker_thread.join();
}

int SocketManager::create(int type){
    

    for(int i = 0; i < socketList.size(); i++){
        if(socketList[i] == nullptr){

            socketList[i] = std::make_unique<Socket>(type);
            return i;
        }
    }

    socketList.push_back(std::make_unique<Socket>(type));
    return socketList.size()-1;
}

Socket* SocketManager::get(SocketHandle h){
    the_core->printLn("Script tries to get pointer to socket using pawn handle");
    if(h > socketList.size()-1 || !socketList[h]) return nullptr;
    the_core->printLn("Pawn Handle for Socket is VALID.");
    return socketList[h].get();
}

void SocketManager::destroy(SocketHandle h){
    if(h > socketList.size()-1 || !socketList[h]) return;

    socketList[h].reset();
    socketList[h] = nullptr;
}





void SocketManager::run(){
    while(running_){
        //this is running!
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        fd_set checkset;
        FD_ZERO(&checkset);

        for(int s = 0; s < socketList.size(); s++){
            Socket* theSocket = socketList[s].get();
            if(theSocket != nullptr)
            {
                FD_SET(theSocket->getOSHandle(), &checkset);
            }            
        }
        int readyCount;
        timeval time;
        time.tv_sec = 0;
        time.tv_usec = 500000;
        //the_core->printLn("worker thread checking for new responses...");
        readyCount = ::select(0, &checkset, nullptr, nullptr, &time);
        if(!readyCount) continue;
        //the_core->printLn("%d", readyCount);
        for(int s1 = 0; s1 < socketList.size(); s1++){
            Socket* theSocket = socketList[s1].get();

            if(theSocket != nullptr){
                QueuedResponse response;
                SOCKET OSHandle = theSocket->getOSHandle();
                if(FD_ISSET(OSHandle, &checkset)){
                    switch(theSocket->protocol()){
                        case 1: { //UDP 
                            response.result.recvLen = theSocket->RecvFrom(response.result.buffer, sizeof(response.result.buffer), response.result.fromIp, response.result.fromPort);
                            break;
                        }
                        case 2: { //TCP
                            response.result.recvLen = theSocket->Recv(response.result.buffer, sizeof(response.result.buffer));
                            break;
                        }

                    }
                    response.pawn_socket_origin = s1;
                    response.processed = false;
                    Queue.push_back(response);
                }
            }

        }
        continue;
    }
}

void SocketManager::dispatch(){
    if(Queue.empty()) return;
    for(int s = 0; s < Queue.size(); s++)
    {

        QueuedResponse* response = &Queue[s];

        if(response->processed == true) continue;

        Socket* theSocket = SocketManager::get(response->pawn_socket_origin);

        if(theSocket == nullptr)
            continue;
        
        int pawnHandle = response->pawn_socket_origin;
                    
        

        int callIdx;

        cell
            amx_buffer[3072]
        ;

        for(int i = 0; i < sizeof(response->result.buffer); i++)
            amx_buffer[i] = response->result.buffer[i];

        
        switch(theSocket->protocol()){
        case 1: { //UDP 
                for(int scr = 0; scr < g_amxScripts.size(); scr++)
                {
                    cell
                        arr_addr,
                        str_addr,
                        *phys_addr    
                    ;
                    AMX* theScript = g_amxScripts[scr];
                    if(theScript == nullptr) continue;
                    if(response->callback.empty())
                        amx_FindPublic(theScript, "OnIncomingUDP", &callIdx);
                    else
                        amx_FindPublic(theScript, response->callback.c_str(), &callIdx);


                    amx_Push(theScript, (cell)response->result.fromPort);
                    amx_PushString(theScript, &str_addr, NULL, response->result.fromIp.c_str(), NULL, NULL);
                    amx_Push(theScript, (cell)response->result.recvLen);

                    ::amx_PushArray(theScript, &arr_addr, NULL, amx_buffer, sizeof(amx_buffer) / sizeof(cell));

                    amx_Push(theScript, pawnHandle);
                    amx_Exec(theScript, NULL, callIdx);
                    amx_Release(theScript, arr_addr);
                    amx_Release(theScript, str_addr);
                    break;
                }
            }
            case 2: { //TCP
                for(int scr = 0; scr < g_amxScripts.size(); scr++)
                {
                    cell
                        arr_addr,
                        *phys_addr
                    ;
                    AMX* theScript = g_amxScripts[scr];
                    if(theScript == nullptr) continue;
                    if(response->callback.empty())
                        amx_FindPublic(theScript, "OnIncomingTCP", &callIdx);
                    else
                        amx_FindPublic(theScript, response->callback.c_str(), &callIdx);

                    amx_Push(theScript, (cell)response->result.recvLen);

                    ::amx_PushArray(theScript, &arr_addr, NULL, amx_buffer, sizeof(amx_buffer) / sizeof(cell));

                    amx_Push(theScript, pawnHandle);

                    amx_Exec(theScript, NULL, callIdx);
                    amx_Release(theScript, arr_addr);
                    break;
                }
            }
        }
        response->processed = true;
        
    }
    
    Queue.erase(std::remove_if(Queue.begin(), Queue.end(),
        [](const QueuedResponse& r) {
            return r.processed;
        }),
        Queue.end()
    );
}
