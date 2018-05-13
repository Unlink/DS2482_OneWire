/**
 * \file OneWire.cpp
 *
 * Portions Copyright (C) 2017 Gerad Munsch <gmunsch@unforgivendevelopment.com>
 * See README.md for additional author/copyright info.
 */

/* ---------------------------------------------------------------------------- */
/* INCLUDES                                                                     */
/* ---------------------------------------------------------------------------- */
#include "OneWire.h"
#include <Wire.h>

/**
 * Constructor with no parameters for compatability with OneWire lib
 */
OneWire::OneWire() {
	/* Address is determined by two pins on the DS2482 AD1/AD0 */
	/* Pass 0b00, 0b01, 0b10 or 0b11 */
	mAddress = 0x18;
	mError = 0;
	Wire.begin();
}


OneWire::OneWire(uint8_t address) {
	/* Address is determined by two pins on the DS2482 AD1/AD0 */
	/* Pass 0b00, 0b01, 0b10 or 0b11 */
	mAddress = 0x18 | address;
	mError = 0;
	Wire.begin();
}


uint8_t OneWire::getAddress() {
	return mAddress;
}

uint8_t OneWire::getError() {
	return mError;
}

/**
 * Helper functions to make dealing with I2C side easier
 */
void OneWire::begin() {
	Wire.beginTransmission(mAddress);
}


uint8_t OneWire::end() {
	return Wire.endTransmission();
}


void OneWire::writeByte(uint8_t data) {
	Wire.write(data);
}


uint8_t OneWire::readByte() {
	Wire.requestFrom(mAddress, 1u);
	return Wire.read();
}

/**
 * Simply starts and ends an Wire transmission
 * If no devices are present, this returns false
 */
uint8_t OneWire::checkPresence() {
	begin();
	return !end() ? true : false;
}

/**
 * Performs a global reset of device state machine logic. This action
 * terminates any ongoing 1-Wire communication.
 */
void OneWire::deviceReset() {
	begin();
	write(DS2482_COMMAND_RESET);
	end();
}


void OneWire::setReadPointer(uint8_t readPointer) {
	begin();
	writeByte(DS2482_COMMAND_SRP);
	writeByte(readPointer);
	end();
}


/**
 * Read the status register
 */
uint8_t OneWire::readStatus() {
	setReadPointer(DS2482_POINTER_STATUS);
	return readByte();
}


/**
 * Read the data register
 */
uint8_t OneWire::readData() {
	setReadPointer(DS2482_POINTER_DATA);
	return readByte();
}


/**
 * Read the config register
 */
uint8_t OneWire::readConfig() {
	setReadPointer(DS2482_POINTER_CONFIG);
	return readByte();
}


/**
 * Set the strong pullup bit. The strong pullup bit is used to activate the strong pullup function prior to a '1-Wire
 * Write Byte' or a '1-Wire Single Bit' command. Strong pullup is commonly used with 1-Wire EEPROM devices when copying
 * scratchpad data to the main memory, when performing an SHA-1 computation, and/or with parasitically-powered devices,
 * such as temperature sensors or A/D converters. See the respective device data sheets for information as to the timing
 * and use of the SPU (strong pullup) bit throughout the communications protocol.
 *
 * The SPU bit must be set immediately prior to issuing the command that puts the 1-Wire device into the state where it
 * needs the extra power.
 *
 * IMPORTANT: The SPU bit also affects the 1-Wire Reset command. If enabled, it can cause incorrect reading of the
 *            presence pulse, and may cause a violation of the device's absolute maximum rating.
 *
 * Many details about the use of the SPU bit are located on Page 7 of the DS2482-100 datasheet.
 *
 * @brief Activates the strong pullup function for the following transaction.
 */
void OneWire::setStrongPullup() {
	writeConfig(readConfig() | DS2482_CONFIG_SPU);
}


/**
 * Manually clears the SPU (strong pullup) bit manually, in the event that the other triggers have not been fired.
 *
 * @brief Manually clear the strong pullup bit in the DS2482 config register.
 */
void OneWire::clearStrongPullup() {
	writeConfig(readConfig() & !DS2482_CONFIG_SPU);
}


/**
 * Wait for a limited period of time for the busy bit in the status register to clear. If the timeout is reached, it is
 * likely an error has occurred.
 *
 * @brief Wait for a brief period of time to allow the 1-Wire bus to become free.
 */
