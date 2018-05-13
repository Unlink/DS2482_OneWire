/**
 * \file OneWireExtraHelpers.h
 *
 * \author	Gerad Munsch <gmunsch@unforgivendevelopment.com>
 * \date	2017
 * \note	See README.md for any additional author/copyright info.
 */
 
#ifndef _DS2482OW__SRC_ONEWIREEXTRAHELPERS_H__
#define _DS2482OW__SRC_ONEWIREEXTRAHELPERS_H__

#include <inttypes.h>

#ifdef USE_DALLASTEMPERATURE_LIBRARY
	#include <DallasTemperature.h>

#endif

class OneWireHelpersClass {
public:
	/**
	 * \fn printDeviceAddress	Provides a function to print a 1-Wire device's serial number to the serial console, as a
	 *							nicely formatted string.
	 *
	 * \param[in]	devAddr	An array of 8 unsigned byte values which represent the device's serial number.
	 */
	void printDeviceAddress(uint8_t devAddr[8]);

#ifdef USE_DALLASTEMPERATURE_LIBRARY
	/**
	 * \fn printDeviceAddress	Provides a function to print a 1-Wire device's serial number to the serial console, as a
	 *							nicely formatted string.
	 *
	 * \param[in]	devAddr	The 64-bit serial number of the 1-Wire device, stored in a DeviceAddress object, which is
	 *						provided by the DallasTemperature library.
	 */
	void printDeviceAddress(DeviceAddress devAddr);
#endif
};


#endif	/* _DS2482OW__SRC_ONEWIREEXTRAHELPERS_H__ */
