/**
 * \file OneWire.h
 * Defines functions to use the DS2482 I²C to 1-Wire controller as a (mostly) "drop-in" replacement for the commonly
 * used OneWire library on Arduino-like platforms.
 *
 * \date		2017
 * \author		Gerad Munsch <gmunsch@unforgivendevelopment.com>
 * \copyright	See README.md for more information about authors and copyrights.
 */

#ifndef _DS2482OW__SRC_ONEWIRE_H__
#define _DS2482OW__SRC_ONEWIRE_H__


#include <inttypes.h>

#if defined(__AVR__)
/* ---( AVR-specific stuff )--- */
#include <util/crc16.h>
#define PLATFORM_HAS_PROGMEM_AVAILABLE		/*! Indicates that the platform has the various \b PROGMEM features */
#endif



#if ARDUINO >= 100
/* ---( Includes that are specific versions 1.0 and up of the Arduino IDE )--- */
#include "Arduino.h"		/* Required for delayMicroseconds, digitalPinToBitMask, etc.. */
#else
/* ---( Includes that are specific to OLD versions of the Arduino IDE, which are not in common use anymore )--- */
#include "WProgram.h"		/* Required for delayMicroseconds */
#include "pins_arduino.h"	/* Required for digitalPinToBitMask, etc.. */
#endif


/**
 * \def ONEWIRE_USE_CRC8_TABLE	Defines the behavior of CRC calculation. The behavior is dependent on the value defined;
 *								if the value is defined as 1, use the pre-computed, table-based CRC calculation. If the
 *								defined value is 0, or if the value is not defined at all, compute the CRC values in an
 *								on-demand fashion. Other values will result in an error.
 *
 * \note	Pre-computed, table-based CRC calculations result in faster calculations, at the expense of consuming more
 *			of the available flash memory.
 *			Real-time computation of CRC values results in a smaller code size, uses less flash memory, however, on many
 *			devices, computation of CRC values is significantly slower than calculation via the lookup table method.
 */
#define ONEWIRE_USE_CRC8_TABLE 			1


/**
 * \defgroup definitions	Define device constants, such as commands, register values, etc.
 * @{
 */
#define DS2482_COMMAND_RESET		0xF0	/*!The command to issue a reset. */
#define DS2482_COMMAND_SRP			0xE1	/*!The command to set the read pointer. */

#define DS2482_POINTER_STATUS		0xF0	/*! */
#define DS2482_POINTER_DATA			0xE1	/*! */
#define DS2482_POINTER_CONFIG		0xC3	/*! */

#define DS2482_COMMAND_WRITECONFIG	0xD2	/*! */
#define DS2482_COMMAND_RESETWIRE	0xB4	/*! */
#define DS2482_COMMAND_WRITEBYTE	0xA5	/*! */
#define DS2482_COMMAND_READBYTE		0x96	/*! */
#define DS2482_COMMAND_SINGLEBIT	0x87	/*! */
#define DS2482_COMMAND_TRIPLET		0x78	/*! */

#define WIRE_COMMAND_SKIP			0xCC	/*! The 1-Wire protocol command to issue a "SKIP_ROM" */
#define WIRE_COMMAND_SELECT			0x55	/*! The 1-Wire protocol command to issue a "SELECT_ROM" */
#define WIRE_COMMAND_SEARCH			0xF0	/*! The 1-Wire protocol command to issue a "SEARCH_ROM" */


#define DS2482_STATUS_BUSY			(1<<0)	/*! */
#define DS2482_STATUS_PPD			(1<<1)	/*! */
#define DS2482_STATUS_SD			(1<<2)	/*! */
#define DS2482_STATUS_LL			(1<<3)	/*! */
#define DS2482_STATUS_RST 			(1<<4)	/*! */
#define DS2482_STATUS_SBR			(1<<5)	/*! */
#define DS2482_STATUS_TSB 			(1<<6)	/*! */
#define DS2482_STATUS_DIR 			(1<<7)	/*! */

#define DS2482_CONFIG_APU			(1<<0)	/*! */
#define DS2482_CONFIG_SPU			(1<<2)	/*! */
#define DS2482_CONFIG_1WS			(1<<3)	/*! */

#define DS2482_ERROR_TIMEOUT		(1<<0)	/*! */
#define DS2482_ERROR_SHORT			(1<<1)	/*! */
#define DS2482_ERROR_CONFIG			(1<<2)	/*! */


/**
 * \class OneWire	Provides an interface to the 1-Wire bus.
 */
