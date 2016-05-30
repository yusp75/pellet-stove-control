/*
 * This file is part of the SSN project.
 *
 * Copyright (C) 2014-2015 Ernold Vasiliev <ericv@mail.ru>
 *
*/

/*
 * owi_device.c
 *
 *  Created on: 25 §æ§Ö§Ó§â. 2014 §Ô.
 *      Author: eric
 *      Common functions for bit banding control device
 */

#include "OneWire.h"
#include "FreeRTOS.h"


void delay_us(sGrpDev* pGrpDev, uint32_t nCount) {
	volatile uint16_t TIMCounter = nCount;
	/* Counter enable. */
	/* Reset prescaler value. */
        pGrpDev->pTimer->Init.Prescaler = 71;
        pGrpDev->pTimer->Init.CounterMode = TIM_COUNTERMODE_DOWN;
	HAL_TIM_Base_Init(pGrpDev->pTimer);
        
	/* Start timer. */
	HAL_TIM_Base_Start(pGrpDev->pTimer);
        
	while (TIMCounter > 1) {
		TIMCounter = TIM_MST->CNT;
	}
	HAL_TIM_Base_Stop(pGrpDev->pTimer);

}

void delay_ms(sGrpDev* pGrpDev, uint32_t nCount) {
	volatile uint16_t TIMCounter = nCount;
	/* Counter enable. */
	/* Reset prescaler value. */
        pGrpDev->pTimer->Init.Prescaler = 7199;
        pGrpDev->pTimer->Init.CounterMode = TIM_COUNTERMODE_DOWN;
	HAL_TIM_Base_Init(pGrpDev->pTimer);
        
	/* Start timer. */
	HAL_TIM_Base_Start(pGrpDev->pTimer);
        
	while (TIMCounter > 1) {
		TIMCounter = TIM_MST->CNT;
	}
	HAL_TIM_Base_Stop(pGrpDev->pTimer);
}

// Read input data bit from pGrpDev->ucPin
uint8_t bb_read_wire_data_bit(sGrpDev* pGrpDev)
{
	volatile uint8_t result = 0;
	result = HAL_GPIO_ReadPin(pGrpDev->pPort, pGrpDev->ucPin);

	if (result != 0)
	  {
		result = 1;
	  }
	  else
	  {
		 result = 0;
	  }
	return result;
}


/* clear pGrpDev->ucPin
*
*/
void bb_clear_wire(sGrpDev* pGrpDev)
{
	HAL_GPIO_WritePin(pGrpDev->pPort, 1<<pGrpDev->ucPin, GPIO_PIN_RESET);
}

/* set pGrpDev->ucPin
*
*/
void bb_set_wire(sGrpDev* pGrpDev)
{
	HAL_GPIO_WritePin(pGrpDev->pPort, 1<<pGrpDev->ucPin, GPIO_PIN_SET);
}


//*********************************************************************************************
//function  reset pulse for OWI device				                                          //
//argument  device group                                                                      //
//return    0 - §å§ã§ä§â§à§Û§ã§ä§Ó§à §à§Ò§ß§Ñ§â§å§Ø§Ö§ß, 1 - §ß§Ö §à§Ò§ß§Ñ§â§å§Ø§Ö§ß§à, 2 - §Ü.§Ù. §ß§Ñ §Ý§Ú§ß§Ú§Ú                   //
//*********************************************************************************************
uint8_t owi_reset_pulse(sGrpDev* pGrpDev)
{
	uint16_t result = 0;

	if (bb_read_wire_data_bit(pGrpDev) == 0)
		return 2; // check line for sort circuit

	bb_clear_wire(pGrpDev);
	delay_us(pGrpDev, 480);

	result = bb_read_wire_data_bit(pGrpDev);
	bb_set_wire(pGrpDev);
	delay_us(pGrpDev, 500);
	if (result)
		return 1;                            // owi sensor not found
	return 0;                                // Success
}

//*********************************************************************************************
//function  one bit send                            	                                     //
//argument  bit value, device group information		                                         //
//return    none                                                                             //
//*********************************************************************************************
void owi_write_bit(volatile uint8_t bit, sGrpDev* pGrpDev)
{
	bb_clear_wire(pGrpDev);
	delay_us(pGrpDev, 2);
	if (bit) {
//		send '1'
		bb_set_wire(pGrpDev);
	}
	delay_us(pGrpDev, 60);
}

