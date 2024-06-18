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
 * |     PWM        |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	 B-1A	 	| 	GPIO_16		|
 * | 	 B-1B	 	| 	GPIO_17		|
 * | 	 A-1A	 	| 	GPIO_22		|
 * | 	 A-1B	 	| 	GPIO_23		|
 * | 	 VCC	 	| 	 +5V		|
 * | 	 GND	 	| 	 GND		|
 *
 * |  Servo SG90    |   ESP32   	|
 * |:--------------:|:--------------|
 * |    CONTROL	    | 	GPIO_21		|
 * | 	 VCC	 	|   +5V 		|
 * | 	 GND	 	| 	 GND		|
 *
 * |   HC SR04      |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	 VCC	 	| 	  +5V		|
 * | 	 ECHO	 	| 	GPIO_3		|
 * |    TRIGGER	 	| 	GPIO_2		|
 * | 	 GND	 	| 	  GND		|
 *
 * |      LDR       |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	 VCC	 	| 	 +3.3V		|
 * | 	ETRADA	 	| 	  CH1		|
 * | 	 GND	 	| 	  GND		|
 * 
 * |      LED       |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	SALIDA	 	| 	GPIO_18		|
 * | 	 GND	 	| 	  GND		|
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description        |
 * |:----------:|:-------------------|
 * | 24/05/2024 | Document creation	 |
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
#include "ble_mcu.h"
#include "LDR.h"
#include "analog_io_mcu.h"
/*==================[macros and definitions]=================================*/
// motor izquierdo B
#define MIA GPIO_16 // amarillo
#define MIB GPIO_22 // blanco
// motor derecho A
#define MDA GPIO_17 // violeta
#define MDB GPIO_23 // gris

#define PERIODO 1000000 // periodo lectura del hc sr04
#define giroDer -90
#define giroIzq 90
#define posCentral 0

#define CONFIG_BLINK_PERIOD 500
#define LED_BT LED_1

#define PERIODO 500000 // periodo lectura del LDR
#define GPIO_LED GPIO_18
#define LUX_NORMAL 500
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

/*!
 * @brief Almacena el valor de intensidad leido por el sensor
 */
uint32_t intensidad;

TaskHandle_t control_distancia_handle = NULL;
TaskHandle_t control_intensidad_handle = NULL;
/*==================[internal functions declaration]=========================*/
/*!
 * @brief Envia una notificacion a la funcion controlDistancia
 */
void FuncTimerA_Distancia(void *param)
{
    vTaskNotifyGiveFromISR(control_distancia_handle, pdFALSE);
}

/*!
 * @brief Envia una notificacion a la funcion controlIntensidad
 */
void FuncTimerB_Intesnidad(void *param)
{
    vTaskNotifyGiveFromISR(control_intensidad_handle, pdFALSE);
}

/*!
 * @brief Recibe y envia mensajes por bluethoot
 * @param data
 * @param length
 */
void read_data(uint8_t *data, uint8_t length)
{
    // Si el boton esta en On ('O') el robot debe encenderse
    // Si el boton esta en Off ('o') el robot debe apagarse
    switch (data[0])
    {
    case 'O':
        control = true;
        BleSendString("*ERobot encendido\n*"); // Envio mensaje a la app del celular
        break;
    case 'o':
        control = false;
        BleSendString("*ERobot apagado\n*"); // Envio mensaje a la app del celular
        break;
    default:
        break;
    }
}

//     // Si esta encendido
//     if (control == true)
//     {
//         BleSendString(estado); // Envio el estado del robot a la app del celular
//     }
// }

/*!
 * @brief Controla el giro de los motores
 * @param dir Direccion de debe seguir
 */
void controlRuedas(char dir)
{
    switch (dir)
    {
    case 'A': // avanzar
        GPIOOn(MIB);
        GPIOOn(MDB);

        PWMOn(pwm_motores[0]);
        PWMOn(pwm_motores[1]);

        // Velocidad
        PWMSetDutyCycle(pwm_motores[0], 50);
        PWMSetDutyCycle(pwm_motores[1], 50);
        break;

    case 'F': // frenar
        GPIOOn(MIB);
        GPIOOn(MDB);

        PWMSetDutyCycle(pwm_motores[0], 100);
        PWMSetDutyCycle(pwm_motores[1], 100);
        break;

    case 'D': // giro derecha
        GPIOOn(MIB);
        GPIOOn(MDB);

        PWMOn(pwm_motores[0]);
        PWMOn(pwm_motores[1]);

        // velocidad
        PWMSetDutyCycle(pwm_motores[0], 45); // rueda izq
        PWMSetDutyCycle(pwm_motores[1], 60); // rueda der
        break;

    case 'I': // giro izquierda
        GPIOOn(MIB);
        GPIOOn(MDB);

        PWMOn(pwm_motores[0]);
        PWMOn(pwm_motores[1]);

        // velocidad
        PWMSetDutyCycle(pwm_motores[0], 60); // rueda izq
        PWMSetDutyCycle(pwm_motores[1], 45); // rueda der
        break;

    default:
        break;
    }
}

