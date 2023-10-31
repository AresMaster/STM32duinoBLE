/*
  This file is part of the ArduinoBLE library.
  Copyright (c) 2018 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "utility/ATT.h"
#include "utility/GAP.h"
#include "utility/GATT.h"
#include "utility/L2CAPSignaling.h"

#include "BLELocalDevice.h"

BLELocalDevice::BLELocalDevice(HCITransportInterface *HCITransport, uint8_t ownBdaddrType) :
  _HCITransport(HCITransport), _ownBdaddrType(ownBdaddrType)
{
  _advertisingData.setFlags(BLEFlagsGeneralDiscoverable | BLEFlagsBREDRNotSupported);
}

BLELocalDevice::~BLELocalDevice()
{
}

int BLELocalDevice::begin()
{
  HCI.setTransport(_HCITransport);

  if (!HCI.begin()) {
    end();
    return 0;
  }

  delay(100);

  if (HCI.reset() != 0) {
    end();

    return 0;
  }

  uint8_t randomNumber[8];
  if (HCI.leRand(randomNumber) != 0) {
    end();
    return 0;
  }
  /* Random address only requires 6 bytes (48 bits)
   * Force both MSB bits to b00 in order to define Static Random Address
   */
  randomNumber[5] |= 0xC0;

  // Copy the random address in private variable as it will be sent to the BLE chip
  randomAddress [0] = randomNumber[0];
  randomAddress [1] = randomNumber[1];
  randomAddress [2] = randomNumber[2];
  randomAddress [3] = randomNumber[3];
  randomAddress [4] = randomNumber[4];
  randomAddress [5] = randomNumber[5];
  // @note Set Random address only when type is STATIC_RANDOM_ADDR
  if (HCI.leSetRandomAddress((uint8_t*)randomNumber) != 0 && _ownBdaddrType == STATIC_RANDOM_ADDR) {
    end();
    return 0;
  }
  // @note Save address to HCI.localaddress variable, which is used to encryption in pairing
  if(_ownBdaddrType == PUBLIC_ADDR){
    HCI.readBdAddr();
  }else{
    for(int k=0; k<6; k++){
      HCI.localAddr[5-k] = randomAddress[k];
    }
  }

  uint8_t hciVer;
  uint16_t hciRev;
  uint8_t lmpVer;
  uint16_t manufacturer;
  uint16_t lmpSubVer;

  if (HCI.readLocalVersion(hciVer, hciRev, lmpVer, manufacturer, lmpSubVer) != 0) {
    end();
    return 0;
  }

  if (HCI.setEventMask(0x2000F0FFFFFFF0FF) != 0) {
    end();
    return 0;
  }
  if (HCI.setLeEventMask(0x0000000000000193) != 0) {
    end();
    return 0;
  }

  uint16_t pktLen;
  uint8_t maxPkt;

  if (HCI.readLeBufferSize(pktLen, maxPkt) != 0) {
    end();
    return 0;
  }

  /// The HCI should allow automatic address resolution.

  // // If we have callbacks to remember bonded devices:
  // if(HCI._getIRKs!=0){
  //   uint8_t nIRKs = 0;
  //   uint8_t** BADDR_Type = new uint8_t*;
  //   uint8_t*** BADDRs = new uint8_t**;
  //   uint8_t*** IRKs = new uint8_t**;
  //   uint8_t* memcheck;


  //   if(!HCI._getIRKs(&nIRKs, BADDR_Type, BADDRs, IRKs)){
  //     Serial.println("error");
  //   }
  //   for(int i=0; i<nIRKs; i++){
  //     Serial.print("Baddr type: ");
  //     Serial.println((*BADDR_Type)[i]);
  //     Serial.print("BADDR:");
  //     for(int k=0; k<6; k++){
  //       Serial.print(", 0x");
  //       Serial.print((*BADDRs)[i][k],HEX);
  //     }
  //     Serial.println();
  //     Serial.print("IRK:");
  //     for(int k=0; k<16; k++){
  //       Serial.print(", 0x");
  //       Serial.print((*IRKs)[i][k],HEX);
  //     }
  //     Serial.println();

  //     // save this 
  //     uint8_t zeros[16];
  //     for(int k=0; k<16; k++) zeros[15-k] = 0;
      
  //     // HCI.leAddResolvingAddress((*BADDR_Type)[i],(*BADDRs)[i],(*IRKs)[i], zeros);

  //     delete[] (*BADDRs)[i];
  //     delete[] (*IRKs)[i];
  //   }
  //   delete[] (*BADDR_Type);
  //   delete BADDR_Type;
  //   delete[] (*BADDRs);
  //   delete BADDRs;
  //   delete[] (*IRKs);
  //   delete IRKs;
    
  //   memcheck = new uint8_t[1];
  //   Serial.print("nIRK location: 0x");
  //   Serial.println((int)memcheck,HEX);
  //   delete[] memcheck;

  // }

  GATT.begin();

  GAP.setOwnBdaddrType(_ownBdaddrType);

  ATT.setOwnBdaddrType(_ownBdaddrType);

  return 1;
}

