/****************************************************************************
*            half.c
*
* Author: 2009  Daniel Jungmann <dsj@gmx.net>
* Copyright: See COPYING file that comes with this distribution
****************************************************************************/
#include "half.h"
static const Uint32 half_lut[] = {
#include "half.tab"
};
typedef union FloatUint32 {
	float m_float;
	Uint32 m_int;
} FloatUint32;
float half_to_float(const Uint16 value) {
	FloatUint32 tmp;
	tmp.m_int = half_lut[value];
	return tmp.m_float;
}
Uint16 float_to_half(const float value) {
	FloatUint32 f;
	Sint32 new_exponent;
	Uint32 exponent_value;
	Uint32 sign;
	Uint32 mantissa;
	Uint32 exponent;
	f.m_float = value;
	sign = (f.m_int >> 31) & 0x1;
	exponent = (f.m_int >> 23) & 0xFF;
	mantissa = f.m_int & 0x7FFFFF;
	if (exponent == 0) {
		mantissa = 0;
		exponent = 0;
	} else {
		if (exponent == 0xff) {
			// NaN or INF
			mantissa = (mantissa != 0) ? 1 : 0;
			exponent = 31;
		} else {
			// regular number
			new_exponent = exponent - 127;
			if (new_exponent < -24) {
				// this maps to 0
				mantissa = 0;
				exponent = 0;
			} else {
				if (new_exponent < -14) {
					// this maps to a denorm
					exponent = 0;
					exponent_value = (unsigned int)(-14 - new_exponent); // 2^-exp_val
					switch (exponent_value) {
					case 0:
						mantissa = 0;
						break;
					case 1:
						mantissa = 512 + (mantissa >> 14);
						break;
					case 2:
						mantissa = 256 + (mantissa >> 15);
						break;
					case 3:
						mantissa = 128 + (mantissa >> 16);
						break;
					case 4:
						mantissa = 64 + (mantissa >> 17);
						break;
					case 5:
						mantissa = 32 + (mantissa >> 18);
						break;
					case 6:
						mantissa = 16 + (mantissa >> 19);
						break;
					case 7:
						mantissa = 8 + (mantissa >> 20);
						break;
					case 8:
						mantissa = 4 + (mantissa >> 21);
						break;
					case 9:
						mantissa = 2 + (mantissa >> 22);
						break;
					case 10:
						mantissa = 1;
						break;
					}
				} else {
					if (new_exponent > 15) {
						// map this value to infinity
						mantissa = 0;
						exponent = 31;
					} else {
						exponent = new_exponent + 15;
						mantissa = mantissa >> 13;
					}
				}
			}
		}
	}
	return (sign << 15) | (exponent << 10) | mantissa;
}
