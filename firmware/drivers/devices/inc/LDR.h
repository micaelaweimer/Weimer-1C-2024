#ifndef LDR_H
#define LDR_H
/*==================[inclusions]=============================================*/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "analog_io_mcu.h"

/*==================[typedef]================================================*/

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/

void LDRInit(adc_ch_t chanel);

uint32_t LDR_ReadIntensity ();

/*==================[end of file]============================================*/

#endif /* #ifndef LDR_H */