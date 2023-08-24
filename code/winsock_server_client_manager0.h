//add -D WINSOCK_SERVER_CLIENT_DEBUG for debug purpose
#ifndef SECURITY_WINSOCK_SERVER_CLIENT_MANAGER0_H
#define SECURITY_WINSOCK_SERVER_CLIENT_MANAGER0_H

#include <winsock2.h>
#include <ws2tcpip.h>
//u have to link library libws2_32.a to use winsock functions;
//#pragma comment(lib, "ws2_32.lib") //or -lws2_32

#include <stdint.h>

#ifdef WINSOCK_SERVER_CLIENT_DEBUG
#include <stdio.h>
#endif

//returns 0 if success; else returns 1;
uint8_t winsock_initialization();
void winsock_free();

struct Winsock_server_structure
{
    SOCKET listen_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    addrinfo* server_info = NULL;
};

struct Winsock_client_structure
{
    SOCKET server_socket = INVALID_SOCKET;
    addrinfo* server_info = NULL;
    bool connected_with_the_server = 0;
};

class Winsock_server
{

public:
    Winsock_server()
    {

    }

    //create server with non-blocking mode;
    //returns 0 if success; else returns varialbe of WSAGetLastError();
    //returns in variables_out listen_socket and server_info if ends with success;
    //to destroy these initialized variables, u have to run the function: "destroy_server";
    int32_t create_server(const char* const local_ip,const char* const local_port,Winsock_server_structure* const variables_out);

    //returns 0 if success; else returns varialbe of WSAGetLastError();
    //returns in variables_in_out client_socket if ends with success;
    //to destroy these initialized variables, u have to run the function: "reject_client";
    int32_t waiting_for_client(Winsock_server_structure* const variables_in_out);

    //function forces extra attempts to receive all data if first attempt failed to received amount of bytes which count_of_elements_to_take_from_client tells about
    //if "delay_per_attempt_in_ms" variable has been set to 0, then there is no delay between failed attempts;
    //if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
    //if success then returns amount of bytes that has been taken from client;
    //also returns to data_out data that has been taken from client;
    int64_t recv_from_client_force(uint8_t* const data_out,const uint32_t count_of_elements_to_take_from_client,const uint32_t amount_of_extra_attempts_to_force,
                                   const uint32_t delay_per_attempt_in_ms,const Winsock_server_structure* const variables_in);

    //function forces extra attempts to send all data if first attempt failed to send amount of bytes which count_of_elements_to_send_to_client tells about
    //if "delay_per_attempt_in_ms" variable has been set to 0, then there is no delay between failed attempts;
    //if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
    //if success then returns amount of bytes that has been sended to client;
    int64_t send_to_client_force(const uint8_t* const data_in,const uint32_t count_of_elements_to_send_to_client,const uint32_t amount_of_extra_attempts_to_force,
                                 const uint32_t delay_per_attempt_in_ms,const Winsock_server_structure* const variables_in);

    //if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
    //if success then returns amount of bytes that has been taken from client;
    //also returns to data_out data that has been taken from client;
    int64_t recv_from_client(uint8_t* const data_out,const uint32_t count_of_elements_to_take_from_client,const Winsock_server_structure* const variables_in);

    //if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
    //if success then returns amount of bytes that has been sended to client;
    int64_t send_to_client(const uint8_t* const data_in,const uint32_t count_of_elements_to_send_to_client,const Winsock_server_structure* const variables_in);

    //run this function to clear memory after sucessfully called function: "create_server";
    void destroy_server(Winsock_server_structure* variables_in);

    //run this function to clear memory after sucessfully called function: "waiting_for_client";
    void reject_client(Winsock_server_structure* variables_in);

    ~Winsock_server()
    {

    }
};

//add -D WINSOCK_SERVER_CLIENT_DEBUG for debug purpose
class Winsock_client
{

public:
    Winsock_client()
    {

    }

    //creates client with non-blocking mode;
    //returns 0 if success; else returns varialbe of WSAGetLastError();
    //returns in variables_out server_socket and server_info if ends with success;
    //to destroy initialized variables, u have to run the function: "close client";
    int32_t create_client(const char* const server_ip,const char* const server_port,Winsock_client_structure* const variables_out);

    //returns 0 if success; else returns varialbe of WSAGetLastError() or -1;
    //returns in variables_in_out connected_with_the_server == 1 if ends with success;
    int32_t try_to_connect(Winsock_client_structure* const variables_in_out);

    //function forces extra attempts to receive all data if first attempt failed to received amount of bytes which count_of_elements_to_take_from_server tells about
    //if "delay_per_attempt_in_ms" variable has been set to 0, then there is no delay between failed attempts;
    //if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
    //if success then returns amount of bytes that has been taken from server;
    //also returns to data_out data that has been taken from server;
    int64_t recv_from_server_force(uint8_t* const data_out,const uint32_t count_of_elements_to_take_from_server,const uint32_t amount_of_extra_attempts_to_force,
                                   const uint32_t delay_per_attempt_in_ms,const Winsock_client_structure* const variables_in);

    //function forces extra attempts to send all data if first attempt failed to send amount of bytes which count_of_elements_to_send_to_server tells about
    //if "delay_per_attempt_in_ms" variable has been set to 0, then there is no delay between failed attempts;
    //if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
    //if success then returns amount of bytes that has been sended to server;
    int64_t send_to_server_force(const uint8_t* const data_in,const uint32_t count_of_elements_to_send_to_server,const uint32_t amount_of_extra_attempts_to_force,
                                 const uint32_t delay_per_attempt_in_ms,const Winsock_client_structure* const variables_in);

    //if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
    //if success then returns amount of bytes that has been taken from server;
    //also returns to data_out data that has been taken from server;
    int64_t recv_from_server(uint8_t* const data_out,const uint32_t count_of_elements_to_take_from_server,const Winsock_client_structure* const variables_in);

    //if function fails then returns -1; then u can run function "WSAGetLastError()" to see error code;
    //if success then returns amount of bytes that has been sended to server;
    int64_t send_to_server(const uint8_t* const data_in,const uint32_t count_of_elements_to_send_to_server,const Winsock_client_structure* const variables_in);

    //run this function after sucessfully called: "create_client";
    void close_client(Winsock_client_structure* variables_in);

    ~Winsock_client()
    {

    }
};

//add -D WINSOCK_SERVER_CLIENT_DEBUG for debug purpose
//<winsock2.h> <ws2tcpip.h> <stdint.h> <stdio.h>;
//"representation_of_string_data_out" must have 16 or more bytes. otherwise, it can crash;
//into "representation_of_string_data_out" will be put string with "end of string char": '\0';
//if error, returns non 0 number, otherwise returns 0;
//if error occurs then it will print string through stderr buffor, but only if u define macro "WINSOCK_SERVER_CLIENT_DEBUG";
//function also will print some information strings through buffor stdout if macro "WINSOCK_SERVER_CLIENT_DEBUG" is definied;
uint8_t get_public_IPv4(uint8_t* const representation_of_string_data_out,uint32_t* const representation_of_int_data_out);

//converts string like this: "192.0.0.1" into uint32_t;
//string must have at the end char '\0' and there must be dots between the numbers;
//returns 0 if success; else returns 1 if there was some syntax problem in data_in;
//else returns 2 if some ip number was higher than 255;
uint8_t convert_IPv4_from_string_to_uint32(const char* const data_in,uint32_t* data_out);

#endif
