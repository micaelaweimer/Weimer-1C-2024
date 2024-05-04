/*! @mainpage Medidor de distancia por ultrasonido
 *
 * @section Mide distancias mediante un ultrasonido, enciende distintos leds segun a que
 * 			distancia se encuentre el objeto y muestra los valores por display.
 *
 * This section describes how the program works.
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
 * | 19/04/2024 | Document creation		                         |
 *
 * @author Weimer, Micaela (micaela.weimer@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "hc_sr04.h"
#include "led.h"
#include "switch.h"
#include "lcditse0803.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
#include "uart_mcu.h"

/*==================[macros and definitions]=================================*/
#define PERIODO 1000
#define CONFIG_PERIOD 1000000
/*==================[internal data definition]===============================*/
uint16_t distancia_cm;
bool control = false;
bool pause = false;

TaskHandle_t leer_distancia_handle = NULL;
TaskHandle_t mostrar_distancia_handle = NULL;
/*==================[internal functions declaration]=========================*/
/*!
* @brief Funcion que enciende los leds segun la distancia que mide el sensor
* @param distancia distancia que mide el sensor
*/
void voometro(uint16_t distancia)
{
	if (distancia < 10)
	{
		LedOff(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
	}
	else if ((distancia > 10) && (distancia < 20))
	{
		LedOn(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
	}
	else if ((distancia > 20) && (distancia < 30))
	{
		LedOn(LED_1);
		LedOn(LED_2);
		LedOff(LED_3);
	}
	else if (distancia > 30)
	{
		LedOn(LED_1);
		LedOn(LED_2);
		LedOn(LED_3);
	}
}

/*!
* @brief Funcion que cambia el estado de la bandera control para activar o desactivar la medicion
*/
void Tecla1_OnOff(void)
{
	control = !control;
}

/*!
* @brief Funcion que cambia el estado de la bandera pause
*/
void Tecla2_Pause(void)
{
	pause = !pause;
}

/*!
* @brief Funcion que muestra la distancia medida por display y envia los datos
*        convertidos a ASCII mediante puerto serie a la PC
*/
void MostrarDistancia(void *pvParameter)
{
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (control)
		{
			if (!pause)
			{
				LcdItsE0803Write(distancia_cm);
				UartSendString(UART_PC, (const char *)UartItoa(distancia_cm, 10));
				UartSendString(UART_PC, " ");
				UartSendString(UART_PC, "cm");
				UartSendString(UART_PC, "\r\n");
			}
			voometro(distancia_cm);
		}
		else
		{
			LedsOffAll();
			LcdItsE0803Off();
		}
	}
}

/*!
* @brief Lee los valores de distancia obtenidos con el sensor
*/
void LeerDistancia(void *pvParameter)
{
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (control)
		{
			distancia_cm = HcSr04ReadDistanceInCentimeters();
		}
	}
}

/*!
* @brief Funcion que envia una notificacion para que inicien las funciones
*        leer y mostrar distancia
*/
void FuncTimerA_Distancia(void *param)
{
	vTaskNotifyGiveFromISR(leer_distancia_handle, pdFALSE);
	vTaskNotifyGiveFromISR(mostrar_distancia_handle, pdFALSE);
}

/*!
* @brief Funcion que controla el encendido y apagado de los leds y display, y pausa por teclado
*/
void controlTeclado(void *param)
{
	uint8_t tecla;
	UartReadByte(UART_PC, &tecla);
	
	switch (tecla)
	{
	case 'O':
		Tecla1_OnOff();
		
		break;
	case 'H':
		Tecla2_Pause();
		break;
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	/* Inicializaciones */
	LedsInit();
	LcdItsE0803Init();
	SwitchesInit();
	HcSr04Init(GPIO_3, GPIO_2);

	/* Configuracion del timer */
	timer_config_t timer_distancia = {
		.timer = TIMER_A,
		.period = CONFIG_PERIOD,
		.func_p = FuncTimerA_Distancia,
		.param_p = NULL};
	TimerInit(&timer_distancia);

	/* Configuracion del puerto serie */
	serial_config_t puertoSeriePC = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = controlTeclado,
		.param_p = NULL};
	UartInit(&puertoSeriePC);

	// Tareas
	xTaskCreate(&LeerDistancia, "LeerDistancia", 2048, NULL, 5, &leer_distancia_handle);
	xTaskCreate(&MostrarDistancia, "MostrarDistancia", 2048, NULL, 5, &mostrar_distancia_handle);

	// Interrupciones
	SwitchActivInt(SWITCH_1, &Tecla1_OnOff, NULL);
	SwitchActivInt(SWITCH_2, &Tecla2_Pause, NULL);

	TimerStart(timer_distancia.timer);
}
/*==================[end of file]============================================*/