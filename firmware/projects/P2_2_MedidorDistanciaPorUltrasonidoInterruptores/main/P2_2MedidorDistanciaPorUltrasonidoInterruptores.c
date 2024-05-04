/*! @mainpage Medidor de distancia por ultrasonido empleando interrupciones
 *
 * @section genDesc General Description
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
 * | 12/04/2024 | Document creation		                         |
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

/*==================[macros and definitions]=================================*/
#define PERIODO 1000
#define CONFIG_PERIOD 1000000
/*==================[internal data definition]===============================*/
uint16_t distancia_cm;
/*!
* @brief Variable booleana que controla inicio y detencion de la medicion
*/
bool control = false;
/*!
* @brief Variable booleana que controla lo que se muestra por display
*/
bool pause = false;

TaskHandle_t leer_distancia_handle = NULL;
TaskHandle_t mostrar_distancia_handle = NULL;
/*==================[internal functions declaration]=========================*/
/*!
* @brief Enciende y apaga los leds segun la distancia que se lee con el sensor
* @param distancia Distancia leida por el sensor
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
* @brief Controla una variable booleana para encernder y apagar los leds y display
*/
void Tecla1_OnOff(void)
{
	control = !control;
}

/*!
* @brief Controla una variable booleana para fijar una valor de distancia en el display
*/
void Tecla2_Pause(void)
{
	pause = !pause;
}

/*!
* @brief Muestra la distancia en cm por display o apaga los leds y display segun es estado de
*        una variable booleana
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
* @brief Lee la distancia en cm y la almacena en una variable global
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
* @brief Envia una notificacion a las funciones LeerDistancia y MostrarDistancia para que comiencen
*/ 
void FuncTimerA_Distancia(void *param)
{
	vTaskNotifyGiveFromISR(leer_distancia_handle, pdFALSE);
	vTaskNotifyGiveFromISR(mostrar_distancia_handle, pdFALSE);
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

	// Tareas
	xTaskCreate(&LeerDistancia, "LeerDistancia", 2048, NULL, 5, &leer_distancia_handle);
	xTaskCreate(&MostrarDistancia, "MostrarDistancia", 2048, NULL, 5, &mostrar_distancia_handle);

	TimerStart(timer_distancia.timer);

	// Interrupciones
	SwitchActivInt(SWITCH_1, &Tecla1_OnOff, NULL);
	SwitchActivInt(SWITCH_2, &Tecla2_Pause, NULL);
}
/*==================[end of file]============================================*/