#include "avrsim.h"

char *utoa_recursive (unsigned val, char *s, unsigned radix)
{
    int c;

    if (val >= radix)
	s = utoa_recursive (val / radix, s, radix);
    c = val % radix;
    c += (c < 10 ? '0' : 'a' - 10);
    *s++ = c;
    return s;
}

char *itoa (int val, char *s, int radix)
{
    if (radix < 2 || radix > 36) {
	s[0] = 0;
    } else {
	char *p = s;
	if (radix == 10 && val < 0) {
	    val = -val;
	    *p++ = '-';
	}
	*utoa_recursive (val, p, radix) = 0;
    }
    return s;
}

char *ultoa_recursive (unsigned long val, char *s, unsigned radix)
{
    int c;

    if (val >= radix)
	s = ultoa_recursive (val / radix, s, radix);
    c = val % radix;
    c += (c < 10 ? '0' : 'a' - 10);
    *s++ = c;
    return s;
}

char *ltoa (long val, char *s, int radix)
{
    if (radix < 2 || radix > 36) {
	s[0] = 0;
    } else {
	char *p = s;
	if (radix == 10 && val < 0) {
	    val = -val;
	    *p++ = '-';
	}
	*ultoa_recursive (val, p, radix) = 0;
    }
    return s;
}

char *ultoa (unsigned long val, char *s, int radix)
{
    if (radix < 2 || radix > 36)
	s[0] = 0;
    else
	*ultoa_recursive (val, s, radix) = 0;
    return s;
}

char * dtostrf (double val, signed char width, unsigned char prec, char *s)
{
    char fmt[16];	/* "%-110.100F"	*/
    
    sprintf (fmt, "%%%d.%dF", width, prec);
    sprintf (s, fmt, val);
    return s;
}

#define	PZLEN	5	/* protected zone length	*/

void run_dtostrf (const struct dtostrf_s *pt, int testno)
{
    union {
	long lo;
	float fl;
    } val;
    signed char width;
    unsigned char prec;
    static char s[2*PZLEN + sizeof(pt->pattern)];
    char c, *ps;
    const char *pv;
    
    memset (s, testno, sizeof(s));

    val.lo = pt->lo;
    width  = pt->width;
    prec   = pt->prec;
    ps = dtostrf (val.fl, width, prec, s + PZLEN);

    if (ps != s + PZLEN)
	exit (testno + 0x1000);
    for (ps = s; ps != s + PZLEN; ps++) {
	if ((unsigned char)*ps != (testno & 0377))
	    exit (testno + 0x2000);
    }

    pv = (const char*)&pt->pattern;
    do {
	c = *(char *)(pv++);
	if (*ps++ != c) {
	    printf ("*** testno= %d:  must= %s  was= %s\n",
		testno, pt->pattern, s + PZLEN);
	    exit (testno + 0x3000);
	}
    } while (c);

    for (; ps != s + sizeof(s); ps++) {
	if ((unsigned char)*ps != (testno & 0377))
	    exit (testno + 0x4000);
    }
}