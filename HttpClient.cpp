#include "HttpClient.hpp"

#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <cstring>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void HttpClient::init_socket(int &sock, const int &port, std::string host_name)
{
    struct sockaddr_in client;
    hostent * host = gethostbyname(host_name.c_str());

    if (host == NULL || host->h_addr == NULL)
    {
        std::cout << "Error retrieving DNS information\n";
    }

    memset(&client, 0, sizeof(client));
    client.sin_family = AF_INET;
    client.sin_port = htons(port);
    memcpy(&client.sin_addr, host->h_addr, host->h_length);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        std::cout << "Error creating socket\n";
    }

    if (connect(sock, (struct sockaddr *)&client, sizeof(client)) < 0)
    {
        close(sock);
        std::cout << "Could not connect\n";
    }
}

void HttpClient::send_http_request(int &sock)
{
    std::string request = "GET /get\n";

    if (send(sock, request.c_str(), request.length(), 0) != (int)request.length())
    {
        std::cout << "Error sending request\n";
    }
}

void HttpClient::receive_http_response(int &sock, char* &response, size_t &response_size)
{
    const size_t buffer_increase_value = 100;
    char symbol = 0;
    char* buffer = nullptr;
    int buffer_size = 0;

    int i = 0;
    while (read(sock, &symbol, 1) > 0)
    {
        if (i >= buffer_size)
        {
            char* temp_buffer = new char[i + 1 + buffer_increase_value];
            memset(temp_buffer, 0, i + 1 + buffer_increase_value);
            for (int j = 0; j < buffer_size; j++)
            {
                temp_buffer[j] = buffer[j]; 
            }

            delete[] buffer;
            buffer = temp_buffer;
            buffer_size = i + 1 + buffer_increase_value;
        }

        buffer[i] = symbol;
        i++;
    }

    response = buffer;
    response_size = buffer_size;
}

void HttpClient::run(std::vector<std::pair<char*, size_t>> &http_responses, std::mutex &mtx, bool &need_finish)
{
    int sock;
    int port = 80;

    while (true)
    {
        mtx.lock();
        if (need_finish)
        {
            mtx.unlock();
            break;
        }
        mtx.unlock();

        init_socket(sock, port, host);
        send_http_request(sock);

        char* response = nullptr;
        size_t response_size = 0;
        receive_http_response(sock, response, response_size);

        mtx.lock();
        http_responses.push_back(std::make_pair(response, response_size));
        mtx.unlock();

        close(sock);
    }
}
