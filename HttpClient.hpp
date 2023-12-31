#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include <string>

class HttpClient
{
private:
    std::string host;
    void init_socket(int &sock, const int &port, std::string host_name);
    void send_http_request(int &sock);
    void receive_http_response(int &sock, char* &response, size_t &response_size);

public:
    HttpClient(const std::string &host)
    {
        this->host = host;
    }
    
    void run(std::vector<std::pair<char*, size_t>> &http_responses, std::mutex &mtx, std::atomic_bool &need_finish, std::exception_ptr &ex_ptr);
};