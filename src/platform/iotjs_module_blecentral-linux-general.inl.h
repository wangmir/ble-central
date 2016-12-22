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

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "module/iotjs_module_blecentral.h"

#define HCI_STATE_NONE 0
#define HCI_STATE_OPEN 2
#define HCI_STATE_SCANNING 3
#define HCI_STATE_FILTERING 4

#define LE_SET_SCAN_ENABLE_CMD (OCF_LE_SET_SCAN_ENABLE | OGF_LE_CTL << 10)

#define TRUE 1
#define FALSE 0

struct hci_state {
  int device_id;
  int device_handle;
  struct hci_filter of;
  int state;
  uint8_t _scan_duplicates;
  int has_error;
  char error_message[1024];

  // req_data ptr
  iotjs_blecentral_reqdata_t *req_data;
};

#define BLECENTRAL_WORKER_INIT_TEMPLATE                \
  iotjs_blecentral_reqwrap_t *req_wrap =               \
      iotjs_blecentral_reqwrap_from_request(work_req); \
  iotjs_blecentral_reqdata_t *req_data =               \
      iotjs_blecentral_reqwrap_data(req_wrap);

#define LE2HS(ptr) (btohs(*(uint16_t *)(ptr)))

void _LoopWorker(struct hci_state *_hci_state) {
  int len = 0;
  char data[HCI_MAX_EVENT_SIZE];

  len = read(_hci_state->device_handle, data, sizeof(data));

  if (len < 0) {
    if (errno == EINTR)
      return;

    if (errno == EAGAIN || errno == EINTR)
      return;

    _hci_state->has_error = TRUE;
    snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
             "Failed to read socket: %s", strerror(errno));

    printf("%s", _hci_state->error_message);
    getchar();
  }

  if (len > 0) {
    uint8_t ev_type = data[0];

    if (ev_type == HCI_EVENT_PKT) {
      uint8_t subevent = data[1];

      if (subevent == EVT_LE_META_EVENT) {
        char addr[18];
        evt_le_meta_event *meta = (void *)(data + (1 + HCI_EVENT_HDR_SIZE));

        len -= (1 + HCI_EVENT_HDR_SIZE);

        if (meta->subevent == EVT_LE_ADVERTISING_REPORT) {
          char name[30];
          le_advertising_info *info = (le_advertising_info *)(meta->data + 1);
          uint8_t type = info->evt_type;
          uint8_t address_type = info->bdaddr_type; // if 1 'random', 0 'public'
          uint8_t rssi = info->data[info->length];

          if (info->length == 0) // no info
            return;

          memset(name, 0, sizeof(name));

          ba2str(&info->bdaddr, addr); // address in string xx:xx:xx:xx:xx:xx

          _hci_state->req_data->type = type;
          _hci_state->req_data->address = addr;
          _hci_state->req_data->addressType = address_type;
          _hci_state->req_data->eir = (char *)info->data;
          _hci_state->req_data->rssi = rssi;
        }
      }
    } else if (ev_type == HCI_COMMAND_PKT) {
      uint16_t cmd = LE2HS(&data[1]);
      uint8_t len = data[3];

      if (cmd == LE_SET_SCAN_ENABLE_CMD) {
        uint8_t enable = data[4] ? 1 : 0;
        uint8_t duplicates = data[5] ? 1 : 0;

        if (_hci_state->state == HCI_STATE_SCANNING ||
            _hci_state->state == HCI_STATE_FILTERING) {
          if (!enable) {
            // scan stop
            _hci_state->req_data->cmd = kBlecentralCmdScanStop;

          } else if (duplicates != _hci_state->_scan_duplicates) {
            // duplicates changed
            _hci_state->_scan_duplicates = duplicates;
            _hci_state->req_data->cmd = kBlecentralCmdScanStart;
            _hci_state->req_data->duplicates = duplicates;
          }
        } else if ((_hci_state->state == HCI_STATE_OPEN) && enable) {
          // scan started
          _hci_state->req_data->cmd = kBlecentralCmdScanStart;
          _hci_state->req_data->duplicates = duplicates;
        }
      }
    }
    // need to handle else
  }
}

