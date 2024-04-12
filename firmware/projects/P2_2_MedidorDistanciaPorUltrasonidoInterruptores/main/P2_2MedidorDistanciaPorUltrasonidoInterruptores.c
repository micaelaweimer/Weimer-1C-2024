/*! @mainpage Template
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
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
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
bool control = false;
bool pause = false;

TaskHandle_t leer_distancia_handle = NULL;
TaskHandle_t mostrar_distancia_handle = NULL;
/*==================[internal functions declaration]=========================*/
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

void Tecla1_OnOff(void)
{
	control = !control;
}

void Tecla2_Pause(void)
{
	pause = !pause;
}

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

void FuncTimerA_Distancia(void *param)
{
	vTaskNotifyGiveFromISR(leer_distancia_handle, pdFALSE);
	vTaskNotifyGiveFromISR(mostrar_distancia_handle, pdFALSE);
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	LedsInit();
	LcdItsE0803Init();
	SwitchesInit();
	HcSr04Init(GPIO_3, GPIO_2);

	timer_config_t timer_distancia = {
		.timer = TIMER_A,
		.period = CONFIG_PERIOD,
		.func_p = FuncTimerA_Distancia,
		.param_p = NULL};
	TimerInit(&timer_distancia);

	xTaskCreate(&LeerDistancia, "LeerDistancia", 2048, NULL, 5, &leer_distancia_handle);
	xTaskCreate(&MostrarDistancia, "MostrarDistancia", 2048, NULL, 5, &mostrar_distancia_handle);

	TimerStart(timer_distancia.timer);

	// Interrupciones
	SwitchActivInt(SWITCH_1, &Tecla1_OnOff, NULL);
	SwitchActivInt(SWITCH_2, &Tecla2_Pause, NULL);
}
/*==================[end of file]============================================*/