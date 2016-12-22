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


#include "iotjs_def.h"
#include "iotjs_module_blecentral.h"
#include "iotjs_objectwrap.h"


#define THIS iotjs_blecentral_reqwrap_t* blecentral_reqwrap


iotjs_blecentral_reqwrap_t* iotjs_blecentral_reqwrap_create(
    const iotjs_jval_t* jcallback, BlecentralOp op) {
  iotjs_blecentral_reqwrap_t* blecentral_reqwrap =
      IOTJS_ALLOC(iotjs_blecentral_reqwrap_t);
  IOTJS_VALIDATED_STRUCT_CONSTRUCTOR(iotjs_blecentral_reqwrap_t,
                                     blecentral_reqwrap);

  iotjs_reqwrap_initialize(&_this->reqwrap, jcallback, (uv_req_t*)&_this->req);

  _this->req_data.op = op;

  return blecentral_reqwrap;
}


static void iotjs_blecentral_reqwrap_destroy(THIS) {
  IOTJS_VALIDATED_STRUCT_DESTRUCTOR(iotjs_blecentral_reqwrap_t,
                                    blecentral_reqwrap);
  iotjs_reqwrap_destroy(&_this->reqwrap);
  IOTJS_RELEASE(blecentral_reqwrap);
}


void iotjs_blecentral_reqwrap_dispatched(THIS) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_blecentral_reqwrap_t, blecentral_reqwrap);
  iotjs_blecentral_reqwrap_destroy(blecentral_reqwrap);
}


uv_work_t* iotjs_blecentral_reqwrap_req(THIS) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_blecentral_reqwrap_t, blecentral_reqwrap);
  return &_this->req;
}


const iotjs_jval_t* iotjs_blecentral_reqwrap_jcallback(THIS) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_blecentral_reqwrap_t, blecentral_reqwrap);
  return iotjs_reqwrap_jcallback(&_this->reqwrap);
}


iotjs_blecentral_reqwrap_t* iotjs_blecentral_reqwrap_from_request(
    uv_work_t* req) {
  return (
      iotjs_blecentral_reqwrap_t*)(iotjs_reqwrap_from_request((uv_req_t*)req));
}


iotjs_blecentral_reqdata_t* iotjs_blecentral_reqwrap_data(THIS) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_blecentral_reqwrap_t, blecentral_reqwrap);
  return &_this->req_data;
}


#undef THIS


static void iotjs_blecentral_destroy(iotjs_blecentral_t* blecentral);


iotjs_blecentral_t* iotjs_blecentral_create(const iotjs_jval_t* jblecentral) {
  iotjs_blecentral_t* blecentral = IOTJS_ALLOC(iotjs_blecentral_t);
  IOTJS_VALIDATED_STRUCT_CONSTRUCTOR(iotjs_blecentral_t, blecentral);
  iotjs_jobjectwrap_initialize(&_this->jobjectwrap, jblecentral,
                               (JFreeHandlerType)iotjs_blecentral_destroy);
  return blecentral;
}


static void iotjs_blecentral_destroy(iotjs_blecentral_t* blecentral) {
  IOTJS_VALIDATED_STRUCT_DESTRUCTOR(iotjs_blecentral_t, blecentral);
  iotjs_jobjectwrap_destroy(&_this->jobjectwrap);
  IOTJS_RELEASE(blecentral);
}


const iotjs_jval_t* iotjs_blecentral_get_jblecentral() {
  return iotjs_module_get(MODULE_BLECENTRAL);
}


iotjs_blecentral_t* iotjs_blecentral_get_instance() {
  const iotjs_jval_t* jblecentral = iotjs_blecentral_get_jblecentral();
  iotjs_jobjectwrap_t* jobjectwrap =
      iotjs_jobjectwrap_from_jobject(jblecentral);
  return (iotjs_blecentral_t*)jobjectwrap;
}

void *iotjs_blecentral_get_platform_handle() {
  iotjs_blecentral_t *blecentral = iotjs_blecentral_get_instance();
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_blecentral_t, blecentral);
  return _this->platform_handle;
}