class OneWire {
public:
	/**
	 * The constructor for an instance of a OneWire object. This creates a new instance of a OneWire object (ie: a
	 * distinct physical bus) which is able to be utilized.
	 *
	 * \brief Instantiates a OneWire bus object.
	 */
	OneWire();

	/**
	 * The constructor for an instance of a OneWire object, with an I²C address specified for the DS2482 chip. This
	 * creates a new instance of a OneWire object (ie: a distinct physical bus) which is able to be utilized.
	 *
	 * \param[in]	address	The 7-bit I²C address of the DS2482 chip, in the form of an unsigned integer.
	 */
	OneWire(uint8_t address);

	/**
	 * \fn getAddress
	 *
	 * \return
	 */
	uint8_t getAddress();

	/**
	 * \fn getError
	 *
	 * \return
	 */
	uint8_t getError();

	/**
	 * \fn checkPresence
	 *
	 * \return
	 */
	uint8_t checkPresence();

	/**
	 * \fn deviceReset
	 */
	void deviceReset();

	/**
	 * Sets the read pointer to the specified register. This action overwrites the read pointer position of any 1-Wire
	 * communication command that is currently in progress.
	 *
	 * \brief Sets the read pointer to the specified register
	 *
	 * \param[in]	readPointer		An unsigned integer indicating the desired location of the read pointer.
	 */
	void setReadPointer(uint8_t readPointer);

	/**
	 *
	 * \return
	 */
	uint8_t readStatus();

	/**
	 *
	 * \return
	 */
	uint8_t readData();

	/**
	 *
	 * \return
	 */
	uint8_t waitOnBusy();

	/**
	 *
	 * \return
	 */
	uint8_t readConfig();

	/**
	 * Write data to the config register. The function accepts an unsigned byte value, with the lower 4 bits containing
	 * the desired config register values. The necessary function of setting the upper 4 bits to the one's complement of
	 * the lower 4 bits before writing the data to the register is handled within this function. Confirmation that the
	 * write was successful is done by reading back the config register, and comparing it against the original byte
	 * value, as any read actions from the config register have the upper 4 bits set to 0000b; as such, the values
	 * should match.
	 *
	 * \brief Write data to the config register.
	 *
	 * \param[in]	config	An unsigned byte value, with the lower 4 bits containing the desired config register values.
	 */
	void writeConfig(uint8_t config);

	/**
	 * Enables the strong pullup (SPU) for the next bus operation. After execution of the operation with the strong
	 * pullup enabled, it will be deactivated, and will need to be set once again, if it is necessary for another
	 * operation.
	 *
	 * \brief Enables the strong pullup for the next bus operation.
	 */
	void setStrongPullup();

	/**
	 *
	 */
	void clearStrongPullup();

	/**
	 *
	 * \return
	 */
	uint8_t wireReset();

	/**
	 *
	 * \param data
	 * \param power
	 */
	void wireWriteByte(uint8_t data, uint8_t power = 0);
	
	void wireWriteBytes(const uint8_t *dbuf, uint16_t count, uint8_t power);

	/**
	 *
	 * \return
	 */
	uint8_t wireReadByte();

	/**
	 *
	 * \param data
	 * \param power
	 */
	void wireWriteBit(uint8_t data, uint8_t power = 0);

	/**
	 *
	 * \return
	 */
	uint8_t wireReadBit();

	/**
	 *
	 */
	void wireSkip();

	/**
	 *
	 * \param rom
	 */
	void wireSelect(const uint8_t rom[8]);

	/**
	 *
	 */
	void wireResetSearch();

	/**
	 *
	 * \param address
	 * \return
	 */
	uint8_t wireSearch(uint8_t *address);

	/**
	 * \defgroup emuFuncs	Functions which emulate the "standard" Arduino OneWire library, in an attempt to provide
	 *						"drop-in" replacement compatibility, for easy migration to a dedicated OneWire controller.
	 * @{
	 *
	 * <H1>"Emulated" functions to provide compatibility with the "standard" OneWire library</H1>
	 *
	 * <P>The following functions are intended to match the API and functionality provided by the "standard" library</P>
	 *
	 * <H2>Resources for the "standard" OneWire library</H2><BR>
	 * <UL>
	 *   <LI><A HREF="https://www.pjrc.com/teensy/td_libs_OneWire.html">Library Home Page</A></LI>
	 *   <LI><A HREF="https://github.com/PaulStoffregen/OneWire">Library Source Code</A></LI>
	 * </UL>
	 */