//*********************************************************************************************
//function  one bit read                                                                     //
//argument  device group information                                                         //
//return    read bit                             		                                     //
//*********************************************************************************************
uint16_t owi_read_bit(sGrpDev* pGrpDev) {
	
        uint16_t result = 0;
	bb_clear_wire(pGrpDev);

	delay_us(pGrpDev, 15);
	result = bb_read_wire_data_bit(pGrpDev);

	delay_us(pGrpDev, 45);
	return result;
}

//*********************************************************************************************
//function  one byte write                                                                   //
//argument  sending byte, device group information                                            //
//return    none                                                                             //
//*********************************************************************************************
void owi_write_byte(volatile uint8_t byte, sGrpDev* pGrpDev)
{
   uint8_t i;

   for(i=0; i < 8; i++) {
	   owi_write_bit(byte & (0x01 << i), pGrpDev);
   }
}
//*********************************************************************************************
//function  one byte read                                                                    //
//argument  device group information                                                         //
//return    read byte                                                                 //
//*********************************************************************************************
uint8_t owi_read_byte(sGrpDev* pGrpDev)
{
   uint8_t i,result = 0;

   for(i=0;i<8;i++)
	   if(owi_read_bit(pGrpDev)) result |= 1<<i;
   return result;
}

//*********************************************************************************************
OWI_device*  owi_device_init(sGrpDev* pGrpDev) {

	OWI_device* pOWIDev;
	pOWIDev = (OWI_device*) pvPortMalloc(sizeof(OWI_device));

	if (!pOWIDev) return pOWIDev;

	return pOWIDev;
}

void owi_device_delete (OWI_device* pOWIDev)
{
	vPortFree((void*)pOWIDev);
}


/*! \brief  Compute the CRC8 value of a data set.
 *
 *  This function will compute the CRC8 or DOW-CRC of inData using seed
 *  as inital value for the CRC.
 *
 *  \param  inData  One byte of data to compute CRC from.
 *
 *  \param  seed    The starting value of the CRC.
 *
 *  \return The CRC8 of inData with seed as initial value.
 *
 *  \note   Setting seed to 0 computes the crc8 of the inData.
 *
 *  \note   Constantly passing the return value of this function
 *          As the seed argument computes the CRC8 value of a
 *          longer string of data.
 */
unsigned char owi_ComputeCRC8(unsigned char inData, unsigned char seed)
{
    unsigned char bitsLeft;
    unsigned char temp;

    for (bitsLeft = 8; bitsLeft > 0; bitsLeft--)
    {
        temp = ((seed ^ inData) & 0x01);
        if (temp == 0)
        {
            seed >>= 1;
        }
        else
        {
            seed ^= 0x18;
            seed >>= 1;
            seed |= 0x80;
        }
        inData >>= 1;
    }
    return seed;
}

/*! \brief  Calculate and check the CRC of a 64 bit ROM identifier.
 *
 *  This function computes the CRC8 value of the first 56 bits of a
 *  64 bit identifier. It then checks the calculated value against the
 *  CRC value stored in ROM.
 *
 *  \param  romvalue    A pointer to an array holding a 64 bit identifier.
 *
 *  \retval OWI_CRC_OK      The CRC's matched.
 *  \retval OWI_CRC_ERROR   There was a discrepancy between the calculated and the stored CRC.
 */
unsigned char owi_CheckRomCRC(unsigned char * romValue)
{
    unsigned char i;
    unsigned char crc8 = 0;

    for (i = 0; i < 7; i++)
    {
        crc8 = owi_ComputeCRC8(*romValue, crc8);
        romValue++;
    }
    if (crc8 == (*romValue))
    {
        return OWI_CRC_OK;
    }
    return OWI_CRC_ERROR;
}

/*! \brief  Sends the SKIP ROM command to the 1-Wire bus(es).
 *
 *  \param  pins    A bitmask of the buses to send the SKIP ROM command to.
 */
void owi_SkipRom(sGrpDev* pGrpDev)
{
    // Send the SKIP ROM command on the bus.
    owi_write_byte(OWI_ROM_SKIP, pGrpDev);
}

