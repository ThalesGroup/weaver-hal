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

#define LOG_TAG "weaver-parser-impl"
#include <weaver_parser-impl.h>
#include <weaver_utils.h>

WeaverParserImpl *WeaverParserImpl::s_instance = NULL;
std::once_flag WeaverParserImpl::s_instanceFlag;

/* byte info for GP header of weaver commands */
#define CLA 0x80
#define INS_GET_SLOT 0x02
#define INS_READ 0x06
#define INS_WRITE 0x04
#define P1 0x00
#define P2 0x00
#define LE_READ_CMD 0x11
#define LE_GET_SLOT_CMD 0x04
#define LE_WRITE_CMD 0x00

/* Error code for weaver commands response */
#define SUCCESS_SW1 0x90
#define SUCCESS_SW2 0x00
#define INVALID_SLOT_SW1 0x6A
#define INVALID_SLOT_SW2 0x88
#define INVALID_P1P2_SW1 0x6A
#define INVALID_P1P2_SW2 0x86
#define INVALID_LENGTH_SW1 0x67
#define INVALID_LENGTH_SW2 0x00

/* Supported Size by Applet */
#define KEY_SIZE 16
#define VALUE_SIZE 16
#define RES_STATUS_SIZE 2

/* For Applet Read Response TAG */
#define INCORRECT_KEY_TAG 0x7F
#define THROTTING_ENABLED_TAG 0x76
#define READ_SUCCESS_TAG 0x00
#define READ_ERR_CODE_INDEX 0 // Start index of above tag in read response
#define READ_ERR_CODE_SIZE 1  // Size of above tag in read response

#define SLOT_ID_INDEX 0 // Index of slotId in getSlot response

/* For bit shifting mask */
#define SHIFT_MASK 0xff
#define BYTE3_MSB_POS 8
#define BYTE2_MSB_POS 16
#define BYTE1_MSB_POS 24

/* Applet ID to be used for Weaver */
const std::vector<uint8_t> kWeaverAID = {0xA0, 0x00, 0x00, 0x08, 0x44, 0x53, 0xF1, 0x27, 0x56, 0x18, 0x01, 0x00};

/**
 * \brief static function to get the singleton instance of WeaverParserImpl
 * class
 *
 * \retval instance of WeaverParserImpl.
 */
WeaverParserImpl *WeaverParserImpl::getInstance() {
  /* call_once c++11 api which executes the passed function ptr exactly once,
   * even if called concurrently, from several threads
   */
  std::call_once(s_instanceFlag, &WeaverParserImpl::createInstance);
  return s_instance;
}

/* Private function to create the instance of self class
 * Same will be used for std::call_once
 */
void WeaverParserImpl::createInstance() {
  LOG_D(TAG, "Entry");
  s_instance = new WeaverParserImpl;
  LOG_D(TAG, "Exit");
}

/**
 * \brief Function to Frame weaver applet request command for open
 *
 * \param[out]    request - framed open command as vector
 *
 * \retval This function return true in case of success
 *         In case of failure returns false.
 */
bool WeaverParserImpl::FrameOpenCmd(std::vector<uint8_t> &request) {
  LOG_D(TAG, "Entry");
  UNUSED(request);
  LOG_D(TAG, "Exit");
  return true;
}

/**
 * \brief Function to Frame weaver applet request command for getSlots
 *
 * \param[out]    request - framed getslots command as vector
 *
 * \retval This function return true in case of success
 *         In case of failure returns false.
 */
bool WeaverParserImpl::FrameGetSlotCmd(std::vector<uint8_t> &request) {
  LOG_D(TAG, "Entry");
  request.clear();
  request.push_back(CLA);
  request.push_back(INS_GET_SLOT);
  request.push_back(P1);
  request.push_back(P2);
  request.push_back(LE_GET_SLOT_CMD);
  LOG_D(TAG, "Exit");
  return true;
}

/**
 * \brief Function to Frame weaver applet request command for read
 *
 * \param[in]     slotId  - input slotId to be used in read request.
 * \param[in]     key     - input key to be used in read request.
 * \param[out]    request - framed read command as vector
 *
 * \retval This function return true in case of success
 *         In case of failure returns false.
 */
