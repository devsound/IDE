#ifndef __UDAD_AVRSIM__
#define __UDAD_AVRSIM__

// Pulled in for AVR code

//#include <string.h>
#include <stdio.h>
#include <string.h>

#ifndef	PATTERN_SIZE
# define PATTERN_SIZE	15
#endif

typedef const char* PGM_P;

extern "C" {
void exit(int status);
}

struct dtostrf_s {
    union {
	long lo;
	float fl;
    };
    signed char width;
    unsigned char prec;
    char pattern[PATTERN_SIZE];
};

char *utoa_recursive (unsigned val, char *s, unsigned radix);
char *itoa (int val, char *s, int radix);
char *ultoa_recursive (unsigned long val, char *s, unsigned radix);
char *ltoa (long val, char *s, int radix);
char *ultoa (unsigned long val, char *s, int radix);
char * dtostrf (double val, signed char width, unsigned char prec, char *s);
void run_dtostrf (const struct dtostrf_s *pt, int testno);

inline static char*  strcpy_P(char *buffer, PGM_P pstr) {
  return strcpy(buffer, pstr);
}

inline static size_t  strlen_P(PGM_P pstr) {
    return strlen(pstr);
}
                                           
inline static char *utoa (unsigned val, char *s, int radix) {
  if (radix < 2 || radix > 36)
    s[0] = 0;
  else
    *utoa_recursive (val, s, radix) = 0;
  return s;
}

#endif//__UDAD_AVRSIM__