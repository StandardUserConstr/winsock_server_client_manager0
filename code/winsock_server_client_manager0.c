#include "winsock_server_client_manager0.h"

//returns 0 if success; else returns 1;
uint8_t winsock_initialization()
{
    WSADATA wsa;
    if(WSAStartup(514,&wsa)!=0) //514 means to initialize the latest version of winsock library
    {
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"WinSock initialization failed: %d\n",WSAGetLastError());
        #endif
        return 1;
    }
    return 0;
}

void winsock_free()
{
    WSACleanup();
    return;
}



//Winsock_server
//================================================================================================================


//create server with non-blockign mode
//returns 0 if success; else returns varialbe of WSAGetLastError();
//returns in variables_out listen_socket and server_info if ends with success;
int32_t Winsock_server::create_server(const char* const local_ip,const char* const local_port,Winsock_server_structure* const variables_out)
{
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("Winsock_server::create_server\n");
    printf("    local address at: %s\n    local port at: %s\n",local_ip,local_port);
    #endif

    addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = INADDR_ANY;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;


    if(getaddrinfo(local_ip,local_port,&hints,&variables_out->server_info)!=0)
    {
        int32_t error = WSAGetLastError();
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"Error while converting address and port: %d\ncheck if local ip and local port are valid\n",error);
        #endif
        return error;
    }

    variables_out->listen_socket = socket(variables_out->server_info->ai_family,variables_out->server_info->ai_socktype,variables_out->server_info->ai_protocol);
    if((variables_out->listen_socket) == INVALID_SOCKET)
    {
        int32_t error = WSAGetLastError();
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"Could not create socket: %d\ncheck if local ip and local port are valid\n",error);
        #endif
        return error;
    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    Socket created\n");
    #endif

    if(bind(variables_out->listen_socket,variables_out->server_info->ai_addr,variables_out->server_info->ai_addrlen)==SOCKET_ERROR)
    {
        int32_t error = WSAGetLastError();
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"Binding socket with ip and port failed with error: %d\ncheck if local ip and local port are valid\n", error);
        #endif
        return error;
    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    Binding success\n");
    printf("    Server created\n");
    #endif

    if(listen(variables_out->listen_socket,SOMAXCONN/*or use SOMAXCONN*/)==SOCKET_ERROR)  //SOMAXCONN means maximum number of clients in the queue
    {
        int32_t error = WSAGetLastError();
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"Listen failed with error: %d\n",error);
        #endif
        return error;
    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    Server is listening\n");
    #endif

    u_long iMode = 1;
    if(ioctlsocket(variables_out->listen_socket,FIONBIO,&iMode)!=0)
    {
        int32_t error = WSAGetLastError();
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"ioctlsocket failed with error: %d\n",error);
        #endif
        return error;
    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    Server mode has been changed to non-blocking mode\n\n");
    #endif

    return 0;
}

//returns 0 if success; else returns varialbe of WSAGetLastError();
//returns in variables_in_out client_socket if ends with success;
int32_t Winsock_server::waiting_for_client(Winsock_server_structure* const variables_out)
{
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("Winsock_server::waiting_for_client\n");
    #endif
    variables_out->client_socket = accept(variables_out->listen_socket,NULL,NULL);
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    if(variables_out->client_socket!=INVALID_SOCKET)
    {
        return 0;
    }
    else
    {
        int32_t error = WSAGetLastError();
        fprintf(stderr,"Winsock_server::waiting_for_client failed with error: %d\n\n",error);
        return error;
    }
    #else
    if(variables_out->client_socket!=INVALID_SOCKET) return 0;
    else return WSAGetLastError();
    #endif
}

