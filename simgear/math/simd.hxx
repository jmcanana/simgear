// Copyright (C) 2016 Erik Hofman - erik@ehofman.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef __SIMD_H__
#define __SIMD_H__	1

#include <cstring>
#include <cmath>

#include "SGLimits.hxx"
#include "SGMisc.hxx"

template<typename T, int N> class simd4_t;

namespace simd4
{

template<typename T, int N>
inline simd4_t<T,N> min(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    for (int i=0; i<N; ++i) {
        v1[i] = SGMisc<T>::min(v1[i], v2[i]);
    }
    return v1;
}

template<typename T, int N>
inline simd4_t<T,N> max(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    for (int i=0; i<N; ++i) {
        v1[i] = SGMisc<T>::max(v1[i], v2[i]);
    }
    return v1;
}

template<typename T, int N>
inline simd4_t<T,N> abs(simd4_t<T,N> v) {
    for (int i=0; i<N; ++i) {
        v[i] = std::abs(v[i]);
    }
    return v;
}

template<typename T, int N>
inline T magnitude2(simd4_t<T,N> v) {
    T mag2 = 0;
    v = v*v;
    for (int i=0; i<N; ++i) {
        mag2 += v[i];
    }
    return mag2;
}

template<typename T, int N>
inline T magnitude(const simd4_t<T,N>& v) {
    return std::sqrt(magnitude2(v));
}

template<typename T, int N>
inline T normalize(simd4_t<T,N>& v) {
    T mag = simd4::magnitude(v);
    if (mag) {
        v /= mag;
    } else {
        v = T(0);
    }
    return mag;
}

template<typename T, int N>
inline T dot(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    T dp = 0;
    v1 *= v2;
    for (int i=0; i<N; ++i) {
       dp += v1[i];
    }
    return dp;
}

} /* namespace simd4 */


template<typename T, int N>
class simd4_t
{
private:
    union {
       T _v4[4];
       T vec[N];
    };

public:
    simd4_t(void) {}
    simd4_t(T s) {
        for (int i=0; i<N; ++i) vec[i] = s;
        for (int i=N; i<4; ++i) _v4[i] = 0;
    }
    simd4_t(T x, T y) : simd4_t(x,y,0,0) {}
    simd4_t(T x, T y, T z) : simd4_t(x,y,z,0) {}
    simd4_t(T x, T y, T z, T w) {
        _v4[0] = x; _v4[1] = y; _v4[2] = z; _v4[3] = w;
    }
    explicit simd4_t(const T v[N]) {
        std::memcpy(vec, v, sizeof(T[N]));
        for (int i=N; i<4; ++i) _v4[i] = 0;
    }
    template<int M>
    simd4_t(const simd4_t<T,M>& v) {
        std::memcpy(vec, v.ptr(), sizeof(T[M]));
        for (int i=M; i<4; ++i) _v4[i] = 0;
    }
    ~simd4_t(void) {}

    inline T (&v4(void))[4] {
        return vec;
    }

    inline const T (&v4(void) const)[4] {
        return vec;
    }

    inline const T (&ptr(void) const)[N] {
        return vec;
    }

    inline T (&ptr(void))[N] {
        return vec;
    }

    inline operator const T*(void) const {
        return vec;
    }

    inline operator T*(void) {
        return vec;
    }

    inline simd4_t<T,N>& operator=(T s) {
        for (int i=0; i<N; ++i) vec[i] = s;
        for (int i=N; i<4; ++i) _v4[i] = 0;
        return *this;
    }
    inline simd4_t<T,N>& operator=(const T v[N]) {
        std::memcpy(vec, v, sizeof(T[N]));
        for (int i=N; i<4; ++i) _v4[i] = 0;
        return *this;
    }
    template<int M>
    inline simd4_t<T,N>& operator=(const simd4_t<T,M>& v) {
        std::memcpy(vec, v.ptr(), sizeof(T[M]));
        for (int i=M; i<4; ++i) _v4[i] = 0;
        return *this;
    }