bool WeaverParserImpl::FrameReadCmd(uint32_t slotId,
                                    const std::vector<uint8_t> &key,
                                    std::vector<uint8_t> &request) {
  LOG_D(TAG, "Entry");
  request.clear();
  request.push_back(CLA);
  request.push_back(INS_READ);
  request.push_back(P1);
  request.push_back(P2);
  request.push_back(sizeof(uint32_t) + key.size()); // LC
  /* convert and insert 4 Byte integer slot id as byte by byte to vector */
  request.push_back(SHIFT_MASK & (slotId >> BYTE1_MSB_POS));
  request.push_back(SHIFT_MASK & (slotId >> BYTE2_MSB_POS));
  request.push_back(SHIFT_MASK & (slotId >> BYTE3_MSB_POS));
  request.push_back(SHIFT_MASK & slotId);
  request.insert(std::end(request), std::begin(key), std::end(key));
  request.push_back(LE_READ_CMD);
  LOG_D(TAG, "Exit");
  return true;
}

/**
 * \brief Function to Frame weaver applet request command for write
 *
 * \param[in]     slotId  - input slotId to be used in write request.
 * \param[in]     key     - input key to be used in write request.
 * \param[in]     value   - input value to be used in write request.
 * \param[out]    request - framed write command as vector
 *
 * \retval This function return true in case of success
 *         In case of failure returns false.
 */
bool WeaverParserImpl::FrameWriteCmd(uint32_t slotId,
                                     const std::vector<uint8_t> &key,
                                     const std::vector<uint8_t> &value,
                                     std::vector<uint8_t> &request) {
  LOG_D(TAG, "Entry");
  request.clear();
  request.push_back(CLA);
  request.push_back(INS_WRITE);
  request.push_back(P1);
  request.push_back(P2);
  request.push_back(sizeof(uint32_t) + key.size() + value.size()); // LC
  /* convert and insert 4 Byte integer slot id as byte by byte to vector */
  request.push_back(SHIFT_MASK & (slotId >> BYTE1_MSB_POS));
  request.push_back(SHIFT_MASK & (slotId >> BYTE2_MSB_POS));
  request.push_back(SHIFT_MASK & (slotId >> BYTE3_MSB_POS));
  request.push_back(SHIFT_MASK & slotId);
  request.insert(std::end(request), std::begin(key), std::end(key));
  request.insert(std::end(request), std::begin(value), std::end(value));
  request.push_back(LE_WRITE_CMD);
  LOG_D(TAG, "Exit");
  return true;
}

/**
 * \brief Function to Parse getSlots response
 *
 * \param[in]     response  - response from applet.
 * \param[out]    slotInfo  - parsed slots Information read out from applet
 * response.
 *
 * \retval This function return true in case of success
 *         In case of failure returns false.
 */
Status_Weaver WeaverParserImpl::ParseSlotInfo(std::vector<uint8_t> response,
                                              SlotInfo &slotInfo) {
  LOG_D(TAG, "Entry");
  Status_Weaver status = WEAVER_STATUS_FAILED;
  slotInfo.slots = 0;
  if (isSuccess(response)) {
    /* Read 4 bytes for number of slot as integer. */
    uint32_t slots = response.at(SLOT_ID_INDEX) << BYTE1_MSB_POS;
    slots |= response.at(SLOT_ID_INDEX + 1) << BYTE2_MSB_POS;
    slots |= response.at(SLOT_ID_INDEX + 2) << BYTE3_MSB_POS;
    slots |= response.at(SLOT_ID_INDEX + 3);
    slotInfo.slots = slots;
    slotInfo.keySize = KEY_SIZE;
    slotInfo.valueSize = VALUE_SIZE;
    status = WEAVER_STATUS_OK;
  }
  LOG_D(TAG, "Exit");
  return status;
}

/**
 * \brief Function to Parse read response
 *
 * \param[in]     response  - response from applet.
 * \param[out]    readInfo  - parsed read Information read out from applet
 * response.
 *
 * \retval This function return true in case of success
 *         In case of failure returns false.
 */