/*! \brief  Sends the READ ROM command and reads back the ROM id.
 *
 *  \param  romValue    A pointer where the id will be placed.
 *
 *  \param  pin     A bitmask of the bus to read from.
 */
void owi_ReadRom(sGrpDev* pGrpDev, OWI_device* pDev)
{
    unsigned char bytesLeft = 8;
    unsigned char * romValue = pDev->ucROM;

    // Send the READ ROM command on the bus.
    owi_write_byte(OWI_ROM_READ, pGrpDev);

    // Do 8 times.
    while (bytesLeft > 0)
    {
        // Place the received data in memory.
    	*romValue++ = owi_read_byte(pGrpDev);
        bytesLeft--;
    }
}


/*! \brief  Sends the MATCH ROM command and the ROM id to match against.
 *
 *  \param  romValue    A pointer to the ID to match against.
 *
 *  \param  pins    A bitmask of the buses to perform the MATCH ROM command on.
 */
void owi_MatchRom(sGrpDev* pGrpDev, OWI_device* pDev)
{
    unsigned char bytesLeft = 8;
    unsigned char * romValue = pDev->ucROM;
    // Send the MATCH ROM command.
    owi_write_byte(OWI_ROM_MATCH, pGrpDev);

    // Do once for each byte.
    while (bytesLeft > 0)
    {
        // Transmit 1 byte of the ID to match.
        owi_write_byte(*romValue++, pGrpDev);
        bytesLeft--;
    }
}

/*! \brief  Sends the SEARCH ROM command and returns 1 id found on the
 *          1-Wire(R) bus.
 *
 *  \param  bitPattern      A pointer to an 8 byte char array where the
 *                          discovered identifier will be placed. When
 *                          searching for several slaves, a copy of the
 *                          last found identifier should be supplied in
 *                          the array, or the search will fail.
 *
 *  \param  lastDeviation   The bit position where the algorithm made a
 *                          choice the last time it was run. This argument
 *                          should be 0 when a search is initiated. Supplying
 *                          the return argument of this function when calling
 *                          repeatedly will go through the complete slave
 *                          search.
 *
 *  \param  pin             A bit-mask of the bus to perform a ROM search on.
 *
 *  \return The last bit position where there was a discrepancy between slave addresses the last time this function was run. Returns OWI_ROM_SEARCH_FAILED if an error was detected (e.g. a device was connected to the bus during the search), or OWI_ROM_SEARCH_FINISHED when there are no more devices to be discovered.
 *
 *  \note   See main.c for an example of how to utilize this function.
 */
unsigned char owi_search_rom(unsigned char * bitPattern, unsigned char lastDeviation, sGrpDev* pGrpDev)
{
    unsigned char currentBit = 1;
    unsigned char newDeviation = 0;
    unsigned char bitMask = 0x01;
    unsigned char bitA;
    unsigned char bitB;
    uint8_t tmp;

    tmp = owi_reset_pulse(pGrpDev);          // send pulse
    if(tmp) return tmp;

// Send SEARCH ROM command on the bus.
    owi_write_byte(OWI_ROM_SEARCH,pGrpDev);

// Walk through all 64 bits.
    while (currentBit <= 64)
    {
// Read bit from bus twice.
        bitA = owi_read_bit(pGrpDev);
        bitB = owi_read_bit(pGrpDev);

        if (bitA && bitB)
        {
// Both bits 1 (Error).
            newDeviation = OWI_ROM_SEARCH_FAILED;
            return SEARCH_ERROR;
        }
        else if (bitA ^ bitB)
        {
            // Bits A and B are different. All devices have the same bit here.
            // Set the bit in bitPattern to this value.
            if (bitA)
            {
                (*bitPattern) |= bitMask;
            }
            else
            {
                (*bitPattern) &= ~bitMask;
            }
        }
        else // Both bits 0
        {
            // If this is where a choice was made the last time,
            // a '1' bit is selected this time.
            if (currentBit == lastDeviation)
            {
                (*bitPattern) |= bitMask;
            }
            // For the rest of the id, '0' bits are selected when
            // discrepancies occur.
            else if (currentBit > lastDeviation)
            {
                (*bitPattern) &= ~bitMask;
                newDeviation = currentBit;
            }
            // If current bit in bit pattern = 0, then this is
            // out new deviation.
            else if ( !(*bitPattern & bitMask))
            {
                newDeviation = currentBit;
            }
            // IF the bit is already 1, do nothing.
            else
            {

            }
        }


        // Send the selected bit to the bus.
        if ((*bitPattern) & bitMask)
        {
            owi_write_bit(1, pGrpDev);
        }
        else
        {
            owi_write_bit(0, pGrpDev);
        }

        // Increment current bit.
        currentBit++;

        // Adjust bitMask and bitPattern pointer.
        bitMask <<= 1;
        if (!bitMask)
        {
            bitMask = 0x01;
            bitPattern++;
        }
    }
    return newDeviation;
}

