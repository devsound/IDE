/*
* Quadrature decoder driver for DevSound K20 devices
*
* Code is currently  designed for ROTARY ENCODERS WITH DETENTS and  not suitable for other
* encoders, however this should be changed - configuration needs to be implemented.
*
* 2014-02-07 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*/

#ifndef __QUADRATURE_H__
#define __QUADRATURE_H__

#ifdef __cplusplus
extern "C" {
#endif

void quadrature_init();
uint16_t quadrature_value();
int16_t quadrature_delta();

#ifdef __cplusplus
}

static const class EncoderClass {
  public:
    inline EncoderClass() {};
    inline void begin() { quadrature_init(); }
    inline void end() { }
    inline uint16_t getAbsolute() { return quadrature_value(); }
    inline int16_t getDelta() { return quadrature_delta(); }
} Encoder;

#endif

#endif// __QUADRATURE_H__
