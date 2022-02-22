/******************************************************************************
 *
 *  Copyright 2020-2021 NXP
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

#define LOG_TAG "weaver-transport-impl"
#include <vector>
#include <ITransport.h>
#include <TransportFactory.h>
#include <weaver_transport-impl.h>
#include <weaver_utils.h>

WeaverTransportImpl *WeaverTransportImpl::s_instance = NULL;
std::once_flag WeaverTransportImpl::s_instanceFlag;

/* Applet ID to be use for communication */
std::vector<uint8_t> kAppletId;

/* Interface instance of libese-transport library */
static std::unique_ptr<se_transport::TransportFactory> pTransportFactory =
    nullptr;

/**
 * \brief static inline function to get lib-ese-transport interface instance
 */
static inline std::unique_ptr<se_transport::TransportFactory> &
getTransportFactoryInstance() {
  if (pTransportFactory == nullptr) {
    pTransportFactory = std::unique_ptr<se_transport::TransportFactory>(
        new se_transport::TransportFactory(false, kAppletId));
    pTransportFactory->openConnection();
  }
  return pTransportFactory;
}

/**
 * \brief static function to get the singleton instance of WeaverTransportImpl
 * class
 *
 * \retval instance of WeaverTransportImpl.
 */
WeaverTransportImpl *WeaverTransportImpl::getInstance() {
  /* call_once c++11 api which executes the passed function ptr exactly once,
   * even if called concurrently, from several threads
   */
  std::call_once(s_instanceFlag, &WeaverTransportImpl::createInstance);
  return s_instance;
}

/* Private function to create the instance of self class
 * Same will be used for std::call_once
 */
void WeaverTransportImpl::createInstance() {
  LOG_D(TAG, "Entry");
  s_instance = new WeaverTransportImpl;
  LOG_D(TAG, "Exit");
}

/**
 * \brief Function to initilize Weaver Transport Interface
 *
 * \param[in]    aid -  applet id to be set to transport interface
 *
 * \retval This function return true in case of success
 *         In case of failure returns false.
 */
bool WeaverTransportImpl::Init(std::vector<uint8_t> aid) {
  LOG_D(TAG, "Entry");
  kAppletId = aid;
  LOG_D(TAG, "Exit");
  return true;
}

/**
 * \brief Function to open applet connection
 *
 * \param[in]    data -         command for open applet
 * \param[out]   resp -         response from applet
 *
 * \retval This function return true in case of success
 *         In case of failure returns false.
 */
bool WeaverTransportImpl::OpenApplet(std::vector<uint8_t> data,
                                     std::vector<uint8_t> &resp) {
  LOG_D(TAG, "Entry");
  bool status = true;
  UNUSED(data);
  UNUSED(resp);
  // Since libese_transport opens channel as part of send only so open applet is
  // not required
  LOG_D(TAG, "Exit");
  return status;
}

/**
 * \brief Function to close applet connection
 *
 * \retval This function return true in case of success
 *         In case of failure returns false.
 */
bool WeaverTransportImpl::CloseApplet() {
  LOG_D(TAG, "Entry");
  // Close the Applet Channel if opened
  bool status = getTransportFactoryInstance()->closeConnection();
  LOG_D(TAG, "Exit");
  return status;
}

/**
 * \brief Function to send commands to applet
 *
 * \param[in]    data -         command to be send to applet
 * \param[out]   resp -         response from applet
 *
 * \retval This function return true in case of success
 *         In case of failure returns false.
 */
bool WeaverTransportImpl::Send(std::vector<uint8_t> data,
                               std::vector<uint8_t> &resp) {
  LOG_D(TAG, "Entry");
  // Opens the channel with aid and transmit the data
  bool status =
      getTransportFactoryInstance()->sendData(data.data(), data.size(), resp);
  LOG_D(TAG, "Exit");
  return status;
}

/**
 * \brief Function to de-initilize Weaver Transport Interface
 *
 * \retval This function return true in case of success
 *         In case of failure returns false.
 */
bool WeaverTransportImpl::DeInit() {
  LOG_D(TAG, "Entry");
  bool status = CloseApplet();
  LOG_D(TAG, "Exit");
  return status;
}