    inline simd4_t<T,N>& operator+=(T s) {
        for (int i=0; i<N; ++i) {
            vec[i] += s;
        }
        return *this;
    }
    inline simd4_t<T,N>& operator+=(const T v[N]) {
        for (int i=0; i<N; ++i) {
           vec[i] += v[i];
        }
        return *this;
    }
    inline simd4_t<T,N>& operator+=(const simd4_t<T,N>& v) {
        for (int i=0; i<N; ++i) {
           vec[i] += v[i];
        }
        return *this;
    }

    inline simd4_t<T,N>& operator-=(T s) {
        for (int i=0; i<N; ++i) {
           vec[i] -= s;
        }
        return *this;
    }
 
    inline simd4_t<T,N>& operator-=(const T v[N]) {
        for (int i=0; i<N; ++i) {
            vec[i] -= v[i];
        }
        return *this;
    }
    inline simd4_t<T,N>& operator-=(const simd4_t<T,N>& v) {
        for (int i=0; i<N; ++i) {
            vec[i] -= v[i];
        }
        return *this;
    }
    inline simd4_t<T,N>& operator*=(T s) {
        for (int i=0; i<N; ++i) {
           vec[i] *= s;
        }
        return *this;
    }
    inline simd4_t<T,N>& operator*=(const T v[N]) {
        for (int i=0; i<N; ++i) {
           vec[i] *= v[i];
        }
        return *this;
    }
    inline simd4_t<T,N>& operator*=(const simd4_t<T,N>& v) {
        for (int i=0; i<N; ++i) {
           vec[i] *= v[i];
        }
        return *this;
    }

    inline simd4_t<T,N>& operator/=(T s) {
        return operator*=(1/s);
    }
    inline simd4_t<T,N>& operator/=(const T v[N]) {
        for (int i=0; i<N; ++i) {
           vec[i] /= v[i];
        }
        return *this;
    }
    inline simd4_t<T,N>& operator/=(const simd4_t<T,N>& v) {
        for (int i=0; i<N; ++i) {
           vec[i] /= v[i];
        }
        return *this;
    }
};

template<typename T, int N>
inline simd4_t<T,N> operator-(const simd4_t<T,N>& v) {
    simd4_t<T,N> r = T(0);
    r -= v;
    return r;
}

template<typename T, int N>
inline simd4_t<T,N> operator+(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    v1 += v2;
    return v1;
}

template<typename T, int N>
inline simd4_t<T,N> operator-(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    v1 -= v2;
    return v1;
}

template<typename T, int N>
inline simd4_t<T,N> operator*(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    v1 *= v2;
    return v1;
}

template<typename T, int N>
inline simd4_t<T,N> operator/(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    v1 /= v2;
    return v1;
}

template<typename T, int N>
inline simd4_t<T,N> operator*(T f, simd4_t<T,N> v) {
    v *= f;
    return v;
}

template<typename T, int N>
inline simd4_t<T,N> operator*(simd4_t<T,N> v, T f) {
    v *= f;
    return v;
}


# ifdef __MMX__
#  include <mmintrin.h>
# if defined(_MSC_VER)
#  define ALIGN16  __declspec(align(16))
#  define ALIGN16C
# elif defined(__GNUC__)
#  define ALIGN16
#  define ALIGN16C __attribute__((aligned(16)))
# endif
# endif

# ifdef __SSE__
#  include <xmmintrin.h>
# ifdef __SSE3__
#  include <pmmintrin.h>
# endif

template<int N>
class simd4_t<float,N>
{
private:
   typedef float  __vec4f_t[N];

