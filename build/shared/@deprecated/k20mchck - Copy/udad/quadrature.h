/*
* Quadrature decoder driver for MCHCK
* Code is specifically designed for ROTARY ENCODERS WITH DETENTS and is not suitable for other purposes
*
* 2014-02-07 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*
*/

#include <mchck.h>

void quadrature_init();
uint16_t quadrature_value();
int16_t quadrature_delta();