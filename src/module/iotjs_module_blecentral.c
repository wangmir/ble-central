/* Copyright 2016 Eunsoo Park (esevan.park@gmail.com)
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

#include "iotjs_module_blecentral.h"
#include "iotjs_string.h"

static void iotjs_blecentral_destroy(iotjs_blecentral_t *instance) {
  IOTJS_VALIDATED_STRUCT_DESTRUCTOR(iotjs_blecentral_t, instance);

  BlecentralDestroy();

  iotjs_jobjectwrap_destroy(&_this->jobjectwrap);
  IOTJS_RELEASE(instance);
}

iotjs_blecentral_t* iotjs_blecentral_create(const iotjs_jval_t* jblecentral) {
  iotjs_blecentral_t* blecentral = IOTJS_ALLOC(iotjs_blecentral_t);
  IOTJS_VALIDATED_STRUCT_CONSTRUCTOR(iotjs_blecentral_t, blecentral);
  iotjs_jobjectwrap_initialize(&_this->jobjectwrap, jblecentral,
                               (JFreeHandlerType)iotjs_blecentral_destroy);

  return blecentral;
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

#define THIS iotjs_blecentral_reqwrap_t *blecentral_reqwrap
iotjs_blecentral_reqwrap_t *
iotjs_blecentral_reqwrap_create(const iotjs_jval_t *jcallback,
                                BlecentralOp op) {
  iotjs_blecentral_reqwrap_t *blecentral_reqwrap = 
      IOTJS_ALLOC(iotjs_blecentral_reqwrap_t);
  IOTJS_VALIDATED_STRUCT_CONSTRUCTOR(iotjs_blecentral_reqwrap_t,
                                     blecentral_reqwrap);

  iotjs_reqwrap_initialize(&_this->reqwrap, jcallback,
                           (uv_req_t *)&_this->req);

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
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_blecentral_reqwrap_t,
                                blecentral_reqwrap);

  iotjs_blecentral_reqwrap_destroy(blecentral_reqwrap);
}

uv_work_t *iotjs_blecentral_reqwrap_req(THIS) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_blecentral_reqwrap_t,
                                blecentral_reqwrap);

  return &_this->req;
}

const iotjs_jval_t *iotjs_blecentral_reqwrap_jcallback(THIS) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_blecentral_reqwrap_t,
                                blecentral_reqwrap);

  return iotjs_reqwrap_jcallback(&_this->reqwrap);
}

iotjs_blecentral_reqdata_t *iotjs_blecentral_reqwrap_data(THIS) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_blecentral_reqwrap_t,
                                blecentral_reqwrap);

  return &_this->req_data;
}

iotjs_blecentral_reqwrap_t *
iotjs_blecentral_reqwrap_from_request(uv_work_t *req) {
  return (iotjs_blecentral_reqwrap_t *)
      iotjs_reqwrap_from_request((uv_req_t *)req);
}
#undef THIS

void iotjs_blecentral_event_callback(BlecentralEv ev, void *args) {
  const iotjs_jval_t *obj = iotjs_blecentral_get_jblecentral();
  iotjs_jval_t func = iotjs_jval_get_property(obj, "onEvent");
  iotjs_string_t jstr_ev;
  iotjs_jargs_t argv;

  switch (ev) {
    case kBlecentralEvStateChange:
      iotjs_blecentral_state_change_res_t *res =
          (iotjs_blecentral_state_change_res_t *)args;
      
      argv = iotjs_jargs_create(2);
      
      jstr_ev = iotjs_string_create("statChange");
      iotjs_string_t jstr_state = 
          iotjs_string_create((const char*)res->state);

      iotjs_jargs_append_string(&argv, &jstr_ev);
      iotjs_jargs_append_string(&argv, &jsr_state);

      iotjs_make_callback(&func, obj, &argv);

      iotjs_string_destroy(&jsr_ev);
      iotjs_string_destroy(&jsr_state);

      break;
    case kBlecentralEvScanStart:
      argv = iotjs_jargs_create(1);

      jstr_ev = iotjs_string_create("scanStart");
      iotjs_jargs_append_string(&argv, &jstr_ev);

      iotjs_make_callback(&func, obj, &argv);

      iotjs_string_destroy(&jstr_ev);

      break;
    case kBlecentralEvScanStop:
      argv = iotjs_jargs_create(1);

      jstr_ev = iotjs_string_create("scanStop");
      iotjs_jargs_append_string(&argv, &jstr_ev);

      iotjs_make_callback(&func, obj, &argv);

      iotjs_string_destroy(&jstr_ev);

      break;
    case kBlecentralEvDiscover:
      iotjs_blecentral_discover_res_t *res =
          (iotjs_blecentral_discover_res_t *)args;
      argv = iotjs_jargs_create(7);

      jstr_ev = iotjs_string_create("discover");
      iotjs_string_t juuid = 
          iotjs_string_create((const char *)res->uuid);
      iotjs_string_t jaddr =
          iotjs_string_create((const char *)res->address);
      iotjs_string_t jaddr_type =
          iotjs_string_create((res->addr_type == 1)? "random" : "public");
      iotjs_string_t jlocal_name =
          iotjs_string_create((const char *)res->local_name);

      iotjs_jargs_append_string(&argv, &jstr_ev);
      iotjs_jargs_append_string(&argv, &juuid);
      iotjs_jargs_append_string(&argv, &jaddr);
      iotjs_jargs_append_string(&argv, &jaddr_type);
      iotjs_jargs_append_number(&argv, res->connnectable);
      iotjs_jargs_append_string(&argv, &jlocal_name);
      iotjs_jargs_append_number(&argv, res->rssi);

      iotjs_make_callback(&func, obj, &argv);

      iotjs_string_destroy(&jstr_ev);
      iotjs_string_destroy(&juuid);
      iotjs_string_destroy(&jaddr);
      iotjs_string_destroy(&jaddr_type);
      iotjs_string_destroy(&jlocal_name);

      break;
  }


  iotjs_jargs_destroy(&argv);
}

// serviceUuids, allowDuplicates
JHANDLER_FUNCTION(StartScanning) {
  JHANDLER_CHECK_ARGS(2, array, number);

  /* TODO: After array binding implementation
  iotjs_jval_t jsvc_uuid = JHANDLER_GET_ARG(0, array); */
  int jallow_duplicates = (int)JHANDLER_GET_ARG(1, number);

  BlecentralStartScanning(NULL, jallow_duplicates);
}

// void
JHANDLER_FUNCTION(StopScanning) {
  JHANDLER_CHECK_ARGS(0);
  
  BlecentralStopScanning();
}

JHANDLER_FUNCTION(Init) {
  JHANDLER_CHECK_ARGS(0);
  
  BlecentralCreate();
}

iotjs_jval_t InitBlecentral() {
  iotjs_jval_t jblecentral = iotjs_jval_create_object();

  // Ble-central
  iotjs_jval_set_method(&jblecentral, "startScanning", StartScanning);
  iotjs_jval_set_method(&jblecentral, "stopScanning", StopScanning);

  iotjs_jval_t on_event = iotjs_jval_create_object();
  iotjs_jval_set_property_jval(&jblecentral, "onEvent", &on_event);
  // EventListener
  iotjs_jval_set_method(&jblecentral, "init", Init);

  iotjs_blecentral_t* blecentral = iotjs_blecentral_create(&jblecentral);

  return jblecentral;
}
