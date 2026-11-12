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

#define LOG_TAG "weaver-impl"
#include <weaver-impl.h>
#include <weaver_parser-impl.h>
#include <weaver_transport-impl.h>
#include <weaver_utils.h>

WeaverImpl *WeaverImpl::s_instance = NULL;
std::once_flag WeaverImpl::s_instanceFlag;

/**
 * \brief static function to get the singleton instance of WeaverImpl class
 *
 * \retval instance of WeaverImpl.
 */
WeaverImpl *WeaverImpl::getInstance() {
  /* call_once c++11 api which executes the passed function ptr exactly once,
   * even if called concurrently, from several threads
   */
  std::call_once(s_instanceFlag, &WeaverImpl::createInstance);
  return s_instance;
}

/* Private function to create the instance of self class
 * Same will be used for std::call_once
 */
void WeaverImpl::createInstance() {
  LOG_D(TAG, "Entry");
  s_instance = new WeaverImpl;
}

/**
 * \brief Function to initilize Weaver Interface
 *
 * This function initializes the Weaver interface and checks whether the
 * embedded Secure Element (eSE) is currently under a throttling period
 * (e.g., after a reboot). It queries the remaining throttling time using
 * GetMaxRemainingTime() and, if a valid non-zero timeout is returned,
 * maintains the eSE active by acquiring a wake lock for the remaining duration.
 *
 * A retry mechanism is used to handle cases where the applet is not ready
 * immediately after initialization.
 *
 * \retval This function return Weaver_STATUS_OK (0) in case of success
 *         In case of failure returns other Status_Weaver.
 */
Status_Weaver WeaverImpl::Init() {
  LOG_D(TAG, "Entry");
  mTransport = WeaverTransportImpl::getInstance();
  mParser = WeaverParserImpl::getInstance();
  RETURN_IF_NULL(mTransport, WEAVER_STATUS_FAILED, "Transport is NULL");
  RETURN_IF_NULL(mParser, WEAVER_STATUS_FAILED, "Parser is NULL");
  std::vector<uint8_t> aid;
  mParser->getAppletId(aid);
  if (!mTransport->Init(aid)) {
    LOG_E(TAG, "Not able to Initilaize Transport Interface");
    LOG_D(TAG, "Exit : FAILED");
    return WEAVER_STATUS_FAILED;
  }
  
  LOG_D(TAG, "Init: checking pending throttling after reboot");
  int64_t remainingTimeMs = 0;
  for (int i = 0; i < 10; i++) {
    Status_Weaver status = GetMaxRemainingTime(remainingTimeMs);
    if (status == WEAVER_STATUS_OK){
        if(remainingTimeMs > 0) {
          LOG_D(TAG, "Init: GetMaxRemainingTime retry=%d status=%d remaining=%lld",
          i, status, (long long)remainingTimeMs);
          LOG_D(TAG, "Init: pending throttling found, keeping eSE active");
          MaintainESEActive(remainingTimeMs);
        } else {
          LOG_D(TAG, "No pending throttling period detected");
        }
        break;
    }
    usleep(1000 * 1000); // 1s
}
    
  LOG_D(TAG, "Exit : SUCCESS");
  return WEAVER_STATUS_OK;
}

/**
 * \brief Function to read slot information
 * \param[out]   slotInfo - slot information values read out
 *
 * \retval This function return Weaver_STATUS_OK (0) in case of success
 *         In case of failure returns other Status_Weaver errorcodes.
 */
Status_Weaver WeaverImpl::GetSlots(SlotInfo &slotInfo) {
  LOG_D(TAG, "Entry");
  RETURN_IF_NULL(mTransport, WEAVER_STATUS_FAILED, "Transport is NULL");
  RETURN_IF_NULL(mParser, WEAVER_STATUS_FAILED, "Parser is NULL");
  Status_Weaver status = WEAVER_STATUS_FAILED;
  std::vector<uint8_t> getSlotCmd;
  std::vector<uint8_t> resp;
  /* transport library don't require open applet
   * open will be done as part of send */
  if (mParser->FrameGetSlotCmd(getSlotCmd) &&
      mTransport->Send(getSlotCmd, resp)) {
    status = WEAVER_STATUS_OK;
  } else {
    LOG_E(TAG, "Failed to perform getSlot Request");
  }
  /*if (!close()) {
    // Channel Close Failed
    LOG_E(TAG, "Failed to Close Channel");
  }*/
  if (status == WEAVER_STATUS_OK) {
    status = mParser->ParseSlotInfo(resp, slotInfo);
    LOG_D(TAG, "Total Slots (%u) ", slotInfo.slots);
  } else {
    LOG_E(TAG, "Failed Parsing getSlot Response");
  }
  LOG_D(TAG, "Exit");
  return status;
}

