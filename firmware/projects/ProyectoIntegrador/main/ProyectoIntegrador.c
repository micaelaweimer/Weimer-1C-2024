/*! @mainpage Robot esquiva obstaculos
 *
 * @section genDesc General Description
 *
 * Robot capaz de avanzar en línea recta mientras no se presente ningún obstáculo.
 * En caso de detectar un obstáculo, el robot lo esquiva. Para esto apago los motores
 * y evalua cuál es la mejor dirección para avanzar, teniendo en cuenta que no haya
 * otro obstáculo cercano.
 * El robot puede ser controlado desde una aplicación en el celular que permite encender
 * y apagar el movimiento del mismo. Además en la misma aplicación se puede observar en
 * tiempo real el estado del robot. Ademas el robot esta equipado con una luz (led) que
 * enciende en caso de detectar oscuridad en su recorrido.
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
 * | 24/05/2024 | Document creation		                         |
 *
 * @author Weimer Micaela (micaela.weimer@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gpio_mcu.h"
#include "pwm_mcu.h"
#include "servo_sg90.h"
#include "hc_sr04.h"
#include "timer_mcu.h"
#include "led.h"

/*==================[macros and definitions]=================================*/
// motor izquierdo B
#define MIA GPIO_16 // amarillo
#define MIB GPIO_22 // blanco

// motor derecho A
#define MDA GPIO_17 // violeta
#define MDB GPIO_23 // gris

#define PERIODO 1000000
#define giroDer -90
#define giroIzq 90
#define posCentral 0

/*==================[internal data definition]===============================*/

typedef struct
{
    gpio_t pin;
    io_t dir;
} gpioConf_t;

/*!
 * @brief Variable booleana para control de encendido y apagado del programa
 */
bool control = true;

pwm_out_t pwm_motores[2];

/*!
 * @brief Distancia en cm leida por el sensor de ultrasonido
 */
uint16_t distancia;

/*!
 * @brief Almacena el estado del robot
 */
char estado[500];

TaskHandle_t control_distancia_handle = NULL;
/*==================[internal functions declaration]=========================*/
/*!
 * @brief Envia una notificacion a la funcion controlDistancia
 */
void FuncTimerA_Distancia(void *param)
{
    vTaskNotifyGiveFromISR(control_distancia_handle, pdFALSE);
}

// void controlRuedas(char dir)
// {
//     switch (dir)
//     {
//     case 'A': // avanzar
//         GPIOOn(MIB);
//         GPIOOn(MDB);

//         PWMOn(pwm_motores[0]);
//         PWMOn(pwm_motores[1]);

//         // Velocidad
//         PWMSetDutyCycle(pwm_motores[0], 70);
//         PWMSetDutyCycle(pwm_motores[1], 70);
//         break;

//     case 'F': // frenar
//         GPIOOn(MIB);
//         GPIOOn(MDB);

//         PWMSetDutyCycle(pwm_motores[0], 100);
//         PWMSetDutyCycle(pwm_motores[1], 100);
//         break;

//     case 'D': // giro derecha
//         GPIOOn(MIB);
//         GPIOOn(MDB);

//         PWMOn(pwm_motores[0]);
//         PWMOn(pwm_motores[1]);

//         // velocidad
//         PWMSetDutyCycle(pwm_motores[0], 45); // rueda izq
//         PWMSetDutyCycle(pwm_motores[1], 70); // rueda der
//         break;

//     case 'I': // giro izquierda
//         GPIOOn(MIB);
//         GPIOOn(MDB);

//         PWMOn(pwm_motores[0]);
//         PWMOn(pwm_motores[1]);

//         // velocidad
//         PWMSetDutyCycle(pwm_motores[0], 70); // rueda izq
//         PWMSetDutyCycle(pwm_motores[1], 45); // rueda der
//         break;

//     default:
//         break;
//     }
// }

/*!
 * @brief Lee y controla la distancia medida por el sensor
 */
void controlDistancia(void *pvParameter)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (control)
        {
            distancia = HcSr04ReadDistanceInCentimeters();

            uint16_t distDer, distIzq;

            if (distancia > 15)
            {
                sprintf(estado, "*EAvanzando\n*");
                // controlRuedas('A');
            }
            else if (distancia <= 15)
            {
                // apago motores
                sprintf(estado, "*EFreno\n*");
                // controlRuedas('F');

                ServoMove(SERVO_0, giroDer); // se queda en esta posicion
                distDer = HcSr04ReadDistanceInCentimeters();
                vTaskDelay(850 / portTICK_PERIOD_MS);
                ServoMove(SERVO_0, posCentral);
                vTaskDelay(600 / portTICK_PERIOD_MS);
                ServoMove(SERVO_0, giroIzq);
                distIzq = HcSr04ReadDistanceInCentimeters();
                vTaskDelay(850 / portTICK_PERIOD_MS);
                ServoMove(SERVO_0, posCentral);
                vTaskDelay(600 / portTICK_PERIOD_MS);

                if (distDer > distIzq)
                {
                    // grio a la derecha
                    sprintf(estado, "*EGiro a la derecha\n*");
                    // controlRuedas('D');
                    // vTaskDelay(250 / portTICK_PERIOD_MS);
                    // controlRuedas('A');
                }
                else
                {
                    // giro a la izquierda
                    sprintf(estado, "*EGiro a la izquierda\n*");
                    // controlRuedas('I');
                    // vTaskDelay(250 / portTICK_PERIOD_MS);
                    // controlRuedas('A');
                }
            }
        }
    }
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    // inicializo hc sr04 y servo
    HcSr04Init(GPIO_3, GPIO_2);
    ServoInit(SERVO_0, GPIO_21);
    LedsInit();

    /* Configuracion del timer */
    timer_config_t timer_distancia = {
        .timer = TIMER_A,
        .period = PERIODO,
        .func_p = FuncTimerA_Distancia,
        .param_p = NULL};
    TimerInit(&timer_distancia);

    // gpio puente H y motores
    gpioConf_t gpio_motores[4];

    // como solo avanza uso dos como pwm
    gpio_motores[0].pin = MIB;
    gpio_motores[1].pin = MDB;
    gpio_motores[2].pin = MIA;
    gpio_motores[3].pin = MDA;

    // gpio como salida
    for (int i = 0; i < 4; i++)
    {
        gpio_motores[i].dir = GPIO_OUTPUT;
    }

    // inicializo gpio
    for (int j = 0; j < 4; j++)
    {
        GPIOInit(gpio_motores[j].pin, gpio_motores[j].dir);
    }

    // pwm
    pwm_motores[0] = PWM_0;
    pwm_motores[1] = PWM_1;

    // inicializo pwm
    PWMInit(pwm_motores[0], MIA, 50);
    PWMInit(pwm_motores[1], MDA, 50);

    xTaskCreate(&controlDistancia, "controlDistancia", 2048, NULL, 5, &control_distancia_handle);

    TimerStart(timer_distancia.timer);
}
/*==================[end of file]============================================*/