uint8_t OneWire::waitOnBusy() {
	uint8_t status;

	/* Check register status every 20 microseconds */
	for (int i = 1000; i > 0; i--) {
		status = readStatus();

		/* Break out of loop if the busy status bit clears up */
		if (!(status & DS2482_STATUS_BUSY)) {
			break;
		}

		/* Wait 20 microseconds before checking the status again... */
		delayMicroseconds(20);
	}

	/* It is likely an error has occurred if the busy status bit is still set */
	if (status & DS2482_STATUS_BUSY) {
		mError = DS2482_ERROR_TIMEOUT;
	}

	/* Return the status so we don't need to explicitly do it again */
	return status;
}


void OneWire::writeConfig(uint8_t config) {
	waitOnBusy();
	begin();
	writeByte(DS2482_COMMAND_WRITECONFIG);

	/*
	 * The config register expects its data in the following format:
	 * - Bytes 0-3: config data
	 * - Bytes 4-7: one's complement of bytes 0-3
	 */
	writeByte(config | (~config)<<4);
	end();

	/*
	 * Readback of the config register will return data in the following format:
	 * - Bytes 0-3: config data
	 * - Bytes 4-7: 0000b
	 */
	if (readByte() != config) {
		mError = DS2482_ERROR_CONFIG;
	}
}


/**
 * Generates a 1-Wire reset/presence-detect cycle on the 1-Wire line. Note that
 * a diagram of this process can be found at figure 4 of the official datasheet
 * for the DS2482-100.
 *
 * The state of the 1-Wire line is sampled at tSI and tMSP, and the result is
 * reported to the host processor through the status register bits 'PPD' & 'SD'.
 */
uint8_t OneWire::wireReset() {
	waitOnBusy();

	/*
	 * Ensure that the SPU (strong pullup) bit is cleared before execution, as
	 * its use may result in 'PPD' containing invalid data, and/or device(s)
	 * violating their absolute maximum ratings.
	 * (Noted on page 10 of the DS2482-100 datasheet)
	 */
	clearStrongPullup();

	waitOnBusy();

	begin();
	writeByte(DS2482_COMMAND_RESETWIRE);
	end();

	uint8_t status = waitOnBusy();

	if (status & DS2482_STATUS_SD) {
		mError = DS2482_ERROR_SHORT;
	}

	return (status & DS2482_STATUS_PPD) ? true : false;
}


/**
 * Write a single data byte to the 1-Wire line. Optionally, the strong pullup bit may be activated, causing the strong
 * pullup to take effect for this transaction.
 *
 * @brief Write one byte of data to the 1-Wire bus.
 *
 * @param[in]	data	An unsigned byte value containing the byte to be written to the 1-Wire line.
 * @param[in]	power	An optional, unsigned byte value, activates the SPU function when containing a value >= 1.
 */
void OneWire::wireWriteByte(uint8_t data, uint8_t power) {
	waitOnBusy();

	if (power) {
		setStrongPullup();
	}

	begin();
	writeByte(DS2482_COMMAND_WRITEBYTE);
	writeByte(data);
	end();
}

/**
 * Write multiple bytes to the 1-Wire bus.
 */
void OneWire::wireWriteBytes(const uint8_t *dbuf, uint16_t count, uint8_t power) {
	waitOnBusy();

	/* begin the I2C transaction */
	begin();

	/* write out all of our data in one continuous I2C transaction */
	for (uint16_t i = 0; i < count; i++) {
		/* set the strong pullup bit before each write, if necessary */
		if (power) {
			uint8_t cfgreg = 0x00;
			Wire.write(DS2482_COMMAND_SRP);
			Wire.write(DS2482_POINTER_CONFIG);
			Wire.requestFrom(mAddress, 1u);
			cfgreg = Wire.read();
			cfgreg = (cfgreg | DS2482_CONFIG_SPU);
			Wire.write(DS2482_COMMAND_WRITECONFIG);
			Wire.write(cfgreg | (~cfgreg)<<4);
			Wire.requestFrom(mAddress, 1u);
			if (Wire.read() != cfgreg) {
				mError = DS2482_ERROR_CONFIG;
			}
		}

		/* write the current byte */
		Wire.write(DS2482_COMMAND_WRITEBYTE);
		Wire.write(dbuf[i]);
	}

	/* end the I2C transaction */
	end();
}

/**
 * Generates eight read-data time slots on the 1-Wire line and stores result in the Read Data Register.
 */
uint8_t OneWire::wireReadByte() {
	waitOnBusy();

	begin();
	writeByte(DS2482_COMMAND_READBYTE);
	end();

	waitOnBusy();

	return readData();
}