/* Internal close api for transport close */
bool WeaverImpl::close() {
  LOG_D(TAG, "Entry");
  bool status = true;
  RETURN_IF_NULL(mTransport, WEAVER_STATUS_FAILED, "Transport is NULL");
  if (!mTransport->CloseApplet()) {
    status = false;
  }
  LOG_D(TAG, "Exit");
  return status;
}

/**
 * \brief Function to read value of specific key & slotId
 * \param[in]    slotId -       input slotId which's information to be read
 * \param[in]    key -          input key which's information to be read
 * \param[out]   readRespInfo - read information values to be read out
 *
 * \retval This function return Weaver_STATUS_OK (0) in case of success
 *         In case of failure returns other Status_Weaver errorcodes.
 */
Status_Weaver WeaverImpl::Read(uint32_t slotId, const std::vector<uint8_t> &key,
                               ReadRespInfo &readRespInfo) {
  LOG_D(TAG, "Entry");
  RETURN_IF_NULL(mTransport, WEAVER_STATUS_FAILED, "Transport is NULL");
  RETURN_IF_NULL(mParser, WEAVER_STATUS_FAILED, "Parser is NULL");
  Status_Weaver status = WEAVER_STATUS_FAILED;
  std::vector<uint8_t> readCmd;
  std::vector<uint8_t> resp;
  std::vector<uint8_t> aid;
  /* transport library don't require open applet
   * open will be done as part of send */
  LOG_D(TAG, "Read from Slot (%u)", slotId);
  if (mParser->FrameReadCmd(slotId, key, readCmd) &&
      mTransport->Send(readCmd, resp)) {
    status = WEAVER_STATUS_OK;
  }
  /*if (!close()) {
    // Channel Close Failed
    LOG_E(TAG, "Failed to Close Channel");
  }*/
  if (status == WEAVER_STATUS_OK) {
    status = mParser->ParseReadInfo(resp, readRespInfo);
  } else {
    LOG_E(TAG, "Failed to perform Read Request for slot (%u)", slotId);
  }
  LOG_D(TAG, "Exit");
  return status;
}

/**
 * \brief Function to write value to specific key & slotId
 * \param[in]    slotId -       input slotId where value to be write
 * \param[in]    key -          input key where value to be write
 * \param[in]   value -        input value which will be written
 *
 * \retval This function return Weaver_STATUS_OK (0) in case of success
 *         In case of failure returns other Status_Weaver.
 */
Status_Weaver WeaverImpl::Write(uint32_t slotId,
                                const std::vector<uint8_t> &key,
                                const std::vector<uint8_t> &value) {
  LOG_D(TAG, "Entry");
  RETURN_IF_NULL(mTransport, WEAVER_STATUS_FAILED, "Transport is NULL");
  RETURN_IF_NULL(mParser, WEAVER_STATUS_FAILED, "Parser is NULL");
  Status_Weaver status = WEAVER_STATUS_FAILED;
  std::vector<uint8_t> readCmd;
  std::vector<uint8_t> resp;
  std::vector<uint8_t> aid;
  /* transport library don't require open applet
   * open will be done as part of send */
  LOG_D(TAG, "Write to Slot (%u)", slotId);
  if (mParser->FrameWriteCmd(slotId, key, value, readCmd) &&
      mTransport->Send(readCmd, resp)) {
    status = WEAVER_STATUS_OK;
  }
  /*if (!close()) {
    LOG_E(TAG, "Failed to Close Channel");
    // Channel Close Failed
  }*/
  if (status != WEAVER_STATUS_OK || (!mParser->isSuccess(resp))) {
    status = WEAVER_STATUS_FAILED;
  }
  LOG_D(TAG, "Exit");
  return status;
}

 /**
  * \brief Function to warm up the eSE
  */
Status_Weaver WeaverImpl::WarmUp() {
    LOG_D(TAG, "Entry");
    RETURN_IF_NULL(mTransport, WEAVER_STATUS_FAILED, "Transport is NULL");
    std::vector<uint8_t> resp;
    if (!mTransport->OpenApplet({}, resp)) {
        LOG_E(TAG, "WarmUp: failed to open channel to Weaver applet");
        LOG_D(TAG, "Exit : FAILED");
        return WEAVER_STATUS_FAILED;
    }
    LOG_D(TAG, "Exit : SUCCESS");
    return WEAVER_STATUS_OK;
}