//function forces extra attempts to receive all data if first attempt failed to received amount of bytes which count_of_elements_to_take_from_client tells about
//if "delay_per_attempt_in_ms" variable has been set to 0, then there is no delay between failed attempts;
//if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
//if success then returns amount of bytes that has been taken from client;
//also returns to data_out data that has been taken from client;
int64_t Winsock_server::recv_from_client_force(uint8_t* const data_out,const uint32_t count_of_elements_to_take_from_client,const uint32_t amount_of_extra_attempts_to_force,
                                   const uint32_t delay_per_attempt_in_ms,const Winsock_server_structure* const variables_in)
{
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("Winsock_server::recv_from_client_force\n");
    #endif
    if(count_of_elements_to_take_from_client==0) return 0;
    int64_t counter = 0;
    uint32_t total_counter = 0;
    uint32_t count_already_taken = count_of_elements_to_take_from_client;
    int32_t error;
    for(uint32_t i = 0; i!=amount_of_extra_attempts_to_force; i++)
    {
        counter = recv(variables_in->client_socket,(char*)data_out+total_counter,count_already_taken,0);
        if(counter==SOCKET_ERROR)
        {
            error = WSAGetLastError();
            if(error!=WSAEWOULDBLOCK)
            {
                #ifdef WINSOCK_SERVER_CLIENT_DEBUG
                fprintf(stderr,"Winsock_server::recv_from_client_force failed with error: %d\n\n",error);
                return -1;
                #else
                return -1;
                #endif
            }
            if(delay_per_attempt_in_ms!=0) Sleep(delay_per_attempt_in_ms);
            continue;
        }
        total_counter+=counter;
        count_already_taken-=counter;
        if(total_counter<count_of_elements_to_take_from_client)
        {
            if(counter==0&&delay_per_attempt_in_ms!=0) Sleep(delay_per_attempt_in_ms);
            continue;
        }
        else
        {
            #ifdef WINSOCK_SERVER_CLIENT_DEBUG
            printf("    returned %zd after %zd attempts\n\n",total_counter,i);
            #endif
            return total_counter;
        }

    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    returned %zd after %zd attempts\n\n",total_counter,amount_of_extra_attempts_to_force);
    #endif
    return total_counter;
}


//function forces extra attempts to send all data if first attempt failed to send amount of bytes which count_of_elements_to_send_to_client tells about
//if "delay_per_attempt_in_ms" variable has been set to 0, then there is no delay between failed attempts;
//if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
//if success then returns amount of bytes that has been sended to client;
int64_t Winsock_server::send_to_client_force(const uint8_t* const data_in,const uint32_t count_of_elements_to_send_to_client,const uint32_t amount_of_extra_attempts_to_force,
                                             const uint32_t delay_per_attempt_in_ms,const Winsock_server_structure* const variables_in)
{
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("Winsock_server::send_to_client_force\n");
    #endif
    if(count_of_elements_to_send_to_client==0) return 0;

    uint32_t total_counter = 0;
    uint32_t count_already_sended = count_of_elements_to_send_to_client;
    int64_t counter = 0;
    int32_t error;
    for(uint32_t i = 0; i!=amount_of_extra_attempts_to_force; i++)
    {
        counter = send(variables_in->client_socket,(char*)data_in+total_counter,count_already_sended,0);
        if(counter==SOCKET_ERROR)
        {
            error = WSAGetLastError();
            if(error!=WSAEWOULDBLOCK)
            {
                #ifdef WINSOCK_SERVER_CLIENT_DEBUG
                fprintf(stderr,"Winsock_server::send_to_client_force failed with error: %d\n\n",error);
                return -1;
                #else
                return -1;
                #endif
            }
            if(delay_per_attempt_in_ms!=0) Sleep(delay_per_attempt_in_ms);
            continue;
        }
        count_already_sended-=counter;
        total_counter+=counter;
        if(total_counter<count_of_elements_to_send_to_client)
        {
            if(counter==0&&delay_per_attempt_in_ms!=0) Sleep(delay_per_attempt_in_ms);
            continue;
        }
        else
        {
            #ifdef WINSOCK_SERVER_CLIENT_DEBUG
            printf("    returned %zd after %zd attempts\n\n",total_counter,i);
            #endif
            return total_counter;
        }
    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    returned %zd after %zd attempts\n\n",total_counter,amount_of_extra_attempts_to_force);
    #endif
    return total_counter;
}

//if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
//if success then returns amount of bytes that has been taken from client;
//also returns to data_out data that has been taken from client;
int64_t Winsock_server::recv_from_client(uint8_t* const data_out,const uint32_t count_of_elements_to_take_from_client,const Winsock_server_structure* const variables_in)
{
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("Winsock_server::recv_from_client\n");
    #endif
    int64_t counter = 0;
    counter = recv(variables_in->client_socket,(char*)data_out,count_of_elements_to_take_from_client,0);
    if(counter==SOCKET_ERROR )
    {
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        int32_t error = WSAGetLastError();
        fprintf(stderr,"Winsock_server::recv_from_client failed with error: %d\n\n",error);
        return -1;
        #else
        return -1;
        #endif
    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    returned %lld\n\n",counter);
    #endif
    return counter;
}

//if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
//if success then returns amount of bytes that has been sended to client;
int64_t Winsock_server::send_to_client(const uint8_t* const data_in,const uint32_t count_of_elements_to_send_to_client,const Winsock_server_structure* const variables_in)
{
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("Winsock_server::send_to_client\n");
    #endif
    int64_t counter = 0;
    counter = send(variables_in->client_socket,(char*)data_in,count_of_elements_to_send_to_client,0);
    if(counter==SOCKET_ERROR )
    {
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        int32_t error = WSAGetLastError();
        fprintf(stderr,"Winsock_server::send_to_client failed with error: %d\n\n",error);
        return -1;
        #else
        return -1;
        #endif
    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    returned %lld\n\n",counter);
    #endif
    return counter;
}

void Winsock_server::destroy_server(Winsock_server_structure* variables_in)
{
    if(variables_in->server_info!=NULL)
    {
        freeaddrinfo(variables_in->server_info);
        variables_in->server_info = NULL;
    }
    if(variables_in->listen_socket!=INVALID_SOCKET)
    {
        closesocket(variables_in->listen_socket);
        variables_in->listen_socket = INVALID_SOCKET;
    }
    return;

}

void Winsock_server::reject_client(Winsock_server_structure* variables_in)
{
    if(variables_in->client_socket!=INVALID_SOCKET)
    {
        closesocket(variables_in->client_socket);
        variables_in->client_socket = INVALID_SOCKET;
    }
    return;
}
//================================================================================================================





//Winsock_client
//================================================================================================================


//creates client with non-blocking mode;
//returns 0 if success; else returns varialbe of WSAGetLastError();
//returns in variables_out server_socket and server_info if ends with success;
int32_t Winsock_client::create_client(const char* const server_ip,const char* const server_port,Winsock_client_structure* const variables_out)
{
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("Winsock_client::create_client\n");
    printf("    server address at: %s\n    server port at: %s\n",server_ip,server_port);
    #endif

    variables_out->connected_with_the_server = 0;

    struct addrinfo hints;
    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if(getaddrinfo(server_ip,server_port,&hints,&variables_out->server_info)!=0)
    {
        int32_t error = WSAGetLastError();
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"Error while converting address and port: %d\n",error);
        #endif
        return error;
    }

    variables_out->server_socket = socket(variables_out->server_info->ai_family,variables_out->server_info->ai_socktype,variables_out->server_info->ai_protocol);
    if((variables_out->server_socket) == INVALID_SOCKET)
    {
        int32_t error = WSAGetLastError();
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"Could not create socket: %d\n",error);
        #endif
        return error;
    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    Socket created\n");
    #endif

    u_long iMode = 1;
    if(ioctlsocket(variables_out->server_socket,FIONBIO,&iMode)!=0)
    {
        int32_t error = WSAGetLastError();
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"ioctlsocket failed with error: %d\n",error);
        #endif
        return error;
    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    Client mode has been changed to non-blocking mode\n\n");
    #endif

    return 0;
}

//returns 0 if success; else returns variable of WSAGetLastError() or -1;
//returns in variables_in_out connected_with_the_server == 1 if ends with success;
int32_t Winsock_client::try_to_connect(Winsock_client_structure* const variables_in_out)
{
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("Winsock_client::try_to_connect\n");
    #endif
    FD_SET write_set;
    FD_ZERO(&write_set);
    FD_SET(variables_in_out->server_socket,&write_set);

    if(variables_in_out->server_socket!=INVALID_SOCKET) connect(variables_in_out->server_socket,variables_in_out->server_info->ai_addr,variables_in_out->server_info->ai_addrlen);
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    Attempt of connection has been taken\n");
    #endif

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec= 0;
    if(select(0,NULL,&write_set,NULL,&timeout)==SOCKET_ERROR)
    {
        int32_t error = WSAGetLastError();
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"select failed with error: %d\n\n",error);
        #endif
        return error;
    }
    if(FD_ISSET(variables_in_out->server_socket,&write_set))
    {
        FD_CLR(variables_in_out->server_socket,&write_set);
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        printf("    Attempt success\n");
        printf("    Connected with the server\n\n");
        #endif
        variables_in_out->connected_with_the_server = 1;
        return 0;
    }
    else
    {
        FD_CLR(variables_in_out->server_socket,&write_set);
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        printf("    Attempt failed\n\n");
        #endif
        return -1;
    }

}

//function forces extra attempts to receive all data if first attempt failed to received amount of bytes which count_of_elements_to_take_from_server tells about
//if "delay_per_attempt_in_ms" variable has been set to 0, then there is no delay between failed attempts;
//if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
//if success then returns amount of bytes that has been taken from server;
//also returns to data_out data that has been taken from server;
int64_t Winsock_client::recv_from_server_force(uint8_t* const data_out,const uint32_t count_of_elements_to_take_from_server,const uint32_t amount_of_extra_attempts_to_force,
                                               const uint32_t delay_per_attempt_in_ms,const Winsock_client_structure* const variables_in)
{
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("Winsock_client::recv_from_server_force\n");
    #endif
    if(count_of_elements_to_take_from_server==0) return 0;
    int64_t counter = 0;
    uint32_t total_counter = 0;
    uint32_t count_already_taken = count_of_elements_to_take_from_server;
    int32_t error;
    for(uint32_t i = 0; i!=amount_of_extra_attempts_to_force; i++)
    {
        counter = recv(variables_in->server_socket,(char*)data_out+total_counter,count_already_taken,0);
        if(counter==SOCKET_ERROR)
        {
            error = WSAGetLastError();
            if(error!=WSAEWOULDBLOCK)
            {
                #ifdef WINSOCK_SERVER_CLIENT_DEBUG
                fprintf(stderr,"Winsock_client::recv_from_server_force failed with error: %d\n\n",error);
                return -1;
                #else
                return -1;
                #endif
            }
            if(delay_per_attempt_in_ms!=0) Sleep(delay_per_attempt_in_ms);
            continue;
        }
        total_counter+=counter;
        count_already_taken-=counter;
        if(total_counter<count_of_elements_to_take_from_server)
        {
            if(counter==0&&delay_per_attempt_in_ms!=0) Sleep(delay_per_attempt_in_ms);
            continue;
        }
        else
        {
            #ifdef WINSOCK_SERVER_CLIENT_DEBUG
            printf("    returned %zd after %zd attempts\n\n",total_counter,i);
            #endif
            return total_counter;
        }

    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    returned %zd after %zd attempts\n\n",total_counter,amount_of_extra_attempts_to_force);
    #endif
    return total_counter;
}


//function forces extra attempts to send all data if first attempt failed to send amount of bytes which count_of_elements_to_send_to_server tells about
//if "delay_per_attempt_in_ms" variable has been set to 0, then there is no delay between failed attempts;
//if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
//if success then returns amount of bytes that has been sended to server;
int64_t Winsock_client::send_to_server_force(const uint8_t* const data_in,const uint32_t count_of_elements_to_send_to_server,const uint32_t amount_of_extra_attempts_to_force,
                                             const uint32_t delay_per_attempt_in_ms,const Winsock_client_structure* const variables_in)
{
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("Winsock_client::send_to_server_force\n");
    #endif
    if(count_of_elements_to_send_to_server==0) return 0;

    uint32_t total_counter = 0;
    uint32_t count_already_sended = count_of_elements_to_send_to_server;
    int64_t counter = 0;
    int32_t error;
    for(uint32_t i = 0; i!=amount_of_extra_attempts_to_force; i++)
    {
        counter = send(variables_in->server_socket,(char*)data_in+total_counter,count_already_sended,0);
        if(counter==SOCKET_ERROR)
        {
            error = WSAGetLastError();
            if(error!=WSAEWOULDBLOCK)
            {
                #ifdef WINSOCK_SERVER_CLIENT_DEBUG
                fprintf(stderr,"Winsock_client::send_to_server_force failed with error: %d\n\n",error);
                return -1;
                #else
                return -1;
                #endif
            }
            if(delay_per_attempt_in_ms!=0) Sleep(delay_per_attempt_in_ms);
            continue;
        }
        count_already_sended-=counter;
        total_counter+=counter;
        if(total_counter<count_of_elements_to_send_to_server)
        {
            if(counter==0&&delay_per_attempt_in_ms!=0) Sleep(delay_per_attempt_in_ms);
            continue;
        }
        else
        {
            #ifdef WINSOCK_SERVER_CLIENT_DEBUG
            printf("    returned %zd after %zd attempts\n\n",total_counter,i);
            #endif
            return total_counter;
        }
    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    returned %zd after %zd attempts\n\n",total_counter,amount_of_extra_attempts_to_force);
    #endif
    return total_counter;
}

//if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
//if success then returns amount of bytes that has been taken from server;
//also returns to data_out data that has been taken from server;
int64_t Winsock_client::recv_from_server(uint8_t* const data_out,const uint32_t count_of_elements_to_take_from_server,const Winsock_client_structure* const variables_in)
{
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("Winsock_client::recv_from_server\n");
    #endif
    int64_t counter = 0;
    counter = recv(variables_in->server_socket,(char*)data_out,count_of_elements_to_take_from_server,0);
    if(counter==SOCKET_ERROR )
    {
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        int error = WSAGetLastError();
        fprintf(stderr,"Winsock_client::recv_from_server failed with error: %d\n\n",error);
        return -1;
        #else
        return -1;
        #endif
    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    returned %lld\n\n",counter);
    #endif
    return counter;
}

//if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
//if success then returns amount of bytes that has been sended to server;
int64_t Winsock_client::send_to_server(const uint8_t* const data_in,const uint32_t count_of_elements_to_send_to_server,const Winsock_client_structure* const variables_in)
{
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("Winsock_client::send_to_server\n");
    #endif
    int64_t counter = 0;
    counter = send(variables_in->server_socket,(char*)data_in,count_of_elements_to_send_to_server,0);
    if(counter==SOCKET_ERROR )
    {
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        int32_t error = WSAGetLastError();
        fprintf(stderr,"Winsock_client::send_to_server failed with error: %d\n\n",error);
        return -1;
        #else
        return -1;
        #endif
    }
    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    returned %lld\n\n",counter);
    #endif
    return counter;
}

void Winsock_client::close_client(Winsock_client_structure* variables_in)
{
    if(variables_in->server_info!=NULL)
    {
        freeaddrinfo(variables_in->server_info);
        variables_in->server_info = NULL;
    }
    if(variables_in->server_socket!=INVALID_SOCKET)
    {
        closesocket(variables_in->server_socket);
        variables_in->server_socket = INVALID_SOCKET;
    }
    variables_in->connected_with_the_server = 0;
    return;
}


uint8_t get_public_IPv4(uint8_t* const representation_of_string_data_out,uint32_t* const representation_of_int_data_out)
{
    struct addrinfo hints = {0}, *addrs;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("get_public_IPv4:\n    receiving information from api.bigdatacloud.net/data/client-ip\n");
    #endif
    int32_t error_buffor = getaddrinfo("api.bigdatacloud.net", "80", &hints, &addrs);
    if(error_buffor!=0)
    {
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"getaddrinfo failed: %s\n\n",gai_strerror(error_buffor));
        #endif
        return 1;
    }

    int32_t socket0 = socket(addrs->ai_family,addrs->ai_socktype,addrs->ai_protocol);
    if(socket0 == -1)
    {
        freeaddrinfo(addrs);
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"socket failed\n\n");
        #endif
        return 1;
    }
    error_buffor = connect(socket0,addrs->ai_addr,addrs->ai_addrlen);
    if(error_buffor == -1)
    {
        closesocket(socket0);
        freeaddrinfo(addrs);
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"connect failed\n\n");
        #endif
        return 1;
    }
    char request[] = "GET /data/client-ip HTTP/1.1\r\nHost: api.bigdatacloud.net\r\n\r\n";  //whatismyip.akamai.com is alternative
    error_buffor = send(socket0,request,strlen(request),0);
    if(error_buffor==-1)
    {
        closesocket(socket0);
        freeaddrinfo(addrs);
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"send failed\n\n");
        #endif
        return 1;
    }

    char* buffor = (char*)malloc(512);
    error_buffor = recv(socket0, buffor, 512, 0);
    if(error_buffor == -1)
    {
        closesocket(socket0);
        freeaddrinfo(addrs);
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"recv failed\n\n");
        #endif
        free(buffor);
        return 1;
    }
    buffor[error_buffor] = '\0';

    int32_t counter_searching0 = error_buffor - 112;
    if(counter_searching0<0)
    {
        closesocket(socket0);
        freeaddrinfo(addrs);
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"failed while receiving ip from buffor\n\n");
        #endif
        free(buffor);
        return 1;
    }

    for(; counter_searching0!=error_buffor; counter_searching0++)
    {
        if(buffor[counter_searching0]==':'&&buffor[counter_searching0+1]==32&&buffor[counter_searching0+2]=='\"')
        {
            counter_searching0+=3;
            if(!(buffor[counter_searching0]>47&&buffor[counter_searching0]<58))
            {
                closesocket(socket0);
                freeaddrinfo(addrs);
                #ifdef WINSOCK_SERVER_CLIENT_DEBUG
                fprintf(stderr,"failed while receiving ip from buffor\n\n");
                #endif
                free(buffor);
                return 1;
            }
            break;
        }
    }
    if(counter_searching0==error_buffor)
    {
        closesocket(socket0);
        freeaddrinfo(addrs);
        #ifdef WINSOCK_SERVER_CLIENT_DEBUG
        fprintf(stderr,"failed while receiving ip from buffor\n\n");
        #endif
        free(buffor);
        return 1;
    }
    int32_t counter_searching1 = 0;
    for(; buffor[counter_searching0]!='\"'; counter_searching0++,counter_searching1++)
    {
        representation_of_string_data_out[counter_searching1] = buffor[counter_searching0];
    }
    representation_of_string_data_out[counter_searching1] = '\0';


    int32_t result = 0;
    for(int32_t i = 0,counter_string = 0; ; i++)
    {

        result = 0;
        for(; representation_of_string_data_out[counter_string]!='.'&&representation_of_string_data_out[counter_string]!='\0'; counter_string++)
        {
            if(representation_of_string_data_out[counter_string]>='0'&&representation_of_string_data_out[counter_string]<='9')
            {
                result = result*10+representation_of_string_data_out[counter_string]-'0';
            }
            else
            {
                closesocket(socket0);
                freeaddrinfo(addrs);
                #ifdef WINSOCK_SERVER_CLIENT_DEBUG
                fprintf(stderr,"failed while receiving ip from buffor\n\n");
                #endif
                free(buffor);
                return 1;
            }

        }
        *((uint8_t*)representation_of_int_data_out+i) = result;

        if(representation_of_string_data_out[counter_string]=='\0')
        {
            if(i!=3)
            {
                closesocket(socket0);
                freeaddrinfo(addrs);
                #ifdef WINSOCK_SERVER_CLIENT_DEBUG
                fprintf(stderr,"failed while receiving ip from buffor\n\n");
                #endif
                free(buffor);
                return 1;
            }
            break;
        }
        counter_string++;
    }

    #ifdef WINSOCK_SERVER_CLIENT_DEBUG
    printf("    successively received: %zd.%zd.%zd.%zd\n\n",*((uint8_t*)representation_of_int_data_out),
           *((uint8_t*)representation_of_int_data_out+1),*((uint8_t*)representation_of_int_data_out+2),
           *((uint8_t*)representation_of_int_data_out+3));
    #endif

    freeaddrinfo(addrs);
    closesocket(socket0);
    free(buffor);
    return 0;
}

