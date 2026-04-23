#ifndef RC522_H
#define RC522_H

#include "stm32f1xx_hal.h"

/* -------------------- Constants -------------------- */

#define MAX_LEN 16

#define MI_OK    0
#define MI_NOTAGERR 1
#define MI_ERR   2

/* -------------------- MFRC522 Registers -------------------- */

#define CommandReg      0x01
#define ComIEnReg       0x02
#define ComIrqReg       0x04
#define ErrorReg        0x06
#define FIFODataReg     0x09
#define FIFOLevelReg    0x0A
#define ControlReg      0x0C
#define BitFramingReg   0x0D
#define ModeReg         0x11
#define TxControlReg    0x14
#define TxASKReg        0x15
#define TModeReg        0x2A
#define TPrescalerReg   0x2B
#define TReloadRegH     0x2C
#define TReloadRegL     0x2D
#define RFCfgReg        0x26

/* -------------------- MFRC522 Commands -------------------- */

#define PCD_IDLE        0x00
#define PCD_TRANSCEIVE  0x0C
#define PCD_RESETPHASE  0x0F

/* -------------------- PICC Commands -------------------- */

#define PICC_REQIDL     0x26
#define PICC_ANTICOLL   0x93

/* -------------------- Function Prototypes -------------------- */

void MFRC522_Init(void);
void MFRC522_WriteReg(uint8_t addr, uint8_t val);
uint8_t MFRC522_ReadReg(uint8_t addr);
uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *TagType);
uint8_t MFRC522_Anticoll(uint8_t *serNum);
uint8_t MFRC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen,
                       uint8_t *backData, uint32_t *backLen);
HAL_StatusTypeDef MFRC522_Check(uint8_t *id);

#endif /* RC522_H */