/*!
 * @brief Lee y controla la distancia medida por el sensor (hc sr04)
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
               // sprintf(estado, "*EAvanzando\n*");
                controlRuedas('A');
            }
            else if (distancia <= 15)
            {
                // apago motores
                //sprintf(estado, "*EFreno\n*");
                controlRuedas('F');

                ServoMove(SERVO_2, giroDer); // se queda en esta posicion
                distDer = HcSr04ReadDistanceInCentimeters();
                vTaskDelay(850 / portTICK_PERIOD_MS);
                ServoMove(SERVO_2, posCentral);
                vTaskDelay(600 / portTICK_PERIOD_MS);
                ServoMove(SERVO_2, giroIzq);
                distIzq = HcSr04ReadDistanceInCentimeters();
                vTaskDelay(850 / portTICK_PERIOD_MS);
                ServoMove(SERVO_2, posCentral);
                vTaskDelay(600 / portTICK_PERIOD_MS);

                if (distDer > distIzq)
                {
                    // grio a la derecha
                   // sprintf(estado, "*EGiro a la derecha\n*");
                    controlRuedas('D');
                    vTaskDelay(350 / portTICK_PERIOD_MS);
                    controlRuedas('A');
                }
                else
                {
                    // giro a la izquierda
                    //sprintf(estado, "*EGiro a la izquierda\n*");
                    controlRuedas('I');
                    vTaskDelay(350 / portTICK_PERIOD_MS);
                    controlRuedas('A');
                }
            }
        }
    }
}

/*!
 * @brief Lee y controla la intensidad medida por el sensor (LDR)
 */
void controlIntensidad(void *pvParameter)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (control)
        {
            // El valor leído por LDR_ReadIntensity() será un número entre 0 y 1023
            // 0 significa máxima resistencia (menos luz) y 1023 mínima resistencia (más luz)
            intensidad = LDR_ReadIntensity();

            if (intensidad > LUX_NORMAL)
            {
                // PRENDO UNA LUZ (LED)
                GPIOOn(GPIO_LED);
            }
            else if (intensidad < LUX_NORMAL)
            {
                // APAGO LA LUZ
                GPIOOff(GPIO_LED);
            }
        }
    }
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    // inicializo hc sr04, servo y LDR
    HcSr04Init(GPIO_3, GPIO_2);
    ServoInit(SERVO_2, GPIO_21);
    LDRInit(CH1);

    LedsInit();

    // inicializo Bluetooth
    ble_config_t ble_configuration = {
        "ROBOT_MICA",
        read_data};
    BleInit(&ble_configuration);

    /* Configuracion e inicializacion del timer A*/
    timer_config_t timer_distancia = {
        .timer = TIMER_A,
        .period = PERIODO,
        .func_p = FuncTimerA_Distancia,
        .param_p = NULL};
    TimerInit(&timer_distancia);

    /* Configuracion e inicializacion del timer B*/
    timer_config_t timer_oscuridad = {
        .timer = TIMER_B,
        .period = PERIODO,
        .func_p = FuncTimerB_Intesnidad,
        .param_p = NULL};
    TimerInit(&timer_oscuridad);

    // Inicializo gpio para el led
    gpioConf_t LED = {
        .pin = GPIO_LED,
        .dir = GPIO_OUTPUT};
    GPIOInit(LED.pin, LED.dir);

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

    // Los leds cambian segun el estado del BLE:
    // Conectado prende led verde, si se desconecta el led amarillo y si se apaga el led rojo
    // while (1)
    // {
    //     vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    //     switch (BleStatus())
    //     {
    //     case BLE_OFF:
    //         LedOn(LED_3);
    //         break;
    //     case BLE_DISCONNECTED:
    //         LedOff(LED_1);
    //         LedToggle(LED_2);
    //         break;
    //     case BLE_CONNECTED:
    //         LedOff(LED_2);
    //         LedOn(LED_1);
    //         break;
    //     }
    // }

    xTaskCreate(&controlDistancia, "controlDistancia", 2048, NULL, 5, &control_distancia_handle);
    xTaskCreate(&controlIntensidad, "controlIntensidad", 2048, NULL, 5, &control_intensidad_handle);

    TimerStart(timer_distancia.timer);
    TimerStart(timer_oscuridad.timer);
}
/*==================[end of file]============================================*/