uint8_t convert_IPv4_from_string_to_uint32(const char* const data_in,uint32_t* data_out)
{
    int32_t result = 0;
    for(int32_t i = 0,counter_string = 0; ; i++)
    {
        result = 0;
        for(; data_in[counter_string]!='.'&&data_in[counter_string]!='\0'; counter_string++)
        {
            if(data_in[counter_string]>='0'&&data_in[counter_string]<='9')
            {
                result = result*10+data_in[counter_string]-'0';
            }
            else
            {
                return 1;
            }

        }
        if(result>255) return 2;
        *((uint8_t*)data_out+i) = result;

        if(data_in[counter_string]=='\0')
        {
            if(i!=3)
            {
                return 1;
            }
            break;
        }
        counter_string++;
    }

    int32_t counter0 = 0;
    if(data_in[counter0]=='0'&&data_in[counter0+1]!='.') return 1;
    for(; data_in[counter0]!='.'; counter0++) {}
    counter0++;
    if(data_in[counter0]=='0'&&data_in[counter0+1]!='.') return 1;
    for(; data_in[counter0]!='.'; counter0++) {}
    counter0++;
    if(data_in[counter0]=='0'&&data_in[counter0+1]!='.') return 1;
    for(; data_in[counter0]!='.'; counter0++) {}
    counter0++;
    if(data_in[counter0]=='0'&&data_in[counter0+1]!='\0') return 1;

    return 0;
}
