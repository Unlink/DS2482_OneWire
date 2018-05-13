/**
 * \file OneWireExtraHelpers.cpp
 *
 * \author	Gerad Munsch <gmunsch@unforgivendevelopment.com>
 * \date	2017
 * \note	See README.md for any additional author/copyright info.
 */

#include "OneWireExtraHelpers.h"

#ifdef USE_DALLASTEMPERATURE_LIBRARY
#include <DallasTemperature.h>

void OneWireHelpersClass::printDeviceAddress(uint8_t devAddr[8]) {
	Serial.print("{ ");

	for (uint8_t i = 0; i < 8; i++) {
		/* Print "0x" hexadecimal prefix */
		Serial.print("0x");

		/* Pad printed value with a zero if the value is less than 16; this ensures consistent 2-digit octets */
		if (devAddr[i] < 16) {
			Serial.print("0");
		}

		/* Print value as hexadecimal */
		Serial.print(devAddr[i], HEX);
		if (i < 7) {
			Serial.print(", ");
		}
	}

	Serial.print(" }");
}
#endif


#ifdef USE_DALLASTEMPERATURE_LIBRARY
void OneWireHelpersClass::printDeviceAddress(DeviceAddress devAddr) {
	Serial.print("{ ");

	for (uint8_t i = 0; i < 8; i++) {
		/* Print "0x" hexadecimal prefix */
		Serial.print("0x");

		/* Pad printed value with a zero if the value is less than 16; this ensures consistent 2-digit octets */
		if (devAddr[i] < 16) {
			Serial.print("0");
		}

		/* Print value as hexadecimal */
		Serial.print(devAddr[i], HEX);
		if (i < 7) {
			Serial.print(", ");
		}
	}

	Serial.print(" }");
}
#endif