Status_Weaver WeaverParserImpl::ParseReadInfo(std::vector<uint8_t> response,
                                              ReadRespInfo &readInfo) {
  LOG_D(TAG, "Entry");
  Status_Weaver status = WEAVER_STATUS_FAILED;
  if (response.size() < RES_STATUS_SIZE) {
    LOG_E(TAG, "Exit Invalid Response Size");
    return status;
  }
  if (isSuccess(response)) {
    readInfo.timeout = 0; // Applet not supporting timeout value
    switch (response.at(READ_ERR_CODE_INDEX)) {
    case INCORRECT_KEY_TAG:
      LOG_E(TAG, "INCORRECT_KEY");
      status = WEAVER_STATUS_INCORRECT_KEY;
      readInfo.value.resize(0);
      break;
    case THROTTING_ENABLED_TAG:
      LOG_E(TAG, "THROTTING_ENABLED");
      status = WEAVER_STATUS_THROTTLE;
      readInfo.value.resize(0);
      break;
    case READ_SUCCESS_TAG:
      if ((VALUE_SIZE + READ_ERR_CODE_SIZE + RES_STATUS_SIZE) ==
          response.size()) {
        LOG_D(TAG, "SUCCESS");
        readInfo.value.clear();
        readInfo.value.insert(std::end(readInfo.value),
                              std::begin(response) + READ_ERR_CODE_SIZE,
                              std::end(response) - RES_STATUS_SIZE);
        status = WEAVER_STATUS_OK;
      } else {
        LOG_E(TAG, "Invalid Response");
      }
      break;
    default:
      LOG_E(TAG, "Unkown Tag for Read Response");
    }
  }
  LOG_D(TAG, "Exit");
  return status;
}

/**
 * \brief Function to check if response from applet is Success or not
 *
 * \param[in]     response  - response from applet.
 *
 * \retval This function return true if response code from applet is success
 *         and false in other cases.
 */
bool WeaverParserImpl::isSuccess(std::vector<uint8_t> response) {
  return (checkStatus(response) == APP_SUCCESS) ? true : false;
}

/**
 * \brief Private internal Function to check the response status code
 *
 * \param[in]    response  - response from  weaver applet.
 *
 * \retval This function return errorcode from APP_ERR_CODE type
 */
WeaverParserImpl::APP_ERR_CODE
WeaverParserImpl::checkStatus(std::vector<uint8_t> response) {
  LOG_D(TAG, "Entry");
  APP_ERR_CODE status = APP_FAILED;
  if (RES_STATUS_SIZE > response.size()) {
    LOG_E(TAG, "Response is too short");
    status = APP_FAILED;
  } else if (response.at(response.size() - 2) == SUCCESS_SW1 &&
             response.at(response.size() - 1) == SUCCESS_SW2) {
    LOG_D(TAG, "SUCCESS");
    status = APP_SUCCESS;
  } else if (response.at(response.size() - 2) == INVALID_SLOT_SW1 &&
             response.at(response.size() - 1) == INVALID_SLOT_SW2) {
    // Invalid Slot ID
    LOG_E(TAG, "Invalid Slot");
    status = APP_INVALID_SLOT;
  } else if (response.at(response.size() - 2) == INVALID_P1P2_SW1 &&
             response.at(response.size() - 1) == INVALID_P1P2_SW2) {
    // Invalid P1/P2
    LOG_E(TAG, "Invalid P1/P2");
    status = APP_INVALID_P1_P2;
  } else if (response.at(response.size() - 2) == INVALID_LENGTH_SW1 &&
             response.at(response.size() - 1) == INVALID_LENGTH_SW2) {
    // Invalid Length
    LOG_E(TAG, "Invalid Length");
    status = APP_INVALID_LEN;
  }
  LOG_D(TAG, "Exit");
  return status;
}

/**
 * \brief Function to get Weaver Applet ID
 *
 * \param[out]    aid  - applet id of the weaver applet.
 *
 * \retval This function return true in case of success
 *         In case of failure returns false.
 */
bool WeaverParserImpl::getAppletId(std::vector<uint8_t> &aid) {
  LOG_D(TAG, "Entry");
  bool status = false;
  if (kWeaverAID.size() > 0) {
    aid = kWeaverAID;
    status = true;
  }
  LOG_D(TAG, "Exit");
  return status;
}
