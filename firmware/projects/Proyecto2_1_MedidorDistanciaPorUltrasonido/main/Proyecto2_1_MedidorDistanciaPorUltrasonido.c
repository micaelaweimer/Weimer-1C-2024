/*! @mainpage Medidor de distancia por ultrasonido
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
 * | 05/04/2024 | Document creation		                         |
 *
 * @author Weimer, Micaela (amicaela.weimer@ingenieria.uner.edu.ar)
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
/*!
* @brief Variable booleana que controla inicio y detencion de la medicion
*/
bool control = true;
/*!
* @brief Variable booleana que controla lo que se muestra por display
*/
bool pause = false;
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

/*!
* @brief Lee la distancia en cm y la almacena en una variable global
*/ 
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

/*!
* @brief Lee las teclas presionadas y realiza la accion correspondiente
*/
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

/*!
* @brief Muestra la distancia en cm por display
*/
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