    union ALIGN16 {
        __m128 simd4;
        __vec4f_t vec;
        float _v4[4];
    } ALIGN16C;

public:
    simd4_t(void) {}
    simd4_t(float f) {
        simd4 = _mm_set1_ps(f);
        for (int i=N; i<4; ++i) _v4[i] = 0.0f;
    }
    simd4_t(float x, float y) : simd4_t(x,y,0,0) {}
    simd4_t(float x, float y, float z) : simd4_t(x,y,z,0) {}
    simd4_t(float x, float y, float z, float w) {
        simd4 = _mm_set_ps(w,z,y,x);
    }
    explicit simd4_t(const __vec4f_t v) {
        simd4 = _mm_loadu_ps(v);
        for (int i=N; i<4; ++i) _v4[i] = 0.0f;
    }
    template<int M>
    simd4_t(const simd4_t<float,M>& v) {
        simd4 = v.v4();
        for (int i=M; i<4; ++i) _v4[i] = 0.0f;
    }
    simd4_t(const __m128& v) {
        simd4 = v;
    }

    inline const __m128 (&v4(void) const) {
        return simd4;
    }
    inline __m128 (&v4(void)) {
        return simd4;
    }

    inline const float (&ptr(void) const)[N] {
        return vec;
    }
    inline float (&ptr(void))[N] {
        return vec;
    }

    inline operator const float*(void) const {
        return vec;
    }
    inline operator float*(void) {
        return vec;
    }

    inline simd4_t<float,N>& operator=(float f) {
        simd4 = _mm_set1_ps(f);
        for (int i=N; i<4; ++i) _v4[i] = 0.0f;
        return *this;
    }
    inline simd4_t<float,N>& operator=(const __vec4f_t v) {
        simd4 = _mm_loadu_ps(v);
        for (int i=N; i<4; ++i) _v4[i] = 0.0f;
        return *this;
    }
    template<int M>
    inline simd4_t<float,N>& operator=(const simd4_t<float,M>& v) {
        simd4 = v.v4();
        for (int i=M; i<4; ++i) _v4[i] = 0.0f;
        return *this;
    }

    inline simd4_t<float,N>& operator+=(float f) {
        return operator+=(simd4_t<float,N>(f));
    }
    inline simd4_t<float,N>& operator+=(const simd4_t<float,N>& v) {
        simd4 = _mm_add_ps(simd4, v.v4());
        return *this;
    }

    inline simd4_t<float,N>& operator-=(float f) {
        return operator-=(simd4_t<float,N>(f));
    }
    inline simd4_t<float,N>& operator-=(const simd4_t<float,N>& v) {
        simd4 = _mm_sub_ps(simd4, v.v4());
        return *this;
    }

    inline simd4_t<float,N>& operator*=(float f) {
        return operator*=(simd4_t<float,N>(f));
    }
    inline simd4_t<float,N>& operator*=(const simd4_t<float,N>& v) {
        simd4 = _mm_mul_ps(simd4, v.v4());
        return *this;
    }

    inline simd4_t<float,N>& operator/=(float f) {
        return operator/=(simd4_t<float,N>(f));
    }
    inline simd4_t<float,N>& operator/=(const simd4_t<float,N>& v) {
        simd4 = _mm_div_ps(simd4, v.v4());
        return *this;
    }
};

namespace simd4
{
// http://stackoverflow.com/questions/6996764/fastest-way-to-do-horizontal-float-vector-sum-on-x86
# ifdef __SSE3__
  inline float hsum_ps_sse(__m128 v) {
    __m128 shuf = _mm_movehdup_ps(v);
    __m128 sums = _mm_add_ps(v, shuf);
    shuf        = _mm_movehl_ps(shuf, sums);
    sums        = _mm_add_ss(sums, shuf);
    return        _mm_cvtss_f32(sums);
  }
# else /* SSE */
  inline float hsum_ps_sse(__m128 v) {
    __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 sums = _mm_add_ps(v, shuf);
    shuf        = _mm_movehl_ps(shuf, sums);
    sums        = _mm_add_ss(sums, shuf);
    return      _mm_cvtss_f32(sums);
  }
# endif

template<>
inline float magnitude2(simd4_t<float,4> v) {
    return hsum_ps_sse(v.v4()*v.v4());
}

template<>
inline float dot(simd4_t<float,4> v1, const simd4_t<float,4>& v2) {
    return hsum_ps_sse(v1.v4()*v2.v4());
}

template<int N>
inline simd4_t<float,N> min(simd4_t<float,N> v1, const simd4_t<float,N>& v2) {
    v1.v4() = _mm_min_ps(v1.v4(), v2.v4());
    return v1;
}

template<int N>
inline simd4_t<float,N> max(simd4_t<float,N> v1, const simd4_t<float,N>& v2) {
    v1.v4() = _mm_max_ps(v1.v4(), v2.v4());
    return v1;
}

template<int N>
inline simd4_t<float,N>abs(simd4_t<float,N> v) {
    static const __m128 sign_mask = _mm_set1_ps(-0.f); // -0.f = 1 << 31
    v.v4() = _mm_andnot_ps(sign_mask, v.v4());
    return v;
}

} /* namsepace simd4 */

