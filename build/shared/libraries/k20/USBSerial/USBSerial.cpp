// Public Methods //////////////////////////////////////////////////////////////
#include "USBSerial.h"

USBSerialClass USBSerial = USBSerialClass();

void USBSerialClass::begin() {
  usbserial_begin();
}
