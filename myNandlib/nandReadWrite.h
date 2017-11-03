/*
 * nandReadWrite.h
 *
 *  Created on: 2017Äê11ÔÂ3ÈÕ
 *      Author: zhuji
 */

#ifndef NANDREADWRITE_H_
#define NANDREADWRITE_H_

#include "nandlib.h"
#include "nand_emifa2.h"

#define NAND_DEVICE_ID                          (0xCC)
#define NAND_PAGE_SIZE_IN_BYTES                 (NAND_PAGESIZE_2048BYTES)
#define NAND_BLOCK_SIZE_IN_BYTES                (NAND_BLOCKSIZE_128KB)
#define NAND_NUMOF_BLK                          (4096)
/******************************************************************************
*                                                                             *
* \brief  Function to initialize the device and controller info.              *
*                                                                             *
* \param  nandInfo      : Pointer to structure containing controller and      *
*                         device information.                                 *
*                                                                             *
* \param  csNum         : Chip select where device is interfaced.             *
*                                                                             *
* \return none.                                                               *
*                                                                             *
******************************************************************************/
extern void NANDInfoInit(NandInfo_t *nandInfo, unsigned int cs);


#endif /* NANDREADWRITE_H_ */
