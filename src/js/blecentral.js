/* The MIT License (MIT)
 *
 * Copyright (c) 2013 Sandeep Mistry
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

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

/* This file include some APIs in 'noble'.
 * (https://github.com/sandeepmistry/noble)
 */

var EventEmiter = require('events').EventEmitter;
var util = require('util');
var blecentral = process.binding(process.binding.blecentral);

function Blecentral(){

  console.log('start blecentral');
  this.state = 'unknown';
  this.address = 'unknown';

  this._peripherals = {};
  this._discoveredPeripheralUuids = [];

  this._scanState = null;
  this.scanFilterDuplicates = null;
  this._discoveredPeripheralUuids = [];
  this._discoveries = {};

  EventEmiter.call(this);

  // this.on('stateChange', this.onStateChange.bind(this));

  // TODO: need to implement more functionalities.

  this.on('warning', function(message) {
    if(this.listeners('warning').length === 1) {
      console.log('blecentral warning: ' + message);
    }
  }.bind(this));

  console.log('pre init');

  blecentral.init(function(state){
    // this.emit('stateChange', state);
    this.onStateChange(state).bind(this);
  }.bind(this));

  console.log('init complete');

  process.on('exit', function(){
    if(this._scanState != 'stopped') {
      this.stopScanning();
    }
  }.bind(this));
}

util.inherits(Blecentral, EventEmiter);

Blecentral.prototype._runLoop = function() {
  console.log('_runLoop');
  if(this._scanState = 'started') {
    blecentral.runLoop(this.loopCallback.bind(this));
  }
};

// callback function for various event function
Blecentral.prototype.loopCallback = function(cmd, a1, a2, a3, a4, a5, a6) {

  console.log('loopcallback');

  if(cmd == 'stateChange'){
    // args = state
    if(a1 != this.state) {
      this.emit('stateChange', a1);
    }
  } else if(cmd == 'advertisingReport'){
    // args = status, type, address, addressType, eir, rssi
    this.advertisingReport(a1, a2, a3, a4, a5, a6).bind(this);
  }

  setTimeout(this._runLoop.bind(this), 1000);
}

Blecentral.prototype.onStateChange = function(state) {
  console.log('on state change');
  this.state = state;

  if(state == 'unauthorized'){
    this.emit('warning', 'adapter state un authorized.');
  }

  this.emit('stateChange', state);
};

Blecentral.prototype.startScanning = function(serviceUuids,
                                              allowDuplicates, callback) {

  console.log('start scanning');
  if(this.state !== 'poweredOn') {
    var error = 'Could not start scanning, state is '
        + this.state + '(not poweredOn)';
    console.error(error);
    if(typeof callback === 'function') {
      callback(error);
    }
  } else {

    this_scanState = 'starting';

    this._discoveredPeripheralUuids = [];
    this._allowDuplicates = allowDuplicates;

    this._scanServiceUuids = serviceUuids || [];
    this._scanFilterDuplicates = !allowDuplicates;

    blecentral.startScanning(this._scanFilterDuplicates, function(err) {
      if(err !== 0) {
        // need to handle unexpected error in here
        callback(err);
        return;
      }
      if(this._scanState === 'starting') {
        this._scanState = 'started';

        this.emit('scanStart', this._scanFilterDuplicates);
      } else {
        // the _scanState is changed elsewhere (not mine)
        // need to handle this
      }
    }.bind(this));

    setTimeout(this._runLoop.bind(this), 1000);
  }
};

Blecentral.prototype.stopScanning = function(callback){
  this._scanState = 'stopping';

  blecentral.stopScanning(function(err) {
    if(err !== 0) {
      callback(err);
      return;
    }
    if(this._scanState === 'stopping') {
      this._scanState = 'stopped';

      this.emit('scanStop');
    }
  }.bind(this));
};

Blecentral.prototype.leDiscover = function(status, address, addressType,
                                           connectable, advertisement, rssi) {
  // if(type === 0x04 || discoveryCount > 1 && !hasScanResponse || ...)

  var uuid = address.split(':').join('');
  var peripheral = this_peripherals[uuid];

  if(!peripheral) {
    pheripheral = new Peripheral(this, uuid, address, addressType, connectable,
                                 advertisement, rssi);

    this._peripherals[uuid] = peripheral;
    this._services[uuid] = {};
    this._characteristics[uuid] = {};
    this._descriptors[uuid] = {};
  } else {
    for (var i in advertisement) {
      if ( advertisement[i] !== undefined) {
        peripheral.advertisement[i] = advertisement[i];
      }
    }

    peripheral.rssi = rssi;
  }

  if(this._allowDuplicates) {
    this.emit('discover', peripheral);
  }
};

Blecentral.prototype.advertisingReport = function(status, type, address,
                                                    addressType, eir, rssi) {
  var previouslyDiscovered = !!this._discoveries[address];
  var advertisement = previouslyDiscovered ?
      this._discoveries[address].advertisement : {
        localName: undefined,
        txPowerLevel: undefined,
        manufacturerData: undefined,
        serviceData: [],
        serviceUuids: [],
        solicitationServiceUuids:[]
      };

  var discoveryCount = previouslyDiscovered ?
      this._discoveries[address].count : 0;
  var hasScanResponse = previouslyDiscovered ?
      this._discoveries[address].hasScanResponse : false;

  if (type === 0x04) {
    hasScanResponse = true;
  } else {
    advertisement.serviceData = [];
    advertisement.serviceUuids = [];
    advertisement.serviceSolicitationUuids = [];
  }

  discoveryCount++;

  var connectable = (type=== 0x04 && previosulyDiscovered) ?
      this._discoveries[address].connectable : (type !== 0x03);

  this._discoveries[address] = {
    address: address,
    addressType: addressType,
    connectable: connectable,
    rssi: rssi,
    count: discoveryCount,
    hasScanResponse: hasScanResponse
  };

  this.leDiscover(status, address, addressType, connectable, advertisement,
                  rssi);
};

function Peripheral(blecentral, id, address, addressType, connectable,
                    advertisement, rssi) {
  this._blecentral = blecentral;
  this.id = id;
  this.uuid = id;
  this.address = address;
  this.addressType = addressType;
  this.connectable = connectable;
  this.advertisement = advertisement;
  this.rssi = rssi;
  this.services = null;
  this.state = 'disconnected';
};

module.exports = new Blecentral();
