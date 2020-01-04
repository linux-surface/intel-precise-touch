// SPDX-License-Identifier: GPL-2.0-or-later

#include "fpmath.h"

#define M_PI 3.1415926535897932384626


static inline void fsincos(float x, float *sin, float *cos) {
        asm volatile (
                "FLD %2 \n"
                "FSINCOS \n"
                "FSTP %0 \n"
                "FSTP %1 \n"
                : "=m"(*cos), "=m"(*sin)
                : "m"(x)
        );
}

static inline float fpatan(float x, float y) {
	float result = 0.0;

        asm volatile (
                "FLD %1 \n"
                "FLD %2 \n"
                "FPATAN \n"
                "FSTP %0 \n"
                : "=m"(result)
                : "m"(x), "m"(y)
        );

	return result;
}


static inline float deg2r(int d) {
        return (((float)d) / 18000.0) * M_PI;
}

static inline int r2deg(float r) {
        return (int)((r / M_PI) * 18000.0);
}

/**
 * Convert altitude in range [0, 9000] and azimuth in range [0, 36000] to
 * x-/y-tilt in range [-9000, 9000]. Azimuth is given counter-clockwise,
 * starting with zero on the right. Altitude is given as angle between stylus
 * and z-axis.
 * 
 * Must be executed inbetween `kernel_fpu_begin` and `kernel_fpu_end` calls.
 */
void fpm_altitude_azimuth_to_tilt(int alt, int azm, int *tilt_x, int *tilt_y) {
        float sin_alt, cos_alt;
        float sin_azm, cos_azm;
        float x, y, z;

        // use sin/cos from -pi to pi instead of 0 to 2pi for accuracy
        if (azm > 18000)
                azm = azm - 36000;

        // compute point on sphere
        fsincos(deg2r(alt), &sin_alt, &cos_alt);
        fsincos(deg2r(azm), &sin_azm, &cos_azm);

        x = sin_alt * cos_azm;
        y = sin_alt * sin_azm;
        z = cos_alt;

        // compute xz-/yz-aligned angles from point
        *tilt_x = 9000 - r2deg(fpatan(z, x));
        *tilt_y = r2deg(fpatan(z, y)) - 9000;
}