void BLELocalDevice::end()
{
  GATT.end();

  HCI.end();
}

void BLELocalDevice::getRandomAddress(uint8_t buff[6])
{
  buff [0] = randomAddress[0];
  buff [1] = randomAddress[1];
  buff [2] = randomAddress[2];
  buff [3] = randomAddress[3];
  buff [4] = randomAddress[4];
  buff [5] = randomAddress[5];
}

void BLELocalDevice::poll()
{
  HCI.poll();
}

void BLELocalDevice::poll(unsigned long timeout)
{
  HCI.poll(timeout);
}

bool BLELocalDevice::connected() const
{
  HCI.poll();

  return ATT.connected();
}

/*
 * Whether there is at least one paired device
 */
bool BLELocalDevice::paired()
{
  HCI.poll();

  return ATT.paired();
}

bool BLELocalDevice::disconnect()
{
  return ATT.disconnect();
}

String BLELocalDevice::address() const
{
  uint8_t addr[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  // @note return correct device address when is set to STATIC RANDOM (set by HCI)
  if(_ownBdaddrType==PUBLIC_ADDR)
    HCI.readBdAddr(addr);
  else
    for(int k=0; k<6; k++){
        addr[k]=HCI.localAddr[5-k];
    }

  char result[18];
  sprintf(result, "%02x:%02x:%02x:%02x:%02x:%02x", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

  return result;
}

int BLELocalDevice::rssi()
{
  BLEDevice central = ATT.central();

  if (central) {
    return central.rssi();
  }

  return 127;
}

bool BLELocalDevice::setAdvertisedServiceUuid(const char* advertisedServiceUuid)
{
  return _advertisingData.setAdvertisedServiceUuid(advertisedServiceUuid);
}

bool BLELocalDevice::setAdvertisedService(const BLEService& service)
{
  return setAdvertisedServiceUuid(service.uuid());
}

bool BLELocalDevice::setAdvertisedServiceData(uint16_t uuid, const uint8_t data[], int length)
{
  return _advertisingData.setAdvertisedServiceData(uuid, data, length);
}

bool BLELocalDevice::setManufacturerData(const uint8_t manufacturerData[], int manufacturerDataLength)
{
  return _advertisingData.setManufacturerData(manufacturerData, manufacturerDataLength);
}

bool BLELocalDevice::setManufacturerData(const uint16_t companyId, const uint8_t manufacturerData[], int manufacturerDataLength)
{
  return _advertisingData.setManufacturerData(companyId, manufacturerData, manufacturerDataLength);
}

bool BLELocalDevice::setLocalName(const char *localName)
{
  return _scanResponseData.setLocalName(localName);  
}

void BLELocalDevice::setAdvertisingData(BLEAdvertisingData& advertisingData)
{
  _advertisingData = advertisingData;
  if (!_advertisingData.hasFlags()) {
    _advertisingData.setFlags(BLEFlagsGeneralDiscoverable | BLEFlagsBREDRNotSupported);
  }
}

void BLELocalDevice::setScanResponseData(BLEAdvertisingData& scanResponseData)
{
  _scanResponseData = scanResponseData;
}

BLEAdvertisingData& BLELocalDevice::getAdvertisingData()
{
  return _advertisingData;
}

BLEAdvertisingData& BLELocalDevice::getScanResponseData()
{
  return _scanResponseData;
}

void BLELocalDevice::setDeviceName(const char* deviceName)
{
  GATT.setDeviceName(deviceName);
}

void BLELocalDevice::setAppearance(uint16_t appearance)
{
  GATT.setAppearance(appearance);
}

void BLELocalDevice::addService(BLEService& service)
{
  GATT.addService(service);
}

int BLELocalDevice::advertise()
{
  _advertisingData.updateData();
  _scanResponseData.updateData();
  return GAP.advertise( _advertisingData.data(), _advertisingData.dataLength(), 
                        _scanResponseData.data(), _scanResponseData.dataLength());
}

void BLELocalDevice::stopAdvertise()
{
  GAP.stopAdvertise();
}

int BLELocalDevice::scan(bool withDuplicates)
{
  return GAP.scan(withDuplicates);
}

int BLELocalDevice::scanForName(String name, bool withDuplicates)
{
  return GAP.scanForName(name, withDuplicates);
}

int BLELocalDevice::scanForUuid(String uuid, bool withDuplicates)
{
  return GAP.scanForUuid(uuid, withDuplicates);
}

int BLELocalDevice::scanForAddress(String address, bool withDuplicates)
{
  return GAP.scanForAddress(address, withDuplicates);
}

int BLELocalDevice::stopScan()
{
  return GAP.stopScan();
}

BLEDevice BLELocalDevice::central()
{
  HCI.poll();

  return ATT.central();
}

BLEDevice BLELocalDevice::available()
{
  HCI.poll();

  return GAP.available();
}

void BLELocalDevice::setEventHandler(BLEDeviceEvent event, BLEDeviceEventHandler eventHandler)
{
  if (event == BLEDiscovered) {
    GAP.setEventHandler(event, eventHandler);
  } else {
    ATT.setEventHandler(event, eventHandler);
  }
}

void BLELocalDevice::setAdvertisingInterval(uint16_t advertisingInterval)
{
  GAP.setAdvertisingInterval(advertisingInterval);
}

void BLELocalDevice::setConnectionInterval(uint16_t minimumConnectionInterval, uint16_t maximumConnectionInterval)
{
  L2CAPSignaling.setConnectionInterval(minimumConnectionInterval, maximumConnectionInterval);
}

void BLELocalDevice::setSupervisionTimeout(uint16_t supervisionTimeout)
{
  L2CAPSignaling.setSupervisionTimeout(supervisionTimeout);
}

void BLELocalDevice::setConnectable(bool connectable)
{
  GAP.setConnectable(connectable);
}

void BLELocalDevice::setTimeout(unsigned long timeout)
{
  ATT.setTimeout(timeout);
}

/*
 * Control whether pairing is allowed or rejected
 * Use true/false or the Pairable enum
 */
void BLELocalDevice::setPairable(uint8_t pairable)
{
  L2CAPSignaling.setPairingEnabled(pairable);
}

/*
 * Whether pairing is currently allowed
 */
bool BLELocalDevice::pairable()
{
  return L2CAPSignaling.isPairingEnabled();
}

void BLELocalDevice::setGetIRKs(int (*getIRKs)(uint8_t* nIRKs, uint8_t** BADDR_type, uint8_t*** BADDRs, uint8_t*** IRKs)){
  HCI._getIRKs = getIRKs;
}
void BLELocalDevice::setGetLTK(int (*getLTK)(uint8_t* BADDR, uint8_t* LTK)){
  HCI._getLTK = getLTK;
}
void BLELocalDevice::setStoreLTK(int (*storeLTK)(uint8_t*, uint8_t*)){
  HCI._storeLTK = storeLTK;
}
void BLELocalDevice::setStoreIRK(int (*storeIRK)(uint8_t*, uint8_t*)){
  HCI._storeIRK = storeIRK;
}
void BLELocalDevice::setDisplayCode(void (*displayCode)(uint32_t confirmationCode)){
  HCI._displayCode = displayCode;
}
void BLELocalDevice::setBinaryConfirmPairing(bool (*binaryConfirmPairing)()){
  HCI._binaryConfirmPairing = binaryConfirmPairing;
}

void BLELocalDevice::debug(Stream& stream)
{
  HCI.debug(stream);
}

void BLELocalDevice::noDebug()
{
  HCI.noDebug();
}
