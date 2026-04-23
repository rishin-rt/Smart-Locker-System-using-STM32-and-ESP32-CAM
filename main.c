/* ============================================================
 * Smart Locker System — STM32F103C8T6 Main Firmware
 * KIIT University — ED Lab Project (March 2026)
 *
 * Authentication Methods:
 *   1. RFID Card (RC522 via SPI)
 *   2. PIN Keypad (4x4 matrix)
 *   3. Face Recognition (ESP32-CAM via UART)
 *
 * Hardware:
 *   - STM32F103C8T6 (Blue Pill)
 *   - RC522 RFID Reader
 *   - 4x4 Matrix Keypad
 *   - SSD1306 OLED Display (I2C)
 *   - Servo Motor (PWM)
 *   - Buzzer
 *   - ESP32-CAM (UART)
 * ============================================================ */

#include "main.h"
#include "ssd1306.h"
#include "ssd1306_conf.h"
#include "rc522.h"
#include <stdio.h>
#include <string.h>

/* -------------------- Peripheral Handles -------------------- */
I2C_HandleTypeDef  hi2c1;
SPI_HandleTypeDef  hspi1;
TIM_HandleTypeDef  htim2;
UART_HandleTypeDef huart1;

/* -------------------- OLED Fonts -------------------- */
extern SSD1306_Font_t Font_7x10;
extern SSD1306_Font_t Font_11x18;

/* -------------------- UART Receive Variables -------------------- */
uint8_t uart_rx;
uint8_t rx_buffer[20];
uint8_t rx_index    = 0;
uint8_t unlock_flag = 0;
uint8_t system_busy = 0;

/* -------------------- PIN Variables -------------------- */
char password[5]   = "1234";   // Default PIN — change here
char input_code[5] = "";
int  input_index   = 0;

/* ============================================================
 * Function Prototypes
 * ============================================================ */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);

char Keypad_Scan(void);
void Set_Servo_Angle(int angle);
void Buzzer_Beep(int duration);
void Update_OLED(char* line1, char* line2);

/* ============================================================
 * Main Function
 * ============================================================ */
int main(void)
{
    uint8_t str[MAX_LEN];

    /* MCU Init */
    HAL_Init();
    SystemClock_Config();

    /* Peripheral Init */
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_I2C1_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();

    /* Start UART interrupt receive */
    HAL_UART_Receive_IT(&huart1, &uart_rx, 1);

    /* Start PWM for Servo */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

    /* Init OLED */
    ssd1306_Init();

    /* Init RFID */
    MFRC522_Init();
    HAL_Delay(100);

    /* Clear RFID buffer */
    memset(str, 0, MAX_LEN);

    /* Welcome Screen */
    Update_OLED("SMART LOCKER", "READY...");
    HAL_Delay(1000);

    /* ============================================================
     * Main Loop
     * ============================================================ */
    while (1)
    {
        /* ================= KEYPAD SECTION ================= */
        char key = Keypad_Scan();

        if (key != 0)
        {
            Buzzer_Beep(50);

            if (key == 'A')   // Submit PIN
            {
                if (strcmp(input_code, password) == 0)
                {
                    Update_OLED("SUCCESS", "OPENING");
                    Buzzer_Beep(200);
                    Set_Servo_Angle(90);
                    HAL_Delay(3000);
                    Set_Servo_Angle(0);
                }
                else
                {
                    Update_OLED("WRONG PIN", "DENIED");
                    Buzzer_Beep(1000);
                    HAL_Delay(1000);
                }

                memset(input_code, 0, sizeof(input_code));
                input_index = 0;
                Update_OLED("LOCKED", "Enter PIN:");
            }
            else if (key == '#')   // Clear input
            {
                memset(input_code, 0, sizeof(input_code));
                input_index = 0;
                Update_OLED("CLEARED", "");
            }
            else if (input_index < 4)   // Add digit
            {
                input_code[input_index++] = key;
                char mask[5] = "****";
                mask[input_index] = '\0';
                Update_OLED("INPUT:", mask);
            }
        }

        /* ================= RFID SECTION ================= */
        if (MFRC522_Check(str) == HAL_OK)
        {
            if (str[0] != 0 && str[0] != 0xFF)
            {
                Update_OLED("CARD DETECTED", "WELCOME");
                Buzzer_Beep(200);
                Set_Servo_Angle(90);
                HAL_Delay(3000);
                Set_Servo_Angle(0);
                Update_OLED("LOCKED", "Enter PIN:");

                /* Stop RFID */
                MFRC522_WriteReg(CommandReg, PCD_IDLE);
                MFRC522_WriteReg(FIFOLevelReg, 0x80);
                HAL_Delay(500);
            }
        }
        memset(str, 0, MAX_LEN);

        /* ================= FACE RECOGNITION (UART from ESP32) ================= */
        if (unlock_flag && system_busy == 0)
        {
            system_busy = 1;
            unlock_flag = 0;

            Update_OLED("FACE OK", "OPENING");
            Buzzer_Beep(200);
            Set_Servo_Angle(90);
            HAL_Delay(3000);
            Set_Servo_Angle(0);

            Update_OLED("LOCKED", "READY");
            HAL_Delay(500);

            system_busy = 0;
        }

        /* Heartbeat LED */
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(50);
    }
}

