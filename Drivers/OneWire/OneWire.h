/*
 * This file is part of the SSN project.
 *
 * Copyright (C) 2014-2015 Ernold Vasiliev <ericv@mail.ru>
 *
 */

#ifndef _OWI_DEVICE_H_
#define _OWI_DEVICE_H_

#include "stm32f1xx_hal.h"

#define mainMAX_OWI_DEVICES				(30) // default max number of 1-wire devices

/****************************************************************************
 ROM commands (common)
****************************************************************************/
#define     OWI_ROM_READ    0x33    //!< READ ROM command code.
#define     OWI_ROM_SKIP    0xcc    //!< SKIP ROM command code.
#define     OWI_ROM_MATCH   0x55    //!< MATCH ROM command code.
#define     OWI_ROM_SEARCH  0xf0    //!< SEARCH ROM command code.
#define     OWI_READ_POWER_SUPPLY 0xB4

/****************************************************************************
 Return codes
****************************************************************************/
#define     OWI_ROM_SEARCH_FINISHED     0x00    //!< Search finished return code.
#define     OWI_ROM_SEARCH_FAILED       0xff    //!< Search failed return code.
#define     OWI_CRC_OK      0x00    //!< CRC check succeded
#define     OWI_CRC_ERROR   0x01    //!< CRC check failed

#define SEARCH_SUCCESSFUL     0x00
#define SEARCH_CRC_ERROR      0x01
#define SEARCH_ERROR          0xff
#define AT_FIRST              0xff

#define TIM_MST TIM3

// Common structure for all OWI devices
typedef struct
{
     unsigned char ucROM[8];
} OWI_device;

typedef struct
{
    GPIO_TypeDef	*pPort;		// the port of the device group
    uint16_t  	         ucPin;		// the pin of the device group or SDA for soft i2c
    TIM_HandleTypeDef 	*pTimer; // the timer of the device group
} sGrpDev;

typedef struct
{
    uint16_t	uiObj;			// device's object
	uint8_t 	uiGroup;		// device group number
//	uint8_t 	ucDevRate;		// group scan rate
	uint8_t 	iDevQty;		// device quantity in group
    sGrpDev 	GrpDev;			//  physical describing of the device group
} sGrpInfo;

//  -- common functions for all bb-control devices
void 	delay_us(sGrpDev* pGrpDev, uint32_t nCount);	// nCount 16 bit!!!
void 	delay_ms(sGrpDev* pGrpDev, uint32_t nCount);	// nCount 16 bit!!!

uint8_t bb_read_wire_data_bit(sGrpDev* pGrpDev);
void	bb_clear_wire(sGrpDev* pGrpDev);
void 	bb_set_wire(sGrpDev* pGrpDev);

// -- for owi devices:
OWI_device* owi_device_init(sGrpDev* pGrpDev);
void 	owi_device_delete (OWI_device* pOWIDev);

uint8_t owi_reset_pulse(sGrpDev* pGrpDev);
void 	owi_write_bit(volatile uint8_t bit,sGrpDev* pGrpDev);
uint16_t owi_read_bit(sGrpDev* pGrpDev);
void 	owi_write_byte(volatile uint8_t byte, sGrpDev* pGrpDev);
uint8_t owi_read_byte(sGrpDev* pGrpDev);
unsigned char owi_search_rom(unsigned char * bitPattern, unsigned char lastDeviation, sGrpDev* pGrpDev);
unsigned char owi_search_devices(OWI_device * devices, uint8_t numDevices, sGrpInfo* pGroup, uint8_t *num);
unsigned char owi_ComputeCRC8(unsigned char inData, unsigned char seed);
unsigned char owi_CheckRomCRC(unsigned char * romValue);
void	owi_SkipRom(sGrpDev* pGrpDev);
void	owi_ReadRom(sGrpDev* pGrpDev, OWI_device* pDev);
void 	owi_MatchRom(sGrpDev* pGrpDev, OWI_device* pDev);
void 	owi_device_put_rom(OWI_device* pDev, char * romValue);
uint8_t charToInt(char letter);

#endif