# endif


# ifdef __SSE2__
#  include <emmintrin.h>

template<int N>
class simd4_t<double,N>
{
private:
   typedef double  __vec4d_t[N];

    union ALIGN16 {
        __m128d simd4[2];
        __vec4d_t vec;
        double _v4[4];
    } ALIGN16C;

public:
    simd4_t(void) {}
    simd4_t(double d) {
        simd4[0] = simd4[1] = _mm_set1_pd(d);
        for (int i=N; i<4; ++i) _v4[i] = 0.0;
    }
    simd4_t(double x, double y) : simd4_t(x,y,0,0) {}
    simd4_t(double x, double y, double z) : simd4_t(x,y,z,0) {}
    simd4_t(double x, double y, double z, double w) {
        simd4[0] = _mm_set_pd(y,x);
        simd4[1] = _mm_set_pd(w,z);
    }
    explicit simd4_t(const __vec4d_t v) {
        simd4[0] = _mm_loadu_pd(v);
        simd4[1] = _mm_loadu_pd(v+2);
        for (int i=N; i<4; ++i) _v4[i] = 0.0;
    }
    template<int M>
    simd4_t(const simd4_t<double,M>& v) {
        simd4[0] = v.v4()[0];
        simd4[1] = v.v4()[1];
        for (int i=M; i<4; ++i) _v4[i] = 0.0;
    }
    simd4_t(const __m128d v[2]) {
        simd4[0] = v[0];
        simd4[1] = v[1];
    }

        inline const __m128d (&v4(void) const)[2] {
        return simd4;
    }
    inline __m128d (&v4(void))[2] {
        return simd4;
    }

    inline const double (&ptr(void) const)[N] {
        return vec;
    }
    inline double (&ptr(void))[N] {
        return vec;
    }

    inline operator const double*(void) const {
        return vec;
    }
    inline operator double*(void) {
        return vec;
    }

    inline simd4_t<double,N>& operator=(double d) {
        simd4[0] = simd4[1] = _mm_set1_pd(d);
        for (int i=N; i<4; ++i) _v4[i] = 0.0;
        return *this;
    }
    inline simd4_t<double,N>& operator=(const __vec4d_t v) {
        simd4[0] = _mm_loadu_pd(v);
        simd4[1] = _mm_loadu_pd(v+2);
        for (int i=N; i<4; ++i) _v4[i] = 0.0;
        return *this;
    }
    template<int M>
    inline simd4_t<double,N>& operator=(const simd4_t<double,M>& v) {
        simd4[0] = v.v4()[0];
        simd4[1] = v.v4()[1];
        for (int i=M; i<4; ++i) _v4[i] = 0.0;
        return *this;
    }
    inline simd4_t<double,N>& operator=(const __m128d v[2]) {
        simd4[0] = v[0];
        simd4[1] = v[1];
        return *this;
    }

    inline simd4_t<double,N>& operator+=(double d) {
        return operator+=(simd4_t<double,N>(d));
    }
    inline simd4_t<double,N>& operator+=(const simd4_t<double,N>& v) {
        simd4[0] = _mm_add_pd(simd4[0], v.v4()[0]);
        simd4[1] = _mm_add_pd(simd4[1], v.v4()[1]);
        return *this;
    }

