#include "core.hpp"
#include "SocketManager.hpp"

std::unique_ptr<SocketManager> socket_manager;

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
    std::lock_guard<std::mutex> lock(socketList_mutex);


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
    std::lock_guard<std::mutex> lock(socketList_mutex);
    if(h >= socketList.size()-1 || !socketList[h]) return nullptr;
    return socketList[h].get();
}

void SocketManager::destroy(SocketHandle h){
    std::lock_guard<std::mutex> lock(socketList_mutex);
    if(h > socketList.size()-1 || !socketList[h]) return;

    socketList[h].reset();
    socketList[h] = nullptr;
}





void SocketManager::run(){
    while(running_){
        //this is running!
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        fd_set readSet;
        FD_ZERO(&readSet);
        fd_set writeSet;
        FD_ZERO(&writeSet);

        int max_FD = 0;

        {
            std::lock_guard<std::mutex> lock(socketList_mutex);
            for(int s = 0; s < socketList.size(); s++){
                Socket* theSocket = socketList[s].get();
                if(theSocket != nullptr)
                {
                    if(theSocket->GetState() == Socket::SocketState::Connecting){
                        FD_SET(theSocket->GetOSHandle(), &writeSet);
                        max_FD++;
                        continue;
                    }
                    if(theSocket->GetState() == Socket::SocketState::Connected || theSocket->GetProtocol() == 1){
                        FD_SET(theSocket->GetOSHandle(), &readSet);
                        max_FD++;
                    }


                }            
            }
        }
        int readyCount;
        timeval time;
        time.tv_sec = 0;
        time.tv_usec = 500000;

        

        readyCount = ::select(max_FD, &readSet, &writeSet, nullptr, &time);
        if(!readyCount) continue;

        {
            std::lock_guard<std::mutex> lock(socketList_mutex);
            for(int s1 = 0; s1 < socketList.size(); s1++)
            {
                Socket* theSocket = socketList[s1].get();
                if(theSocket == nullptr) continue;
                
                SOCKET OSHandle = theSocket->GetOSHandle();
                if(FD_ISSET(OSHandle, &writeSet))
                {
                    //Socket is trying to connect to somewhere
                    if(theSocket->GetState() == Socket::SocketState::Connecting){
                        int err = 0;
                        socklen_t len = sizeof(err);
                        ::getsockopt(theSocket->GetOSHandle(), SOL_SOCKET, SO_ERROR, (char*)(&err), &len);
                        ConnectionResponse response;
                        response.pawn_socket_origin = s1;
                        response.success = (err == 0);
                        if(!err) theSocket->SetState(Socket::SocketState::Connected);
                        else
                        {
                            theSocket->SetState(Socket::SocketState::Error);
                            theSocket->SetLastError(err);
                        }

                        ConnectQueue.push(response);

                    }
                }
                if(FD_ISSET(OSHandle, &readSet))
                {
                    //Data incoming
                    

                    switch(theSocket->GetProtocol())
                    {
                        case 1: 
                        { //UDP 
                            IncomingData response;
                            response.result.recvLen = theSocket->RecvFrom(response.result.buffer, sizeof(response.result.buffer), response.result.fromIp, response.result.fromPort);
                            response.pawn_socket_origin = s1;
                            IncomingQueue.push(response);
                            break;
                        }
                        case 2: 
                        { //TCP
                            if(theSocket->GetState() == Socket::SocketState::Connected){
                                IncomingData response;
                                response.result.recvLen = theSocket->Recv(response.result.buffer, sizeof(response.result.buffer));
                                if(response.result.recvLen != 0) //Disconnected
                                {
                                    response.pawn_socket_origin = s1;
                                    IncomingQueue.push(response);
                                }
                                else
                                {
                                    theSocket->SetState(Socket::SocketState::Disconnected);
                                    DropQueue.push(s1); 
                                }
                                
                            }                
                            break;
                        }
                    }
                    
                }
            }
        }
        continue;
    }
}

void SocketManager::dispatch()
{
    //handling incoming data
    
    if(!IncomingQueue.empty()){
        IncomingData* response = &IncomingQueue.front();

        Socket* theSocket = nullptr;
        int protocol;
        {
            std::lock_guard<std::mutex> lock(socketList_mutex);
            if(response->pawn_socket_origin < socketList.size() && 
               socketList[response->pawn_socket_origin]) {
                theSocket = socketList[response->pawn_socket_origin].get();
                if(theSocket == nullptr) IncomingQueue.pop();
                protocol = theSocket->GetProtocol();
            }
        }
        

        int pawnHandle = response->pawn_socket_origin;

        int callIdx;

        cell
            amx_buffer[3072]
        ;

        for(int i = 0; i < sizeof(response->result.buffer); i++)
            amx_buffer[i] = response->result.buffer[i];

        
        switch(protocol){
        case 1: 
            { //UDP 
                for(int scr = 0; scr < g_amxScripts.size(); scr++)
                {
                    cell
                        arr_addr,
                        str_addr   
                    ;
                    AMX* theScript = g_amxScripts[scr];
                    if(theScript == nullptr) continue;
                    if(amx_FindPublic(theScript, "OnIncomingUDP", &callIdx) != AMX_ERR_NONE) continue;


                    //forward OnIncomingUDP(Socket:id, const data[], data_len, const remote_client_ip[], remote_client_port);
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
            case 2: 
            { //TCP
                for(int scr = 0; scr < g_amxScripts.size(); scr++)
                {
                    cell
                        arr_addr
                    ;
                    AMX* theScript = g_amxScripts[scr];
                    if(theScript == nullptr) continue;
                    if(amx_FindPublic(theScript, "OnIncomingTCP", &callIdx) != AMX_ERR_NONE) continue;
                    
                
                    //forward OnIncomingTCP(Socket:id, const data[], data_len);
                    //data_len
                    amx_Push(theScript, (cell)response->result.recvLen);

                    //data[]
                    ::amx_PushArray(theScript, &arr_addr, NULL, amx_buffer, sizeof(amx_buffer) / sizeof(cell));

                    //Socket:id (handle)
                    amx_Push(theScript, pawnHandle);

                    amx_Exec(theScript, NULL, callIdx);
                    amx_Release(theScript, arr_addr);
                    break;
                }
            }
        }
        IncomingQueue.pop();
    }
    


    if(!ConnectQueue.empty())
    {
        ConnectionResponse* conn_response = &ConnectQueue.front();
        int callIdx;
        for(int scr = 0; scr < g_amxScripts.size(); scr++)
        {
            AMX* theScript = g_amxScripts[scr];
            if(theScript == nullptr) continue;

            if(amx_FindPublic(theScript, "OnSocketConnect", &callIdx) != AMX_ERR_NONE) continue;


            amx_Push(theScript, (cell)conn_response->success);
            amx_Push(theScript, (cell)conn_response->pawn_socket_origin);

            amx_Exec(theScript, NULL, callIdx);

        }

        ConnectQueue.pop();
    }    


    if(!DropQueue.empty())
    {
        SocketHandle theHandle = DropQueue.front();

        int callIdx;
        for(int scr = 0; scr < g_amxScripts.size(); scr++)
        {
            AMX* theScript = g_amxScripts[scr];
            if(theScript == nullptr) continue;

            if(amx_FindPublic(theScript, "OnSocketDisconnect", &callIdx) != AMX_ERR_NONE) continue;

            amx_Push(theScript, (cell)theHandle);

            amx_Exec(theScript, NULL, callIdx);

        }

        DropQueue.pop();
    }

    return;
}
