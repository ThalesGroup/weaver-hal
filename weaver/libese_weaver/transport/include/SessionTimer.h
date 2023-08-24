/******************************************************************************
 **
 ** Copyright ©2023-2024 THALES. All rights Reserved.
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 ** http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 **
 ** 
 **
 *********************************************************************************/
#pragma once
#include <iostream>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <csignal>
#include <unistd.h>
#include <atomic>
#ifdef OMAPI_TRANSPORT
#include "OmapiTransport.h"
#else
#include "HalToHalTransport.h"
#endif

#define SESSION_TIMEOUT_3S (3000)  // 3 s

class Timer {
public:
    int count = 0;
    Timer() : is_running(false) {}

    ~Timer() {
        stop();
    }

    // Start the timer with the specified timeout and call closeChannel if timeout is reached
    void start(int timeout_ms, void* ptr) {
        if (!is_running) {
            is_running = true;
#ifdef OMAPI_TRANSPORT
            transport_ptr = ptr;
#else
            transport_ptr_a = ptr;
#endif
            
            if (std::signal(SIGALRM, &timerCallback) == SIG_ERR) {
                LOG(ERROR) << "Error setting up signal handler for SIGALRM. " << std::endl;
                return;
            }
            
            if (alarm(timeout_ms / 1000) != 0) {
                LOG(ERROR) << "Error setting the alarm. " << std::endl;
                return;
            }
        }
    }

    // Stop the timer
    void stop() {
        if (is_running) {
            is_running = false;
            alarm(0);
        }
    }

private:
    std::atomic<bool> is_running;
#ifdef OMAPI_TRANSPORT
    static void* transport_ptr;
#else
    static void* transport_ptr_a;
#endif

    // Static callback function required by timer_create
    static void timerCallback(int signal) {
        LOG(DEBUG) << "signal: " << signal;
#ifdef OMAPI_TRANSPORT
        keymint::javacard::OmapiTransport *transport = (keymint::javacard::OmapiTransport*)transport_ptr;
#else
        keymint::javacard::HalToHalTransport *transport = (keymint::javacard::HalToHalTransport*)transport_ptr_a;
#endif
        if (transport != nullptr) {
            transport->closeConnection();
        }
    }
};