void __open_default_hci_device(struct hci_state *_hci_state) {
  _hci_state->device_id = hci_get_route(NULL);

  if ((_hci_state->device_handle = hci_open_dev(_hci_state->device_id)) < 0) {
    _hci_state->has_error = TRUE;
    snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
             "Could not open device: %s", strerror(errno));
    _hci_state->req_data->state = kBlecentralStateUnknown;
    return;
  }

  int on = 1;

  if (ioctl(_hci_state->device_handle, FIONBIO, (char *)&on) < 0) {
    _hci_state->has_error = TRUE;
    snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
             "Could not set device to non-blocking: %s", strerror(errno));
    _hci_state->req_data->state = kBlecentralStateUnknown;
    return;
  }

  _hci_state->state = HCI_STATE_OPEN;

  _hci_state->req_data->state = kBlecentralStatePoweredOn;
}

void __do_start_scanning(struct hci_state *_hci_state, int allow_duplicates) {
  int err, opt, dd;
  uint8_t own_type = 0x00;
  uint8_t scan_type = 0x01;
  uint8_t filter_type = 0;
  uint8_t filter_policy = 0x00;
  uint16_t interval = htobs(0x0010);
  uint16_t window = htobs(0x0010);
  uint8_t filter_dup = allow_duplicates ? 1 : 0;

  dd = _hci_state->device_handle;

  _hci_state->_scan_duplicates = filter_dup;

  err = hci_le_set_scan_parameters(dd, scan_type, interval, window, own_type,
                                   filter_policy, 1000);
  if (err < 0) {
    _hci_state->has_error = TRUE;
    snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
             "Failed to set scan parameters: %s", strerror(errno));
    return;
  }

  err = hci_le_set_scan_enable(dd, 0x01, filter_dup, 1000);
  if (err < 0) {
    _hci_state->has_error = TRUE;
    snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
             "Failed to set scan enable: %s", strerror(errno));
    return;
  }

  _hci_state->state = HCI_STATE_SCANNING;

  // save HCI filter
  socklen_t len = sizeof(_hci_state->of);
  err = getsockopt(dd, SOL_HCI, HCI_FILTER, &_hci_state->of, &len);
  if (err < 0) {
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
  if (err < 0) {
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

  if (_hci_state->state == HCI_STATE_FILTERING) {
    err = setsockopt(dd, SOL_HCI, HCI_FILTER, &_hci_state->of,
                     sizeof(_hci_state->of));
    if (err < 0) {
      _hci_state->has_error = TRUE;
      snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
               "Failed to set socket option: %s", strerror(errno));
      return;
    }

    _hci_state->state = HCI_STATE_SCANNING;
  }

  err = hci_le_set_scan_enable(dd, 0x00, 1, 1000);
  if (err < 0) {
    _hci_state->has_error = TRUE;
    snprintf(_hci_state->error_message, sizeof(_hci_state->error_message),
             "Failed to set scan disable: %s", strerror(errno));
    return;
  }

  _hci_state->state = HCI_STATE_OPEN;
}

void InitWorker(uv_work_t *work_req) {
  BLECENTRAL_WORKER_INIT_TEMPLATE;

  struct hci_state *ble = IOTJS_ALLOC(struct hci_state);

  iotjs_blecentral_set_platform_handle((void *)ble);
  // platform_handle = (void *)ble;
  ble->req_data = req_data;

  __open_default_hci_device(ble);
}

void RunLoopWorker(uv_work_t *work_req) {
  BLECENTRAL_WORKER_INIT_TEMPLATE;

  void *platform_handle = iotjs_blecentral_get_platform_handle();
  struct hci_state *ble = (struct hci_state *)platform_handle;

  ble->req_data = req_data;

  _LoopWorker(ble);
}

void StartScanningWorker(uv_work_t *work_req) {
  BLECENTRAL_WORKER_INIT_TEMPLATE;

  void *platform_handle = iotjs_blecentral_get_platform_handle();
  // need to handle service uuids
  struct hci_state *ble = (struct hci_state *)platform_handle;

  ble->req_data = req_data;

  __do_start_scanning(ble, req_data->duplicates);
}

void StopScanningWorker(uv_work_t *work_req) {
  BLECENTRAL_WORKER_INIT_TEMPLATE;

  void *platform_handle = iotjs_blecentral_get_platform_handle();
  struct hci_state *ble = (struct hci_state *)platform_handle;
  
  ble->req_data = req_data;

  __do_stop_scanning(ble);
}

#endif