/* ============================================================
 * UART Interrupt Callback — Receives face recognition result
 * from ESP32-CAM via UART1
 * ============================================================ */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (uart_rx == '\n')
        {
            rx_buffer[rx_index] = '\0';

            if (strncmp((char*)rx_buffer, "ID:", 3) == 0)
            {
                int received_id = atoi((char*)&rx_buffer[3]);
                if (received_id >= 0 && system_busy == 0)
                    unlock_flag = 1;
            }

            rx_index = 0;
        }
        else
        {
            if (rx_index < sizeof(rx_buffer) - 1)
                rx_buffer[rx_index++] = uart_rx;
        }

        HAL_UART_Receive_IT(&huart1, &uart_rx, 1);
    }
}

/* ============================================================
 * Helper Functions
 * ============================================================ */

char Keypad_Scan(void)
{
    GPIO_TypeDef* row_ports[] = {GPIOA, GPIOB, GPIOB, GPIOB};
    uint16_t row_pins[]       = {GPIO_PIN_8, GPIO_PIN_15, GPIO_PIN_14, GPIO_PIN_13};
    uint16_t col_pins[]       = {GPIO_PIN_12, GPIO_PIN_11, GPIO_PIN_10, GPIO_PIN_1};
    char keys[4][4]           = {{'1','2','3','A'},
                                  {'4','5','6','B'},
                                  {'7','8','9','C'},
                                  {'*','0','#','D'}};

    for (int i = 0; i < 4; i++)
    {
        HAL_GPIO_WritePin(row_ports[i], row_pins[i], GPIO_PIN_SET);
        for (int j = 0; j < 4; j++)
        {
            if (HAL_GPIO_ReadPin(GPIOB, col_pins[j]) == GPIO_PIN_SET)
            {
                HAL_Delay(20);  // Debounce
                while (HAL_GPIO_ReadPin(GPIOB, col_pins[j]) == GPIO_PIN_SET);
                HAL_GPIO_WritePin(row_ports[i], row_pins[i], GPIO_PIN_RESET);
                return keys[i][j];
            }
        }
        HAL_GPIO_WritePin(row_ports[i], row_pins[i], GPIO_PIN_RESET);
    }
    return 0;
}

void Set_Servo_Angle(int angle)
{
    uint32_t pulse = 600 + ((angle * 1800) / 180);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pulse);
}

void Buzzer_Beep(int duration)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_Delay(duration);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
}

void Update_OLED(char* line1, char* line2)
{
    ssd1306_Fill(Black);
    ssd1306_SetCursor(2, 0);
    ssd1306_WriteString(line1, Font_7x10, White);
    ssd1306_SetCursor(2, 25);
    ssd1306_WriteString(line2, Font_11x18, White);
    ssd1306_UpdateScreen();
}

/* ============================================================
 * Peripheral Initialization Functions (STM32CubeMX Generated)
 * ============================================================ */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

static void MX_I2C1_Init(void)
{
    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = 100000;
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void MX_SPI1_Init(void)
{
    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial     = 10;
    HAL_SPI_Init(&hspi1);
}

static void MX_TIM2_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 7;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 9999;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim2);

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2);
}

static void MX_USART1_UART_Init(void)
{
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);

    /* PC13 - Heartbeat LED */
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* PA2 (RC522 RST), PA4 (RC522 CS), PA8 (Keypad Row1) */
    GPIO_InitStruct.Pin   = GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_8;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PB0 (Buzzer), PB13/14/15 (Keypad Rows 2/3/4) */
    GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PB1/10/11/12 - Keypad Columns (Input) */
    GPIO_InitStruct.Pin  = GPIO_PIN_1 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}
