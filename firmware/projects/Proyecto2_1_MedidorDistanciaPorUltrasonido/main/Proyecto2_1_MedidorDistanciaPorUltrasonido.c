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
 * @author Weimer Micaela (amicaela.weimer@ingenieria.uner.edu.ar)
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

/*==================[macros and definitions]=================================*/
#define PERIODO 1000
#define PERIODO_TECLAS 100
/*==================[internal data definition]===============================*/
uint16_t distancia_cm;
bool control = true;
bool pause = false;
/*==================[internal functions declaration]=========================*/
void voometro(uint16_t distancia)
{
	if (distancia < 10)
	{
		LedOff(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
	}
	else if (distancia > 10 && distancia < 20)
	{
		LedOn(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
	}
	else if (distancia > 20 && distancia < 30)
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
void LeerDistancia(void *pvParameter)
{
	while (1)
	{
		if (control)
		{
			distancia_cm = HcSr04ReadDistanceInCentimeters();
		}
		vTaskDelay(PERIODO / portTICK_PERIOD_MS);
	}
}
void LeerTecla(void *pvParameter)
{
	uint8_t teclas;
	while (1)
	{
		teclas = SwitchesRead();
		switch (teclas)
		{
		case SWITCH_1:
			control = !control;
			break;
		case SWITCH_2:
			pause = !pause;
			break;
		}
		vTaskDelay(PERIODO_TECLAS / portTICK_PERIOD_MS);
	}
}
void MostrarDistancia(void *pvParameter)
{
	while (1)
	{
		if (control)
		{
			if (!pause)
			{
				LcdItsE0803Write(distancia_cm);
			}
			voometro(distancia_cm);
		}
		vTaskDelay(PERIODO / portTICK_PERIOD_MS);
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	LedsInit();
	LcdItsE0803Init();
	SwitchesInit();
	HcSr04Init(GPIO_3, GPIO_2);

	xTaskCreate(&LeerDistancia, "LeerDistancia", 2048, NULL, 5, NULL);
	xTaskCreate(&LeerTecla, "LeerTecla", 512, NULL, 5, NULL);
	xTaskCreate(&MostrarDistancia, "MostrarDistancia", 2048, NULL, 5, NULL);
}
/*==================[end of file]============================================*/