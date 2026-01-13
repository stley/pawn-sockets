//Pawn Sockets component


enum SocketType {
	UDP = 1,
	TCP = 2
};

native Socket:socket_create(SocketType:TCP); // UDP-TCP

native socket_bind(Socket:id, port); // UDP-TCP

native socket_connect(Socket:id, const host[], port); // UDP-TCP

native socket_listen(Socket:id, backlog = 16); // UDP-TCP

native socket_destroy(Socket:id); // UDP-TCP

native socket_send(Socket:id, data[], len); // UDP-TCP

native socket_sendto(Socket:id, const ip[], port, const data[], len); //UDP-TCP

native is_socket_valid(Socket:id); // UDP-TCP
native socket_errno(Socket:id);

forward OnIncomingUDP(Socket:id, const data[], data_len, const remote_client_ip[], remote_client_port);
forward OnIncomingTCP(Socket:id, const data[], data_len);