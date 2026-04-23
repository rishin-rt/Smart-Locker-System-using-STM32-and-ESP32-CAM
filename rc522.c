/* ============================================================
 * RC522 RFID Driver — STM32F103C8T6
 * Smart Locker System — KIIT University ED Lab (March 2026)
 *
 * Interface : SPI1
 * CS Pin    : PA4
 * RST Pin   : PA2
 * ============================================================ */

#include "rc522.h"
#include <string.h>

extern SPI_HandleTypeDef hspi1;

/* ============================================================
 * Low Level SPI Functions
 * ============================================================ */

/**
 * @brief Pull CS pin LOW to select RC522
 */
void MFRC522_Select(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}

/**
 * @brief Pull CS pin HIGH to deselect RC522
 */
void MFRC522_Unselect(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

/**
 * @brief Write a value to an RC522 register
 * @param addr : Register address
 * @param val  : Value to write
 */
void MFRC522_WriteReg(uint8_t addr, uint8_t val)
{
    uint8_t data[2];
    data[0] = (addr << 1) & 0x7E;   // Address byte: bit7=0 (write), bit0=0
    data[1] = val;

    MFRC522_Select();
    HAL_SPI_Transmit(&hspi1, data, 2, 100);
    MFRC522_Unselect();
}

/**
 * @brief Read a value from an RC522 register
 * @param addr : Register address
 * @return     : Register value
 */
uint8_t MFRC522_ReadReg(uint8_t addr)
{
    uint8_t tx = ((addr << 1) & 0x7E) | 0x80;   // bit7=1 (read)
    uint8_t rx = 0;

    MFRC522_Select();
    HAL_SPI_Transmit(&hspi1, &tx, 1, 100);
    HAL_SPI_Receive(&hspi1, &rx, 1, 100);
    MFRC522_Unselect();

    return rx;
}

/* ============================================================
 * Core Communication
 * ============================================================ */

/**
 * @brief Send command and data to RC522, receive response
 * @param command  : RC522 command (PCD_TRANSCEIVE etc.)
 * @param sendData : Data to send
 * @param sendLen  : Length of data to send
 * @param backData : Buffer for received data
 * @param backLen  : Length of received data (bits)
 * @return MI_OK / MI_ERR
 */
uint8_t MFRC522_ToCard(uint8_t command,
                       uint8_t *sendData,
                       uint8_t sendLen,
                       uint8_t *backData,
                       uint32_t *backLen)
{
    uint8_t status  = MI_ERR;
    uint8_t irqEn   = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t lastBits;
    uint8_t n;

    if (command == PCD_TRANSCEIVE)
    {
        irqEn   = 0x77;
        waitIRq = 0x30;
    }

    /* Setup interrupts and FIFO */
    MFRC522_WriteReg(ComIEnReg,   irqEn | 0x80);
    MFRC522_WriteReg(ComIrqReg,   0x7F);          // Clear all IRQ flags
    MFRC522_WriteReg(FIFOLevelReg, 0x80);         // Flush FIFO
    MFRC522_WriteReg(CommandReg,  PCD_IDLE);

    /* Write data to FIFO */
    for (uint8_t i = 0; i < sendLen; i++)
        MFRC522_WriteReg(FIFODataReg, sendData[i]);

    /* Execute command */
    MFRC522_WriteReg(CommandReg, command);

    if (command == PCD_TRANSCEIVE)
        MFRC522_WriteReg(BitFramingReg,
                         MFRC522_ReadReg(BitFramingReg) | 0x80);  // StartSend=1

    /* Wait for completion with 50ms timeout */
    uint32_t start = HAL_GetTick();
    do
    {
        n = MFRC522_ReadReg(ComIrqReg);
    }
    while (!(n & 0x01) &&
           !(n & waitIRq) &&
           (HAL_GetTick() - start < 50));

    /* Stop transmission */
    MFRC522_WriteReg(BitFramingReg,
                     MFRC522_ReadReg(BitFramingReg) & 0x7F);

    /* Timeout check */
    if (HAL_GetTick() - start >= 50)
        return MI_ERR;

    /* Error check */
    if (!(MFRC522_ReadReg(ErrorReg) & 0x1B))
    {
        status = MI_OK;

        n        = MFRC522_ReadReg(FIFOLevelReg);
        lastBits = MFRC522_ReadReg(ControlReg) & 0x07;

        if (lastBits)
            *backLen = (n - 1) * 8 + lastBits;
        else
            *backLen = n * 8;

        if (n == 0) n = 1;

        /* Read received data from FIFO */
        for (uint8_t i = 0; i < n; i++)
            backData[i] = MFRC522_ReadReg(FIFODataReg);
    }

    return status;
}

/* ============================================================
 * PICC Commands
 * ============================================================ */

/**
 * @brief Send REQA/WUPA command to detect card
 * @param reqMode : PICC_REQIDL or PICC_REQALL
 * @param TagType : Buffer for card type response
 * @return MI_OK if card detected
 */
uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *TagType)
{
    uint32_t backLen;

    MFRC522_WriteReg(BitFramingReg, 0x07);   // 7 bits for short frame

    TagType[0] = reqMode;

    uint8_t status = MFRC522_ToCard(PCD_TRANSCEIVE,
                                    TagType,
                                    1,
                                    TagType,
                                    &backLen);

    if ((status != MI_OK) || (backLen != 0x10))
        status = MI_ERR;

    return status;
}

