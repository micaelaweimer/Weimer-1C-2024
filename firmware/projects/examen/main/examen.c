/*! @mainpage Examen 11/06/2024
 *
 * @section genDesc General Description
 *
 * Aplicacion que se encarga del control de riego y pH de una plantera.
 * La misma enciende una bomba de agua cuando el sensor de riego detecta que la humedad es inferior
 * a la necesaria.
 * Ademas, cuando el sensor de pH detecta un pH mayor a 6.7 se enciende una bomba de pHA y cuando el pH es
 * inferior a 6 una bomba de pHB.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral   |      ESP32   	|
 * |:--------------: | :--------------: |
 * | sensor humedad	 |  	GPIO_16		|
 * |   sensor ph	 |  	GPIO_1		|
 * |  bomba agua	 |  	GPIO_17		|
 * |   bomba phA	 |  	GPIO_21		|
 * |   bomba phB	 |  	GPIO_22		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 11/06/2024 | Document creation		                         |
 *
 * @author Weimer, Micaela
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>

#include "gpio_mcu.h"
#include "analog_io_mcu.h"
#include "switch.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "switch.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/*==================[macros and definitions]=================================*/
#define PERIODO 3000	  // 3 seg
#define PERIODO_UART 5000 // 5 seg
#define PERIODO_TECLAS 100

/*==================[internal data definition]===============================*/
/*!
 * @brief Variable booleana que controla inicio y detencion de la medicion
 */
bool control = false;
/*!
 * @brief Variable booleana que controla deteccion de humedad
 */
bool deteccion = false;

/*!
 * @brief Variable entera de 16 bits para almacenar lectura del sensor de ph
 */
float lectura;

typedef struct
{
	gpio_t pin; /*!< GPIO pin number */
	io_t dir;	/*!< GPIO digection '0' IN;  '1' OUT*/
} gpioConf_t;

gpioConf_t controlHumedad, controlBombaAgua, bombaPhA, bombaPhB;

TaskHandle_t control_agua_handle = NULL;
TaskHandle_t control_ph_handle = NULL;
TaskHandle_t mensajes_handle = NULL;
TaskHandle_t leer_teclas_handle = NULL;
/*==================[internal functions declaration]=========================*/
/*!
 * @brief Funcion que lee las teclas presionadas y realiza la accion correspondiente
 */
void leerTeclas(void *pvParameter)
{
	uint8_t teclas;
	while (1)
	{
		teclas = SwitchesRead();
		switch (teclas)
		{
		case SWITCH_1:
			control = true;
			break;
		case SWITCH_2:
			control = false;
			break;
		}
		vTaskDelay(PERIODO_TECLAS / portTICK_PERIOD_MS);
	}
}

/*!
 * @brief Funcion lee el valor del sensor de humedad y
 *        controla la humedad de la plantera a traves del control de una bomba
 */
void controlAgua(void *pvParameter)
{
	while (1)
	{
		if (control)
		{
			deteccion = GPIORead(controlHumedad.pin);

			if (deteccion == true)
			{
				GPIOOn(controlBombaAgua.pin); // enciendo bomba
			}
			else
			{
				GPIOOff(controlBombaAgua.pin); // apago bomba
			}
		}
		vTaskDelay(PERIODO / portTICK_PERIOD_MS);
	}
}

/*!
 * @brief Funcion que lee el valor de pH y lo controla a traves del control de dos bombas
 */
