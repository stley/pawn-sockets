/*
 *  This Source Code Form is subject to the terms of the Mozilla Public License,
 *  v. 2.0. If a copy of the MPL was not distributed with this file, You can
 *  obtain one at http://mozilla.org/MPL/2.0/.
 *
 *  The original code is copyright (c) 2022, open.mp team and contributors.
 */

#include "main.hpp"

#include "core.hpp"


ICore* core_ = nullptr;

std::unique_ptr<SocketManager> socket_manager;

using namespace Impl;

using std::string;

ICore* getCore(){
    return core_;
}


SCRIPT_API(socket_create, int(int type)){
    if(type < 1 || type > 2) return -1;
    int pawnHandle = socket_manager->create(type);
    return pawnHandle;
}


SCRIPT_API(socket_bind, int(int socket_handle, int port)){
    Socket* sock = socket_manager->get(socket_handle);
    if(sock == nullptr) return -1;
    return sock->Bind((uint16_t)port);
}

SCRIPT_API(socket_connect, int(int socket_handle, string const& ip, int port)){
    Socket* sock = socket_manager->get(socket_handle);
    if(sock == nullptr) return -1;
    
    return sock->Connect(ip, (uint16_t)port);
}

SCRIPT_API(socket_listen, int(int socket_handle, int backlog)){
    Socket* sock = socket_manager->get(socket_handle);
    if(sock == nullptr) return -1;
    return sock->Listen(backlog);
}


SCRIPT_API(socket_send, int(int socket_handle, string const& data, size_t size)){
    Socket* sock = socket_manager->get(socket_handle);
    if(sock == nullptr) return -1;
    
    return sock->Send(data.data(), size);
}

SCRIPT_API(socket_sendto, int(int socket_handle, string const& ip, int port, string const& data, size_t size)){
    Socket* sock = socket_manager->get(socket_handle);
    if(sock == nullptr) return -1;

    core_->printLn("data size: %d", data.size());
    core_->printLn("data capacity: %d", data.capacity());

    for(int i = 0; i < data.size(); i++){
        core_->printLn("%02x - %d - %c", data[i], data[i], data[i]);
    }

    //SendTo(const void* data, size_t size, const std::string& ip, uint16_t port);

    int socket_ret = sock->SendTo(data.data(), size, ip, (uint16_t)port);
    
    return socket_ret;
}

SCRIPT_API(is_socket_valid, bool(int socket_handle)){
    Socket* sock = socket_manager->get(socket_handle);
    if(sock == nullptr) return false;
    else return true;
}

SCRIPT_API(socket_errno, int(int socket_handle)){
    Socket* sock = socket_manager->get(socket_handle);
    if(sock == nullptr) return -1;

    return sock->getLastError();
}