/**
 * \brief Function to get current throttling period
 * \param[in]    slotId -     input slotId to query
 * \param[out]   timeout -      returned timeout value in milliseconds
 *
 * \retval This function return Weaver_STATUS_OK (0) in case of success
 *         In case of failure returns other Status_Weaver.
 */
Status_Weaver WeaverImpl::GetTimeout(uint32_t slotId, uint64_t &timeout) {
    LOG_D(TAG, "Entry");
    
    // 1. Defensive checks
    RETURN_IF_NULL(mTransport, WEAVER_STATUS_FAILED, "Transport is NULL");
    RETURN_IF_NULL(mParser, WEAVER_STATUS_FAILED, "Parser is NULL");

    Status_Weaver status = WEAVER_STATUS_FAILED;
    std::vector<uint8_t> cmd;
    std::vector<uint8_t> resp;

    LOG_D(TAG, "GetTimeOut for Slot (%u)", slotId);

    // 2. Frame, Send, and verify communication success
    if (mParser->FrameGetTimeoutCmd(slotId, cmd) &&
        mTransport->Send(cmd, resp)) {
        
        // 3. Parse the response
        if (mParser->ParseGetTimeoutResponse(resp, timeout)) {
            status = WEAVER_STATUS_OK;
        } else {
            LOG_E(TAG, "Failed to parse GetTimeout response for slot (%u)", slotId);
        }
    } else {
        LOG_E(TAG, "Failed to perform GetTimeout request for slot (%u)", slotId);
    }

    LOG_D(TAG, "Exit");
    return status;
}

/**
 * \brief Retrieve maximum remaining throttling time from Weaver applet
 *
 * This function sends a request to the Weaver applet to obtain the maximum
 * remaining throttling timeout. The returned value indicates how long the
 * device is still in a throttled state before normal operations can resume.
 *
 * \param[out]    remainingTime  - Remaining throttling time in milliseconds.
 *
 * \retval WEAVER_STATUS_OK           In case of successful retrieval.
 * \retval WEAVER_STATUS_FAILED       In case of communication or parsing failure.
 */
Status_Weaver WeaverImpl::GetMaxRemainingTime(int64_t &remainingTime) {
    LOG_D(TAG, "Entry");

    std::vector<uint8_t> cmd, resp;

    if (mParser->FrameGetMaxRemainingTimeCmd(cmd) != WEAVER_STATUS_OK)  {
        LOG_E(TAG, "FrameGetMaxRemainingTimeCmd failed");
        return WEAVER_STATUS_FAILED;
    }

    if (!mTransport->Send(cmd, resp)) {
        LOG_E(TAG, "Send failed");
        return WEAVER_STATUS_FAILED;
    }

    Status_Weaver status = mParser->ParseMaxRemainingTimeResponse(resp, remainingTime);

    LOG_E(TAG, "DEBUG_MAX_IMPL: parse status=%d remaining=%lld",
          status, (long long)remainingTime);
    LOG_D(TAG, "Exit");
    return status;
}

/**
 * \brief Function to de-initilize Weaver Interface
 *
 * \retval This function return Weaver_STATUS_OK (0) in case of success
 *         In case of failure returns other Status_Weaver.
 */
Status_Weaver WeaverImpl::DeInit() {
  LOG_D(TAG, "Entry");
  if (mTransport != NULL) {
    mTransport->DeInit();
  }
  LOG_D(TAG, "Exit");
  return WEAVER_STATUS_OK;
}

/**
 * \brief Maintain eSE active state using wake lock for a specified duration
 *
 * This function ensures that the embedded Secure Element (eSE) remains active
 * for the given duration by delegating wake lock management to
 * EseWakeLockManager. If a wake lock is already held, the active period will
 * be extended if the new duration is longer.
 *
 * \param[in]    durationMs  - Duration in milliseconds to keep the eSE active.
 *
 * \retval This function does not return a value.
 *         If durationMs <= 0, no action is performed.
 */
void WeaverImpl::MaintainESEActive(int64_t durationMs) {
    if (durationMs <= 0) {
        LOG_D(TAG, "No pending throttling period detected");
        return;
    }

    LOG_D(TAG, "Maintaining eSE active for %lld ms", (long long)durationMs);

    mEseWakeLockManager.maintainActive(durationMs);
}
