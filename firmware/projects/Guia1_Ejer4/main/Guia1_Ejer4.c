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
 * @author Weimer, Micaela
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "gpio_mcu.h"

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;


/*==================[internal functions declaration]=========================*/
void  convertToBcdArray (uint32_t data, uint8_t digits, uint8_t * bcd_number)
{
	for (int i=0; i<digits; i++){
		bcd_number[digits-1-i]=data%10;
		data/=10;
	}
}

void cambiarEstado (int bcd, gpioConf_t *gpio)
{
	for(int i=0; i<4; i++){
		if(bcd & (1<<i)){
			GPIOOn(gpio[i].pin);
			}
		else{
			GPIOOff(gpio[i].pin);
		}
	}
}
void  mostrarDisplay (uint32_t dato, uint8_t digitos, gpioConf_t *gpio_pos, gpioConf_t *gpio_bcd)
{
	uint8_t num_bcd[digitos];
	convertToBcdArray (dato, digitos, num_bcd);

	for(int i=0; i<3; i++){
		cambiarEstado(num_bcd[i], gpio_bcd);
		GPIOOn(gpio_pos[i].pin);
		GPIOOff(gpio_pos[i].pin);
	}
}

/*==================[external functions definition]==========================*/
void app_main(void){
	uint32_t bin=123;
	uint8_t cantDigitos=3;

	gpioConf_t gpioBCD [4];
	gpioBCD[0].pin=GPIO_20;
	gpioBCD[1].pin=GPIO_21;
	gpioBCD[2].pin=GPIO_22;
	gpioBCD[3].pin=GPIO_23;

	for(int i=0; i<4; i++){
		gpioBCD[i].dir=GPIO_OUTPUT;
	}
	
	for(int i=0; i<4; i++){
		GPIOInit(gpioBCD[i].pin, gpioBCD[i].dir);
	}

	gpioConf_t gpioPos [3];
	gpioPos[0].pin=GPIO_19;
	gpioPos[1].pin=GPIO_18;
	gpioPos[2].pin=GPIO_9;

	for(int i=0; i<3; i++){
		gpioPos[i].dir=GPIO_OUTPUT;
	}
	
	for(int i=0; i<3; i++){
		GPIOInit(gpioPos[i].pin, gpioPos[i].dir);
	}

	mostrarDisplay(bin, cantDigitos, gpioPos, gpioBCD);
		
}
/*==================[end of file]============================================*/