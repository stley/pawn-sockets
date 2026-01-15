

// Include the pawn component information.
#include <Server/Components/Pawn/pawn.hpp>

// Include pawn-natives macros (`SCRIPT_API`) and lookups (`IPlayer&`).
#include <Server/Components/Pawn/Impl/pawn_natives.hpp>

#include "SocketManager.hpp"
#include "core.hpp"


using namespace Impl;

using std::string;



SCRIPT_API(socket_create, int(int type)){
    if(type < 1 || type > 2) return -1;
    int pawnHandle = socket_manager->create(type);
    return pawnHandle;
}

SCRIPT_API(socket_destroy, int(int socket_handle)){
    Socket* sock = socket_manager->get(socket_handle);
    if(sock == nullptr) return -1;
    socket_manager->destroy(socket_handle);
    return 0;
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
    
    for(int i = 0; i < data.size(); i++){
        getCore()->printLn("%02x - %d - %c", data[i], data[i], data[i]);
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

    return sock->GetLastError();
}