void iotjs_blecentral_set_platform_handle(void *handle) {
  iotjs_blecentral_t *blecentral = iotjs_blecentral_get_instance();
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_blecentral_t, blecentral);
  _this->platform_handle = handle;
}

void afterWork(uv_work_t* work_req, int status) {
  iotjs_blecentral_t* blecentral = iotjs_blecentral_get_instance();

  iotjs_blecentral_reqwrap_t* req_wrap =
      iotjs_blecentral_reqwrap_from_request(work_req);
  iotjs_blecentral_reqdata_t* req_data =
      iotjs_blecentral_reqwrap_data(req_wrap);

  iotjs_jargs_t jargs;

  if (status) {
    iotjs_jval_t error = iotjs_jval_create_error("System error");
    iotjs_jargs_append_jval(&jargs, &error);
    iotjs_jval_destroy(&error);
  } else {
    switch (req_data->op) {
      case kBlecentralOpInit: {
        jargs = iotjs_jargs_create(1);

        if (req_data->state == kBlecentralStatePoweredOn) {
          iotjs_jargs_append_string_raw(&jargs, "poweredOn");
        } else if (req_data->state == kBlecentralStatePoweredOff) {
          iotjs_jargs_append_string_raw(&jargs, "poweredOff");
        } else if (req_data->state == kBlecentralStateUnauthorized) {
          iotjs_jargs_append_string_raw(&jargs, "unauthorized");
        } else if (req_data->state == kBlecentralStateUnsupported) {
          iotjs_jargs_append_string_raw(&jargs, "unsupported");
        } else {
          iotjs_jargs_append_string_raw(&jargs, "unknown");
        }
        break;
      }
      case kBlecentralOpStartScanning: {
        jargs = iotjs_jargs_create(1);

        iotjs_jargs_append_null(&jargs);
        break;
      }
      case kBlecentralOpStopScanning: {
        jargs = iotjs_jargs_create(1);

        iotjs_jargs_append_null(&jargs);
        break;
      }
      case kBlecentralOpRunLoop: {
        if (req_data->cmd == kBlecentralCmdStateChange) {
          // args = state
          jargs = iotjs_jargs_create(1);
          if (req_data->state == kBlecentralStatePoweredOn) {
            iotjs_jargs_append_string_raw(&jargs, "poweredOn");
          } else if (req_data->state == kBlecentralStatePoweredOff) {
            iotjs_jargs_append_string_raw(&jargs, "poweredOff");
          } else if (req_data->state == kBlecentralStateUnauthorized) {
            iotjs_jargs_append_string_raw(&jargs, "unauthorized");
          } else if (req_data->state == kBlecentralStateUnsupported) {
            iotjs_jargs_append_string_raw(&jargs, "unsupported");
          } else {
            iotjs_jargs_append_string_raw(&jargs, "unknown");
          }
        } else if (req_data->cmd == kBlecentralCmdAdvertisingReport) {
          // args = status, type, address, addressType, eir, rssi
          jargs = iotjs_jargs_create(6);
          // status
          iotjs_jargs_append_number(&jargs, 0);
          // type
          iotjs_jargs_append_number(&jargs, req_data->type);
          // address (length is 18)
          iotjs_string_t address =
              iotjs_string_create_with_size(req_data->address, 18);
          iotjs_jargs_append_string(&jargs, &address);
          // addressType
          if (req_data->addressType) {
            iotjs_jargs_append_string_raw(&jargs, "random");
          } else {
            iotjs_jargs_append_string_raw(&jargs, "public");
          }
          // eir
          iotjs_string_t eir =
              iotjs_string_create_with_size(req_data->eir,
                                            req_data->eir_length);
          iotjs_jargs_append_string(&jargs, &eir);
          // rssi
          iotjs_jargs_append_number(&jargs, req_data->rssi);
        }

        // kBlecentralCmdScanStart, kBlecentralCmdScanStop is not implemented
        // yet

        break;
      }
      default: {
        IOTJS_ASSERT(!"Unreachable");
        break;
      }
    }
  }

  const iotjs_jval_t* jcallback = iotjs_blecentral_reqwrap_jcallback(req_wrap);
  const iotjs_jval_t* jblecentral = iotjs_blecentral_get_jblecentral();
  iotjs_make_callback(jcallback, jblecentral, &jargs);

  iotjs_jargs_destroy(&jargs);
  iotjs_blecentral_reqwrap_dispatched(req_wrap);
}