void controlPh(void *pvParameter)
{
	while (1)
	{
		if (control)
		{
			float lectura_aux, voltaje;
			AnalogInputReadSingle(CH1, &lectura_aux); // leo el dato
			voltaje = lectura_aux * (3 / 1024);		  // ajusto segun voltaje maximo
			lectura = voltaje * (14 / 3);			  // encuentro el valor entre 0 y 14V

			if (lectura > 6.7)
			{
				GPIOOn(bombaPhA.pin); // enciendo bomba pHA
				GPIOOff(bombaPhB.pin);
			}
			else if (lectura < 6.7 | lectura > 6.0)
			{
				GPIOOff(bombaPhA.pin); // apago bombas
				GPIOOff(bombaPhB.pin);
			}
			else if (lectura < 6.0)
			{
				GPIOOn(bombaPhB.pin); // enciendo bomba pHB
				GPIOOff(bombaPhA.pin);
			}
		}
		vTaskDelay(PERIODO / portTICK_PERIOD_MS);
	}
}

/*!
 * @brief Funcion que envia mensajes de estado a traves de la UART
 */
void mensajes(void *pvParameter)
{
	while (1)
	{
		if (control)
		{
			if (deteccion == true)
			{
				UartSendString(UART_PC, "Bomba de agua encendida");
				UartSendString(UART_PC, "\r\n");
			}
			else if (lectura > 6.7)
			{
				UartSendString(UART_PC, "pH: ");
				UartSendString(UART_PC, (const char *)UartItoa(lectura, 10));
				UartSendString(UART_PC, ", humedad incorrecta");
				UartSendString(UART_PC, "\r\n");
				UartSendString(UART_PC, "Bombra de phA encendida");
				UartSendString(UART_PC, "\r\n");
			}
			else if (lectura < 6.7 | lectura > 6.0)
			{
				UartSendString(UART_PC, "pH: ");
				UartSendString(UART_PC, (const char *)UartItoa(lectura, 10));
				UartSendString(UART_PC, ", humedad correcta");
				UartSendString(UART_PC, "\r\n");
			}
			else if (lectura < 6.0)
			{
				UartSendString(UART_PC, "pH: ");
				UartSendString(UART_PC, (const char *)UartItoa(lectura, 10));
				UartSendString(UART_PC, ", humedad incorrecta");
				UartSendString(UART_PC, "\r\n");
				UartSendString(UART_PC, "Bombra de phB encendida");
				UartSendString(UART_PC, "\r\n");
			}
		}
		vTaskDelay(PERIODO_UART / portTICK_PERIOD_MS);
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	SwitchesInit();

	// GPIOs para control de humedad y bomba de agua
	controlHumedad.pin = GPIO_16;
	controlHumedad.dir = GPIO_INPUT;
	GPIOInit(controlHumedad.pin, controlHumedad.dir);

	controlBombaAgua.pin = GPIO_17;
	controlBombaAgua.dir = GPIO_OUTPUT;
	GPIOInit(controlBombaAgua.pin, controlBombaAgua.dir);

	// GPIOs para las bombas de pH
	bombaPhA.pin = GPIO_21;
	bombaPhA.dir = GPIO_OUTPUT;
	GPIOInit(bombaPhA.pin, bombaPhA.dir);

	bombaPhB.pin = GPIO_22;
	bombaPhB.dir = GPIO_OUTPUT;
	GPIOInit(bombaPhB.pin, bombaPhB.dir);

	/* Configuracion de la entrada analogica */
	analog_input_config_t senialPH = {
		.input = CH1,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL};
	AnalogInputInit(&senialPH);

	/* Configuracion del puerto serie */
	serial_config_t puertoPC = {
		.port = UART_PC,
		.baud_rate = 115200, // debo fijarme si el puerto serie esta en este mismo valor
		.func_p = NULL,
		.param_p = NULL};
	UartInit(&puertoPC);

	// Tareas
	xTaskCreate(&controlAgua, "ControlAgua", 2048, NULL, 5, &control_agua_handle);
	xTaskCreate(&controlPh, "ControlPh", 2048, NULL, 5, &control_ph_handle);
	xTaskCreate(&mensajes, "Mensajes", 2048, NULL, 5, &mensajes_handle);
	xTaskCreate(&leerTeclas, "LeerTecla", 2048, NULL, 5, &leer_teclas_handle);
}
/*==================[end of file]============================================*/