    inline simd4_t<double,N>& operator-=(double d) {
        return operator-=(simd4_t<double,N>(d));
    }
    inline simd4_t<double,N>& operator-=(const simd4_t<double,N>& v) {
        simd4[0] = _mm_sub_pd(simd4[0], v.v4()[0]);
        simd4[1] = _mm_sub_pd(simd4[1], v.v4()[1]);
        return *this;
    }

    inline simd4_t<double,N>& operator*=(double d) {
        return operator*=(simd4_t<double,N>(d));
    }
    inline simd4_t<double,N>& operator*=(const simd4_t<double,N>& v) {
        simd4[0] = _mm_mul_pd(simd4[0], v.v4()[0]);
        simd4[1] = _mm_mul_pd(simd4[1], v.v4()[1]);
        return *this;
    }

    inline simd4_t<double,N>& operator/=(double d) {
        return operator/=(simd4_t<double,N>(d));
    }
    inline simd4_t<double,N>& operator/=(const simd4_t<double,N>& v) {
        simd4[0] = _mm_div_pd(simd4[0], v.v4()[0]);
        simd4[1] = _mm_div_pd(simd4[1], v.v4()[1]);
        return *this;
    }
};

namespace simd4
{
// http://stackoverflow.com/questions/6996764/fastest-way-to-do-horizontal-float-vector-sum-on-x86
inline double hsum_pd_sse(const __m128d vd[2]) {
    __m128 undef    = _mm_setzero_ps();
    __m128 shuftmp1 = _mm_movehl_ps(undef, _mm_castpd_ps(vd[0]));
    __m128 shuftmp2 = _mm_movehl_ps(undef, _mm_castpd_ps(vd[1]));
    __m128d shuf1   = _mm_castps_pd(shuftmp1);
    __m128d shuf2   = _mm_castps_pd(shuftmp2);
    return  _mm_cvtsd_f64(_mm_add_sd(vd[0], shuf1)) +
            _mm_cvtsd_f64(_mm_add_sd(vd[1], shuf2));
}

template<>
inline double magnitude2(simd4_t<double,4> v) {
    v *= v;
    return hsum_pd_sse(v.v4());
}

template<>
inline double dot(simd4_t<double,4> v1, const simd4_t<double,4>& v2) {
    v1 *= v2;
    return hsum_pd_sse(v1.v4());
}

template<int N>
inline simd4_t<double,N> min(simd4_t<double,N> v1, const simd4_t<double,N>& v2) {
    v1.v4()[0] = _mm_min_pd(v1.v4()[0], v2.v4()[0]);
    v1.v4()[1] = _mm_min_pd(v1.v4()[1], v2.v4()[1]);
    return v1;
}

template<int N>
inline simd4_t<double,N> max(simd4_t<double,N> v1, const simd4_t<double,N>& v2) {
    v1.v4()[0] = _mm_max_pd(v1.v4()[0], v2.v4()[0]);
    v1.v4()[1] = _mm_max_pd(v1.v4()[1], v2.v4()[1]);
    return v1;
}

template<int N>
inline simd4_t<double,N>abs(simd4_t<double,N> v) {
    static const __m128d sign_mask = _mm_set1_pd(-0.); // -0. = 1 << 63
    v.v4()[0] = _mm_andnot_pd(sign_mask, v.v4()[0]);
    v.v4()[1] = _mm_andnot_pd(sign_mask, v.v4()[1]);
    return v;
}

} /* namespace simd4 */

# endif


# ifdef __SSE2__
#  include <emmintrin.h>
#  ifdef __SSE4_1__
#   include <smmintrin.h>
#  endif

template<int N>
class simd4_t<int,N>
{
private:
   typedef int  __vec4i_t[N];

