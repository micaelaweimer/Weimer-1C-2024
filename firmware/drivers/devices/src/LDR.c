/*==================[inclusions]=============================================*/
#include "LDR.h"

/*==================[macros and definitions]=================================*/
#define LDR_DARK 1000 /*Resistencia en oscuridad, en KΩ*/
#define LDR_10LUX 15 /*Resistencia en 10 lux, aprox en KΩ*/
#define R_CALIBRACION 47 /*Resistencia calibracion en KΩ*/
#define LUX_NORMAL 500 /*lux normal*/

analog_input_config_t LDR; 

/*==================[internal data definition]===============================*/
uint16_t lecturaLDR;
uint32_t ilum_lux;

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/

void LDRInit(adc_ch_t chanel){
	
	LDR.input=chanel;
	LDR.mode=ADC_SINGLE;
	LDR.func_p = NULL;
    LDR.param_p = NULL;
}

uint32_t LDR_ReadIntensity(){
	
	AnalogInputInit(&LDR);
	AnalogInputReadSingle(LDR.input, &lecturaLDR);
	ilum_lux=(uint32_t)((lecturaLDR*LDR_DARK*10)/(LDR_10LUX*R_CALIBRACION*(1024-lecturaLDR)));
	
	return ilum_lux;
}

/*==================[end of file]============================================*/