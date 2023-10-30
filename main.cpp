#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <vector>
#include <exception>
#include <termios.h>
#include <unistd.h>

#include "HttpClient.hpp"

void check_keystroke(std::atomic_bool &is_ESC_pressed, std::mutex &mtx, std::exception_ptr &ex_ptr, std::atomic_bool &need_finish)
{
    char ESC = 27;
    char ch = 0;
    while (!need_finish)
    {
        try
        {
            struct termios old = {0};
            if (tcgetattr(0, &old) < 0)
            {
                throw std::runtime_error("tcsetattr()");
            }
            
            old.c_lflag &= ~ICANON;
            old.c_lflag &= ~ECHO;
            old.c_cc[VMIN] = 1;
            old.c_cc[VTIME] = 0;
            
            if (tcsetattr(0, TCSANOW, &old) < 0)
            {
                throw std::runtime_error("tcsetattr ICANON");
            }
            if (read(0, &ch, 1) < 0)
            {
                throw std::runtime_error("read()");
            }
            old.c_lflag |= ICANON;
            old.c_lflag |= ECHO;
            if (tcsetattr(0, TCSADRAIN, &old) < 0)
            {
                throw std::runtime_error("tcsetattr ~ICANON");
            }

            if (ch == ESC)
            {
                is_ESC_pressed = true;
                break;
            }
        }
        catch (std::exception &e) 
        {
            mtx.lock();
            ex_ptr = std::current_exception();
            mtx.unlock();
            break;
        }
    }
}

bool handle_eptr(std::exception_ptr eptr)
{
    try
    {
        if (eptr)
        {
            std::rethrow_exception(eptr);
        }
        return false;
    }
    catch(const std::exception& e)
    {
        std::cout << "Caught exception: '" << e.what() << "'\n";
        return true;
    }
}

int main()
{
    const std::string host = "httpbin.org";
    std::vector<std::pair<char*, size_t>> http_responses;

    // create HTTP Client thread
    std::atomic_bool http_client_need_finish = false;
    std::exception_ptr http_client_ex_ptr = nullptr;
    HttpClient http_client(host);
    std::mutex http_client_mtx;
    std::thread http_client_thread(&HttpClient::run, http_client, std::ref(http_responses),
        std::ref(http_client_mtx), std::ref(http_client_need_finish), std::ref(http_client_ex_ptr));

    // create key reading thread
    std::atomic_bool key_reading_need_finish = false;
    std::atomic_bool is_ESC_pressed = false;
    std::exception_ptr key_reading_ex_ptr = nullptr;
    std::mutex key_reading_mtx;
    std::thread key_reading_thread(check_keystroke, std::ref(is_ESC_pressed), std::ref(key_reading_mtx),
        std::ref(key_reading_ex_ptr), std::ref(key_reading_need_finish));

    while (true)
    {
        // check pressed ESC or exceptions
        http_client_mtx.lock();
        key_reading_mtx.lock();
        if (http_client_ex_ptr || key_reading_ex_ptr || is_ESC_pressed)
        {
            // finish http_client_thread
            http_client_need_finish = true;
            key_reading_mtx.unlock();
            http_client_mtx.unlock();
            break;
        }
        key_reading_mtx.unlock();
        http_client_mtx.unlock();

        // output http responses
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

    http_client_thread.join();

    // output remaining http responses
    if (http_responses.size() != 0)
    {
        std::cout << "Remainig http responses:\n";
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

    if (http_client_ex_ptr)
    {
        key_reading_need_finish = true;
        handle_eptr(http_client_ex_ptr);
    }
    else if (key_reading_ex_ptr)
    {
        handle_eptr(key_reading_ex_ptr);
    }

    key_reading_thread.join();

    return 0;
}