/**
 * @brief Anti-collision — read card UID
 * @param serNum : Buffer to store 5-byte UID (4 bytes + checksum)
 * @return MI_OK if successful
 */
uint8_t MFRC522_Anticoll(uint8_t *serNum)
{
    uint32_t backLen;
    uint8_t  check = 0;

    MFRC522_WriteReg(BitFramingReg, 0x00);

    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;

    uint8_t status = MFRC522_ToCard(PCD_TRANSCEIVE,
                                    serNum,
                                    2,
                                    serNum,
                                    &backLen);

    if (status == MI_OK)
    {
        /* Verify checksum byte */
        for (uint8_t i = 0; i < 4; i++)
            check ^= serNum[i];

        if (check != serNum[4])
            status = MI_ERR;
    }

    return status;
}

/* ============================================================
 * Initialization
 * ============================================================ */

/**
 * @brief Initialize RC522 module
 *        - Hardware reset via PA2
 *        - Configure timer, RF, and antenna
 */
void MFRC522_Init(void)
{
    /* Hardware reset */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
    HAL_Delay(50);

    MFRC522_WriteReg(CommandReg, PCD_RESETPHASE);
    HAL_Delay(50);

    /* Timer configuration */
    MFRC522_WriteReg(TModeReg,     0x8D);
    MFRC522_WriteReg(TPrescalerReg, 0x3E);
    MFRC522_WriteReg(TReloadRegL,  30);
    MFRC522_WriteReg(TReloadRegH,  0);

    /* RF configuration */
    MFRC522_WriteReg(TxASKReg,  0x40);
    MFRC522_WriteReg(ModeReg,   0x3D);
    MFRC522_WriteReg(RFCfgReg,  0x40);   // Max gain

    /* Turn antenna ON */
    MFRC522_WriteReg(TxControlReg,
                     MFRC522_ReadReg(TxControlReg) | 0x03);
}

/* ============================================================
 * Public API
 * ============================================================ */

/**
 * @brief Check if a valid RFID card is present and read its UID
 * @param id : Buffer to store card UID (MAX_LEN bytes)
 * @return HAL_OK if card detected, HAL_ERROR otherwise
 */
HAL_StatusTypeDef MFRC522_Check(uint8_t *id)
{
    if (MFRC522_Request(PICC_REQIDL, id) == MI_OK)
    {
        if (MFRC522_Anticoll(id) == MI_OK)
            return HAL_OK;
    }

    memset(id, 0, MAX_LEN);
    return HAL_ERROR;
}
