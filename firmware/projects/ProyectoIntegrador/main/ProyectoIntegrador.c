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
 * | 20/06/2024 | Finished document	 |
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
/*!
 * @brief definicion de gpio para motor
 */
#define MIA GPIO_16
/*!
 * @brief definicion de gpio para motor
 */
#define MIB GPIO_22 
/*!
 * @brief definicion de gpio para motor
 */
#define MDA GPIO_17
/*!
 * @brief definicion de gpio para motor
 */
#define MDB GPIO_23 

/*!
 * @brief periodo de lectura del sensor hc sr04
 */
#define PERIODO 25000
/*!
 * @brief grados de giro para el servo
 */
#define giroDer -90
/*!
 * @brief grados de giro para el servo
 */
#define giroIzq 90
/*!
 * @brief grados de giro para el servo
 */
#define posCentral 0

/*!
 * @brief periodo para bluetooth
 */
#define CONFIG_BLINK_PERIOD 500

/*!
 * @brief periodo de lectura del LDR
 */
#define PERIODO_LED 500000
/*!
 * @brief definicion del gpio para led
 */
#define GPIO_LED GPIO_18
/*!
 * @brief lux normal para comparacion
 */
#define LUX_NORMAL 5003

/*==================[internal data definition]===============================*/
/*!
 * @brief Estructura para configuracion de gpio
 */
typedef struct
{
    gpio_t pin;
    io_t dir;
} gpioConf_t;

/*!
 * @brief Variable booleana para control de encendido y apagado del programa
 */
volatile bool control = false;

/*!
 * @brief vector de pwm para control de motores
 */
pwm_out_t pwm_motores[2];

/*!
 * @brief Distancia en cm leida por el sensor de ultrasonido
 */
uint16_t distancia;

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

/*!
 * @brief Controla el giro de los motores
 * @param dir Direccion que debe seguir
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
        PWMSetDutyCycle(pwm_motores[0], 60);
        PWMSetDutyCycle(pwm_motores[1], 60);
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
        PWMSetDutyCycle(pwm_motores[0], 35); // rueda izq
        PWMSetDutyCycle(pwm_motores[1], 65); // rueda der
        break;

    case 'I': // giro izquierda
        GPIOOn(MIB);
        GPIOOn(MDB);

        PWMOn(pwm_motores[0]);
        PWMOn(pwm_motores[1]);

        // velocidad
        PWMSetDutyCycle(pwm_motores[0], 65); // rueda izq
        PWMSetDutyCycle(pwm_motores[1], 35); // rueda der
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

            static uint16_t distDer, distIzq;

            if (distancia > 15)
            {
                BleSendString("*EAvanzando\n*");
                controlRuedas('A');
            }
            else if (distancia <= 15)
            {
                // apago motores
                BleSendString("*EFreno\n*");
                controlRuedas('F');

                ServoMove(SERVO_2, giroDer); // se queda en esta posicion
                vTaskDelay(850 / portTICK_PERIOD_MS);
                distDer = HcSr04ReadDistanceInCentimeters();
                ServoMove(SERVO_2, posCentral);
                vTaskDelay(600 / portTICK_PERIOD_MS);
                ServoMove(SERVO_2, giroIzq);
                vTaskDelay(850 / portTICK_PERIOD_MS);
                distIzq = HcSr04ReadDistanceInCentimeters();
                ServoMove(SERVO_2, posCentral);
                vTaskDelay(600 / portTICK_PERIOD_MS);

                if (distDer > distIzq)
                {
                    // grio a la derecha
                    BleSendString("*EGiro a la derecha\n*");
                    controlRuedas('D');
                    vTaskDelay(650 / portTICK_PERIOD_MS);
                    controlRuedas('A');
                }
                else if (distDer < distIzq)
                {
                    // giro a la izquierda
                    BleSendString("*EGiro a la izquierda\n*");
                    controlRuedas('I');
                    vTaskDelay(650 / portTICK_PERIOD_MS);
                    controlRuedas('A');
                }
            }
        }
        else
        {
            controlRuedas('F');
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

            if (intensidad < LUX_NORMAL)
            {
                // PRENDO UNA LUZ (LED)
                GPIOOn(GPIO_LED);
            }
            else if (intensidad > LUX_NORMAL)
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
    // inicializo hc sr04, servo, LDR y leds
    HcSr04Init(GPIO_3, GPIO_2);
    ServoInit(SERVO_2, GPIO_21);
    LDRInit(CH1);
    LedsInit();

    // inicializo Bluetooth
    ble_config_t ble_configuration = {
        "ROBOT_MICA",
        read_data};
    BleInit(&ble_configuration);

    /* Configuracion e inicializacion del timer A */
    timer_config_t timer_distancia = {
        .timer = TIMER_A,
        .period = PERIODO,
        .func_p = FuncTimerA_Distancia,
        .param_p = NULL};
    TimerInit(&timer_distancia);

    /* Configuracion e inicializacion del timer B */
    timer_config_t timer_oscuridad = {
        .timer = TIMER_B,
        .period = PERIODO_LED,
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

    xTaskCreate(&controlDistancia, "controlDistancia", 2048, NULL, 5, &control_distancia_handle);
    xTaskCreate(&controlIntensidad, "controlIntensidad", 2048, NULL, 5, &control_intensidad_handle);

    TimerStart(timer_distancia.timer);
    TimerStart(timer_oscuridad.timer);

    // Los leds cambian segun el estado del BLE:
    // Conectado prende led verde, si se desconecta el led amarillo y si se apaga el led rojo
    while (1)
    {
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        switch (BleStatus())
        {
        case BLE_OFF:
            LedOn(LED_3);
            break;
        case BLE_DISCONNECTED:
            LedOff(LED_1);
            LedToggle(LED_2);
            break;
        case BLE_CONNECTED:
            LedOff(LED_2);
            LedOn(LED_1);
            break;
        }
    }
}
/*==================[end of file]============================================*/