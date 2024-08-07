/*
 * Copyright (C) 2016-2018 by Erik Hofman.
 * Copyright (C) 2016-2018 by Adalin B.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provimed that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice,
 *        this list of conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provimed with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY ADALIN B.V. ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL ADALIN B.V. OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR 
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUTOF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Adalin B.V.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <ostream>

#include <aax/aeonwave>
namespace aax = aeonwave;

#define PRINT_ROW(mtx, r, c) \
        printf ("% 6.3f % 6.3f % 6.3f % 6.3f%c", \
            mtx[0][r],mtx[1][r],mtx[2][r],mtx[3][r],c);

#define PRINT_MATRIX(mtx) \
    PRINT_ROW(mtx, 0, '\n'); PRINT_ROW(mtx, 1, '\n'); \
    PRINT_ROW(mtx, 2, '\n'); PRINT_ROW(mtx, 3, '\n');

#define LOG(s,a,b,c) \
    if (((a)==(b))==(c)) printf((c)?"M1 == M2, succes":"M1 != M2, succes"); \
    else { printf(c?"M1 == M2, failed:\n":"M1 != M2, failed:\n"); \
           std::cout << "m1:\n" << (a); std::cout << "m2:\n" << (b); }\
    printf(": %s\n",(s));

#define PVEC(c,v1,v2,v3) \
    printf("Vector(%2.1f, %2.1f, %2.1f) %c Vector(%2.1f, %2.1f, %2.1f) = Vector(%2.1f, %2.1f, %2.1f)\n", v1[0], v1[1], v1[2], c, v2[0], v2[1], v2[2], v3[0], v3[1], v3[2]);

aaxVec3f at  = { 2.0f, 0.0f, -1.0f };
aaxVec3f up  = { 2.0f, 1.0f,  0.0f };
aaxVec3d pos = { 1.0,  3.0,  -2.5  };
aaxVec3f pos32 = { 1.0f, 3.0f, -2.5f };

int main(int argc, char **argv)
{
    aax::Vector v1(at);
    aax::Vector v2(pos);
    aax::Vector v3;

    v3 = v1+v2; PVEC('+',v1,v2,v3);
    v3 = v1-v2; PVEC('-',v1,v2,v3);
    v3 = v1*v2; PVEC('*',v1,v2,v3);
    v3 = v1/v2; PVEC('/',v1,v2,v3);

    v3 = v2.normalized();
    printf("Normalized Vector(%2.1f, %2.1f, %2.1f) = Vector(%2.1f, %2.1f, %2.1f)\n", v2[0], v2[1], v2[2], v3[0], v3[1], v3[2]);

    v3 = 3.1415926536;
    printf("set Vector(%2.1f, %2.1f, %2.1f) = 3.1415926535: Vector(%5.4f, %5.4f, %5.4f)\n", v2[0], v2[1], v2[2], v3[0], v3[1], v3[2]);

    aax::Matrix64 im, m1, m2;

    LOG("Initializing",m1,m2,true);

    m1.rotate(0.13, 1.0, 0.0, 0.0);
    LOG("Rotating",m1,m2,false);

    m1 += pos;
    aax::Matrix64 m3 = m1 * im;
    LOG("Multiplying identity matrix by m1",m3,m1,true);

    std::cout << std::endl << "Matrix mtx and mtx64:\n" << (m3);

    aaxVec3d _pos;
    aaxVec3f _at, _up;
    m3.get(_pos, _at, _up);
    printf("mtx:   at(%2.1f, %2.1f, %2.1f), up(%2.1f, %2.1f, %2.1f), pos(%2.1f, %2.1f, %2.1f)\n", _at[0], _at[1], _at[2], _up[0], _up[1], _up[2], _pos[0], _pos[1], _pos[2]);

    return 0;
}
