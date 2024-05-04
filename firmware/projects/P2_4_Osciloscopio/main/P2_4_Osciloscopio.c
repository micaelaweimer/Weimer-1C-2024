/*! @mainpage Osciloscopio
 *
 * @section genDesc General Description
 *
 * 
 * 
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 26/04/2024 | Document creation		                         |
 *
 * @author Weimer Micaela (micaela.weimer@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_PERIOD_AD 2000
#define CONFIG_PERIOD_DA 4000

/*!
* @brief Tamaño del vector de la señal de ECG
*/
#define BUFFER_SIZE 231
/*==================[internal data definition]===============================*/
TaskHandle_t conversion_AD_handle = NULL;
TaskHandle_t  conversion_DA_handle = NULL;

/*!
* @brief Indice del vector de ECG
*/
uint8_t indice=0;

TaskHandle_t main_task_handle = NULL;
/*!
* @brief Señal digital de ECG
*/
const char ecg[BUFFER_SIZE] = {
    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76,
};

/*==================[internal functions declaration]=========================*/
/*!
* @brief Conversor analogico-digital
*/
void conversionAD(void *pvParameter)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint16_t lecturaECG;
        AnalogInputReadSingle(CH1, &lecturaECG);
        UartSendString(UART_PC, (const char *)UartItoa(lecturaECG, 10)); // envio el dato convertido a ASCII
        UartSendString(UART_PC, "\r\n");
    }
}
/*!
* @brief Envia una notificacion a la funcion conversionAD
*/
void FuncTimerA(void *param)
{
    vTaskNotifyGiveFromISR(conversion_AD_handle, pdFALSE);
}
/*!
* @brief Envia una notificacion a la funcion conversionDA
*/
void FuncTimerB(void *param)
{
    vTaskNotifyGiveFromISR(conversion_DA_handle, pdFALSE);
}
/*!
* @brief Conversor digital-analogico
*/
void conversionDA(void *pvParameter)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (indice < BUFFER_SIZE)
        {
            AnalogOutputWrite(ecg[i]);
            indice++;
        }
        else if (indice=BUFFER_SIZE)
        {
            indice = 0;
        }
    }
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
    /* Configuracion de la entrada analogica */
    analog_input_config_t senial_analogica = {
        .input = CH1,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL};
    AnalogInputInit(&senial_analogica);
    AnalogOutputInit();

    /* Configuracion del timer analogico-digital */
    timer_config_t temp_AD = {
        .timer = TIMER_A,
        .period = CONFIG_PERIOD_AD,
        .func_p = FuncTimerA,
        .param_p = NULL};
    TimerInit(&temp_AD);

    /* Configuracion del timer digital-analogico */
    timer_config_t temp_DA = {
        .timer = TIMER_B,
        .period = CONFIG_PERIOD_DA,
        .func_p = FuncTimerB,
        .param_p = NULL};
    TimerInit(&temp_DA);

    /* Configuracion del puerto serie */
    serial_config_t puertoSeriePC = {
        .port = UART_PC,
        .baud_rate = 115200,
        .func_p = NULL,
        .param_p = NULL};
    UartInit(&puertoSeriePC);

    // Tareas
    xTaskCreate(&conversionAD, "ConversionAD", 2048, NULL, 5, &conversion_AD_handle);
    xTaskCreate(&conversionDA, "ConversionDA", 2048, NULL, 5, &conversion_DA_handle);

    // SIEMPRE AL FINAL
    TimerStart(temp_AD.timer);
    TimerStart(temp_DA.timer);
}
/*==================[end of file]============================================*/