/*! \brief  Perform a 1-Wire search
 *
 *  This function discover all slaves on the bus. It will also CRC check the 64 bit
 *  identifiers.
 *  \param  devices Pointer to an array of type OWI_device. The discovered
 *                  devices will be placed from the beginning of this array.
 *  \OWI_device*		- pointer to array of owi devices for recording
 *  \param  numDevices  - the number of the device array.
 *  \param  sGrpInfo	- device group
 *  \param	num			- number discovered devices
 *
 *  \retval SEARCH_SUCCESSFUL   Search completed successfully.
 *  \retval SEARCH_CRC_ERROR    A CRC error occured. Probably because of noise
 *                              during transmission.
 */
unsigned char owi_search_devices(OWI_device * devices, uint8_t numDevices, sGrpInfo* pGroup, uint8_t *num)
{
    unsigned char i, j;
    unsigned char * newID;
    unsigned char * currentID;
    unsigned char lastDeviation;
    unsigned char numFoundDevices;
    unsigned char flag = SEARCH_SUCCESSFUL;
    sGrpDev* pGrpDev = &pGroup->GrpDev;

    // reset 1Wire devices addresses
    for (i = 0; i < numDevices; i++)
    {
        for (j = 0; j < 8; j++)
        {
            devices[i].ucROM[j] = 0x00;
        }
    }

    numFoundDevices = 0;
    (*num) = 0;
    newID = devices[0].ucROM;
    lastDeviation = 0;
    currentID = newID;

    do
    {
      for (i = 0; i < 8; i++) {newID[i]=currentID[i]; };

      flag = owi_reset_pulse(pGrpDev);
      if(flag) return flag;

      lastDeviation = owi_search_rom(newID, lastDeviation, pGrpDev);
      currentID = newID;
      if (newID[0]!=0) {
    	  numFoundDevices++;
      }
      newID=devices[numFoundDevices].ucROM;
    } while(lastDeviation != OWI_ROM_SEARCH_FINISHED);


    // Go through all the devices and do CRC check.
    for (i = 0; i < numFoundDevices && i < numDevices; i++)
    {
        // If any id has a crc error, return error.
        if(owi_CheckRomCRC(devices[i].ucROM) != OWI_CRC_OK)
        {
            flag = SEARCH_CRC_ERROR;
        }
        else
        {
           (*num)++;
        }
    }
    // Else, return Successful.
    return flag;
}

void owi_device_put_rom(OWI_device* pDev, char * romValue)
{
	uint8_t i, higherNibble, lowerNibble;
// romValue is array of 16 chars
	for(i = 0; i<8; i++)
	 {
		higherNibble = charToInt(romValue[i*2]);
		lowerNibble = charToInt(romValue[i*2+1]);
		pDev->ucROM[i] = ((higherNibble & 0x0F) << 4) | (lowerNibble & 0x0F);
	 }

}

uint8_t charToInt(char letter)
{
	uint8_t myNumerical;
    // First we want to check if its 0-9, A-F, or a-f) --> See ASCII Table
    if(letter > 47 && letter < 58)
    {
        // 0-9
        myNumerical = letter-48;
        // The Letter "0" is in the ASCII table at position 48 -> meaning if we subtract 48 we get 0 and so on...
    }
    else if(letter > 64 && letter < 71)
    {
       // A-F
       myNumerical = letter-55;
       // The Letter "A" (dec 10) is at Pos 65 --> 65-55 = 10 and so on..
    }
    else if(letter > 96 && letter < 103)
    {
       // a-f
       myNumerical = letter-87;
       // The Letter "a" (dec 10) is at Pos 97--> 97-87 = 10 and so on...
    }
    else
    {
       // Not supported letter...
       myNumerical = -1;
    }
    return myNumerical;
}