/**
 * Generates a single 1-Wire Time Slot with a bit value “V”, as specified by the bit byte, at the 1-Wire line (Table 2).
 * A "V" value of 0b generates a Write-Zero Time Slot (Figure 5); a "V" value of 1b generates a Write-One Time Slot,
 * which also functions as a Read-Data Time Slot (Figure 6). In any case, the logic level at the 1-Wire line is queried
 * at 'tMSR', and 'SBR' is updated.
 *
 * NOTE: See the DS2482-100 datasheet for the tables and figures referenced above.
 *
 * @brief Generates a single 1-Wire timeslot.
 *
 * @param[in]	data	An unsigned byte value, with the MSB containing the bit which determines the action to be taken.
 * @param[in]	power	An optional, unsigned byte value, activates the SPU function when containing a value >= 1.
 */
void OneWire::wireWriteBit(uint8_t data, uint8_t power) {
	waitOnBusy();
	if (power)
		setStrongPullup();
	begin();
	writeByte(DS2482_COMMAND_SINGLEBIT);
	writeByte(data ? 0x80 : 0x00);
	end();
}

// As wireWriteBit
uint8_t OneWire::wireReadBit() {
	wireWriteBit(1);
	uint8_t status = waitOnBusy();
	return status & DS2482_STATUS_SBR ? 1 : 0;
}

// 1-Wire skip
void OneWire::wireSkip() {
	wireWriteByte(WIRE_COMMAND_SKIP);
}

void OneWire::wireSelect(const uint8_t rom[8]) {
	wireWriteByte(WIRE_COMMAND_SELECT);
	for (int i=0;i<8;i++)
		wireWriteByte(rom[i]);
}

//  1-Wire reset seatch algorithm
void OneWire::wireResetSearch() {
	searchLastDiscrepancy = 0;
	searchLastDeviceFlag = 0;

	for (int i = 0; i < 8; i++) 	{
		searchAddress[i] = 0;
	}

}

// Perform a search of the 1-Wire bus
uint8_t OneWire::wireSearch(uint8_t *address) {
	uint8_t direction;
	uint8_t last_zero=0;

	if (searchLastDeviceFlag) {
		return 0;
	}

	if (!wireReset()) {
		return 0;
	}

	waitOnBusy();

	wireWriteByte(WIRE_COMMAND_SEARCH);

	for (uint8_t i = 0; i < 64; i++) {
		int searchByte = i / 8;
		int searchBit = 1 << i % 8;

		if (i < searchLastDiscrepancy) {
			direction = searchAddress[searchByte] & searchBit;
		} else {
			direction = i == searchLastDiscrepancy;
		}

		waitOnBusy();
		begin();
		writeByte(DS2482_COMMAND_TRIPLET);
		writeByte(direction ? 0x80 : 0x00);
		end();

		uint8_t status = waitOnBusy();

		uint8_t id = status & DS2482_STATUS_SBR;
		uint8_t comp_id = status & DS2482_STATUS_TSB;
		direction = status & DS2482_STATUS_DIR;

		if (id && comp_id) {
			return 0;
		} else {
			if (!id && !comp_id && !direction) {
				last_zero = i;
			}
		}

		if (direction) {
			searchAddress[searchByte] |= searchBit;
		} else {
			searchAddress[searchByte] &= ~searchBit;
		}
	}

	searchLastDiscrepancy = last_zero;

	if (!last_zero) {
		searchLastDeviceFlag = 1;
	}

	for (uint8_t i = 0; i < 8; i++) {
		address[i] = searchAddress[i];
	}

	return 1;
}

#if (ONEWIRE_USE_CRC8_TABLE == 1)
/**
 * \var dscrc_table	A pre-computed table (array) used in the calculation of CRC values.
 *
 * \note		Note that, on the AVR architecture, the array is stored in PROGMEM.
 * \note		This table comes from Dallas sample code, where it is indicated to be freely reusable.
 * \copyright	Dallas Semiconductor Corporation
 * \date		2000
 */
#ifdef PLATFORM_HAS_PROGMEM_AVAILABLE
/**
 * \var dscrc_table	A pre-computed table (array) used in the calculation of CRC values.
 *
 * \note			For devices which provide PROGMEM functionality, dscrc_table is declared as a PROGMEM variable type,
 *					which causes the pre-computed tables to be stored in the device's flash memory, as opposed to being
 *					stored in RAM -- which can make a significant difference on embedded devices, which typically have a
 *					significantly larger amount of flash memory as compared to RAM.
 * \note			This table comes from Dallas sample code, where it is indicated to be freely reusable.
 * \copyright		Dallas Semiconductor Corporation
 * \date			2000
 */
