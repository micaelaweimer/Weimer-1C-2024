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
 * @author Weimer, Micalea
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "led.h"
#include "switch.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/*==================[macros and definitions]=================================*/
#define ON 00000001
#define OFF 00000000
#define TOGGLE 00000010

/*==================[internal data definition]===============================*/
struct leds
{
	uint8_t n_led;      //indica el número de led a controlar
	uint8_t n_ciclos;   //indica la cantidad de ciclos de encendido/apagado
	uint16_t periodo;   //indica el tiempo de cada ciclo
	uint8_t mode;       //ON, OFF, TOGGLE
} my_leds; 

/*==================[internal functions declaration]=========================*/
void control_led (struct leds *auxled)
{
	switch (auxled->mode)
	{
	case ON:
		LedOn(auxled->n_led);
		break;
	case OFF:
		LedOff(auxled->n_led);
		break;
	case TOGGLE:
		int j=0;
		while (j<auxled->n_ciclos)
		{
			LedToggle(auxled->n_led);
			for(int i=0; i<auxled->periodo/100; i++){
			vTaskDelay(100 / portTICK_PERIOD_MS);
			}
			j++;
		}
		break;
	default:
		break;
	}
}

/*==================[external functions definition]==========================*/
void app_main(void){
	uint8_t teclas;
	LedsInit();
	SwitchesInit();

	my_leds.mode=TOGGLE;
	my_leds.n_ciclos=100;
	my_leds.periodo=500;
	my_leds.n_led=LED_1;
	
	control_led(&my_leds);

}
/*==================[end of file]============================================*/