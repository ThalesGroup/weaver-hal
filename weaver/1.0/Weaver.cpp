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
#define LOG_TAG "Weaver@1.0-service"

#include "Weaver.h"
#include <log/log.h>
#include <string.h>
#include <hidl/LegacySupport.h>

#define UNUSED(x) (void)(x)

/* Mutex to synchronize multiple transceive */

namespace android {
namespace hardware {
namespace weaver {
namespace V1_0 {
namespace implementation {

  Weaver::Weaver() {
    ALOGI("INITILIZING WEAVER");
  }

  Return<void> Weaver::getConfig(getConfig_cb _hidl_cb) {
    ALOGI("GETCONFIG API ENTRY");
    WeaverConfig configResp;
    _hidl_cb(WeaverStatus::FAILED, configResp);
    return Void();
  }

  Return<::android::hardware::weaver::V1_0::WeaverStatus>
    Weaver::write(uint32_t slotId, const hidl_vec<uint8_t>& key, const hidl_vec<uint8_t>& value) {
      ALOGI("Write API ENTRY");
      UNUSED(slotId);
      UNUSED(key);
      UNUSED(value);
      WeaverStatus status = WeaverStatus::FAILED;
      return status;
    }

  Return<void>
    Weaver::read(uint32_t slotId, const hidl_vec<uint8_t>& key, read_cb _hidl_cb) {
      ALOGI("Read API ENTRY");
      UNUSED(slotId);
      UNUSED(key);
      WeaverReadResponse readResp;
      _hidl_cb(WeaverReadStatus::FAILED, readResp);
      return Void();
    }

  void Weaver::serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/) {
  }
}
}
}
}
}