static const uint8_t dscrc_table[] PROGMEM = {
#else
/**
 * A pre-computed table (array) used in the calculation of CRC values.
 *
 * \note Note that, on the AVR architecture, the array is stored in PROGMEM.
 * \note This table comes from Dallas sample code, where it is indicated to be freely reusable.
 * \author		Dallas Semiconductor Corporation
 * \copyright	Dallas Semiconductor Corporation
 * \date		2000
 *
 * \note Converted to hexadecimal by Gerad Munsch
 * \author		Gerad Munsch <gmunsch@unforgivendevelopment.com>
 * \date		2017-07-31
 */
static const uint8_t dscrc_table[] = {
#endif
	0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
	0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E, 0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC,
	0x23, 0x7D, 0x9F, 0xC1, 0x42, 0x1C, 0xFE, 0xA0, 0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62,
	0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D, 0x7C, 0x22, 0xC0, 0x9E, 0x1D, 0x43, 0xA1, 0xFF,
	0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5, 0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07,
	0xDB, 0x85, 0x67, 0x39, 0xBA, 0xE4, 0x06, 0x58, 0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A,
	0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6, 0xA7, 0xF9, 0x1B, 0x45, 0xC6, 0x98, 0x7A, 0x24,
	0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B, 0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9,
	0x8C, 0xD2, 0x30, 0x6E, 0xED, 0xB3, 0x51, 0x0F, 0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD,
	0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92, 0xD3, 0x8D, 0x6F, 0x31, 0xB2, 0xEC, 0x0E, 0x50,
	0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C, 0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE,
	0x32, 0x6C, 0x8E, 0xD0, 0x53, 0x0D, 0xEF, 0xB1, 0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73,
	0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49, 0x08, 0x56, 0xB4, 0xEA, 0x69, 0x37, 0xD5, 0x8B,
	0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4, 0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16,
	0xE9, 0xB7, 0x55, 0x0B, 0x88, 0xD6, 0x34, 0x6A, 0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8,
	0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7, 0xB6, 0xE8, 0x0A, 0x54, 0xD7, 0x89, 0x6B, 0x35
};



/**
 * Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM and the registers.
 *
 * \note This might better be done without the table, it would certainly be smaller and likely fast enough
 * compared to all those delayMicrosecond() calls.  But I got
 * confused, so I use this table from the examples.)
 */
uint8_t OneWire::crc8(const uint8_t *addr, uint8_t len) {
	uint8_t crc = 0;
	
	while (len--) {
#ifdef PLATFORM_HAS_PROGMEM_AVAILABLE
		crc = *(pgm_read_byte(dscrc_table + (crc ^ *addr++)));
#else
		crc = *(dscrc_table + (crc ^ *addr++));
#endif
	}

	return crc;
}

#elif (ONEWIRE_USE_CRC8_TABLE == 0) || !defined(ONEWIRE_USE_CRC8_TABLE)

// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but much smaller, than the lookup table.
//
uint8_t OneWire::crc8(const uint8_t *addr, uint8_t len) {
	uint8_t crc = 0;

	while (len--) {
		uint8_t inbyte = *addr++;
		for (uint8_t i = 8; i; i--) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;

			if (mix) {
				crc ^= 0x8C;
			}

			inbyte >>= 1;
		}
	}

	return crc;
}
#endif	/* ONEWIRE_USE_CRC8_TABLE */

// ****************************************
// These are here to mirror the functions in the original OneWire
// ****************************************

// This is a lazy way of getting compatibility with DallasTemperature
// Not all functions are implemented, only those used in DallasTemeperature
void OneWire::reset_search() {
	wireResetSearch();
}

uint8_t OneWire::search(uint8_t *newAddr) {
	return wireSearch(newAddr);
}

// Perform a 1-Wire reset cycle. Returns 1 if a device responds
// with a presence pulse.  Returns 0 if there is no device or the
// bus is shorted or otherwise held low for more than 250uS
uint8_t OneWire::reset(void) {
	return wireReset();
}

// Issue a 1-Wire rom select command, you do the reset first.
void OneWire::select(const uint8_t rom[8]) {
	wireSelect(rom);
}

// Issue a 1-Wire rom skip command, to address all on bus.
void OneWire::skip(void) {
	wireSkip();
}


void OneWire::write(uint8_t v, uint8_t power) {
	wireWriteByte(v, power);
}


uint8_t OneWire::read(void) {
	return wireReadByte();
}


uint8_t OneWire::read_bit(void) {
	return wireReadBit();
}


void OneWire::write_bit(uint8_t v) {
	wireWriteBit(v);
}

// ****************************************
// End mirrored functions
// ****************************************