    union ALIGN16 {
        __m128i simd4;
        __vec4i_t vec;
        int _v4[4];
    } ALIGN16C;

public:
    simd4_t(void) {}
    simd4_t(int i) {
        simd4 = _mm_set1_epi32(i);
        for (int i=N; i<4; ++i) _v4[i] = 0;
    }
    simd4_t(int x, int y) : simd4_t(x,y,0,0) {}
    simd4_t(int x, int y, int z) : simd4_t(x,y,z,0) {}
    simd4_t(int x, int y, int z, int w) {
        simd4 = _mm_set_epi32(w,z,y,x);
    }
    explicit simd4_t(const __vec4i_t v) {
        simd4 = _mm_loadu_si128((__m128i*)v);
        for (int i=N; i<4; ++i) _v4[i] = 0;
    }
    template<int M>
    simd4_t(const simd4_t<int,M>& v) {
        simd4 = v.v4();
        for (int i=M; i<4; ++i) _v4[i] = 0;
    }
    simd4_t(const __m128i& v) {
        simd4 = v;
    }

    inline __m128i (&v4(void)) {
        return simd4;
    }

    inline const __m128i (&v4(void) const) {
        return simd4;
    }

    inline const int (&ptr(void) const)[N] {
        return vec;
    }

    inline int (&ptr(void))[N] {
        return vec;
    }

    inline operator const int*(void) const {
        return vec;
    }

    inline operator int*(void) {
        return vec;
    }

    inline simd4_t<int,N>& operator=(int i) {
        simd4 = _mm_set1_epi32(i);
        for (int i=N; i<4; ++i) _v4[i] = 0;
        return *this;
    }
    inline simd4_t<int,N>& operator=(const __vec4i_t v) {
        simd4 = _mm_loadu_si128((__m128i*)v);
        for (int i=N; i<4; ++i) _v4[i] = 0;
        return *this;
    }
    template<int M>
    inline simd4_t<int,N>& operator=(const simd4_t<int,M>& v) {
        simd4 = v.v4();
        for (int i=M; i<4; ++i) _v4[i] = 0;
        return *this;
    }

    inline simd4_t<int,N>& operator+=(int i) {
        return operator+=(simd4_t<int,N>(i));
    }
    inline simd4_t<int,N>& operator+=(const simd4_t<int,N>& v) {
        simd4 = _mm_add_epi32(simd4, v.v4());
        return *this;
    }

    inline simd4_t<int,N>& operator-=(int i) {
        return operator-=(simd4_t<int,N>(i));
    }
    inline simd4_t<int,N>& operator-=(const simd4_t<int,N>& v) {
        simd4 = _mm_sub_epi32(simd4, v.v4());
        return *this;
    }

    inline simd4_t<int,N>& operator*=(int i) {
        return operator*=(simd4_t<int,N>(i));
    }
    inline simd4_t<int,N>& operator*=(const simd4_t<int,N>& v) {
         return operator*=(v.v4());
    }
    // https://software.intel.com/en-us/forums/intel-c-compiler/topic/288768
    inline simd4_t<int,N>& operator*=(const __m128i& v) {
#ifdef __SSE4_1__
        simd4 = _mm_mullo_epi32(simd4, v);
#else
        __m128i tmp1 = _mm_mul_epu32(simd4, v);
        __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(simd4,4),
                       _mm_srli_si128(v,4));
        simd4 =_mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1,_MM_SHUFFLE (0,0,2,0)),
                                  _mm_shuffle_epi32(tmp2, _MM_SHUFFLE (0,0,2,0)));
#endif
        return *this;
    }

    inline simd4_t<int,N>& operator/=(int s) {
        return operator*=(1/s);
    }
};

namespace simd4
{

#  ifdef __SSE4_1__
template<int N>
inline simd4_t<int,N> min(simd4_t<int,N> v1, const simd4_t<int,N>& v2) {
    v1.v4() = _mm_min_epi32(v1.v4(), v2.v4());
    return v1;
}

template<int N>
inline simd4_t<int,N> max(simd4_t<int,N> v1, const simd4_t<int,N>& v2) {
    v1.v4() = _mm_max_epi32(v1.v4(), v2.v4());
    return v1;
}
#  endif /* __SSE4_1__ */

} /* namespace simd4 */

# endif

#endif /* __SIMD_H__ */
