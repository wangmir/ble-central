/* Copyright 2016 Jhuyeong Jhin <jjysienna@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IOTJS_MODULE_BLECENTRAL_LINUX_GENERAL_INL_H
#define IOTJS_MODULE_BLECENTRAL_LINUX_GENERAL_INL_H

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "module/iotjs_module_blecentral.h"

#define HCI_STATE_NONE 0
#define HCI_STATE_OPEN 2
#define HCI_STATE_SCANNING 3
#define HCI_STATE_FILTERING 4

struct ble_common {
  struct hci _hci;
  struct gap _gap;
  struct bindings _bindings;
};

struct hci {
  struct hci_state _hci_state;
};

struct hci_state {
  int device_id;
  int device_handle;
  struct hci_filter of;
  int state;
  int has_error;
  char error_message[1024];
} hci_state;

#define EIR_FLAGS 0x01
#define EIR_NAME_SHORT 0x08
#define EIR_NAME_COMPLETE 0x09
#define EIR_MANUFACTURE_SPECIFIC 0xff

#define BLECENTRAL_WORKER_INIT_TEMPLATE    \
    iotjs_blecentral_reqwrap_t *req_wrap  \
    = iotjs_blecentral_reqwrap_from_request(work_req);  \
    iotjs_blecentral_reqdata_t *req_data  \
    = iotjs_blecentral_reqwrap_data(req_wrap);

void __close_device(struct hci_state _hci_state) {

  if(_hci_state.state == HCI_STATE_OPEN)
    hci_close_dev(_hci_state.device_handle);
}


void __open_default_hci_device(struct hci_state *_hci_state){

  _hci_state->device_id = hci_get_route(NULL);

  if((_hci_state->device_handle = hci_open_dev(_hci_state->device_id)) < 0){

    _hci_state->has_error = TRUE;
    snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
            "Could not open device: %s", strerror(errno));
    return NULL;
  }

  int on = 1;

  if(ioctl(_hci_state->device_handle, FIONBIO, (char *)&on) <0) {

    _hci_state->has_error = TRUE;
    snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
             "Could not set device to non-blocking: %s", strerror(errno));
    return NULL;
  }

  _hci_state->state = HCI_STATE_OPEN;

}

void __do_start_scanning(struct hci_state *_hci_state,
                         char *service_uuids, int allow_duplicates) {

  int err, opt, dd;
  uint8_t own_type = 0x00;
  uint8_t scan_type = 0x01;
  uint8_t filter_type = 0;
  uint8_t filter_policy = 0x00;
  uint16_t interval = htobs(0x0010);
  uint16_t window = htobs(0x0010);
  uint8_t filter_dup = allow_duplicates ? 1 : 0;

  dd = _hci_state->device_handle;

  err = hci_le_set_scan_parameters(dd, scan_type, interval, window,
                                   own_type, filter_policy, 1000);

  if(err < 0){
    _hci_state->has_error = TRUE;
    snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
             "Failed to set scan parameters: %s", strerror(errno));
    return;
  }

  err = hci_le_set_scan_enable(dd, 0x01, filter_dup, 1000);
  if(err < 0){ 
    _hci_state->has_error = TRUE;
    snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
             "Failed to set scan enable: %s", strerror(errno));
    return;
  }

  _hci_state->state = HCI_STATE_SCANNING;

  //save HCI filter
  socklen_t len = sizeof(_hci_state->of);
  err = getsockopt(dd, SOL_HCI, HCI_FILTER, &_hci_state->of, &olen);
  if(err < 0){ 
    _hci_state->has_error = TRUE;
    snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
             "Failed to get socket option: %s", strerror(errno));
    return;
  }

  struct hci_filter nf;

  hci_filter_clear(&nf);
  hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
  hci_filter_set_event(EVT_LE_META_EVENT, &nf);

  err = setsockopt(dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf));
  if(err < 0){ 
    _hci_state->has_error = TRUE;
    snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
             "Failed to set socket option: %s", strerror(errno));
    return;
  }

  _hci_state->state = HCI_STATE_FILTERING;
}

void __do_stop_scanning(struct hci_state *_hci_state) {
  
  int dd, err;
  dd = _hci_state->device_handle;

  if(_hci_state->state == HCI_STATE_FILTERING) {

    err = setsockopt(dd, SOL_HCI, HCI_FILTER, &_hci_state->of,
                     sizeof(_hci_state->of));
    if(err < 0){ 
      _hci_state->has_error = TRUE;
      snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
               "Failed to set socket option: %s", strerror(errno));
      return;
    }

    _hci_state->state = HCI_STATE_SCANNING;
  }

  err = hci_le_set_scan_enable(dd, 0x00, 1, 1000);
  if(err < 0){ 
    _hci_state->has_error = TRUE;
    snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
             "Failed to set scan disable: %s", strerror(errno));
    return;
  }

  _hci_state->state = HCI_STATE_OPEN;
}

void BlecentralDestroy(){

  struct ble_common *ble 
      = (struct ble_common *)iotjs_blecentral_get_instance()->platform_handle;

  __close_device(ble->hci._hci_state);
  free(ble);
}

void BlecentralCreate(){

  struct ble_common *ble = IOTJS_ALLOC(struct ble_common);

  iotjs_blecentral_t *blecentral = iotjs_blecentral_get_instance();
  blecentral->platform_handle = (void *)ble;

  __open_default_hci_device(&ble->hci._hci_state);

  if(ble->hci._hci_state.has_error){
    // something wrong about creating
    iotjs_blecentral_event_callback(kBlecentralEvError,
                                    ble->hci._hci_state.error_message);
    free(ble);
  }

}

void StartScanningWorker(uv_work_t *work_req) {
  // need to handle service uuids
  BLECENTRAL_WORKER_INIT_TEMPLATE;

  struct ble_common *ble
      = (struct ble_common *)iotjs_blecentral_get_instance()->platform_handle;

  __do_start_scanning(&ble->hci._hci_state, NULL, req_data->duplicates);

  if(ble->hci._hci_state.has_error){
    // something wrong about creating
    iotjs_blecentral_event_callback(kBlecentralEvError,
                                    ble->hci._hci_state.error_message);
  } 
}


void StopScanningWorker(uv_work_t *work_req) {
  // TODO: not implemented
  struct ble_common *ble
      = (struct ble_common *)iotjs_blecentral_get_instance()->platform_handle;
  __do_stop_scanning(&ble->hci._hci_state);

  if(ble->hci._hci_state.has_error){
    // something wrong about creating
    iotjs_blecentral_event_callback(kBlecentralEvError,
                                    ble->hci._hci_state.error_message);
  } 
}

#endif
