import objc
from Foundation import NSObject, NSRunLoop, NSDate, NSDefaultRunLoopMode
from CoreBluetooth import (CBCentralManager, CBCentralManagerStatePoweredOn,
                           CBUUID, CBAdvertisementDataServiceUUIDsKey,
                           CBAdvertisementDataManufacturerDataKey,
                           CBAdvertisementDataLocalNameKey)
import time, sys

TARGET_UUID = CBUUID.UUIDWithString_("0000bd26-0000-1000-8000-00805f9b34fb")
found = {}

class ScanDelegate(NSObject):
    def init(self):
        self = objc.super(ScanDelegate, self).init()
        self.ready = False
        return self

    def centralManagerDidUpdateState_(self, mgr):
        if mgr.state() == CBCentralManagerStatePoweredOn:
            self.ready = True
            print("BLE powered on, scanning...")
            mgr.scanForPeripheralsWithServices_options_([TARGET_UUID], None)
        else:
            print(f"BLE state: {mgr.state()}")

    def centralManager_didDiscoverPeripheral_advertisementData_RSSI_(self, mgr, peripheral, ad, rssi):
        addr = peripheral.identifier().UUIDString()
        if addr in found:
            return
        mfg = ad.get(CBAdvertisementDataManufacturerDataKey)
        cs, sc, st = '', 0, 0
        if mfg and len(mfg) >= 13:
            raw = bytes(mfg)
            cs = raw[:10].decode('utf-8', errors='replace').rstrip('\x00')
            sc = raw[10] | (raw[11] << 8)
            st = raw[12]
        found[addr] = (cs, sc, st, int(rssi))
        print(f"  FOUND: {cs} rssi={rssi} score={sc} status={st} addr={addr}")

delegate = ScanDelegate.alloc().init()
mgr = CBCentralManager.alloc().initWithDelegate_queue_(delegate, None)

print("Scanning for BSidesKC badges (15s)...")
deadline = time.time() + 15
while time.time() < deadline:
    NSRunLoop.currentRunLoop().runMode_beforeDate_(NSDefaultRunLoopMode, NSDate.dateWithTimeIntervalSinceNow_(0.5))

mgr.stopScan()
if not found:
    print("No badges found. Is BLE enabled?")
else:
    print(f"\n{len(found)} badge(s) detected")