#define BLECENTRAL_ASYNC(op)                                           \
  do {                                                                 \
    uv_loop_t* loop = iotjs_environment_loop(iotjs_environment_get()); \
    uv_work_t* req = iotjs_blecentral_reqwrap_req(req_wrap);           \
    uv_queue_work(loop, req, op##Worker, afterWork);                   \
  } while (0)

JHANDLER_FUNCTION(Init) {
  JHANDLER_CHECK_ARGS(1, function);

  const iotjs_jval_t* jcallback = JHANDLER_GET_ARG(0, function);

  iotjs_blecentral_reqwrap_t* req_wrap =
      iotjs_blecentral_reqwrap_create(jcallback, kBlecentralOpInit);

  BLECENTRAL_ASYNC(Init);

  iotjs_jhandler_return_null(jhandler);
}

JHANDLER_FUNCTION(StartScanning) {
  JHANDLER_CHECK_ARGS(2, number, function);

  const iotjs_jval_t* jcallback = JHANDLER_GET_ARG(1, function);

  iotjs_blecentral_reqwrap_t* req_wrap =
      iotjs_blecentral_reqwrap_create(jcallback, kBlecentralOpStartScanning);

  iotjs_blecentral_reqdata_t* req_data =
      iotjs_blecentral_reqwrap_data(req_wrap);

  req_data->duplicates = JHANDLER_GET_ARG(0, number);

  BLECENTRAL_ASYNC(StartScanning);

  iotjs_jhandler_return_null(jhandler);
}

JHANDLER_FUNCTION(StopScanning) {
  JHANDLER_CHECK_ARGS(1, function);

  const iotjs_jval_t* jcallback = JHANDLER_GET_ARG(0, function);

  iotjs_blecentral_reqwrap_t* req_wrap =
      iotjs_blecentral_reqwrap_create(jcallback, kBlecentralOpStopScanning);

  iotjs_blecentral_reqdata_t* req_data =
      iotjs_blecentral_reqwrap_data(req_wrap);

  BLECENTRAL_ASYNC(StopScanning);

  iotjs_jhandler_return_null(jhandler);
}

JHANDLER_FUNCTION(RunLoop) {
  JHANDLER_CHECK_ARGS(1, function);

  const iotjs_jval_t* jcallback = JHANDLER_GET_ARG(0, function);

  iotjs_blecentral_reqwrap_t* req_wrap =
      iotjs_blecentral_reqwrap_create(jcallback, kBlecentralOpRunLoop);

  iotjs_blecentral_reqdata_t* req_data =
      iotjs_blecentral_reqwrap_data(req_wrap);

  BLECENTRAL_ASYNC(RunLoop);

  iotjs_jhandler_return_null(jhandler);
}

iotjs_jval_t InitBlecentral() {
  iotjs_jval_t jblecentral = iotjs_jval_create_object();

  iotjs_jval_set_method(&jblecentral, "init", Init);
  iotjs_jval_set_method(&jblecentral, "startScanning", StartScanning);
  iotjs_jval_set_method(&jblecentral, "stopScanning", StopScanning);
  iotjs_jval_set_method(&jblecentral, "runLoop", RunLoop);

  iotjs_blecentral_t* blecentral = iotjs_blecentral_create(&jblecentral);
  IOTJS_ASSERT(
      blecentral ==
      (iotjs_blecentral_t*)(iotjs_jval_get_object_native_handle(&jblecentral)));

  return jblecentral;
}
