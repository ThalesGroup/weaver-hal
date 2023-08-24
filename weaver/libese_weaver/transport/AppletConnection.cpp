/*
 **
 ** Copyright 2020, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */
/******************************************************************************
 **
 ** The original Work has been changed by THALES.
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
 ** Copyright ©2023-2024 THALES. All rights Reserved.
 **
 *********************************************************************************/
#ifndef OMAPI_TRANSPORT
#define LOG_TAG "AppletConnection"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <log/log.h>
#include <signal.h>
#include <iomanip>
#include <mutex>
#include <string>
#include <vector>

#include <AppletConnection.h>
#include <EseTransportUtils.h>
#include <SignalHandler.h>
#include <stdlib.h>
#include <binder/IBinder.h>
#include <binder/IServiceManager.h>

#include <aidl/android/hardware/secure_element/BnSecureElementCallback.h>


using aidl::android::hardware::secure_element::BnSecureElementCallback;

using aidl::android::hardware::secure_element::LogicalChannelResponse;
using ndk::ScopedAStatus;
using ndk::SharedRefBase;
using ndk::SpAIBinder;

using android::base::StringPrintf;

using namespace android;

namespace keymint::javacard {

static bool isStrongBox = false; // true when linked with StrongBox HAL process
const std::vector<uint8_t> kStrongBoxAppletAID = {0xA0, 0x00, 0x00, 0x00, 0x62};

AppletConnection::AppletConnection(const std::vector<uint8_t>& aid) : kAppletAID(aid) {
    if (kAppletAID == kStrongBoxAppletAID) {
        isStrongBox = true;
    }
}

class MySecureElementCallback : public BnSecureElementCallback {
  public:
    ScopedAStatus onStateChange(bool state, const std::string& debugReason) override {
        LOG(INFO) << "onStateChange state = "<< state;
        LOG(INFO) << "onStateChange debugReason = "<< debugReason;
        return ScopedAStatus::ok();
    };
};

std::shared_ptr<MySecureElementCallback> secure_element_callback_;

bool AppletConnection::connectToSEService() {
    if (!SignalHandler::getInstance()->isHandlerRegistered()) {
        LOG(INFO) << "register signal handler";
        SignalHandler::getInstance()->installHandler(this);
    }
    if (mSEClient != nullptr) {
        LOG(INFO) <<"Already connected";
        return true;
    }

    uint8_t retry = 0;
    bool status = false;
    while (( mSEClient == nullptr ) && retry++ < MAX_GET_SERVICE_RETRY ){ // How long should we try before giving up !
      SpAIBinder binder = SpAIBinder(AServiceManager_waitForService("android.hardware.secure_element.ISecureElement/eSE1"));

      mSEClient = ISecureElement::fromBinder(binder);

      if(mSEClient == nullptr){
        LOG(ERROR) << "failed to get eSE HAL service : retry after 1 sec , retry cnt = " << android::hardware::toString(retry) ;
      }else {
        LOG(INFO) << " !!! SuccessFully got Handle to eSE HAL service" ;
        break;
      }
      usleep(ONE_SEC);
    }
    
    secure_element_callback_ = SharedRefBase::make<MySecureElementCallback>();
    if(secure_element_callback_ == nullptr) return false;

    mSEClient->init(secure_element_callback_);
    
    return status;
}

bool AppletConnection::selectApplet(std::vector<uint8_t>& resp, uint8_t p2) {
  bool stat = false;
  auto res = ndk::ScopedAStatus::ok();
  LogicalChannelResponse logical_channel_response;
  
  res = mSEClient->openLogicalChannel(kAppletAID, p2, &logical_channel_response);
  if (!res.isOk()) {
      LOG(ERROR) << "openLogicalChannel error: " << res.getMessage();
      return false;
  }
  if (logical_channel_response.channelNumber == 0) {
      LOG(ERROR) << "Could not open channel null";
      return false;
  }
  mOpenChannel = logical_channel_response.channelNumber;
  resp = logical_channel_response.selectResponse;
  stat = true;
  LOG(INFO) << "openLogicalChannel: channelNumber ="
            << ::android::hardware::toString(logical_channel_response.channelNumber) << " "
            << logical_channel_response.selectResponse;

  return stat;
}
void prepareErrorRepsponse(std::vector<uint8_t>& resp){
        resp.clear();
        resp.push_back(0xFF);
        resp.push_back(0xFF);
}
bool AppletConnection::openChannelToApplet(std::vector<uint8_t>& resp) {
  bool ret = false;
  uint8_t retry = 0;
  if (mSEClient == nullptr) {
    mOpenChannel = -1;
    if (!connectToSEService()) {
      LOG(ERROR) << "Not connected to eSE Service";
      return ret;
    }
  }
  if (isChannelOpen()) {
    LOG(INFO) << "channel Already opened";
    return true;
  }
  ret = selectApplet(resp, 0x0);
  return ret;
}

bool AppletConnection::transmit(std::vector<uint8_t>& CommandApdu , std::vector<uint8_t>& output){
    std::vector<uint8_t> cmd = CommandApdu;
    std::vector<uint8_t> transmitResponse;
    cmd[0] |= mOpenChannel ;
    LOGD_OMAPI("Channel number " << ::android::hardware::toString(mOpenChannel));

    if (mSEClient == nullptr) return false;
    
    // block any fatal signal delivery
    SignalHandler::getInstance()->blockSignals();

    auto res = ndk::ScopedAStatus::ok();
    res = mSEClient->transmit(cmd, &transmitResponse);
    output = transmitResponse;
    LOG(INFO) << "recieved response size = " << ::android::hardware::toString(transmitResponse.size()) << " data = " << transmitResponse;
    LOG(INFO) << "STATUS OF TRNSMIT: " << res.getExceptionCode()
          << " Message: " << res.getMessage();
    if (!res.isOk()) {
	LOG(ERROR) << "transmit error: " << res.getMessage();
	return false;
    }

    // un-block signal delivery
    SignalHandler::getInstance()->unblockSignals();
    return true;
}

int AppletConnection::getSessionTimeout() {
    return 0;
}

bool AppletConnection::close() {
    std::lock_guard<std::mutex> lock(channel_mutex_);
    if (mSEClient == nullptr) {
         LOG(ERROR) << "Channel couldn't be closed mSEClient handle is null";
         return false;
    }
    if(mOpenChannel < 0){
       LOG(INFO) << "Channel is already closed";
       return true;
    }
    auto res = ndk::ScopedAStatus::ok();
    
    res = mSEClient->closeChannel(mOpenChannel);
    if (!res.isOk()) {
        /*
         * reason could be SE reset or HAL deinit triggered from other client
         * which anyway closes all the opened channels
         * */
        LOG(ERROR) << "closeChannel failed";
        mOpenChannel = -1;
        return true;
    }
    LOG(INFO) << "Channel closed";
    mOpenChannel = -1;
    return true;
}

bool AppletConnection::isChannelOpen() {
    std::lock_guard<std::mutex> lock(channel_mutex_);
    return mOpenChannel >= 0;
}

}  // namespace keymint::javacard
#endif
