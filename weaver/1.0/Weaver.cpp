/******************************************************************************
 *
 *  Copyright 2020 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#define LOG_TAG "Weaver-service-default"

#include "Weaver.h"
#include <log/log.h>
#include <weaver-impl.h>
#include <weaver_interface.h>

namespace aidl::android::hardware::weaver {

using ::aidl::android::hardware::weaver::WeaverReadStatus;

WeaverInterface* pInterface = nullptr;
Weaver::Weaver() {
    ALOGI("INITIALIZING WEAVER");
    pInterface = WeaverImpl::getInstance();
    if (pInterface != NULL) {
        pInterface->Init();
    }
}

// Methods from ::android::hardware::weaver::IWeaver follow.

::ndk::ScopedAStatus Weaver::getConfig(WeaverConfig* configResp) {
    ALOGI("GETCONFIG API ENTRY");
    if (pInterface == NULL) {
        ALOGI("Weaver Interface not defined");
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificError(Weaver::STATUS_FAILED));
    }
    SlotInfo slotInfo;
    Status_Weaver status = pInterface->GetSlots(slotInfo);
    if (status == WEAVER_STATUS_OK) {
        configResp->slots = slotInfo.slots;
        configResp->keySize = slotInfo.keySize;
        configResp->valueSize = slotInfo.valueSize;
        ALOGI("Weaver Success for getSlots Slots :(%d)", configResp->slots);
    } else {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificError(Weaver::STATUS_FAILED));
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Weaver::read(int32_t slotId, const std::vector<uint8_t>& key,
                                  WeaverReadResponse* readResp) {

    ALOGI("Read API ENTRY");

    if (pInterface == NULL) {
        *readResp = {0, {}, WeaverReadStatus::FAILED};
        return ::ndk::ScopedAStatus::ok();
    }
    ReadRespInfo readInfo;
    Status_Weaver status = pInterface->Read(slotId, key, readInfo);
    switch (status) {
    case WEAVER_STATUS_OK:
        ALOGI("Read OK");
        *readResp = {0, readInfo.value, WeaverReadStatus::OK};
        break;
    case WEAVER_STATUS_INCORRECT_KEY:
        ALOGI("Read Incorrect Key");
        *readResp = {readInfo.timeout, {}, WeaverReadStatus::INCORRECT_KEY};
        break;
    case WEAVER_STATUS_THROTTLE:
        ALOGI("Read WEAVER_THROTTLE");
        *readResp = {readInfo.timeout, {}, WeaverReadStatus::THROTTLE};
        break;
    default:
        *readResp = {0, {}, WeaverReadStatus::FAILED};
    }

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Weaver::write(int32_t slotId, const std::vector<uint8_t>& key,
                                   const std::vector<uint8_t>& value) {

    ALOGI("Write API ENTRY");
    if ((pInterface == NULL) || (pInterface->Write(slotId, key, value) != WEAVER_STATUS_OK)) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificError(Weaver::STATUS_FAILED));
    }
    return ::ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::weaver
