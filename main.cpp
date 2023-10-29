#include <thread>
#include <mutex>
#include <iostream>
#include <vector>
#include <termios.h>
#include <unistd.h>

#include "HttpClient.hpp"

void check_keystroke(bool &is_ESC_pressed, std::mutex &mtx)
{
    mtx.lock();
    is_ESC_pressed == false;
    mtx.unlock();

    char ESC = 27;
    char ch;
    while (true)
    {
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0)
        {
            perror("tcsetattr()");
        }
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0)
        {
            perror("tcsetattr ICANON");
        }
        if (read(0, &ch, 1) < 0)
        {
            perror ("read()");
        }
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0)
        {
            perror ("tcsetattr ~ICANON");
        }

        if (ch == ESC)
        {
            break;
        }
    }

    mtx.lock();
    is_ESC_pressed = true;
    mtx.unlock();
}

int main()
{
    const std::string host = "httpbin.org";
    std::vector<std::pair<char*, size_t>> http_responses;

    bool need_finish = false;
    HttpClient http_client(host);
    std::mutex http_client_mtx;
    std::thread http_client_thread(&HttpClient::run, http_client, std::ref(http_responses),
        std::ref(http_client_mtx), std::ref(need_finish));

    bool is_ESC_pressed = false;
    std::mutex key_reading_mtx;
    std::thread key_reading_thread(check_keystroke, std::ref(is_ESC_pressed), std::ref(key_reading_mtx));

    while(true)
    {
        // check ESC
        key_reading_mtx.lock();
        if (is_ESC_pressed)
        {
            break;
        }
        key_reading_mtx.unlock();

        http_client_mtx.lock();
        if (http_responses.size() != 0)
        {
            for (auto& it : http_responses)
            {
                for (int i = 0; i < it.second; i++)
                {
                    std::cout << it.first[i];
                }
                std::cout << "\n";
            }
            http_responses.clear();
        }
        http_client_mtx.unlock();
    }

    // finish http_client_thread
    http_client_mtx.lock();
    need_finish = true;
    http_client_mtx.unlock();

    http_client_thread.join();

    std::cout << "Remainig http responses:\n";

    // output remaining http responses
    if (http_responses.size() != 0)
    {
        for (auto& it : http_responses)
        {
            for (int i = 0; i < it.second; i++)
            {
                std::cout << it.first[i];
            }
            std::cout << "\n";
        }
        http_responses.clear();
    }

    key_reading_thread.join();
    return 0;
}
