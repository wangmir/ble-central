/* Copyright 2016 Samsung Electronics Co., Ltd.
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


#ifndef IOTJS_MODULE_BLECENTRAL_H
#define IOTJS_MODULE_BLECENTRAL_H

#include "iotjs_def.h"
#include "iotjs_objectwrap.h"
#include "iotjs_reqwrap.h"

typedef enum {
  kBlecentralOpInit,
  kBlecentralOpStartScanning,
  kBlecentralOpStopScanning,
  kBlecentralOpRunLoop,
} BlecentralOp;

typedef enum {
  kBlecentralErrOk = 0,
  kBlecentralErrInit = -1,
  kBlecentralErrStart = -2,
  kBlecentralErrStop = -3,
} BlecentralError;

typedef enum {
  kBlecentralStatePoweredOn,
  kBlecentralStatePoweredOff,
  kBlecentralStateUnauthorized,
  kBlecentralStateUnsupported,
  kBlecentralStateUnknown,
} BlecentralState;

typedef enum {
  kBlecentralCmdStateChange,
  kBlecentralCmdScanStart,
  kBlecentralCmdScanStop,
  kBlecentralCmdAdvertisingReport
} BlecentralCmd;

typedef struct {
  BlecentralOp op;
  BlecentralError error;
  BlecentralState state;

  int cmd;
  int duplicates;

  // for advertisingReport
  int type;
  int addressType;
  char* address;
  char* eir;
  int eir_length;
  int rssi;
} iotjs_blecentral_reqdata_t;


typedef struct {
  iotjs_reqwrap_t reqwrap;
  uv_work_t req;
  iotjs_blecentral_reqdata_t req_data;
} IOTJS_VALIDATED_STRUCT(iotjs_blecentral_reqwrap_t);

#define THIS iotjs_blecentral_reqwrap_t* blecentral_reqwrap
iotjs_blecentral_reqwrap_t* iotjs_blecentral_reqwrap_create(
    const iotjs_jval_t* jcallback, BlecentralOp op);
void iotjs_blecentral_reqwrap_dispatched(THIS);
uv_work_t* iotjs_blecentral_reqwrap_req(THIS);
const iotjs_jval_t* iotjs_blecentral_reqwrap_jcallback(THIS);
iotjs_blecentral_reqwrap_t* iotjs_blecentral_reqwrap_from_request(
    uv_work_t* req);
iotjs_blecentral_reqdata_t* iotjs_blecentral_reqwrap_data(THIS);
#undef THIS

// This blecentral class provides interfaces for bleCentral operation.
typedef struct {
  iotjs_jobjectwrap_t jobjectwrap;
  void *platform_handle;
} IOTJS_VALIDATED_STRUCT(iotjs_blecentral_t);

iotjs_blecentral_t* iotjs_blecentral_create(const iotjs_jval_t* jblecentral);
const iotjs_jval_t* iotjs_blecentral_get_jblecentral();
iotjs_blecentral_t* iotjs_blecentral_get_instance();
void iotjs_blecentral_set_platform_handle(void *handle);
void *iotjs_blecentral_get_platform_handle();

void InitWorker(uv_work_t* work_req);
void StartScanningWorker(uv_work_t* work_req);
void StopScanningWorker(uv_work_t* work_req);
void RunLoopWorker(uv_work_t* work_req);

#endif /* IOTJS_MODULE_BLECENTRAL_H */
