
#include <iostream>
#include <thread>
#include <array>
#include <fstream>
#include <mutex>
#include <string>
#include <random>

#include "../../src/wooting-rgb-sdk.h"

std::mutex mtx;
bool run;
uint8_t clr;
std::vector<WOOTING_USB_META> devices;

void profile_get()
{
    std::array<uint8_t, 256> data = { 0 };
    
    while (run)
    {
        for (int i = 0; i < devices.size(); i++)
        {
            const std::lock_guard lock(mtx);

            wooting_usb_select_device(i);
            auto before = std::chrono::high_resolution_clock::now();
            auto response = wooting_usb_send_feature_with_response(data.data(), data.size(), 11,0,0,0,0);
            auto after = std::chrono::high_resolution_clock::now();

            std::cout << "Device: " << devices[i].model << std::endl;
            std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count() << "ms" << std::endl;
            std::cout << "Response: " << response << std::endl;
            std::cout << "Profile: " << std::to_string(data[5]) << std::endl;
            std::cout << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

}

void rgb_run(int idx)
{
    while (run)
    {
        {
            const std::lock_guard lock(mtx);
            
            wooting_usb_select_device(idx);
            const auto device = devices[idx];
            for (uint8_t i = 0; i < device.max_rows; i++)
            {
                for (uint8_t j = 0; j < device.max_columns; j++)
                {
                    wooting_rgb_array_set_single(i, j, clr, clr, clr);
                }
            }
            clr+=8;
            wooting_rgb_array_update_keyboard();
        }

        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main(int argc, char* argv[])
{
    auto con = wooting_rgb_kbd_connected();

    for (auto i = 0; i < wooting_usb_device_count(); i++)
    {
        wooting_usb_select_device(i);
        devices.push_back(*wooting_rgb_device_info());
    }
    
    run = true;
    
    std::thread t1(profile_get);
    std::vector<std::thread> threads;
    for(auto i = 0; i < devices.size(); i++)
    {
        std::thread t(rgb_run, i);
        threads.push_back(std::move(t));
    }

    std::cin.get();

    run = false;
    t1.join();
    for(auto &t : threads)
    {
        if(t.joinable())
            t.join();
    }

    for (uint8_t i = 0; i < wooting_usb_device_count(); i++)
    {
        wooting_usb_select_device(i);
        wooting_rgb_reset_rgb();
    }

    wooting_rgb_close();
}