	/**
	 * Issues a bus reset, which causes all devices on the bus to reset, and issue a presence pulse. Devices are then
	 * left in a state where they are able to be enumerated using the \b SEARCH_ROM 1-Wire protocol command.
	 *
	 * \brief Issues a bus reset.
	 *
	 * \return		Indicates the presence (or lack thereof) of devices on the bus, or a possible issue with the bus.
	 * \retval	0	Indicates that there are no devices on the bus, or that the bus is in an error condition, such as a
	 *				short, or that the bus was held low for too long.
	 * \retval	1	Indicates that one (or more) devices responded to the reset with a presence pulse.
	 */
	uint8_t reset(void);

	/**
	 * Issues a \b SELECT_ROM command, selecting the device indicated by the provided device serial number for any
	 * subsequent communication.
	 *
	 * \brief Issues a SELECT_ROM command
	 *
	 * \param[in]	rom	An array of 8 unsigned integers (bytes), which represents the 64-bit 1-Wire serial number of the
	 *					device to be selected.
	 */
	void select(const uint8_t rom[8]);

	/**
	 * Issues a \b SKIP_ROM command, effectively selecting all devices on the bus. This command can be particularly of
	 * use when the bus contains only a single device, forgoing the need to do a \b SEARCH_ROM and \b SELECT_ROM to use
	 * that device. Another example of a scenario where the \b SKIP_ROM function can be useful is a bus that contains
	 * only one type of device, such as the DS18B20, where the \b CONVERT_T (0x44) command can be issued to all of the
	 * DS18B20 sensors simultaneously, followed by reading out each device's reading using the \b SELECT_ROM method.
	 *
	 * \brief Issues a SKIP_ROM command
	 */
	void skip(void);

	/**
	 * Write one byte to the 1-Wire bus.
	 *
	 * \brief Write one byte.
	 *
	 * \param[in]	v		The byte to be written to the bus.
	 * \param[in]	power	An optional parameter, used when there are parasitically-powered devices present on the bus,
	 *						indicating that the data line should remain high after the operation completes.
	 *
	 * \note	If 'power' was set to '1', then the bus is held high after the operation completes, allowing recovery
	 *			time for parasitically-powered devices. It is necessary to manually "depower" the bus at some point,
	 *			either by calling "depower()", or doing another read or write operation (with 'power' set to '0').
	 */
	void write(uint8_t v, uint8_t power = 0);

	/**
	 * Write multiple bytes to the 1-Wire bus.
	 *
	 * \param buf
	 * \param count
	 * \param power
	 */
	void write_bytes(const uint8_t *buf, uint16_t count, uint8_t power = 0);


	/**
	 * Read a single byte from the 1-Wire bus.
	 *
	 * \brief Read one byte.
	 *
	 * \return The byte that was read from the 1-Wire bus.
	 */
	uint8_t read(void);

	/**
	 * Read multiple bytes from the 1-Wire bus, storing them in a data buffer.
	 *
	 * \param[out]	buf	 	The data buffer to which the data read from the 1-Wire bus will be stored to
	 * \param[in]	count	The amount of bytes to read from the 1-Wire bus
	 */
	void read_bytes(uint8_t *buf, uint16_t count);


	/**
	 * Write one bit to the 1-Wire bus.
	 *
	 * \brief Write one bit.
	 *
	 * \param[in]	v	An unsigned integer, where the least significant bit is the bit to be written to the bus.
	 */
	void write_bit(uint8_t v);


	/**
	 * Read one bit from the 1-Wire bus.
	 *
	 * \brief Read one bit.
	 *
	 * \return	The bit that has been read from the bus, in the form of an unsigned integer, where the least significant
	 *			bit is the bit that was read from the bus.
	 */
	uint8_t read_bit(void);

	/**
	 * Remove power from the bus. Only necessary after a write() call with the 'power' parameter set to '1', or if the
	 * write_bit() function was called.
	 *
	 * \brief Remove power from the bus.
	 *
	 * \note This function may not be applicable in the context of the DS2482 series of devices.
	 */
	void depower(void);

	/**
	 *
	 */
	void reset_search();

	/**
	 *
	 * \param family_code
	 */
	void target_search(uint8_t family_code);

	/**
	 *
	 * \param newAddr
	 * \return
	 */
	uint8_t search(uint8_t *newAddr);

	/**
	 *
	 * \param addr
	 * \param len
	 * \return
	 */
	static uint8_t crc8(const uint8_t *addr, uint8_t len);

	/**
	 * @}
	 */

private:
	void begin();
	uint8_t end();
	void writeByte(uint8_t);
	uint8_t readByte();

	uint8_t mAddress;
	uint8_t mError;

	uint8_t searchAddress[8];
	uint8_t searchLastDiscrepancy;
	uint8_t searchLastDeviceFlag;
};

#endif	/* _DS2482OW__SRC_ONEWIRE_H__ */
