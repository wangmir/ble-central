/* The MIT License (MIT)
 *
 * Copyright (c) 2013 Sandeep Mistry
 * Originated from noble, npm project in node.js community.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Copyright 2016 Kim, Hyukjoong (wangmir@gmail.com)
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

var EventEmitter = require('events').EventEmitter;
var util = require ('util');
var assert = require('assert');

var bleCentralBuiltin = process.binding(process.binding.blecentral);
function bleCentral() {
  this.state = 'unknown';
  this.address = 'unknown';

  this._peripherals = {};
  this._services = {};
  this._characteristics = {};
  this._descriptors = {};
  this._discoveredPeripheralUUids = [];

  EventEmitter.call(this);

  this.on('stateChange', this.onStateChange);
  this.on('scanStart', this.onScanStart);
  this.on('scanStop', this.onScanStop);
  this.on('discover', this.onDiscover);

  this.on('warning', function(message) {
    if(this.listeners('warning').length === 1) {
      console.warn('bleCentral: ' + message);
    }
  }.bind(this));

  bleCentralBuiltin.onEvent = function(event, args){
    switch (event) {
      case 'stateChange' :
        this.emit('stateChange', args[0]);
        break;
      case 'scanStart' :
        this.emit('scanStart', args[0]);
        break;
      case 'scanStop' :
        this.emit('scanStop');
        break;
      case 'discover' :
        this.emit('discover', args[0], args[1], args[2],
                  args[3], args[4], args[5]);
        break;
    }

  }.bind(this));

  bleCentralBuiltin.init();
}

util.inherits(bleCentral, EventEmitter);

bleCentral.prototype.onStateChange = function(state) {

  this.state = state;

  this.emit('stateChange', state);
};

bleCentral.prototype.startScanning = function(serviceUuids,
                                              allowDuplicates, callback) {
  if(this.stae !== 'poweredOn') {
    var error = new Error('Could not start scanning, state is '
                          + this.state + ' (not poweredOn)');

    if(typeof callback === 'function') {
      callback(error);
    } else {
      throw error;
    }
  } else {
    if (callback) {
      this.once('scanStart', callback);
    }

    this._discoveredPeripheralUUids = [];
    this._allowDuplicates = allowDuplicates;

    bleCentralBuiltin.startScanning(serviceUuids, allowDuplicates);
  }
};

bleCentral.prototype.onScanStart = function(filterDuplicates) {
  this.emit('scanStart', filterDuplicates);
};

bleCentral.prototype.stopScanning = function(callback) {
  if(callback){
    this.once('scanStop', callbank);
  }
  bleCentralBuiltin.stopScanning();
};

bleCentral.prototype.onScanStop = function() {
  this.emit('scanStop');
};

bleCentral.prototype.onDiscover = function(
    uuid, address, addressType, connectable, advertisement, rssi) {
  var peripheral = this._peripherals[uuid];

  if(!peripheral) {
    peripheral = new Peripheral(this, uuid, address, addressType,
                                connectable, advertisement, rssi);

    this._peripherals[uuid] = peripheral;
    this._service[uuid] = {};
    this._characteristics[uuid] = {};
    this._descriptors[uuid] = {};
  } else{
    for(var i in advertisement) {
      if(advertisement[i] !== undefined) {
        peripheral.advertisement[i] = advertisement[i];
      }
    }

    peripheral.rssi = rssi;
  }

  var previouslyDiscoverd =
      (this._discoveredPeripheralUUids.indexOf(uuid) !== -1);

  if(!previouslyDiscoverd) {
    this._discoveredPeripheralUUids.push(uuid);
  }

  if(this._allowDuplicates || !previouslyDiscoverd) {
    this.emit('discover', peripheral);
  }
};

// Peripheral part (originated from peripheral.js on Noble)
function Peripheral(bleCentral, id, address, addressType, connectable,
                    advertisement, rssi) {
  this._bleCentral = bleCentral;

  this.id = id;
  this.uuid = id;
  this.address = address;
  this.addressType = addressType;
  this.connectable = connectable;
  this.advertisement = advertisement;
  this.rssi = rssi;
  this.services = null;
  this.state = 'disconnected';
}

module.exports = blecentral;
