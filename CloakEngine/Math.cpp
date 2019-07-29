#include "stdafx.h"
#include "CloakEngine/Global/Math.h"

#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Global/Memory.h"
#include "Implementation\Files\Scene.h"

#include <cmath>
#include <intrin.h>
#include <assert.h>

#define ALLOW_VARIANT_INSTRUCTIONS

#define ROTATION_HELPER(v,row,col,cos,add) (v.row*v.col*(1-(cos)))+(add)

#define FLAG_NAME(Name, v0, v1, v2, v3) const __m128 Name = _mm_castsi128_ps(_mm_set_epi32(v3, v2, v1, v0))
#define SET_NAME(Name, v0, v1, v2, v3) const __m128 Name = _mm_set_ps(v3, v2, v1, v0)

#define FLAG_ALL_0(M, Name, L1, L2, V1, V2, v0, v1, v2, v3) M(Name, v0, v1, v2, v3);
#define FLAG_ALL_1(M, Name, L1, L2, V1, V2, v0, v1, v2, v3) FLAG_ALL_0(M, Name##L1, L1, L2, V1, V2, v0, v1, v2, V1) FLAG_ALL_0(M, Name##L2, L1, L2, V1, V2, v0, v1, v2, V2)
#define FLAG_ALL_2(M, Name, L1, L2, V1, V2, v0, v1, v2, v3) FLAG_ALL_1(M, Name##L1, L1, L2, V1, V2, v0, v1, V1, v3) FLAG_ALL_1(M, Name##L2, L1, L2, V1, V2, v0, v1, V2, v3)
#define FLAG_ALL_3(M, Name, L1, L2, V1, V2, v0, v1, v2, v3) FLAG_ALL_2(M, Name##L1, L1, L2, V1, V2, v0, V1, v2, v3) FLAG_ALL_2(M, Name##L2, L1, L2, V1, V2, v0, V2, v2, v3)
#define FLAG_ALL_4(M, Name, L1, L2, V1, V2, v0, v1, v2, v3) FLAG_ALL_3(M, Name##L1, L1, L2, V1, V2, V1, v1, v2, v3) FLAG_ALL_3(M, Name##L2, L1, L2, V1, V2, V2, v1, v2, v3)
#define CREATE_FLAGS(Name, L1, L2, V1, V2) FLAG_ALL_4(FLAG_NAME, FLAG_##Name##_, L1, L2, V1, V2, 0, 0, 0, 0)
#define CREATE_CONSTANTS(Name, L1, L2, V1, V2) FLAG_ALL_4(SET_NAME, Name##_, L1, L2, V1, V2, 0, 0, 0, 0)

//Some ease-of-use macros:
#define _mm_shuffle_epi32(a,b,crt) _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a),_mm_castsi128_ps(b),crt))
#define _mm256_store_m128(src, hi, lo) {__m256 _t = (src); (lo) = _mm256_castps256_ps128(_t); (hi) = _mm256_extractf128_ps(_t,0x1);}

namespace CloakEngine {
	namespace API {
		namespace Global {
			namespace Math {
				CREATE_FLAGS(SIGN, P, N, 0x00000000, 0x80000000);
				CREATE_FLAGS(SET, S, N, 0xFFFFFFFF, 0x00000000);
				CREATE_FLAGS(ABS, S, N, 0x7FFFFFFF, 0xFFFFFFFF);
				CREATE_CONSTANTS(ROW, Z, O, 0, 1);

				const __m128 ALL_TWO = _mm_set_ps1(2);
				const __m128 ALL_ONE = _mm_set_ps1(1);
				const __m128 ALL_HALF = _mm_set_ps1(0.5f);
				const __m128 ALL_ZERO = _mm_set_ps1(0);
				const __m128 ALL_EPS = _mm_set_ps1(0.001f);
				const __m128 ALL_MAX_IF = *reinterpret_cast<__m128*>(&_mm_set1_epi32(0x4B000000));
				const __m128 ALL_FOPI = _mm_set_ps1(static_cast<float>(4 / PI));

				const __m128 ALL_DP1 = _mm_set_ps1(-0.78515625f);
				const __m128 ALL_DP2 = _mm_set_ps1(-2.4187564849853515625e-4f);
				const __m128 ALL_DP3 = _mm_set_ps1(-3.77489497744594108e-8f);
				const __m128 ALL_COSCOF0 = _mm_set_ps1(2.443315711809948E-005f);
				const __m128 ALL_COSCOF1 = _mm_set_ps1(-1.388731625493765E-003f);
				const __m128 ALL_COSCOF2 = _mm_set_ps1(4.166664568298827E-002f);
				const __m128 ALL_SINCOF0 = _mm_set_ps1(-1.9515295891E-4f);
				const __m128 ALL_SINCOF1 = _mm_set_ps1(8.3321608736E-3f);
				const __m128 ALL_SINCOF2 = _mm_set_ps1(-1.6666654611E-1f);

				const __m128i ALL_ZEROi = _mm_set_epi32(0, 0, 0, 0);
				const __m128i ALL_ONEi = _mm_set_epi32(1, 1, 1, 1);
				const __m128i ALL_TWOi = _mm_set_epi32(2, 2, 2, 2);
				const __m128i ALL_nONEi = _mm_set_epi32(~1, ~1, ~1, ~1);
				const __m128i ALL_FOURi = _mm_set_epi32(4, 4, 4, 4);

				const __m128 WORLD_NODE_SIZE = _mm_set_ps1(Impl::Files::WORLDNODE_SIZE);
				const __m128 WORLD_NODE_HALF_SIZE = _mm_set_ps1(Impl::Files::WORLDNODE_HALFSIZE);

				constexpr float QUATERNION_REV_ROTATION_THRESHOLD = 1 - 1e-6f;
				constexpr float QUATERNION_EULER_THRESHOLD = 1 - 1e-3f;

				//Sincos function is modified from https://github.com\RJVB/sse_mathfun/blob/master/sse_mathfun.h
				inline void CLOAK_CALL_MATH _mm_sincos_ps(In const __m128& i, Out __m128* s, Out __m128* c)
				{
					__m128 sign_bit_sin = _mm_and_ps(i, FLAG_SIGN_NNNN);
					__m128 x = _mm_and_ps(i, FLAG_ABS_SSSS);
					__m128 y = _mm_mul_ps(x, ALL_FOPI);
					__m128i i1 = _mm_cvttps_epi32(y);
					i1 = _mm_and_si128(_mm_add_epi32(i1, ALL_ONEi), ALL_nONEi);
					y = _mm_cvtepi32_ps(i1);

					__m128 swap_sign_bit_sin = _mm_castsi128_ps(_mm_slli_epi32(_mm_and_si128(i1, ALL_FOURi), 29));
					__m128 poly_mask = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_and_si128(i1, ALL_TWOi), ALL_ZEROi));
					//Extended precision modular arithmetic
					x = _mm_add_ps(x, _mm_mul_ps(y, _mm_add_ps(_mm_add_ps(ALL_DP1, ALL_DP2), ALL_DP3)));

					__m128 sign_bit_cos = _mm_castsi128_ps(_mm_slli_epi32(_mm_andnot_si128(_mm_sub_epi32(i1, ALL_TWOi), ALL_FOURi), 29));
					sign_bit_sin = _mm_xor_ps(sign_bit_sin, swap_sign_bit_sin);

					//First polynom:
					__m128 z = _mm_mul_ps(x, x);
					__m128 y1 = _mm_add_ps(_mm_mul_ps(ALL_COSCOF0, z), ALL_COSCOF1);
					y1 = _mm_add_ps(_mm_mul_ps(y1, z), ALL_COSCOF2);
					y1 = _mm_sub_ps(_mm_mul_ps(y1, z), ALL_HALF);
					y1 = _mm_add_ps(_mm_mul_ps(y1, z), ALL_ONE);

					//Second Polynom:
					__m128 y2 = _mm_add_ps(_mm_mul_ps(ALL_SINCOF0, z), ALL_SINCOF1);
					y2 = _mm_add_ps(_mm_mul_ps(y2, z), ALL_SINCOF2);
					y2 = _mm_add_ps(_mm_mul_ps(y2, z), ALL_ONE);
					y2 = _mm_mul_ps(y2, x);

					//Select Result:
					__m128 t1 = _mm_add_ps(_mm_andnot_ps(poly_mask, y1), _mm_and_ps(poly_mask, y2));
					__m128 t2 = _mm_sub_ps(_mm_add_ps(y1, y2), t1);

					*s = _mm_xor_ps(t1, sign_bit_sin);
					*c = _mm_xor_ps(t2, sign_bit_cos);
				}
				inline __m128 CLOAK_CALL_MATH _mm_sin_ps(In const __m128& x)
				{
					__m128 s, c;
					_mm_sincos_ps(x, &s, &c);
					return s;
				}
				inline __m128 CLOAK_CALL_MATH _mm_cos_ps(In const __m128& x)
				{
					__m128 s, c;
					_mm_sincos_ps(x, &s, &c);
					return c;
				}

#ifdef ALLOW_VARIANT_INSTRUCTIONS
#define __PFN_ROUND(name) __m128 CLOAK_CALL_MATH name(In const __m128& x)
#define __PFN_PLANE_INIT(name) __m128 CLOAK_CALL_MATH name(In const __m128& x, In float d)
#define __PFN_PLANE_SDISTANCE(name) float CLOAK_CALL_MATH name(In const __m128& a, In const __m128& b)
#define __PFN_MATRIX_FRUSTRUM(name) void CLOAK_CALL_MATH name(Out __m128 Data[6], In const __m128 M[4])
#define __PFN_VECTOR_DOT(name) float CLOAK_CALL_MATH name(In const __m128& a, In const __m128& b)
#define __PFN_VECTOR_LENGTH(name) float CLOAK_CALL_MATH name(In const __m128& a)
#define __PFN_VECTOR_NORMALIZE(name) __m128 CLOAK_CALL_MATH name(In const __m128& x)
#define __PFN_VECTOR_BARYCENTRIC(name) __m128 CLOAK_CALL_MATH name(In const __m128& a, In const __m128& b, In const __m128& c, In const __m128& p)
#define __PFN_MATRIX_MUL(name) void CLOAK_CALL_MATH name(Inout __m128 A[4], In const __m128 B[4])
#define __PFN_DQ_SLERP(name) void CLOAK_CALL_MATH name(Out __m128 Data[2], In const __m128& ra, In const __m128& da, In const __m128& rb, In const __m128& db, In float alpha)
#define __PFN_DQ_NORMALIZE(name) void CLOAK_CALL_MATH name(Out __m128 Data[2], In const __m128& r, In const __m128& d)
#define __PFN_DQ_MATRIX(name) void CLOAK_CALL_MATH name(Out __m128 Data[4], In const __m128& ra, In const __m128& da)
#define __PFN_Q_SLERP(name) __m128 CLOAK_CALL_MATH name(In const __m128& a, In const __m128& b, In float alpha)
#define __PFN_Q_VROT(name) __m128 CLOAK_CALL_MATH name(In const __m128& a, In const __m128& b)
#define __PFN_Q_ROTATION1(name) __m128 CLOAK_CALL_MATH name(In const __m128& a, In const __m128& b)
#define __PFN_Q_ROTATION2(name) __m128 CLOAK_CALL_MATH name(In float angleX, In float angleY, In float angleZ, In RotationOrder order)
#define __PFN_Q_LENGTH(name) float CLOAK_CALL_MATH name(In const __m128& a)
#define __PFN_Q_NORMALIZE(name) __m128 CLOAK_CALL_MATH name(In const __m128& a)
#define __PFN_Q_INVERSE(name) __m128 CLOAK_CALL_MATH name(In const __m128& a)
#define __PFN_Q_MATRIX(name) void CLOAK_CALL_MATH name(Out __m128 Data[4], In const __m128& a)

				typedef __m128 (CLOAK_CALL_MATH *PFN_ROUND)(In const __m128& x);
				typedef __m128 (CLOAK_CALL_MATH *PFN_PLANE_INIT)(In const __m128& x, In float d);
				typedef float (CLOAK_CALL_MATH *PFN_PLANE_SDISTANCE)(In const __m128& a, In const __m128& b);
				typedef void (CLOAK_CALL_MATH *PFN_MATRIX_FRUSTRUM)(Out __m128 Data[6], In const __m128 M[4]);
				typedef float (CLOAK_CALL_MATH *PFN_VECTOR_DOT)(In const __m128& a, In const __m128& b);
				typedef float (CLOAK_CALL_MATH *PFN_VECTOR_LENGTH)(In const __m128& a);
				typedef __m128 (CLOAK_CALL_MATH *PFN_VECTOR_NORMALIZE)(In const __m128& x);
				typedef __m128 (CLOAK_CALL_MATH *PFN_VECTOR_BARYCENTRIC)(In const __m128& a, In const __m128& b, In const __m128& c, In const __m128& p);
				typedef void (CLOAK_CALL_MATH *PFN_MATRIX_MUL)(Inout __m128 A[4], In const __m128 B[4]);
				typedef void (CLOAK_CALL_MATH *PFN_DQ_SLERP)(Out __m128 Data[2], In const __m128& ra, In const __m128& da, In const __m128& rb, In const __m128& db, In float alpha);
				typedef void (CLOAK_CALL_MATH *PFN_DQ_NORMALIZE)(Out __m128 Data[2], In const __m128& r, In const __m128& d);
				typedef void (CLOAK_CALL_MATH *PFN_DQ_MATRIX)(Out __m128 Data[4], In const __m128& r, In const __m128& d);
				typedef __m128 (CLOAK_CALL_MATH *PFN_Q_SLERP)(In const __m128& a, In const __m128& b, In float alpha);
				typedef __m128 (CLOAK_CALL_MATH* PFN_Q_VROT)(In const __m128& a, In const __m128& b);
				typedef __m128 (CLOAK_CALL_MATH *PFN_Q_ROTATION1)(In const __m128& a, In const __m128& b);
				typedef __m128 (CLOAK_CALL_MATH *PFN_Q_ROTATION2)(In float angleX, In float angleY, In float angleZ, In RotationOrder order);
				typedef float (CLOAK_CALL_MATH *PFN_Q_LENGTH)(In const __m128& a);
				typedef __m128 (CLOAK_CALL_MATH *PFN_Q_NORMALIZE)(In const __m128& a);
				typedef __m128 (CLOAK_CALL_MATH *PFN_Q_INVERSE)(In const __m128& a);
				typedef void (CLOAK_CALL_MATH *PFN_Q_MATRIX)(Out __m128 Data[4], In const __m128& a);
				namespace SSE3 {
					__PFN_ROUND(Floor)
					{
						__m128 m = _mm_cmpgt_ps(_mm_and_ps(x, FLAG_ABS_SSSS), ALL_MAX_IF);
						__m128 fi = _mm_cvtepi32_ps(_mm_cvttps_epi32(x));
						__m128 igx = _mm_cmpgt_ps(fi, x);
						return _mm_xor_ps(_mm_and_ps(m, x), _mm_andnot_ps(m, _mm_sub_ps(fi, _mm_and_ps(igx, ALL_ONE))));
					}
					__PFN_ROUND(Ceil)
					{
						__m128 m = _mm_cmpgt_ps(_mm_and_ps(x, FLAG_ABS_SSSS), ALL_MAX_IF);
						__m128 fi = _mm_cvtepi32_ps(_mm_cvttps_epi32(x));
						__m128 igx = _mm_cmplt_ps(fi, x);
						return _mm_xor_ps(_mm_and_ps(m, x), _mm_andnot_ps(m, _mm_add_ps(fi, _mm_and_ps(igx, ALL_ONE))));
					}
					__PFN_PLANE_INIT(PlaneInit)
					{
						__m128 xs = _mm_and_ps(_mm_mul_ps(x, x), FLAG_SET_SSSN);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);
						__m128 n = _mm_mul_ps(x, _mm_rsqrt_ps(dot));
						return _mm_or_ps(_mm_and_ps(n, FLAG_SET_SSSN), _mm_set_ps(d, 0, 0, 0));
					}
					__PFN_PLANE_SDISTANCE(PlaneSDistance)
					{
						__m128 p = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 3, 3, 3));
						__m128 xs = _mm_and_ps(_mm_mul_ps(a, b), FLAG_SET_SSSN);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);
						return _mm_cvtss_f32(_mm_sub_ps(dot, p));
					}
					__PFN_MATRIX_FRUSTRUM(MatrixFrustrum)
					{
						__m128 r[4];
						for (size_t a = 0; a < 4; a++) { r[a] = M[a]; }
						_MM_TRANSPOSE4_PS(r[0], r[1], r[2], r[3]);
						Data[0] = _mm_add_ps(r[3], r[0]);
						Data[1] = _mm_sub_ps(r[3], r[0]);
						Data[2] = _mm_sub_ps(r[3], r[1]);
						Data[3] = _mm_add_ps(r[3], r[1]);
						Data[4] = _mm_sub_ps(r[3], r[2]);
						Data[5] = r[2];

						for (size_t a = 0; a < 6; a++)
						{
							__m128 xs = _mm_and_ps(_mm_mul_ps(Data[a], Data[a]), FLAG_SET_SSSN);
							__m128 dot = _mm_hadd_ps(xs, xs);
							dot = _mm_hadd_ps(dot, dot);
							Data[a] = _mm_xor_ps(_mm_mul_ps(Data[a], _mm_rsqrt_ps(dot)), FLAG_SIGN_NNNP);
						}
					}
					__PFN_VECTOR_DOT(VectorDot)
					{
						__m128 xs = _mm_and_ps(_mm_mul_ps(a, b), FLAG_SET_SSSN);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);
						return _mm_cvtss_f32(dot);
					}
					__PFN_VECTOR_LENGTH(VectorLength)
					{
						__m128 xs = _mm_and_ps(_mm_mul_ps(a, a), FLAG_SET_SSSN);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);
						return _mm_cvtss_f32(_mm_sqrt_ps(dot));
					}
					__PFN_VECTOR_NORMALIZE(VectorNormalize)
					{
						__m128 xs = _mm_and_ps(_mm_mul_ps(x, x), FLAG_SET_SSSN);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);
						return _mm_mul_ps(x, _mm_rsqrt_ps(dot));
					}
					__PFN_VECTOR_BARYCENTRIC(VectorBarycentric)
					{
						__m128 v0 = _mm_sub_ps(b, a);
						__m128 v1 = _mm_sub_ps(c, a);
						__m128 v2 = _mm_sub_ps(p, a);

						__m128 dot;

						dot = _mm_and_ps(_mm_mul_ps(v0, v0), FLAG_SET_SSSN);
						dot = _mm_hadd_ps(dot, dot);
						__m128 d00 = _mm_hadd_ps(dot, dot);
						dot = _mm_and_ps(_mm_mul_ps(v0, v1), FLAG_SET_SSSN);
						dot = _mm_hadd_ps(dot, dot);
						__m128 d01 = _mm_hadd_ps(dot, dot);
						dot = _mm_and_ps(_mm_mul_ps(v1, v1), FLAG_SET_SSSN);
						dot = _mm_hadd_ps(dot, dot);
						__m128 d11 = _mm_hadd_ps(dot, dot);
						dot = _mm_and_ps(_mm_mul_ps(v2, v0), FLAG_SET_SSSN);
						dot = _mm_hadd_ps(dot, dot);
						__m128 d20 = _mm_hadd_ps(dot, dot);
						dot = _mm_and_ps(_mm_mul_ps(v2, v1), FLAG_SET_SSSN);
						dot = _mm_hadd_ps(dot, dot);
						__m128 d21 = _mm_hadd_ps(dot, dot);

						// d0 = 127 [ d11 | d11 | d00 | d00 ] 0
						__m128 d0 = _mm_shuffle_ps(d00, d11, _MM_SHUFFLE(0, 0, 0, 0));
						// d1 = 127 [ d21 | d21 | d20 | d20 ] 0
						__m128 d1 = _mm_shuffle_ps(d20, d21, _MM_SHUFFLE(0, 0, 0, 0));

						// m0 = 127 [ d00 | d00 | d00 | d11 ] 0
						__m128 m0 = _mm_shuffle_ps(d0, d0, _MM_SHUFFLE(0, 0, 0, 2));
						// m1 = 127 [ d11 | d11 | d21 | d20 ] 0
						__m128 m1 = _mm_shuffle_ps(d1, d11, _MM_SHUFFLE(0, 0, 2, 0));
						// m2 = 127 [ d01 | d01 | d20 | d21 ] 0
						__m128 m2 = _mm_shuffle_ps(d1, d01, _MM_SHUFFLE(0, 0, 0, 2));

						// vw = 127 [ denom | denom | w * denom | v * denom ] 0
						__m128 vw = _mm_sub_ps(_mm_mul_ps(m0, m1), _mm_mul_ps(d01, m2));

						// u = 127 [ 2 * denom | denom * (v + w) | 2 * w * denom | 2 * v * denom ] 0
						__m128 u = _mm_add_ps(_mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 0, 1, 0)), _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 1, 1, 0)));
						// r = 127 [ 2 | v+w | w | v ] 0
						__m128 r = _mm_div_ps(u, _mm_shuffle_ps(u, vw, _MM_SHUFFLE(3, 3, 3, 3)));

						return _mm_xor_ps(FLAG_SIGN_NNPN, _mm_sub_ps(ROW_ZZOZ, r));
					}
					__PFN_MATRIX_MUL(MatrixMul)
					{
						__m128 o[4];
						for (size_t a = 0; a < 4; a++)
						{
							o[a] = _mm_mul_ps(_mm_shuffle_ps(A[a], A[a], _MM_SHUFFLE(0, 0, 0, 0)), B[0]);
							o[a] = _mm_add_ps(o[a], _mm_mul_ps(_mm_shuffle_ps(A[a], A[a], _MM_SHUFFLE(1, 1, 1, 1)), B[1]));
							o[a] = _mm_add_ps(o[a], _mm_mul_ps(_mm_shuffle_ps(A[a], A[a], _MM_SHUFFLE(2, 2, 2, 2)), B[2]));
							o[a] = _mm_add_ps(o[a], _mm_mul_ps(_mm_shuffle_ps(A[a], A[a], _MM_SHUFFLE(3, 3, 3, 3)), B[3]));
						}
						for (size_t a = 0; a < 4; a++) { A[a] = o[a]; }
					}
					__PFN_DQ_SLERP(DQSLerp)
					{
						alpha = min(1, max(0, alpha));

						__m128 xs = _mm_mul_ps(ra, rb);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);
						__m128 rbx = rb;
						__m128 dbx = db;
						if (_mm_cvtss_f32(dot) < 0)
						{
							rbx = _mm_sub_ps(ALL_ZERO, rb);
							dbx = _mm_sub_ps(ALL_ZERO, db);
						}
						__m128 rac = _mm_xor_ps(ra, FLAG_SIGN_NNNP);
						__m128 dac = _mm_sub_ps(ALL_ZERO, da);

						__m128 zwxy = _mm_shuffle_ps(dac, dac, _MM_SHUFFLE(1, 0, 3, 2));
						__m128 yxwz = _mm_shuffle_ps(dac, dac, _MM_SHUFFLE(2, 3, 0, 1));
						__m128 wzyx = _mm_shuffle_ps(zwxy, zwxy, _MM_SHUFFLE(2, 3, 0, 1));

						__m128 r = _mm_xor_ps(_mm_mul_ps(rbx, dac), FLAG_SIGN_NNNP);
						__m128 i = _mm_xor_ps(_mm_mul_ps(rbx, yxwz), FLAG_SIGN_NPPP);
						__m128 j = _mm_xor_ps(_mm_mul_ps(rbx, zwxy), FLAG_SIGN_PPNP);
						__m128 k = _mm_xor_ps(_mm_mul_ps(rbx, wzyx), FLAG_SIGN_PNPP);
						__m128 t1 = _mm_hadd_ps(_mm_hadd_ps(k, j), _mm_hadd_ps(i, r));

						zwxy = _mm_shuffle_ps(rac, rac, _MM_SHUFFLE(1, 0, 3, 2));
						yxwz = _mm_shuffle_ps(rac, rac, _MM_SHUFFLE(2, 3, 0, 1));
						wzyx = _mm_shuffle_ps(zwxy, zwxy, _MM_SHUFFLE(2, 3, 0, 1));

						r = _mm_xor_ps(_mm_mul_ps(dbx, rac), FLAG_SIGN_NNNP);
						i = _mm_xor_ps(_mm_mul_ps(dbx, yxwz), FLAG_SIGN_NPPP);
						j = _mm_xor_ps(_mm_mul_ps(dbx, zwxy), FLAG_SIGN_PPNP);
						k = _mm_xor_ps(_mm_mul_ps(dbx, wzyx), FLAG_SIGN_PNPP);
						__m128 t2 = _mm_hadd_ps(_mm_hadd_ps(k, j), _mm_hadd_ps(i, r));

						__m128 dDiff = _mm_add_ps(t1, t2);

						zwxy = _mm_shuffle_ps(rac, rac, _MM_SHUFFLE(1, 0, 3, 2));
						yxwz = _mm_shuffle_ps(rac, rac, _MM_SHUFFLE(2, 3, 0, 1));
						wzyx = _mm_shuffle_ps(zwxy, zwxy, _MM_SHUFFLE(2, 3, 0, 1));

						r = _mm_xor_ps(_mm_mul_ps(rbx, rac), FLAG_SIGN_NNNP);
						i = _mm_xor_ps(_mm_mul_ps(rbx, yxwz), FLAG_SIGN_NPPP);
						j = _mm_xor_ps(_mm_mul_ps(rbx, zwxy), FLAG_SIGN_PPNP);
						k = _mm_xor_ps(_mm_mul_ps(rbx, wzyx), FLAG_SIGN_PNPP);
						__m128 rDiff = _mm_hadd_ps(_mm_hadd_ps(k, j), _mm_hadd_ps(i, r));

						CLOAK_ALIGN float real[4];
						_mm_store_ps(real, rDiff);

						xs = _mm_and_ps(_mm_mul_ps(rDiff, rDiff), FLAG_SET_SSSN);
						dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);

						__m128 inv = _mm_rsqrt_ps(dot);
						__m128 dir = _mm_mul_ps(rDiff, inv);
						__m128 pitch = _mm_xor_ps(_mm_mul_ps(ALL_TWO, _mm_mul_ps(_mm_shuffle_ps(dDiff, dDiff, _MM_SHUFFLE(3, 3, 3, 3)), inv)), FLAG_SIGN_NNNN);
						__m128 dr = _mm_mul_ps(_mm_mul_ps(dir, pitch), _mm_mul_ps(_mm_shuffle_ps(rDiff, rDiff, _MM_SHUFFLE(3, 3, 3, 3)), ALL_HALF));
						__m128 mom = _mm_mul_ps(_mm_sub_ps(dDiff, dr), inv);

						pitch = _mm_mul_ps(pitch, _mm_set_ps1(alpha));
						const float angle = std::acos(real[3])*alpha;

						__m128 sinA = _mm_set_ps1(std::sin(angle));
						__m128 cosA = _mm_set_ps1(std::cos(angle));
						__m128 sinD = _mm_mul_ps(dir, sinA);
						__m128 cosD = _mm_add_ps(_mm_mul_ps(sinA, mom), _mm_mul_ps(_mm_mul_ps(pitch, ALL_HALF), _mm_mul_ps(cosA, dir)));
						cosD = _mm_shuffle_ps(cosD, _mm_shuffle_ps(cosD, _mm_mul_ps(_mm_xor_ps(pitch, FLAG_SIGN_NNNN), _mm_mul_ps(sinA, ALL_HALF)), _MM_SHUFFLE(0, 0, 2, 2)), _MM_SHUFFLE(3, 1, 1, 0));
						sinD = _mm_shuffle_ps(sinD, _mm_shuffle_ps(sinD, cosA, _MM_SHUFFLE(0, 0, 2, 2)), _MM_SHUFFLE(3, 1, 1, 0));

						zwxy = _mm_shuffle_ps(cosD, cosD, _MM_SHUFFLE(1, 0, 3, 2));
						yxwz = _mm_shuffle_ps(cosD, cosD, _MM_SHUFFLE(2, 3, 0, 1));
						wzyx = _mm_shuffle_ps(zwxy, zwxy, _MM_SHUFFLE(2, 3, 0, 1));

						r = _mm_xor_ps(_mm_mul_ps(ra, cosD), FLAG_SIGN_NNNP);
						i = _mm_xor_ps(_mm_mul_ps(ra, yxwz), FLAG_SIGN_NPPP);
						j = _mm_xor_ps(_mm_mul_ps(ra, zwxy), FLAG_SIGN_PPNP);
						k = _mm_xor_ps(_mm_mul_ps(ra, wzyx), FLAG_SIGN_PNPP);
						t1 = _mm_hadd_ps(_mm_hadd_ps(k, j), _mm_hadd_ps(i, r));

						zwxy = _mm_shuffle_ps(sinD, sinD, _MM_SHUFFLE(1, 0, 3, 2));
						yxwz = _mm_shuffle_ps(sinD, sinD, _MM_SHUFFLE(2, 3, 0, 1));
						wzyx = _mm_shuffle_ps(zwxy, zwxy, _MM_SHUFFLE(2, 3, 0, 1));

						r = _mm_xor_ps(_mm_mul_ps(da, sinD), FLAG_SIGN_NNNP);
						i = _mm_xor_ps(_mm_mul_ps(da, yxwz), FLAG_SIGN_NPPP);
						j = _mm_xor_ps(_mm_mul_ps(da, zwxy), FLAG_SIGN_PPNP);
						k = _mm_xor_ps(_mm_mul_ps(da, wzyx), FLAG_SIGN_PNPP);
						t2 = _mm_hadd_ps(_mm_hadd_ps(k, j), _mm_hadd_ps(i, r));

						Data[1] = _mm_add_ps(t1, t2);

						zwxy = _mm_shuffle_ps(sinD, sinD, _MM_SHUFFLE(1, 0, 3, 2));
						yxwz = _mm_shuffle_ps(sinD, sinD, _MM_SHUFFLE(2, 3, 0, 1));
						wzyx = _mm_shuffle_ps(zwxy, zwxy, _MM_SHUFFLE(2, 3, 0, 1));

						r = _mm_xor_ps(_mm_mul_ps(ra, sinD), FLAG_SIGN_NNNP);
						i = _mm_xor_ps(_mm_mul_ps(ra, yxwz), FLAG_SIGN_NPPP);
						j = _mm_xor_ps(_mm_mul_ps(ra, zwxy), FLAG_SIGN_PPNP);
						k = _mm_xor_ps(_mm_mul_ps(ra, wzyx), FLAG_SIGN_PNPP);
						Data[0] = _mm_hadd_ps(_mm_hadd_ps(k, j), _mm_hadd_ps(i, r));
					}
					__PFN_DQ_NORMALIZE(DQNormalize)
					{
						__m128 xs = _mm_mul_ps(r, r);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);
						__m128 l = _mm_rsqrt_ps(dot);
						Data[0] = _mm_mul_ps(r, l);
						Data[1] = _mm_mul_ps(d, l);
					}
					__PFN_DQ_MATRIX(DQMatrix)
					{
						__m128 xs = _mm_mul_ps(ra, ra);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);
						__m128 l = _mm_rsqrt_ps(dot);
						//r = 127 [RKJI] 0
						__m128 r = _mm_mul_ps(ra, l);
						__m128 d = _mm_mul_ps(da, l);

						//t = 2 * 127[R | -K | -J | -I]0
						__m128 td = _mm_mul_ps(d, ALL_TWO);
						__m128 tr = _mm_xor_ps(r, FLAG_SIGN_NNNP);
						__m128 t1 = _mm_shuffle_ps(td, td, _MM_SHUFFLE(0, 1, 2, 3));
						__m128 t2 = _mm_shuffle_ps(tr, tr, _MM_SHUFFLE(0, 1, 0, 1));
						__m128 t3 = _mm_shuffle_ps(tr, tr, _MM_SHUFFLE(2, 3, 2, 3));
						__m128 t4 = _mm_hsub_ps(_mm_mul_ps(td, t2), _mm_mul_ps(t1, t3));
						__m128 t5 = _mm_hadd_ps(_mm_mul_ps(td, t3), _mm_mul_ps(t1, t2));
						t1 = _mm_addsub_ps(_mm_shuffle_ps(t5, t4, _MM_SHUFFLE(3, 2, 1, 0)), _mm_shuffle_ps(t4, t5, _MM_SHUFFLE(2, 3, 0, 1)));
						__m128 t = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(2, 1, 3, 0));

						__m128 q = _mm_mul_ps(r, r);
						__m128 di = _mm_sub_ps(_mm_add_ps(q, _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 3, 3, 3))), _mm_add_ps(_mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 1, 0, 2))));

						//127 [ RK | KJ | JI | IK ] 0
						__m128 kjik = _mm_mul_ps(r, _mm_shuffle_ps(r, r, _MM_SHUFFLE(2, 1, 0, 2)));
						//127 [RJ | KI | RJ | RI] 0
						__m128 jirr = _mm_mul_ps(r, _mm_shuffle_ps(r, r, _MM_SHUFFLE(1, 0, 3, 3)));

						__m128 p1 = _mm_mul_ps(ALL_TWO, _mm_addsub_ps(_mm_shuffle_ps(kjik, kjik, _MM_SHUFFLE(1, 1, 2, 2)), _mm_shuffle_ps(jirr, kjik, _MM_SHUFFLE(3, 3, 0, 0))));
						__m128 p2 = _mm_mul_ps(ALL_TWO, _mm_addsub_ps(_mm_shuffle_ps(jirr, jirr, _MM_SHUFFLE(2, 2, 2, 2)), _mm_shuffle_ps(jirr, jirr, _MM_SHUFFLE(1, 1, 2, 2))));

						/*
						p1:		127
						| JI + RK |
						| JI - RK |
						| JK + RI |
						| JK - RI |
						0

						p2:		127
						| IK + RJ |
						| IK - RJ |
						| IK + IK |
						|	 0	  |
						0

						di:		127
						|				0				|
						| (q[3] + q[2]) - (q[0] + q[1]) |
						| (q[3] + q[1]) - (q[2] + q[0]) |
						| (q[3] + q[0]) - (q[1] + q[2]) |
						0
						*/

						//row0 = 127 [     0 | p2[3] | p1[2] | di[0] ] 0
						//row1 = 127 [     0 | p1[0] | di[1] | p1[3] ] 0
						//row2 = 127 [     0 | di[2] | p1[1] | p2[2] ] 0
						//row3 = 127 [	   1 |  t[2] |  t[1] |  t[0] ] 0

						//p3 = 127 [ p2[3] | p2[2] | p1[2] | p1[1] ] 0
						__m128 p3 = _mm_shuffle_ps(p1, p2, _MM_SHUFFLE(3, 2, 2, 1));

						//row0 = 127 [ p1[2] | p2[3] | di[3] | di[0] ] 0
						Data[0] = _mm_shuffle_ps(di, p3, _MM_SHUFFLE(1, 3, 3, 0));
						Data[0] = _mm_shuffle_ps(Data[0], Data[0], _MM_SHUFFLE(1, 2, 3, 0));
						//row1 = 127 [ di[3] | di[1] | p1[0] | p1[3] ] 0
						Data[1] = _mm_shuffle_ps(p1, di, _MM_SHUFFLE(3, 1, 0, 3));
						Data[1] = _mm_shuffle_ps(Data[1], Data[1], _MM_SHUFFLE(3, 1, 2, 0));
						Data[2] = _mm_shuffle_ps(p3, di, _MM_SHUFFLE(3, 2, 0, 2));
						Data[3] = _mm_shuffle_ps(t, _mm_shuffle_ps(t, ALL_ONE, _MM_SHUFFLE(0, 0, 2, 2)), _MM_SHUFFLE(2, 0, 1, 0));
					}
					__PFN_Q_SLERP(QSLerp)
					{
						alpha = min(1, max(0, alpha));
						__m128 xs = _mm_mul_ps(a, b);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);
						float d = _mm_cvtss_f32(dot);
						if (d > 0.995f)
						{
							__m128 cx = _mm_xor_ps(a, _mm_and_ps(FLAG_SIGN_NNNN, _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 3, 3, 3))));
							__m128 dx = _mm_xor_ps(b, _mm_and_ps(FLAG_SIGN_NNNN, _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 3, 3, 3))));
							__m128 r = _mm_add_ps(_mm_mul_ps(cx, _mm_set_ps1(1 - alpha)), _mm_mul_ps(dx, _mm_set_ps1(alpha)));

							xs = _mm_mul_ps(r, r);
							dot = _mm_hadd_ps(xs, xs);
							dot = _mm_hadd_ps(xs, xs);
							return _mm_mul_ps(r, _mm_rsqrt_ps(dot));
						}
						d = min(1, max(-1, d));
						float beta = alpha * std::acos(d);
						__m128 v = _mm_sub_ps(b, _mm_mul_ps(a, _mm_set_ps1(d)));
						xs = _mm_mul_ps(v, v);
						dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(xs, xs);
						v = _mm_mul_ps(v, _mm_rsqrt_ps(dot));
						return _mm_add_ps(_mm_mul_ps(a, _mm_set_ps1(std::cos(beta))), _mm_mul_ps(v, _mm_set_ps1(std::sin(beta))));
					}
					__PFN_Q_VROT(QVRot)
					{
						/*
						Calculates:
						t = 2 * cross(v, q.ijk);
						res = v + (q.r * t) + cross(t, q.ijk);
						*/
						__m128 s1 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
						__m128 s2 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2));
						__m128 s3 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2));
						__m128 s4 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));

						__m128 t = _mm_mul_ps(ALL_TWO, _mm_sub_ps(_mm_mul_ps(s1, s2), _mm_mul_ps(s3, s4)));

						__m128 r1 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 3, 3, 3)), t), b);

						s1 = _mm_shuffle_ps(t, t, _MM_SHUFFLE(3, 0, 2, 1));
						s3 = _mm_shuffle_ps(t, t, _MM_SHUFFLE(3, 1, 0, 2));

						__m128 r2 = _mm_sub_ps(_mm_mul_ps(s1, s2), _mm_mul_ps(s3, s4));
						return _mm_add_ps(r1, r2);
					}
					__PFN_Q_ROTATION1(QRotation1)
					{
						__m128 na = _mm_and_ps(a, FLAG_SET_SSSN);
						__m128 nb = _mm_and_ps(b, FLAG_SET_SSSN);

						__m128 xs = _mm_mul_ps(na, na);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);

						__m128 an = _mm_mul_ps(na, _mm_rsqrt_ps(dot));

						xs = _mm_mul_ps(nb, nb);
						dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);

						__m128 bn = _mm_mul_ps(nb, _mm_rsqrt_ps(dot));

						xs = _mm_and_ps(_mm_mul_ps(an, bn), FLAG_SET_SSSN);
						dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);

						const float d = _mm_cvtss_f32(dot);
						if (d > QUATERNION_REV_ROTATION_THRESHOLD) { return _mm_set_ps(1, 0, 0, 0); }
						else if (d < -QUATERNION_REV_ROTATION_THRESHOLD) { return _mm_set_ps(0, 0, 0, 1); }//Rotation about 180 degree
						const float s = std::sqrtf((1 + d) * 2);
						const float i = 1 / s;
						__m128 s1 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
						__m128 s2 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2));
						__m128 s3 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2));
						__m128 s4 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
						__m128 r = _mm_sub_ps(_mm_mul_ps(s1, s2), _mm_mul_ps(s3, s4));
						r = _mm_or_ps(_mm_mul_ps(r, _mm_set_ps(0, i, i, i)), _mm_set_ps(s*.05f, 0, 0, 0));

						xs = _mm_mul_ps(r, r);
						dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);

						r = _mm_mul_ps(r, _mm_rsqrt_ps(dot));
						return r;
					}
					__PFN_Q_ROTATION2(QRotation2)
					{
						__m128 angles = _mm_mul_ps(_mm_set_ps(0, angleZ, angleY, angleX), ALL_HALF);
						//Angles: 127 [0 | angleZ/2 | angleY/2 | angleX/2] 0
						__m128 s, c;
						_mm_sincos_ps(angles, &s, &c);
						//s: 127 [ 0 | sz | sy | sx ] 0
						//c: 127 [ 1 | cz | cy | cx ] 0
						switch (order)
						{
							case CloakEngine::API::Global::Math::RotationOrder::XYZ:
							{
								//t6: 127 [ sx | sz | cz | cz ] 0
								__m128 t6 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(0,2,2,2));
								//t0: 127 [ cx | cx | cx | cy ] 0
								__m128 t0 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 0, 0, 1));
								//t1: 127 [ cy | cy | sy | sx ] 0
								__m128 t1 = _mm_shuffle_ps(s, c, _MM_SHUFFLE(1, 1, 1, 0));
								//t2: 127 [ cz | sz | cz | cz ] 0
								__m128 t2 = _mm_shuffle_ps(c, t6, _MM_SHUFFLE(0, 2, 2, 2));
								//t3: 127 [ sx | cz | cy | cx ] 0
								__m128 t3 = _mm_shuffle_ps(c, t6, _MM_SHUFFLE(3, 0, 1, 0));
								//t4: 127 [ sy | sx | sx | sy ] 0
								__m128 t4 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 0, 0, 1));
								//t5: 127 [ sz | sy | sz | sz ] 0
								__m128 t5 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 1, 2, 2));


								t0 = _mm_mul_ps(t0, _mm_mul_ps(t1, t2));
								t1 = _mm_mul_ps(t3, _mm_mul_ps(t4, t5));
								t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_NPNP));

								__m128 xs = _mm_mul_ps(t0, t0);
								__m128 dot = _mm_hadd_ps(xs, xs);
								dot = _mm_hadd_ps(dot, dot);

								return _mm_mul_ps(t0, _mm_rsqrt_ps(dot));
							}
							case CloakEngine::API::Global::Math::RotationOrder::XZY:
							{
								//t0: 127 [ cx | cy | cz | cx ] 0
								__m128 t0 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 1, 2, 0));
								//t1: 127 [ sz | sx | cz | cx ] 0
								__m128 t1 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(2, 0, 2, 0));
								//t2: 127 [ sz | sz | cy | cx ] 0
								__m128 t2 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(2, 2, 1, 0));
								//t3: 127 [ cy | sz | sy | sy ] 0
								__m128 t3 = _mm_shuffle_ps(s, t2, _MM_SHUFFLE(1, 2, 1, 1));
								//t4: 127 [ cz | cx | cx | sz ] 0
								__m128 t4 = _mm_shuffle_ps(t2, c, _MM_SHUFFLE(2, 0, 0, 2));
								//t5: 127 [ sx | cz | cy | cy ] 0
								__m128 t5 = _mm_shuffle_ps(c, t1, _MM_SHUFFLE(2, 1, 1, 1));
								//t6: 127 [ sy | sy | sz | cz ] 0
								__m128 t6 = _mm_shuffle_ps(t4, s, _MM_SHUFFLE(1, 1, 0, 3));
								//t7: 127 [ sz | sx | sx | sx ] 0
								__m128 t7 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 0, 0, 0));

								t0 = _mm_mul_ps(t0, _mm_mul_ps(t3, t4));
								t1 = _mm_mul_ps(t5, _mm_mul_ps(t6, t7));
								t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_PPNN));

								__m128 xs = _mm_mul_ps(t0, t0);
								__m128 dot = _mm_hadd_ps(xs, xs);
								dot = _mm_hadd_ps(dot, dot);

								return _mm_mul_ps(t0, _mm_rsqrt_ps(dot));
							}
							case CloakEngine::API::Global::Math::RotationOrder::YXZ:
							{
								//t0: 127 [ cx | cz | cx | cy ] 0
								__m128 t0 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 2, 0, 1));
								//t1: 127 [ sy | sx | cz | cx ] 0
								__m128 t1 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(1, 0, 2, 0));
								//t4: 127 [ sx | cx | cy | cx ] 0
								__m128 t4 = _mm_shuffle_ps(c, t1, _MM_SHUFFLE(2, 0, 1, 0));
								//t2: 127 [ cy | sx | sy | sx ] 0
								__m128 t2 = _mm_shuffle_ps(s, t4, _MM_SHUFFLE(1, 3, 1, 0));
								//t3: 127 [ cz | sy | cz | cz ] 0
								__m128 t3 = _mm_shuffle_ps(c, t1, _MM_SHUFFLE(1, 3, 2, 2));
								//t5: 127 [ sy | cy | sx | sy ] 0
								__m128 t5 = _mm_shuffle_ps(s, t2, _MM_SHUFFLE(1, 3, 0, 1));
								//t6: 127 [ sz | sz | sz | sz ] 0
								__m128 t6 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 2, 2, 2));

								t0 = _mm_mul_ps(t0, _mm_mul_ps(t2, t3));
								t1 = _mm_mul_ps(t4, _mm_mul_ps(t5, t6));
								t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_NPPN));

								__m128 xs = _mm_mul_ps(t0, t0);
								__m128 dot = _mm_hadd_ps(xs, xs);
								dot = _mm_hadd_ps(dot, dot);

								return _mm_mul_ps(t0, _mm_rsqrt_ps(dot));
							}
							case CloakEngine::API::Global::Math::RotationOrder::YZX:
							{
								//t1: 127 [ sy | sz | cy | cz ] 0
								__m128 t1 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(1, 2, 1, 2));
								//t4: 127 [ sx | sx | cy | cx ] 0
								__m128 t4 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(0, 0, 1, 0));
								//t0: 127 [ cx | cy | cx | sx ] 0
								__m128 t0 = _mm_shuffle_ps(t4, c, _MM_SHUFFLE(0, 1, 0, 2));
								//t2: 127 [ cy | sz | sy | cz ] 0
								__m128 t2 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(1, 2, 3, 0));
								//t3: 127 [ cz | cx | cz | cy ] 0
								__m128 t3 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(2, 0, 2, 1));
								//t5: 127 [ sy | cz | sz | sy ] 0
								__m128 t5 = _mm_shuffle_ps(s, t2, _MM_SHUFFLE(1, 3, 2, 1));
								//t6: 127 [ sz | sy | sx | sz ] 0
								__m128 t6 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 1, 0, 2));

								t0 = _mm_mul_ps(t0, _mm_mul_ps(t2, t3));
								t1 = _mm_mul_ps(t4, _mm_mul_ps(t5, t6));
								t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_NNPP));

								__m128 xs = _mm_mul_ps(t0, t0);
								__m128 dot = _mm_hadd_ps(xs, xs);
								dot = _mm_hadd_ps(dot, dot);

								return _mm_mul_ps(t0, _mm_rsqrt_ps(dot));
							}
							case CloakEngine::API::Global::Math::RotationOrder::ZXY:
							{
								//t0: 127 [ cx | cx | cx | cy ] 0
								__m128 t0 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 0, 0, 1));
								//t6: 127 [ sz | sx | cz | cz ] 0
								__m128 t6 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(2, 0, 2, 2));
								//t1: 127 [ cy | cy | sy | sx ] 0
								__m128 t1 = _mm_shuffle_ps(s, c, _MM_SHUFFLE(1, 1, 1, 0));
								//t2: 127 [ cz | sz | cz | cz ] 0
								__m128 t2 = _mm_shuffle_ps(c, t6, _MM_SHUFFLE(0, 3, 2, 2));
								//t3: 127 [ sx | cz | cy | cx ] 0
								__m128 t3 = _mm_shuffle_ps(c, t6, _MM_SHUFFLE(2, 0, 1, 0));
								//t4: 127 [ sy | sx | sx | sy ] 0
								__m128 t4 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 0, 0, 1));
								//t5: 127 [ sz | sy | sz | sz ] 0
								__m128 t5 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 1, 2, 2));

								t0 = _mm_mul_ps(t0, _mm_mul_ps(t1, t2));
								t1 = _mm_mul_ps(t3, _mm_mul_ps(t4, t5));
								t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_PNNP));

								__m128 xs = _mm_mul_ps(t0, t0);
								__m128 dot = _mm_hadd_ps(xs, xs);
								dot = _mm_hadd_ps(dot, dot);

								return _mm_mul_ps(t0, _mm_rsqrt_ps(dot));
							}
							case CloakEngine::API::Global::Math::RotationOrder::ZYX:
							{
								//t6: 127 [ sx | sy | cx | cz ] 0
								__m128 t6 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(0, 1, 0, 2));
								//t0: 127 [ cx | cz | cx | cy ] 0
								__m128 t0 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 2, 0, 1));
								//t2: 127 [ cz | sy | cz | cz ] 0
								__m128 t2 = _mm_shuffle_ps(c, t6, _MM_SHUFFLE(0, 2, 2, 2));
								//t3: 127 [ sx | cx | cy | cx ] 0
								__m128 t3 = _mm_shuffle_ps(c, t6, _MM_SHUFFLE(3, 1, 1, 0));
								//t1: 127 [ cy | sx | sy | sx ] 0
								__m128 t1 = _mm_shuffle_ps(s, t3, _MM_SHUFFLE(1, 3, 1, 0));
								//t4: 127 [ sy | cy | sx | sy ] 0
								__m128 t4 = _mm_shuffle_ps(s, t1, _MM_SHUFFLE(1, 3, 0, 1));
								//t5: 127 [ sz | sz | sz | sz ] 0
								__m128 t5 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 2, 2, 2));

								t0 = _mm_mul_ps(t0, _mm_mul_ps(t1, t2));
								t1 = _mm_mul_ps(t3, _mm_mul_ps(t4, t5));
								t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_PNPN));

								__m128 xs = _mm_mul_ps(t0, t0);
								__m128 dot = _mm_hadd_ps(xs, xs);
								dot = _mm_hadd_ps(dot, dot);

								return _mm_mul_ps(t0, _mm_rsqrt_ps(dot));
							}
							default:
								break;
						}
						return ALL_ZERO;
					}
					__PFN_Q_LENGTH(QLength)
					{
						__m128 xs = _mm_mul_ps(a, a);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);

						__m128 l = _mm_sqrt_ps(dot);
						return _mm_cvtss_f32(l);
					}
					__PFN_Q_NORMALIZE(QNormalize)
					{
						__m128 xs = _mm_mul_ps(a, a);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);
						return _mm_mul_ps(a, _mm_rsqrt_ps(dot));
					}
					__PFN_Q_INVERSE(QInverse)
					{
						__m128 xs = _mm_mul_ps(a, a);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);
						return _mm_mul_ps(_mm_xor_ps(a, FLAG_SIGN_NNNP), _mm_rsqrt_ps(dot));
					}
					__PFN_Q_MATRIX(QMatrix)
					{
						__m128 xs = _mm_mul_ps(a, a);
						__m128 dot = _mm_hadd_ps(xs, xs);
						dot = _mm_hadd_ps(dot, dot);

						__m128 l = _mm_rsqrt_ps(dot);
						//r = 127 [RKJI] 0
						__m128 r = _mm_mul_ps(a, l);
						//t = 2 * 127 [R | -K | -J | -I] 0
						__m128 tr = _mm_xor_ps(r, FLAG_SIGN_NNNP);
						__m128 t2 = _mm_shuffle_ps(tr, tr, _MM_SHUFFLE(0, 1, 0, 1));
						__m128 t3 = _mm_shuffle_ps(tr, tr, _MM_SHUFFLE(2, 3, 2, 3));

						__m128 q = _mm_mul_ps(r, r);
						__m128 di = _mm_sub_ps(_mm_add_ps(q, _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 3, 3, 3))), _mm_add_ps(_mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 1, 0, 2))));

						//127 [ RK | KJ | JI | IK ] 0
						__m128 kjik = _mm_mul_ps(r, _mm_shuffle_ps(r, r, _MM_SHUFFLE(2, 1, 0, 2)));
						//127 [ RJ | KI | RJ | RI ] 0
						__m128 jirr = _mm_mul_ps(r, _mm_shuffle_ps(r, r, _MM_SHUFFLE(1, 0, 3, 3)));

						__m128 p1 = _mm_mul_ps(ALL_TWO, _mm_addsub_ps(_mm_shuffle_ps(kjik, kjik, _MM_SHUFFLE(1, 1, 2, 2)), _mm_shuffle_ps(jirr, kjik, _MM_SHUFFLE(3, 3, 0, 0))));
						__m128 p2 = _mm_mul_ps(ALL_TWO, _mm_addsub_ps(_mm_shuffle_ps(jirr, jirr, _MM_SHUFFLE(2, 2, 2, 2)), _mm_shuffle_ps(jirr, jirr, _MM_SHUFFLE(1, 1, 2, 2))));
						/*
						p1:		127
						| JI + RK |
						| JI - RK |
						| JK + RI |
						| JK - RI |
						0

						p2:		127
						| IK + RJ |
						| IK - RJ |
						| IK + IK |
						|	 0	  |
						0

						di:		127
						|				0				|
						| (q[3] + q[2]) - (q[0] + q[1]) |
						| (q[3] + q[1]) - (q[2] + q[0]) |
						| (q[3] + q[0]) - (q[1] + q[2]) |
						0
						*/

						//row0 = 127 [     0 | p2[3] | p1[2] | di[0] ] 0
						//row1 = 127 [     0 | p1[0] | di[1] | p1[3] ] 0
						//row2 = 127 [     0 | di[2] | p1[1] | p2[2] ] 0
						//row3 = 127 [	   1 |  t[2] |  t[1] |  t[0] ] 0

						//p3 = 127 [ p2[3] | p2[2] | p1[2] | p1[1] ] 0
						__m128 p3 = _mm_shuffle_ps(p1, p2, _MM_SHUFFLE(3, 2, 2, 1));

						//row0 = 127 [ p1[2] | p2[3] | di[3] | di[0] ] 0
						__m128 row0 = _mm_shuffle_ps(di, p3, _MM_SHUFFLE(1, 3, 3, 0));
						Data[0] = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(1, 2, 3, 0));
						//row1 = 127 [ di[3] | di[1] | p1[0] | p1[3] ] 0
						__m128 row1 = _mm_shuffle_ps(p1, di, _MM_SHUFFLE(3, 1, 0, 3));
						Data[1] = _mm_shuffle_ps(row1, row1, _MM_SHUFFLE(3, 1, 2, 0));
						Data[2] = _mm_shuffle_ps(p3, di, _MM_SHUFFLE(3, 2, 0, 2));
						Data[3] = ROW_ZZZO;
					}
				}
				namespace SSE4 {
					__PFN_ROUND(Floor)
					{
						return _mm_round_ps(x, _MM_FROUND_FLOOR);
					}
					__PFN_ROUND(Ceil)
					{
						return _mm_round_ps(x, _MM_FROUND_CEIL);
					}
					__PFN_PLANE_INIT(PlaneInit)
					{
						__m128 n = _mm_mul_ps(x, _mm_rsqrt_ps(_mm_dp_ps(x, x, 0x7F)));
						return _mm_or_ps(_mm_and_ps(n, FLAG_SET_SSSN), _mm_set_ps(d, 0, 0, 0));
					}
					__PFN_PLANE_SDISTANCE(PlaneSDistance)
					{
						__m128 p = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 3, 3, 3));
						return _mm_cvtss_f32(_mm_sub_ps(_mm_dp_ps(a, b, 0x7F), p));
					}
					__PFN_MATRIX_FRUSTRUM(MatrixFrustrum)
					{
						__m128 r[4];
						for (size_t a = 0; a < 4; a++) { r[a] = M[a]; }
						_MM_TRANSPOSE4_PS(r[0], r[1], r[2], r[3]);
						Data[0] = _mm_add_ps(r[3], r[0]);
						Data[1] = _mm_sub_ps(r[3], r[0]);
						Data[2] = _mm_sub_ps(r[3], r[1]);
						Data[3] = _mm_add_ps(r[3], r[1]);
						Data[4] = _mm_sub_ps(r[3], r[2]);
						Data[5] = r[2];

						for (size_t a = 0; a < 6; a++)
						{
							Data[a] = _mm_xor_ps(_mm_mul_ps(Data[a], _mm_rsqrt_ps(_mm_dp_ps(Data[a], Data[a], 0x7F))), FLAG_SIGN_NNNP);
						}
					}
					__PFN_VECTOR_DOT(VectorDot)
					{
						return _mm_cvtss_f32(_mm_dp_ps(a, b, 0x7F));
					}
					__PFN_VECTOR_LENGTH(VectorLength)
					{
						return _mm_cvtss_f32(_mm_sqrt_ps(_mm_dp_ps(a, a, 0x7F)));
					}
					__PFN_VECTOR_NORMALIZE(VectorNormalize)
					{
						return _mm_mul_ps(x, _mm_rsqrt_ps(_mm_dp_ps(x, x, 0x7F)));
					}
					__PFN_VECTOR_BARYCENTRIC(VectorBarycentric)
					{
						__m128 v0 = _mm_sub_ps(b, a);
						__m128 v1 = _mm_sub_ps(c, a);
						__m128 v2 = _mm_sub_ps(p, a);

						__m128 d00 = _mm_dp_ps(v0, v0, 0x7F);
						__m128 d01 = _mm_dp_ps(v0, v1, 0x7F);
						__m128 d11 = _mm_dp_ps(v1, v1, 0x7F);
						__m128 d20 = _mm_dp_ps(v2, v0, 0x7F);
						__m128 d21 = _mm_dp_ps(v2, v1, 0x7F);

						// d0 = 127 [ d11 | d11 | d00 | d00 ] 0
						__m128 d0 = _mm_shuffle_ps(d00, d11, _MM_SHUFFLE(0, 0, 0, 0));
						// d1 = 127 [ d21 | d21 | d20 | d20 ] 0
						__m128 d1 = _mm_shuffle_ps(d20, d21, _MM_SHUFFLE(0, 0, 0, 0));

						// m0 = 127 [ d00 | d00 | d00 | d11 ] 0
						__m128 m0 = _mm_shuffle_ps(d0, d0, _MM_SHUFFLE(0, 0, 0, 2));
						// m1 = 127 [ d11 | d11 | d21 | d20 ] 0
						__m128 m1 = _mm_shuffle_ps(d1, d11, _MM_SHUFFLE(0, 0, 2, 0));
						// m2 = 127 [ d01 | d01 | d20 | d21 ] 0
						__m128 m2 = _mm_shuffle_ps(d1, d01, _MM_SHUFFLE(0, 0, 0, 2));

						// vw = 127 [ denom | denom | w * denom | v * denom ] 0
						__m128 vw = _mm_sub_ps(_mm_mul_ps(m0, m1), _mm_mul_ps(d01, m2));

						// u = 127 [ 2 * denom | denom * (v + w) | 2 * w * denom | 2 * v * denom ] 0
						__m128 u = _mm_add_ps(_mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 0, 1, 0)), _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 1, 1, 0)));
						// r = 127 [ 2 | v+w | w | v ] 0
						__m128 r = _mm_div_ps(u, _mm_shuffle_ps(u, vw, _MM_SHUFFLE(3, 3, 3, 3)));

						return _mm_xor_ps(FLAG_SIGN_NNPN, _mm_sub_ps(ROW_ZZOZ, r));
					}
					__PFN_DQ_SLERP(DQSLerp)
					{
						alpha = min(1, max(0, alpha));
						__m128 dot = _mm_dp_ps(ra, rb, 0xFF);
						__m128 rbx = rb;
						__m128 dbx = db;
						if (_mm_cvtss_f32(dot) < 0)
						{
							rbx = _mm_sub_ps(ALL_ZERO, rb);
							dbx = _mm_sub_ps(ALL_ZERO, db);
						}
						__m128 rac = _mm_xor_ps(ra, FLAG_SIGN_NNNP);
						__m128 dac = _mm_sub_ps(ALL_ZERO, da);

						__m128 zwxy = _mm_shuffle_ps(dac, dac, _MM_SHUFFLE(1, 0, 3, 2));
						__m128 yxwz = _mm_shuffle_ps(dac, dac, _MM_SHUFFLE(2, 3, 0, 1));
						__m128 wzyx = _mm_shuffle_ps(zwxy, zwxy, _MM_SHUFFLE(2, 3, 0, 1));

						__m128 r = _mm_xor_ps(_mm_mul_ps(rbx, dac), FLAG_SIGN_NNNP);
						__m128 i = _mm_xor_ps(_mm_mul_ps(rbx, yxwz), FLAG_SIGN_NPPP);
						__m128 j = _mm_xor_ps(_mm_mul_ps(rbx, zwxy), FLAG_SIGN_PPNP);
						__m128 k = _mm_xor_ps(_mm_mul_ps(rbx, wzyx), FLAG_SIGN_PNPP);
						__m128 t1 = _mm_hadd_ps(_mm_hadd_ps(k, j), _mm_hadd_ps(i, r));

						zwxy = _mm_shuffle_ps(rac, rac, _MM_SHUFFLE(1, 0, 3, 2));
						yxwz = _mm_shuffle_ps(rac, rac, _MM_SHUFFLE(2, 3, 0, 1));
						wzyx = _mm_shuffle_ps(zwxy, zwxy, _MM_SHUFFLE(2, 3, 0, 1));

						r = _mm_xor_ps(_mm_mul_ps(dbx, rac), FLAG_SIGN_NNNP);
						i = _mm_xor_ps(_mm_mul_ps(dbx, yxwz), FLAG_SIGN_NPPP);
						j = _mm_xor_ps(_mm_mul_ps(dbx, zwxy), FLAG_SIGN_PPNP);
						k = _mm_xor_ps(_mm_mul_ps(dbx, wzyx), FLAG_SIGN_PNPP);
						__m128 t2 = _mm_hadd_ps(_mm_hadd_ps(k, j), _mm_hadd_ps(i, r));

						__m128 dDiff = _mm_add_ps(t1, t2);

						zwxy = _mm_shuffle_ps(rac, rac, _MM_SHUFFLE(1, 0, 3, 2));
						yxwz = _mm_shuffle_ps(rac, rac, _MM_SHUFFLE(2, 3, 0, 1));
						wzyx = _mm_shuffle_ps(zwxy, zwxy, _MM_SHUFFLE(2, 3, 0, 1));

						r = _mm_xor_ps(_mm_mul_ps(rbx, rac), FLAG_SIGN_NNNP);
						i = _mm_xor_ps(_mm_mul_ps(rbx, yxwz), FLAG_SIGN_NPPP);
						j = _mm_xor_ps(_mm_mul_ps(rbx, zwxy), FLAG_SIGN_PPNP);
						k = _mm_xor_ps(_mm_mul_ps(rbx, wzyx), FLAG_SIGN_PNPP);
						__m128 rDiff = _mm_hadd_ps(_mm_hadd_ps(k, j), _mm_hadd_ps(i, r));

						CLOAK_ALIGN float real[4];
						_mm_store_ps(real, rDiff);

						__m128 inv = _mm_rsqrt_ps(_mm_dp_ps(rDiff, rDiff, 0x7F));
						__m128 dir = _mm_mul_ps(rDiff, inv);
						__m128 pitch = _mm_xor_ps(_mm_mul_ps(ALL_TWO, _mm_mul_ps(_mm_shuffle_ps(dDiff, dDiff, _MM_SHUFFLE(3, 3, 3, 3)), inv)), FLAG_SIGN_NNNN);
						__m128 dr = _mm_mul_ps(_mm_mul_ps(dir, pitch), _mm_mul_ps(_mm_shuffle_ps(rDiff, rDiff, _MM_SHUFFLE(3, 3, 3, 3)), ALL_HALF));
						__m128 mom = _mm_mul_ps(_mm_sub_ps(dDiff, dr), inv);

						pitch = _mm_mul_ps(pitch, _mm_set_ps1(alpha));
						const float angle = std::acos(real[3])*alpha;

						__m128 sinA = _mm_set_ps1(std::sin(angle));
						__m128 cosA = _mm_set_ps1(std::cos(angle));
						__m128 sinD = _mm_mul_ps(dir, sinA);
						__m128 cosD = _mm_add_ps(_mm_mul_ps(sinA, mom), _mm_mul_ps(_mm_mul_ps(pitch, ALL_HALF), _mm_mul_ps(cosA, dir)));
						cosD = _mm_shuffle_ps(cosD, _mm_shuffle_ps(cosD, _mm_mul_ps(_mm_xor_ps(pitch, FLAG_SIGN_NNNN), _mm_mul_ps(sinA, ALL_HALF)), _MM_SHUFFLE(0, 0, 2, 2)), _MM_SHUFFLE(3, 1, 1, 0));
						sinD = _mm_shuffle_ps(sinD, _mm_shuffle_ps(sinD, cosA, _MM_SHUFFLE(0, 0, 2, 2)), _MM_SHUFFLE(3, 1, 1, 0));

						zwxy = _mm_shuffle_ps(cosD, cosD, _MM_SHUFFLE(1, 0, 3, 2));
						yxwz = _mm_shuffle_ps(cosD, cosD, _MM_SHUFFLE(2, 3, 0, 1));
						wzyx = _mm_shuffle_ps(zwxy, zwxy, _MM_SHUFFLE(2, 3, 0, 1));

						r = _mm_xor_ps(_mm_mul_ps(ra, cosD), FLAG_SIGN_NNNP);
						i = _mm_xor_ps(_mm_mul_ps(ra, yxwz), FLAG_SIGN_NPPP);
						j = _mm_xor_ps(_mm_mul_ps(ra, zwxy), FLAG_SIGN_PPNP);
						k = _mm_xor_ps(_mm_mul_ps(ra, wzyx), FLAG_SIGN_PNPP);
						t1 = _mm_hadd_ps(_mm_hadd_ps(k, j), _mm_hadd_ps(i, r));

						zwxy = _mm_shuffle_ps(sinD, sinD, _MM_SHUFFLE(1, 0, 3, 2));
						yxwz = _mm_shuffle_ps(sinD, sinD, _MM_SHUFFLE(2, 3, 0, 1));
						wzyx = _mm_shuffle_ps(zwxy, zwxy, _MM_SHUFFLE(2, 3, 0, 1));

						r = _mm_xor_ps(_mm_mul_ps(da, sinD), FLAG_SIGN_NNNP);
						i = _mm_xor_ps(_mm_mul_ps(da, yxwz), FLAG_SIGN_NPPP);
						j = _mm_xor_ps(_mm_mul_ps(da, zwxy), FLAG_SIGN_PPNP);
						k = _mm_xor_ps(_mm_mul_ps(da, wzyx), FLAG_SIGN_PNPP);
						t2 = _mm_hadd_ps(_mm_hadd_ps(k, j), _mm_hadd_ps(i, r));

						Data[1] = _mm_add_ps(t1, t2);

						zwxy = _mm_shuffle_ps(sinD, sinD, _MM_SHUFFLE(1, 0, 3, 2));
						yxwz = _mm_shuffle_ps(sinD, sinD, _MM_SHUFFLE(2, 3, 0, 1));
						wzyx = _mm_shuffle_ps(zwxy, zwxy, _MM_SHUFFLE(2, 3, 0, 1));

						r = _mm_xor_ps(_mm_mul_ps(ra, sinD), FLAG_SIGN_NNNP);
						i = _mm_xor_ps(_mm_mul_ps(ra, yxwz), FLAG_SIGN_NPPP);
						j = _mm_xor_ps(_mm_mul_ps(ra, zwxy), FLAG_SIGN_PPNP);
						k = _mm_xor_ps(_mm_mul_ps(ra, wzyx), FLAG_SIGN_PNPP);
						Data[0] = _mm_hadd_ps(_mm_hadd_ps(k, j), _mm_hadd_ps(i, r));
					}
					__PFN_DQ_NORMALIZE(DQNormalize)
					{
						__m128 l = _mm_rsqrt_ps(_mm_dp_ps(r, r, 0xFF));
						Data[0] = _mm_mul_ps(r, l);
						Data[1] = _mm_mul_ps(d, l);
					}
					__PFN_DQ_MATRIX(DQMatrix)
					{
						__m128 l = _mm_rsqrt_ps(_mm_dp_ps(ra, ra, 0xFF));
						//r = 127 [RKJI] 0
						__m128 r = _mm_mul_ps(ra, l);
						__m128 d = _mm_mul_ps(da, l);

						//t = 2 * 127[R | -K | -J | -I]0
						__m128 td = _mm_mul_ps(d, ALL_TWO);
						__m128 tr = _mm_xor_ps(r, FLAG_SIGN_NNNP);
						__m128 t1 = _mm_shuffle_ps(td, td, _MM_SHUFFLE(0, 1, 2, 3));
						__m128 t2 = _mm_shuffle_ps(tr, tr, _MM_SHUFFLE(0, 1, 0, 1));
						__m128 t3 = _mm_shuffle_ps(tr, tr, _MM_SHUFFLE(2, 3, 2, 3));
						__m128 t4 = _mm_hsub_ps(_mm_mul_ps(td, t2), _mm_mul_ps(t1, t3));
						__m128 t5 = _mm_hadd_ps(_mm_mul_ps(td, t3), _mm_mul_ps(t1, t2));
						t1 = _mm_addsub_ps(_mm_shuffle_ps(t5, t4, _MM_SHUFFLE(3, 2, 1, 0)), _mm_shuffle_ps(t4, t5, _MM_SHUFFLE(2, 3, 0, 1)));
						__m128 t = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(2, 1, 3, 0));

						__m128 q = _mm_mul_ps(r, r);
						__m128 di = _mm_sub_ps(_mm_add_ps(q, _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 3, 3, 3))), _mm_add_ps(_mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 1, 0, 2))));

						//127 [ RK | KJ | JI | IK ] 0
						__m128 kjik = _mm_mul_ps(r, _mm_shuffle_ps(r, r, _MM_SHUFFLE(2, 1, 0, 2)));
						//127 [RJ | KI | RJ | RI] 0
						__m128 jirr = _mm_mul_ps(r, _mm_shuffle_ps(r, r, _MM_SHUFFLE(1, 0, 3, 3)));

						__m128 p1 = _mm_mul_ps(ALL_TWO, _mm_addsub_ps(_mm_shuffle_ps(kjik, kjik, _MM_SHUFFLE(1, 1, 2, 2)), _mm_shuffle_ps(jirr, kjik, _MM_SHUFFLE(3, 3, 0, 0))));
						__m128 p2 = _mm_mul_ps(ALL_TWO, _mm_addsub_ps(_mm_shuffle_ps(jirr, jirr, _MM_SHUFFLE(2, 2, 2, 2)), _mm_shuffle_ps(jirr, jirr, _MM_SHUFFLE(1, 1, 2, 2))));

						/*
						p1:		127
						| JI + RK |
						| JI - RK |
						| JK + RI |
						| JK - RI |
						0

						p2:		127
						| IK + RJ |
						| IK - RJ |
						| IK + IK |
						|	 0	  |
						0

						di:		127
						|				0				|
						| (q[3] + q[2]) - (q[0] + q[1]) |
						| (q[3] + q[1]) - (q[2] + q[0]) |
						| (q[3] + q[0]) - (q[1] + q[2]) |
						0
						*/

						//row0 = 127 [     0 | p2[3] | p1[2] | di[0] ] 0
						//row1 = 127 [     0 | p1[0] | di[1] | p1[3] ] 0
						//row2 = 127 [     0 | di[2] | p1[1] | p2[2] ] 0
						//row3 = 127 [	   1 |  t[2] |  t[1] |  t[0] ] 0

						//p3 = 127 [ p2[3] | p2[2] | p1[2] | p1[1] ] 0
						__m128 p3 = _mm_shuffle_ps(p1, p2, _MM_SHUFFLE(3, 2, 2, 1));

						//row0 = 127 [ p1[2] | p2[3] | di[3] | di[0] ] 0
						Data[0] = _mm_shuffle_ps(di, p3, _MM_SHUFFLE(1, 3, 3, 0));
						Data[0] = _mm_shuffle_ps(Data[0], Data[0], _MM_SHUFFLE(1, 2, 3, 0));
						//row1 = 127 [ di[3] | di[1] | p1[0] | p1[3] ] 0
						Data[1] = _mm_shuffle_ps(p1, di, _MM_SHUFFLE(3, 1, 0, 3));
						Data[1] = _mm_shuffle_ps(Data[1], Data[1], _MM_SHUFFLE(3, 1, 2, 0));
						Data[2] = _mm_shuffle_ps(p3, di, _MM_SHUFFLE(3, 2, 0, 2));
						Data[3] = _mm_shuffle_ps(t, _mm_shuffle_ps(t, ALL_ONE, _MM_SHUFFLE(0, 0, 2, 2)), _MM_SHUFFLE(2, 0, 1, 0));
					}
					__PFN_Q_SLERP(QSLerp)
					{
						alpha = min(1, max(0, alpha));
						__m128 dot = _mm_dp_ps(a, b, 0xFF);
						float d = _mm_cvtss_f32(dot);
						if (d > 0.995f)
						{
							__m128 cx = _mm_xor_ps(a, _mm_and_ps(FLAG_SIGN_NNNN, _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 3, 3, 3))));
							__m128 dx = _mm_xor_ps(b, _mm_and_ps(FLAG_SIGN_NNNN, _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 3, 3, 3))));
							__m128 r = _mm_add_ps(_mm_mul_ps(cx, _mm_set_ps1(1 - alpha)), _mm_mul_ps(dx, _mm_set_ps1(alpha)));
							return _mm_mul_ps(r, _mm_rsqrt_ps(_mm_dp_ps(r, r, 0xFF)));
						}
						d = min(1, max(-1, d));
						float beta = alpha * std::acos(d);
						__m128 v = _mm_sub_ps(b, _mm_mul_ps(a, _mm_set_ps1(d)));
						v = _mm_mul_ps(v, _mm_rsqrt_ps(_mm_dp_ps(v, v, 0xFF)));
						return _mm_add_ps(_mm_mul_ps(a, _mm_set_ps1(std::cos(beta))), _mm_mul_ps(v, _mm_set_ps1(std::sin(beta))));
					}
					__PFN_Q_VROT(QVRot)
					{
						/*
						Calculates:
						res = 2 * ((q.ijk * dot(q.ijk, v)) + (cross(v, q.ijk) * q.r)) + (v * (q.r * q.r - dot(q.ijk, q.ijk)))
						*/
						__m128 w = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 3, 3, 3));
						__m128 r1 = _mm_mul_ps(a, _mm_dp_ps(a, b, 0x7F));
						__m128 r2 = _mm_mul_ps(b, _mm_sub_ps(_mm_mul_ps(w, w), _mm_dp_ps(a, a, 0x7F)));

						__m128 s1 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
						__m128 s2 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2));
						__m128 s3 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2));
						__m128 s4 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));

						__m128 r3 = _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(s1, s2), _mm_mul_ps(s3, s4)), w);

						return _mm_add_ps(_mm_mul_ps(ALL_TWO, _mm_add_ps(r1, r3)), r2);
					}
					__PFN_Q_ROTATION1(QRotation1)
					{
						__m128 an = _mm_mul_ps(a, _mm_rsqrt_ps(_mm_dp_ps(a, a, 0x77)));
						__m128 bn = _mm_mul_ps(b, _mm_rsqrt_ps(_mm_dp_ps(b, b, 0x77)));
						const float d = _mm_cvtss_f32(_mm_dp_ps(an, bn, 0x71));
						if (d > QUATERNION_REV_ROTATION_THRESHOLD) { return _mm_set_ps(1, 0, 0, 0); }
						else if (d < -QUATERNION_REV_ROTATION_THRESHOLD) { return _mm_set_ps(0, 0, 0, 1); }//Rotation about 180 degree
						const float s = std::sqrtf((1 + d) * 2);
						const float i = 1 / s;
						__m128 s1 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
						__m128 s2 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2));
						__m128 s3 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2));
						__m128 s4 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
						__m128 r = _mm_sub_ps(_mm_mul_ps(s1, s2), _mm_mul_ps(s3, s4));
						r = _mm_or_ps(_mm_mul_ps(r, _mm_set_ps(0, i, i, i)), _mm_set_ps(s*.05f, 0, 0, 0));
						r = _mm_mul_ps(r, _mm_rsqrt_ps(_mm_dp_ps(r, r, 0xFF)));
						return r;
					}
					__PFN_Q_ROTATION2(QRotation2)
					{
						__m128 angles = _mm_mul_ps(_mm_set_ps(0, angleZ, angleY, angleX), ALL_HALF);
						//Angles: 127 [0 | angleZ/2 | angleY/2 | angleX/2] 0
						__m128 s, c;
						_mm_sincos_ps(angles, &s, &c);
						
						//s: 127 [ 0 | sz | sy | sx ] 0
						//c: 127 [ 1 | cz | cy | cx ] 0
						switch (order)
						{
							case CloakEngine::API::Global::Math::RotationOrder::XYZ:
							{
								//t6: 127 [ sx | sz | cz | cz ] 0
								__m128 t6 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(0, 2, 2, 2));
								//t0: 127 [ cx | cx | cx | cy ] 0
								__m128 t0 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 0, 0, 1));
								//t1: 127 [ cy | cy | sy | sx ] 0
								__m128 t1 = _mm_shuffle_ps(s, c, _MM_SHUFFLE(1, 1, 1, 0));
								//t2: 127 [ cz | sz | cz | cz ] 0
								__m128 t2 = _mm_shuffle_ps(c, t6, _MM_SHUFFLE(0, 2, 2, 2));
								//t3: 127 [ sx | cz | cy | cx ] 0
								__m128 t3 = _mm_shuffle_ps(c, t6, _MM_SHUFFLE(3, 0, 1, 0));
								//t4: 127 [ sy | sx | sx | sy ] 0
								__m128 t4 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 0, 0, 1));
								//t5: 127 [ sz | sy | sz | sz ] 0
								__m128 t5 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 1, 2, 2));

								t0 = _mm_mul_ps(t0, _mm_mul_ps(t1, t2));
								t1 = _mm_mul_ps(t3, _mm_mul_ps(t4, t5));
								t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_NPNP));
								return _mm_mul_ps(t0, _mm_rsqrt_ps(_mm_dp_ps(t0, t0, 0xFF)));
							}
							case CloakEngine::API::Global::Math::RotationOrder::XZY:
							{
								//t0: 127 [ cx | cy | cz | cx ] 0
								__m128 t0 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 1, 2, 0));
								//t1: 127 [ sz | sx | cz | cx ] 0
								__m128 t1 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(2, 0, 2, 0));
								//t2: 127 [ sz | sz | cy | cx ] 0
								__m128 t2 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(2, 2, 1, 0));
								//t3: 127 [ cy | sz | sy | sy ] 0
								__m128 t3 = _mm_shuffle_ps(s, t2, _MM_SHUFFLE(1, 2, 1, 1));
								//t4: 127 [ cz | cx | cx | sz ] 0
								__m128 t4 = _mm_shuffle_ps(t2, c, _MM_SHUFFLE(2, 0, 0, 2));
								//t5: 127 [ sx | cz | cy | cy ] 0
								__m128 t5 = _mm_shuffle_ps(c, t1, _MM_SHUFFLE(2, 1, 1, 1));
								//t6: 127 [ sy | sy | sz | cz ] 0
								__m128 t6 = _mm_shuffle_ps(t4, s, _MM_SHUFFLE(1, 1, 0, 3));
								//t7: 127 [ sz | sx | sx | sx ] 0
								__m128 t7 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 0, 0, 0));

								t0 = _mm_mul_ps(t0, _mm_mul_ps(t3, t4));
								t1 = _mm_mul_ps(t5, _mm_mul_ps(t6, t7));
								t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_PPNN));
								return _mm_mul_ps(t0, _mm_rsqrt_ps(_mm_dp_ps(t0, t0, 0xFF)));
							}
							case CloakEngine::API::Global::Math::RotationOrder::YXZ:
							{
								//t0: 127 [ cx | cz | cx | cy ] 0
								__m128 t0 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 2, 0, 1));
								//t1: 127 [ sy | sx | cz | cx ] 0
								__m128 t1 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(1, 0, 2, 0));
								//t4: 127 [ sx | cx | cy | cx ] 0
								__m128 t4 = _mm_shuffle_ps(c, t1, _MM_SHUFFLE(2, 0, 1, 0));
								//t2: 127 [ cy | sx | sy | sx ] 0
								__m128 t2 = _mm_shuffle_ps(s, t4, _MM_SHUFFLE(1, 3, 1, 0));
								//t3: 127 [ cz | sy | cz | cz ] 0
								__m128 t3 = _mm_shuffle_ps(c, t1, _MM_SHUFFLE(1, 3, 2, 2));
								//t5: 127 [ sy | cy | sx | sy ] 0
								__m128 t5 = _mm_shuffle_ps(s, t2, _MM_SHUFFLE(1, 3, 0, 1));
								//t6: 127 [ sz | sz | sz | sz ] 0
								__m128 t6 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 2, 2, 2));

								t0 = _mm_mul_ps(t0, _mm_mul_ps(t2, t3));
								t1 = _mm_mul_ps(t4, _mm_mul_ps(t5, t6));
								t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_NPPN));
								return _mm_mul_ps(t0, _mm_rsqrt_ps(_mm_dp_ps(t0, t0, 0xFF)));
							}
							case CloakEngine::API::Global::Math::RotationOrder::YZX:
							{
								//t1: 127 [ sy | sz | cy | cz ] 0
								__m128 t1 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(1, 2, 1, 2));
								//t4: 127 [ sx | sx | cy | cx ] 0
								__m128 t4 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(0, 0, 1, 0));
								//t0: 127 [ cx | cy | cx | sx ] 0
								__m128 t0 = _mm_shuffle_ps(t4, c, _MM_SHUFFLE(0, 1, 0, 2));
								//t2: 127 [ cy | sz | sy | cz ] 0
								__m128 t2 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(1, 2, 3, 0));
								//t3: 127 [ cz | cx | cz | cy ] 0
								__m128 t3 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(2, 0, 2, 1));
								//t5: 127 [ sy | cz | sz | sy ] 0
								__m128 t5 = _mm_shuffle_ps(s, t2, _MM_SHUFFLE(1, 3, 2, 1));
								//t6: 127 [ sz | sy | sx | sz ] 0
								__m128 t6 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 1, 0, 2));

								t0 = _mm_mul_ps(t0, _mm_mul_ps(t2, t3));
								t1 = _mm_mul_ps(t4, _mm_mul_ps(t5, t6));
								t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_NNPP));
								return _mm_mul_ps(t0, _mm_rsqrt_ps(_mm_dp_ps(t0, t0, 0xFF)));
							}
							case CloakEngine::API::Global::Math::RotationOrder::ZXY:
							{
								//t0: 127 [ cx | cx | cx | cy ] 0
								__m128 t0 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 0, 0, 1));
								//t6: 127 [ sz | sx | cz | cz ] 0
								__m128 t6 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(2, 0, 2, 2));
								//t1: 127 [ cy | cy | sy | sx ] 0
								__m128 t1 = _mm_shuffle_ps(s, c, _MM_SHUFFLE(1, 1, 1, 0));
								//t2: 127 [ cz | sz | cz | cz ] 0
								__m128 t2 = _mm_shuffle_ps(c, t6, _MM_SHUFFLE(0, 3, 2, 2));
								//t3: 127 [ sx | cz | cy | cx ] 0
								__m128 t3 = _mm_shuffle_ps(c, t6, _MM_SHUFFLE(2, 0, 1, 0));
								//t4: 127 [ sy | sx | sx | sy ] 0
								__m128 t4 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 0, 0, 1));
								//t5: 127 [ sz | sy | sz | sz ] 0
								__m128 t5 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 1, 2, 2));
								
								t0 = _mm_mul_ps(t0, _mm_mul_ps(t1, t2));
								t1 = _mm_mul_ps(t3, _mm_mul_ps(t4, t5));
								t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_PNNP));
								return _mm_mul_ps(t0, _mm_rsqrt_ps(_mm_dp_ps(t0, t0, 0xFF)));
							}
							case CloakEngine::API::Global::Math::RotationOrder::ZYX:
							{
								//t6: 127 [ sx | sy | cx | cz ] 0
								__m128 t6 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(0,1,0,2));
								//t0: 127 [ cx | cz | cx | cy ] 0
								__m128 t0 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 2, 0, 1));
								//t2: 127 [ cz | sy | cz | cz ] 0
								__m128 t2 = _mm_shuffle_ps(c, t6, _MM_SHUFFLE(0, 2, 2, 2));
								//t3: 127 [ sx | cx | cy | cx ] 0
								__m128 t3 = _mm_shuffle_ps(c, t6, _MM_SHUFFLE(3, 1, 1, 0));
								//t1: 127 [ cy | sx | sy | sx ] 0
								__m128 t1 = _mm_shuffle_ps(s, t3, _MM_SHUFFLE(1, 3, 1, 0));
								//t4: 127 [ sy | cy | sx | sy ] 0
								__m128 t4 = _mm_shuffle_ps(s, t1, _MM_SHUFFLE(1, 3, 0, 1));
								//t5: 127 [ sz | sz | sz | sz ] 0
								__m128 t5 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 2, 2, 2));

								t0 = _mm_mul_ps(t0, _mm_mul_ps(t1, t2));
								t1 = _mm_mul_ps(t3, _mm_mul_ps(t4, t5));
								t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_PNPN));
								return _mm_mul_ps(t0, _mm_rsqrt_ps(_mm_dp_ps(t0, t0, 0xFF)));
							}
							default:
								break;
						}
						return ALL_ZERO;
					}
					__PFN_Q_LENGTH(QLength)
					{
						__m128 l = _mm_sqrt_ps(_mm_dp_ps(a, a, 0xFF));
						return _mm_cvtss_f32(l);
					}
					__PFN_Q_NORMALIZE(QNormalize)
					{
						return _mm_mul_ps(a, _mm_rsqrt_ps(_mm_dp_ps(a, a, 0xFF)));
					}
					__PFN_Q_INVERSE(QInverse)
					{
						return _mm_mul_ps(_mm_xor_ps(a, FLAG_SIGN_NNNP), _mm_rsqrt_ps(_mm_dp_ps(a, a, 0xFF)));
					}
					__PFN_Q_MATRIX(QMatrix)
					{
						__m128 l = _mm_rsqrt_ps(_mm_dp_ps(a, a, 0xFF));
						//r = 127 [RKJI] 0
						__m128 r = _mm_mul_ps(a, l);
						//t = 2 * 127 [R | -K | -J | -I] 0
						__m128 tr = _mm_xor_ps(r, FLAG_SIGN_NNNP);
						__m128 t2 = _mm_shuffle_ps(tr, tr, _MM_SHUFFLE(0, 1, 0, 1));
						__m128 t3 = _mm_shuffle_ps(tr, tr, _MM_SHUFFLE(2, 3, 2, 3));

						__m128 q = _mm_mul_ps(r, r);
						__m128 di = _mm_sub_ps(_mm_add_ps(q, _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 3, 3, 3))), _mm_add_ps(_mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 1, 0, 2))));

						//127 [ RK | KJ | JI | IK ] 0
						__m128 kjik = _mm_mul_ps(r, _mm_shuffle_ps(r, r, _MM_SHUFFLE(2, 1, 0, 2)));
						//127 [ RJ | KI | RJ | RI ] 0
						__m128 jirr = _mm_mul_ps(r, _mm_shuffle_ps(r, r, _MM_SHUFFLE(1, 0, 3, 3)));
						
						__m128 p1 = _mm_mul_ps(ALL_TWO, _mm_addsub_ps(_mm_shuffle_ps(kjik, kjik, _MM_SHUFFLE(1, 1, 2, 2)), _mm_shuffle_ps(jirr, kjik, _MM_SHUFFLE(3, 3, 0, 0))));
						__m128 p2 = _mm_mul_ps(ALL_TWO, _mm_addsub_ps(_mm_shuffle_ps(jirr, jirr, _MM_SHUFFLE(2, 2, 2, 2)), _mm_shuffle_ps(jirr, jirr, _MM_SHUFFLE(1, 1, 2, 2))));
						/*
						p1:		127
						| JI + RK |
						| JI - RK |
						| JK + RI |
						| JK - RI |
						0

						p2:		127
						| IK + RJ |
						| IK - RJ |
						| IK + IK |
						|	 0	  |
						0

						di:		127
						|				0				|
						| (q[3] + q[2]) - (q[0] + q[1]) |
						| (q[3] + q[1]) - (q[2] + q[0]) |
						| (q[3] + q[0]) - (q[1] + q[2]) |
						0
						*/

						//row0 = 127 [     0 | p2[3] | p1[2] | di[0] ] 0
						//row1 = 127 [     0 | p1[0] | di[1] | p1[3] ] 0
						//row2 = 127 [     0 | di[2] | p1[1] | p2[2] ] 0
						//row3 = 127 [	   1 |  t[2] |  t[1] |  t[0] ] 0

						//p3 = 127 [ p2[3] | p2[2] | p1[2] | p1[1] ] 0
						__m128 p3 = _mm_shuffle_ps(p1, p2, _MM_SHUFFLE(3, 2, 2, 1));

						//row0 = 127 [ p1[2] | p2[3] | di[3] | di[0] ] 0
						__m128 row0 = _mm_shuffle_ps(di, p3, _MM_SHUFFLE(1, 3, 3, 0));
						Data[0] = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(1, 2, 3, 0));
						//row1 = 127 [ di[3] | di[1] | p1[0] | p1[3] ] 0
						__m128 row1 = _mm_shuffle_ps(p1, di, _MM_SHUFFLE(3, 1, 0, 3));
						Data[1] = _mm_shuffle_ps(row1, row1, _MM_SHUFFLE(3, 1, 2, 0));
						Data[2] = _mm_shuffle_ps(p3, di, _MM_SHUFFLE(3, 2, 0, 2));
						Data[3] = ROW_ZZZO;
					}
				}
				namespace AVX {
					__PFN_MATRIX_MUL(MatrixMul)
					{
						__m256 t[2];
						t[0] = _mm256_set_m128(A[1], A[0]);
						t[1] = _mm256_set_m128(A[3], A[2]);

						__m256 o[2];
						for (size_t a = 0; a < 2; a++)
						{
							o[a] = _mm256_mul_ps(_mm256_shuffle_ps(t[a], t[a], _MM_SHUFFLE(0, 0, 0, 0)), _mm256_broadcast_ps(&B[0]));
							o[a] = _mm256_add_ps(o[a], _mm256_mul_ps(_mm256_shuffle_ps(t[a], t[a], _MM_SHUFFLE(1, 1, 1, 1)), _mm256_broadcast_ps(&B[1])));
							o[a] = _mm256_add_ps(o[a], _mm256_mul_ps(_mm256_shuffle_ps(t[a], t[a], _MM_SHUFFLE(2, 2, 2, 2)), _mm256_broadcast_ps(&B[2])));
							o[a] = _mm256_add_ps(o[a], _mm256_mul_ps(_mm256_shuffle_ps(t[a], t[a], _MM_SHUFFLE(3, 3, 3, 3)), _mm256_broadcast_ps(&B[3])));
						}

						_mm256_store_m128(o[0], A[1], A[0]);
						_mm256_store_m128(o[1], A[3], A[2]);
					}
				}

				PFN_ROUND VectorFloor = nullptr;
				PFN_ROUND VectorCeil = nullptr;
				PFN_PLANE_INIT PlaneInit = nullptr;
				PFN_PLANE_SDISTANCE PlaneSignedDistance = nullptr;
				PFN_MATRIX_FRUSTRUM MatrixFrustrum = nullptr;
				PFN_VECTOR_DOT VectorDot = nullptr;
				PFN_VECTOR_LENGTH VectorLength = nullptr;
				PFN_VECTOR_NORMALIZE VectorNormalize = nullptr;
				PFN_VECTOR_BARYCENTRIC VectorBarycentric = nullptr;
				PFN_MATRIX_MUL MatrixMul = nullptr;
				PFN_DQ_SLERP DQSLerp = nullptr;
				PFN_DQ_NORMALIZE DQNormalize = nullptr;
				PFN_DQ_MATRIX DQMatrix = nullptr;
				PFN_Q_SLERP QSLerp = nullptr;
				PFN_Q_VROT QVRot = nullptr;
				PFN_Q_ROTATION1 QRotation1 = nullptr;
				PFN_Q_ROTATION2 QRotation2 = nullptr;
				PFN_Q_LENGTH QLength = nullptr;
				PFN_Q_NORMALIZE QNormalize = nullptr;
				PFN_Q_INVERSE QInverse = nullptr;
				PFN_Q_MATRIX QMatrix = nullptr;
#endif

				CLOAK_FORCEINLINE void CLOAK_CALL AdjustWorldPos(Inout Vector* pos, Inout IntVector* node)
				{
					__m128 n = _mm_div_ps(_mm_add_ps(pos->Data, WORLD_NODE_HALF_SIZE), WORLD_NODE_SIZE);
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					n = VectorFloor(n);
#else
					n = _mm_floor_ps(n);
#endif
					pos->Data = _mm_sub_ps(pos->Data, _mm_mul_ps(n, WORLD_NODE_HALF_SIZE));
					node->Data = _mm_add_epi32(node->Data, _mm_cvtps_epi32(n));
				}

				namespace Noise {
					namespace Perlin {
						inline float CLOAK_CALL fade(In float x) { return ((((6 * x) - 15)*x) + 10)*x*x*x; }
						inline float CLOAK_CALL grad(In uint8_t hash, In float x, In float y, In float z)
						{
							switch (hash & 0xF)
							{
								case 0x00: return  x + y;
								case 0x01: return -x + y;
								case 0x02: return  x - y;
								case 0x03: return -x - y;
								case 0x04: return  x + z;
								case 0x05: return -x + z;
								case 0x06: return  x - z;
								case 0x07: return -x - z;
								case 0x08: return  y + z;
								case 0x09: return -y + z;
								case 0x0A: return  y - z;
								case 0x0B: return -y - z;
								case 0x0C: return  y + x;
								case 0x0D: return -y + z;
								case 0x0E: return  y - x;
								case 0x0F: return -y - z;
								default: return 0;
							}
						}
						inline float CLOAK_CALL lerp(In float a, In float b, In float c) { return a + (c * (b - a)); }
						inline void CLOAK_CALL generatePermutation(In std::default_random_engine& e, Out uint8_t P[256])
						{
							//Generate random permutation array:
							API::List<uint8_t> p;
							p.reserve(256);
							for (size_t a = 0; a < 256; a++) { p.push_back(static_cast<uint8_t>(a)); }
							std::shuffle(p.begin(), p.end(), e);
							for (size_t a = 0; a < 256; a++) { P[a] = p[a]; }
						}
						inline float CLOAK_CALL noise(In float x, In float y, In float z, In const uint8_t P[256])
						{
							//Initialize values:
							float nx, ny, nz;
							const float xf = std::modf(x, &nx);
							const float yf = std::modf(y, &ny);
							const float zf = std::modf(z, &nz);
							const uint8_t xi = static_cast<uint8_t>(static_cast<uint32_t>(nx) & 0xFF);
							const uint8_t yi = static_cast<uint8_t>(static_cast<uint32_t>(ny) & 0xFF);
							const uint8_t zi = static_cast<uint8_t>(static_cast<uint32_t>(nz) & 0xFF);
							const float u = fade(xf);
							const float v = fade(yf);
							const float w = fade(zf);

							//Hash:
							const uint8_t aaa = P[P[P[xi + 0] + yi + 0] + zi + 0];
							const uint8_t aab = P[P[P[xi + 0] + yi + 0] + zi + 1];
							const uint8_t aba = P[P[P[xi + 0] + yi + 1] + zi + 0];
							const uint8_t abb = P[P[P[xi + 0] + yi + 1] + zi + 1];
							const uint8_t baa = P[P[P[xi + 1] + yi + 0] + zi + 0];
							const uint8_t bab = P[P[P[xi + 1] + yi + 0] + zi + 1];
							const uint8_t bba = P[P[P[xi + 1] + yi + 1] + zi + 0];
							const uint8_t bbb = P[P[P[xi + 1] + yi + 1] + zi + 1];

							//Lerp:
							float x1 = lerp(grad(aaa, xf, yf, zf), grad(baa, xf - 1, yf, zf), u);
							float x2 = lerp(grad(aba, xf, yf - 1, zf), grad(bba, xf - 1, yf - 1, zf), u);
							float y1 = lerp(x1, x2, v);
							x1 = lerp(grad(aab, xf, yf, zf - 1), grad(bab, xf - 1, yf, zf - 1), u);
							x2 = lerp(grad(abb, xf, yf - 1, zf - 1), grad(bbb, xf - 1, yf - 1, zf - 1), u);
							float y2 = lerp(x1, x2, v);
							return 0.5f*(lerp(y1, y2, w) + 1);
						}
					}
					namespace Prime {
						constexpr uint32_t PRIMES[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53 };
						inline float CLOAK_CALL func(In float x)
						{
							float n;
							float y = std::modf(x, &n);
							return (((y - 2) * y) + 1) * 16 * y * y;
						}
						inline void CLOAK_CALL generateWeights(In std::default_random_engine& e, Out float W[ARRAYSIZE(PRIMES)])
						{
							std::uniform_real_distribution<float> d(0.0001f, 1.0f);
							float m = 0;
							for (size_t a = 0; a < ARRAYSIZE(PRIMES); a++) 
							{ 
								W[a] = d(e)*(0.5f + (static_cast<float>(a + 1) / (ARRAYSIZE(PRIMES) << 1)));
								m += W[a];
							}
							for (size_t a = 0; a < ARRAYSIZE(PRIMES); a++)
							{
								W[a] /= m;
							}
						}
						inline float CLOAK_CALL noise(In float x, In const float W[ARRAYSIZE(PRIMES)])
						{
							float r = 0;
							for (size_t a = 0; a < ARRAYSIZE(PRIMES); a++)
							{
								const float b = std::abs(x + a) / PRIMES[a];
								const float c = W[a] * (func((a & 0x1) == 0 ? b : b + 0.5f));
								r += c;
							}
							return (2 * r) - 1;
						}
					}
				}

				CLOAK_CALL PerlinNoise::PerlinNoise()
				{
					std::random_device r;
					std::default_random_engine e(r());
					Noise::Perlin::generatePermutation(e, m_P);
				}
				CLOAK_CALL PerlinNoise::PerlinNoise(In const std::seed_seq& seed)
				{
					std::default_random_engine e(seed);
					Noise::Perlin::generatePermutation(e, m_P);
				}
				CLOAK_CALL PerlinNoise::PerlinNoise(In const unsigned long long seed)
				{
					std::default_random_engine e(static_cast<unsigned int>(seed));
					Noise::Perlin::generatePermutation(e, m_P);
				}
				float CLOAK_CALL_THIS PerlinNoise::Generate(In float x, In_opt float y, In_opt float z)
				{
					return Noise::Perlin::noise(x, y, z, m_P);
				}
				float CLOAK_CALL_THIS PerlinNoise::operator()(In float x, In_opt float y, In_opt float z)
				{
					return Noise::Perlin::noise(x, y, z, m_P);
				}

				CLOAK_CALL PrimeNoise::PrimeNoise()
				{
					std::random_device r;
					std::default_random_engine e(r());
					Noise::Prime::generateWeights(e, &m_W[0]);
					Noise::Prime::generateWeights(e, &m_W[16]);
					Noise::Prime::generateWeights(e, &m_W[32]);
				}
				CLOAK_CALL PrimeNoise::PrimeNoise(In const std::seed_seq& seed)
				{
					std::default_random_engine e(seed);
					Noise::Prime::generateWeights(e, &m_W[0]);
					Noise::Prime::generateWeights(e, &m_W[16]);
					Noise::Prime::generateWeights(e, &m_W[32]);
				}
				CLOAK_CALL PrimeNoise::PrimeNoise(In const unsigned long long seed)
				{
					std::default_random_engine e(static_cast<unsigned int>(seed));
					Noise::Prime::generateWeights(e, &m_W[0]);
					Noise::Prime::generateWeights(e, &m_W[16]);
					Noise::Prime::generateWeights(e, &m_W[32]);
				}
				float CLOAK_CALL_THIS PrimeNoise::Generate(In float x)
				{
					return Noise::Prime::noise(x, &m_W[0]);
				}
				float CLOAK_CALL_THIS PrimeNoise::operator()(In float x)
				{
					return Noise::Prime::noise(x, &m_W[0]);
				}
				float CLOAK_CALL_THIS PrimeNoise::Generate(In float x, In float y)
				{
					return 0.5f*(Noise::Prime::noise(x, &m_W[0]) + Noise::Prime::noise(y, &m_W[16]));
				}
				float CLOAK_CALL_THIS PrimeNoise::operator()(In float x, In float y)
				{
					return 0.5f*(Noise::Prime::noise(x, &m_W[0]) + Noise::Prime::noise(y, &m_W[16]));
				}
				float CLOAK_CALL_THIS PrimeNoise::Generate(In float x, In float y, In float z)
				{
					return (Noise::Prime::noise(x, &m_W[0]) + Noise::Prime::noise(y, &m_W[16]) + Noise::Prime::noise(z, &m_W[32])) / 3;
				}
				float CLOAK_CALL_THIS PrimeNoise::operator()(In float x, In float y, In float z)
				{
					return (Noise::Prime::noise(x, &m_W[0]) + Noise::Prime::noise(y, &m_W[16]) + Noise::Prime::noise(z, &m_W[32])) / 3;
				}

				CLOAKENGINE_API CLOAK_CALL Plane::Plane()
				{
					Data = ALL_ZERO;
				}
				CLOAKENGINE_API CLOAK_CALL Plane::Plane(In const __m128& d)
				{
					Data = d;
				}
				CLOAKENGINE_API CLOAK_CALL Plane::Plane(In const Plane& p)
				{
					Data = p.Data;
				}
				CLOAKENGINE_API CLOAK_CALL Plane::Plane(In const Vector& normal, In float d)
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					Data = PlaneInit(normal.Data, d);
#else
					__m128 n = _mm_mul_ps(normal.Data, _mm_rsqrt_ps(_mm_dp_ps(normal.Data, normal.Data, 0x7F)));
					Data = _mm_or_ps(_mm_and_ps(n, FLAG_SET_SSSN), _mm_set_ps(d, 0, 0, 0));
#endif
				}
				CLOAKENGINE_API CLOAK_CALL Plane::Plane(In const Vector& normal, In const Vector& point) : Plane(normal, normal.Dot(point))
				{

				}
				CLOAKENGINE_API CLOAK_CALL Plane::Plane(In const Vector& p1, In const Vector& p2, In const Vector& p3)
				{
					const Vector n = (p2 - p1).Cross(p3 - p1).Normalize();
					const float d = n.Dot(p1);
					Data = _mm_or_ps(_mm_and_ps(n.Data, FLAG_SET_SSSN), _mm_set_ps(d, 0, 0, 0));
				}
				CLOAKENGINE_API Plane& CLOAK_CALL_THIS Plane::operator=(In const Plane& p)
				{
					Data = p.Data;
					return *this;
				}
				CLOAKENGINE_API float CLOAK_CALL_THIS Plane::SignedDistance(In const Vector& point) const
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return PlaneSignedDistance(Data, point.Data);
#else
					__m128 p = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(3, 3, 3, 3));
					return _mm_cvtss_f32(_mm_sub_ps(_mm_dp_ps(Data, point.Data, 0x7F), p));
#endif
				}
				CLOAKENGINE_API Vector CLOAK_CALL_THIS Plane::GetNormal() const
				{
					return Vector(_mm_and_ps(Data, FLAG_SET_SSSN));
				}
				CLOAKENGINE_API void* CLOAK_CALL Plane::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void* CLOAK_CALL Plane::operator new[](In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void CLOAK_CALL Plane::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
				CLOAKENGINE_API void CLOAK_CALL Plane::operator delete[](In void* s) { return API::Global::Memory::MemoryPool::Free(s); }

				CLOAKENGINE_API CLOAK_CALL Frustum::Frustum()
				{

				}
				CLOAKENGINE_API CLOAK_CALL Frustum::Frustum(In const Matrix& m)
				{
					operator=(m);
				}
				CLOAKENGINE_API CLOAK_CALL Frustum::Frustum(In const Frustum& f)
				{
					operator=(f);
				}
				CLOAKENGINE_API Frustum& CLOAK_CALL_THIS Frustum::operator=(In const Matrix& m)
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					__m128 d[6];
					MatrixFrustrum(d, m.Row);
					for (size_t a = 0; a < 6; a++) { Data[a].Data = d[a]; }
#else
					__m128 r[4];
					for (size_t a = 0; a < 4; a++) { r[a] = m.Row[a]; }
					_MM_TRANSPOSE4_PS(r[0], r[1], r[2], r[3]);
					Data[0] = _mm_add_ps(r[3], r[0]);
					Data[1] = _mm_sub_ps(r[3], r[0]);
					Data[2] = _mm_sub_ps(r[3], r[1]);
					Data[3] = _mm_add_ps(r[3], r[1]);
					Data[4] = _mm_sub_ps(r[3], r[2]);
					Data[5] = r[2];

					for (size_t a = 0; a < 6; a++)
					{
						Data[a].Data = _mm_xor_ps(_mm_mul_ps(Data[a].Data, _mm_rsqrt_ps(_mm_dp_ps(Data[a].Data, Data[a].Data, 0x7F))), FLAG_SIGN_NNNP);
					}
#endif
					return *this;
				}
				CLOAKENGINE_API Frustum& CLOAK_CALL_THIS Frustum::operator=(In const Frustum& f)
				{
					for (size_t a = 0; a < 6; a++) { Data[a] = f.Data[a]; }
					return *this;
				}
				CLOAKENGINE_API const Plane& CLOAK_CALL_THIS Frustum::operator[](In size_t p) const
				{
					assert(p < 6);
					return Data[p];
				}
				CLOAKENGINE_API void* CLOAK_CALL Frustum::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void* CLOAK_CALL Frustum::operator new[](In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void CLOAK_CALL Frustum::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
				CLOAKENGINE_API void CLOAK_CALL Frustum::operator delete[](In void* s) { return API::Global::Memory::MemoryPool::Free(s); }

				CLOAKENGINE_API CLOAK_CALL OctantFrustum::OctantFrustum()
				{
					CenterDistance = 0;
				}
				CLOAKENGINE_API CLOAK_CALL OctantFrustum::OctantFrustum(In const Matrix& m) : Base(m)
				{
					operator=(m);
				}
				CLOAKENGINE_API CLOAK_CALL OctantFrustum::OctantFrustum(In const Frustum& m) : Base(m)
				{
					operator=(m);
				}
				CLOAKENGINE_API CLOAK_CALL OctantFrustum::OctantFrustum(In const OctantFrustum& o) : Base(o.Base)
				{
					for (size_t a = 0; a < 3; a++) { Octants[a] = o.Octants[a]; }
					CenterDistance = o.CenterDistance;
				}

				CLOAKENGINE_API OctantFrustum& CLOAK_CALL_THIS OctantFrustum::operator=(In const Matrix& m)
				{
					return operator=(static_cast<Frustum>(m));
				}
				CLOAKENGINE_API OctantFrustum& CLOAK_CALL_THIS OctantFrustum::operator=(In const Frustum& m)
				{
					Base = m;
					//r0-r2 are the (denormalized) half-way-planes (Octant planes)
					__m128 r0 = _mm_sub_ps(Base[0].Data, Base[1].Data); //Oriented to left
					__m128 r1 = _mm_sub_ps(Base[2].Data, Base[3].Data); //Oriented to top
					__m128 r2 = _mm_sub_ps(Base[4].Data, Base[5].Data); //Oriented to near
					__m128 r3 = ROW_ZZZO;
					Vector v[3];
					v[0].Data = r0;
					v[1].Data = r1;
					v[2].Data = r2;
					Matrix cm;
					cm.Row[0] = r0;
					cm.Row[1] = r1;
					cm.Row[2] = r2;
					cm.Row[3] = r3;
					//Matrix multiplied by (0,0,0,1) results in center
					Vector center = _mm_xor_ps(cm.Inverse().Transpose().Row[3], FLAG_SIGN_NNNN);
					CenterDistance = 0;
					for (size_t a = 0; a < 6; a++)
					{
						const float d = fabsf(Base[a].SignedDistance(center));
						if (a == 0 || CenterDistance > d) { CenterDistance = d; }
					}
					Octants[0] = Plane(v[0].Normalize(), center);
					Octants[1] = Plane(v[1].Normalize(), center);
					Octants[2] = Plane(v[2].Normalize(), center);
					return *this;
				}
				CLOAKENGINE_API OctantFrustum& CLOAK_CALL_THIS OctantFrustum::operator=(In const OctantFrustum& o)
				{
					Base = o.Base;
					for (size_t a = 0; a < 3; a++) { Octants[a] = o.Octants[a]; }
					CenterDistance = o.CenterDistance;
					return *this;
				}
				CLOAKENGINE_API size_t CLOAK_CALL_THIS OctantFrustum::LocateOctant(In const Vector& center) const
				{
					const bool lh = Octants[0].SignedDistance(center) >= 0;
					const bool th = Octants[1].SignedDistance(center) >= 0;
					const bool nh = Octants[2].SignedDistance(center) >= 0;
					return (nh ? 0 : 4) | (th ? 0 : 2) | (lh ? 0 : 1);
				}
				CLOAKENGINE_API void* CLOAK_CALL OctantFrustum::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void* CLOAK_CALL OctantFrustum::operator new[](In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void CLOAK_CALL OctantFrustum::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
				CLOAKENGINE_API void CLOAK_CALL OctantFrustum::operator delete[](In void* s) { return API::Global::Memory::MemoryPool::Free(s); }

				CLOAKENGINE_API CLOAK_CALL IntPoint::IntPoint() : IntPoint(0, 0, 0, 1) {}
				CLOAKENGINE_API CLOAK_CALL IntPoint::IntPoint(In int32_t x, In int32_t y, In int32_t z, In_opt int32_t w)
				{
					X = x;
					Y = y;
					Z = z;
					W = w;
				}
				CLOAKENGINE_API CLOAK_CALL IntPoint::IntPoint(In const IntPoint& p)
				{
					for (size_t a = 0; a < ARRAYSIZE(Data); a++) { Data[a] = p.Data[a]; }
				}
				CLOAKENGINE_API CLOAK_CALL IntPoint::IntPoint(In const __m128i& d)
				{
					_mm_store_ps(reinterpret_cast<float*>(Data), _mm_castsi128_ps(d));
					W = 1;
				}
				CLOAKENGINE_API const IntPoint CLOAK_CALL_MATH IntPoint::operator+(In const IntVector& v) const
				{
					return IntPoint(*this) += v;
				}
				CLOAKENGINE_API const IntPoint CLOAK_CALL_MATH IntPoint::operator-(In const IntVector& v) const
				{
					return IntPoint(*this) -= v;
				}
				CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH IntPoint::operator=(In const IntPoint& p)
				{
					for (size_t a = 0; a < ARRAYSIZE(Data); a++) { Data[a] = p.Data[a]; }
					return *this;
				}
				CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH IntPoint::operator+=(In const IntVector& v)
				{
					IntVector w = *this;
					w += v;
					_mm_store_ps(reinterpret_cast<float*>(Data), _mm_castsi128_ps(w.Data));
					W = 1;
					return *this;
				}
				CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH IntPoint::operator-=(In const IntVector& v)
				{
					IntVector w = *this;
					w -= v;
					_mm_store_ps(reinterpret_cast<float*>(Data), _mm_castsi128_ps(w.Data));
					W = 1;
					return *this;
				}
				CLOAKENGINE_API const Point CLOAK_CALL_MATH IntPoint::operator*(In float a) const
				{
					return Point(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z), static_cast<float>(W)) *= a;
				}
				CLOAKENGINE_API const IntPoint CLOAK_CALL_MATH IntPoint::operator*(In int32_t a) const
				{
					return IntPoint(*this) *= a;
				}
				CLOAKENGINE_API const IntPoint CLOAK_CALL_MATH IntPoint::operator*(In const IntVector& v) const
				{
					return IntPoint(*this) *= v;
				}
				CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH IntPoint::operator*=(In int32_t a)
				{
					X *= a;
					Y *= a;
					Z *= a;
					return *this;
				}
				CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH IntPoint::operator*=(In const IntVector& v)
				{
					IntVector w = *this;
					w *= v;
					_mm_store_ps(reinterpret_cast<float*>(Data), _mm_castsi128_ps(w.Data));
					return *this;
				}
				CLOAKENGINE_API const Point CLOAK_CALL_MATH IntPoint::operator/(In float a) const
				{
					return Point(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z), static_cast<float>(W)) /= a;
				}
				CLOAKENGINE_API const IntPoint CLOAK_CALL_MATH IntPoint::operator/(In int32_t a) const
				{
					return IntPoint(*this) /= a;
				}
				CLOAKENGINE_API const IntPoint CLOAK_CALL_MATH IntPoint::operator/(In const IntVector& v) const
				{
					return IntPoint(*this) /= v;
				}
				CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH IntPoint::operator/=(In int32_t a)
				{
					X /= a;
					Y /= a;
					Z /= a;
					return *this;
				}
				CLOAKENGINE_API IntPoint& CLOAK_CALL_MATH IntPoint::operator/=(In const IntVector& v)
				{
					IntVector w = *this;
					w /= v;
					_mm_store_ps(reinterpret_cast<float*>(Data), _mm_castsi128_ps(w.Data));
					return *this;
				}
				CLOAKENGINE_API int32_t& CLOAK_CALL_MATH IntPoint::operator[](In size_t a)
				{
					CLOAK_ASSUME(a < ARRAYSIZE(Data));
					return Data[a];
				}
				CLOAKENGINE_API const int32_t& CLOAK_CALL_MATH IntPoint::operator[](In size_t a) const
				{
					CLOAK_ASSUME(a < ARRAYSIZE(Data));
					return Data[a];
				}

				CLOAKENGINE_API void* CLOAK_CALL IntPoint::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void* CLOAK_CALL IntPoint::operator new[](In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void CLOAK_CALL IntPoint::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
				CLOAKENGINE_API void CLOAK_CALL IntPoint::operator delete[](In void* s) { return API::Global::Memory::MemoryPool::Free(s); }

				CLOAKENGINE_API CLOAK_CALL Point::Point() : Point(0, 0, 0, 1) {}
				CLOAKENGINE_API CLOAK_CALL Point::Point(In const __m128& d) { _mm_store_ps(Data, d); W = 1; }
				CLOAKENGINE_API CLOAK_CALL Point::Point(In float x, In float y, In float z, In_opt float w)
				{
					X = x;
					Y = y;
					Z = z;
					W = w;
				}
				CLOAKENGINE_API CLOAK_CALL Point::Point(In const Point& p)
				{
					X = p.X;
					Y = p.Y;
					Z = p.Z;
					W = p.W;
				}
				CLOAKENGINE_API const Point CLOAK_CALL_MATH Point::operator+(In const Vector& v) const
				{
					return Point(*this) += v;
				}
				CLOAKENGINE_API const Point CLOAK_CALL_MATH Point::operator-(In const Vector& v) const
				{
					return Point(*this) -= v;
				}
				CLOAKENGINE_API Point& CLOAK_CALL_MATH Point::operator=(In const Point& p)
				{
					for (size_t a = 0; a < 4; a++) { Data[a] = p.Data[a]; }
					return *this;
				}
				CLOAKENGINE_API Point& CLOAK_CALL_MATH Point::operator+=(In const Vector& v)
				{
					Vector w = *this;
					w += v;
					_mm_store_ps(Data, w.Data);
					W = 1;
					return *this;
				}
				CLOAKENGINE_API Point& CLOAK_CALL_MATH Point::operator-=(In const Vector& v)
				{
					Vector w = *this;
					w -= v;
					_mm_store_ps(Data, w.Data);
					W = 1;
					return *this;
				}
				CLOAKENGINE_API const Point CLOAK_CALL_MATH Point::operator*(In float a) const
				{
					return Point(*this) *= a;
				}
				CLOAKENGINE_API const Point CLOAK_CALL_MATH Point::operator*(In const Vector& v) const
				{
					return Point(*this) *= v;
				}
				CLOAKENGINE_API Point& CLOAK_CALL_MATH Point::operator*=(In float a)
				{
					X *= a;
					Y *= a;
					Z *= a;
					return *this;
				}
				CLOAKENGINE_API Point& CLOAK_CALL_MATH Point::operator*=(In const Vector& v)
				{
					Vector w = *this;
					w *= v;
					_mm_store_ps(Data, w.Data);
					W = 1;
					return *this;
				}
				CLOAKENGINE_API const Point CLOAK_CALL_MATH Point::operator/(In float a) const
				{
					return Point(*this) /= a;
				}
				CLOAKENGINE_API const Point CLOAK_CALL_MATH Point::operator/(In const Vector& v) const
				{
					return Point(*this) /= v;
				}
				CLOAKENGINE_API Point& CLOAK_CALL_MATH Point::operator/=(In float a)
				{
					X /= a;
					Y /= a;
					Z /= a;
					return *this;
				}
				CLOAKENGINE_API Point& CLOAK_CALL_MATH Point::operator/=(In const Vector& v)
				{
					Vector w = *this;
					w /= v;
					_mm_store_ps(Data, w.Data);
					W = 1;
					return *this;
				}
				CLOAKENGINE_API float& CLOAK_CALL_MATH Point::operator[](In size_t a)
				{
					assert(a < ARRAYSIZE(Data));
					return Data[a];
				}
				CLOAKENGINE_API const float& CLOAK_CALL_MATH Point::operator[](In size_t a) const
				{
					assert(a < ARRAYSIZE(Data));
					return Data[a];
				}
				CLOAKENGINE_API void* CLOAK_CALL Point::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void* CLOAK_CALL Point::operator new[](In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void CLOAK_CALL Point::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
				CLOAKENGINE_API void CLOAK_CALL Point::operator delete[](In void* s) { return API::Global::Memory::MemoryPool::Free(s); }

				CLOAKENGINE_API CLOAK_CALL EulerAngles::EulerAngles()
				{
					Roll = Pitch = Yaw = 0;
				}
				CLOAKENGINE_API CLOAK_CALL EulerAngles::EulerAngles(In float roll, In float pitch, In float yaw)
				{
					Roll = roll;
					Pitch = pitch;
					Yaw = yaw;
				}
				CLOAKENGINE_API CLOAK_CALL EulerAngles::EulerAngles(In const EulerAngles& e)
				{
					for (size_t a = 0; a < ARRAYSIZE(Data); a++) { Data[a] = e.Data[a]; }
				}
				CLOAKENGINE_API Quaternion CLOAK_CALL EulerAngles::ToQuaternion(In RotationOrder order) const { return Quaternion::Rotation(Roll, Pitch, Yaw, order); }
				CLOAKENGINE_API Matrix CLOAK_CALL EulerAngles::ToMatrix(In RotationOrder order) const { return Matrix::Rotation(Roll, Pitch, Yaw, order); }
				CLOAKENGINE_API Vector CLOAK_CALL EulerAngles::ToAngularVelocity() const { return Vector(Roll, Pitch, Yaw); }
				CLOAKENGINE_API float& CLOAK_CALL_MATH EulerAngles::operator[](In size_t a)
				{
					assert(a < ARRAYSIZE(Data));
					return Data[a];
				}
				CLOAKENGINE_API const float& CLOAK_CALL_MATH EulerAngles::operator[](In size_t a) const
				{
					assert(a < ARRAYSIZE(Data));
					return Data[a];
				}

				CLOAKENGINE_API void* CLOAK_CALL EulerAngles::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void* CLOAK_CALL EulerAngles::operator new[](In size_t s) {return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void CLOAK_CALL EulerAngles::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
				CLOAKENGINE_API void CLOAK_CALL EulerAngles::operator delete[](In void* s) {return API::Global::Memory::MemoryPool::Free(s); }

				CLOAKENGINE_API Quaternion CLOAK_CALL EulerAngles::ToQuaternion(In const EulerAngles& e, In RotationOrder order) { return e.ToQuaternion(order); }
				CLOAKENGINE_API Matrix CLOAK_CALL EulerAngles::ToMatrix(In const EulerAngles& e, In RotationOrder order) { return e.ToMatrix(order); }

				CLOAKENGINE_API CLOAK_CALL IntVector::IntVector() : Data(_mm_set1_epi32(0)) {}
				CLOAKENGINE_API CLOAK_CALL IntVector::IntVector(In int32_t X, In int32_t Y, In int32_t Z) : Data(_mm_set_epi32(0, Z, Y, X)) {}
				CLOAKENGINE_API CLOAK_CALL IntVector::IntVector(In int32_t X, In int32_t Y, In int32_t Z, In int32_t W) : Data(_mm_set_epi32(W, Z, Y, X)) {}
				CLOAKENGINE_API CLOAK_CALL IntVector::IntVector(In const IntPoint& p) : Data(_mm_set_epi32(p.W, p.Z, p.Y, p.X)) {}
				CLOAKENGINE_API CLOAK_CALL IntVector::IntVector(In const IntVector& v) : Data(v.Data) {}
				CLOAKENGINE_API CLOAK_CALL IntVector::IntVector(In const __m128i& d) : Data(d) {}
				CLOAKENGINE_API CLOAK_CALL_MATH IntVector::operator IntPoint() const
				{
					IntPoint r;
					_mm_store_ps(reinterpret_cast<float*>(r.Data), _mm_castsi128_ps(Data));
					return r;
				}
				CLOAKENGINE_API CLOAK_CALL_MATH IntVector::operator Point() const
				{
					return Point(_mm_cvtepi32_ps(Data));
				}
				CLOAKENGINE_API CLOAK_CALL_MATH IntVector::operator Quaternion() const
				{
					return Quaternion(_mm_and_ps(_mm_cvtepi32_ps(Data), FLAG_SET_SSSN));
				}
				CLOAKENGINE_API const IntVector CLOAK_CALL_MATH IntVector::operator+(In const IntVector& v) const
				{
					return IntVector(*this) += v;
				}
				CLOAKENGINE_API const IntVector CLOAK_CALL_MATH IntVector::operator-(In const IntVector& v) const
				{
					return IntVector(*this) -= v;
				}
				CLOAKENGINE_API const Vector CLOAK_CALL_MATH IntVector::operator+(In const Vector& v) const
				{
					return Vector(_mm_add_ps(_mm_cvtepi32_ps(Data), v.Data));
				}
				CLOAKENGINE_API const Vector CLOAK_CALL_MATH IntVector::operator-(In const Vector& v) const
				{
					return Vector(_mm_sub_ps(_mm_cvtepi32_ps(Data), v.Data));
				}
				CLOAKENGINE_API const IntVector CLOAK_CALL_MATH IntVector::operator*(In const IntVector& v) const
				{
					return IntVector(*this) *= v;
				}
				CLOAKENGINE_API const IntVector CLOAK_CALL_MATH IntVector::operator*(In int32_t v) const
				{
					return IntVector(*this) *= v;
				}
				CLOAKENGINE_API const Vector CLOAK_CALL_MATH IntVector::operator*(In const Vector& v) const
				{
					return Vector(_mm_mul_ps(_mm_cvtepi32_ps(Data), v.Data));
				}
				CLOAKENGINE_API const Vector CLOAK_CALL_MATH IntVector::operator*(In float v) const
				{
					return Vector(_mm_mul_ps(_mm_cvtepi32_ps(Data), _mm_set1_ps(v)));
				}
				CLOAKENGINE_API const IntVector CLOAK_CALL_MATH IntVector::operator/(In const IntVector& v) const
				{
					return IntVector(*this) /= v;
				}
				CLOAKENGINE_API const IntVector CLOAK_CALL_MATH IntVector::operator/(In int32_t v) const
				{
					return IntVector(*this) /= v;
				}
				CLOAKENGINE_API const Vector CLOAK_CALL_MATH IntVector::operator/(In const Vector& v) const
				{
					return Vector(_mm_div_ps(_mm_cvtepi32_ps(Data), v.Data));
				}
				CLOAKENGINE_API const Vector CLOAK_CALL_MATH IntVector::operator/(In float v) const
				{
					return Vector(_mm_div_ps(_mm_cvtepi32_ps(Data), _mm_set1_ps(v)));
				}
				CLOAKENGINE_API const IntVector CLOAK_CALL_MATH IntVector::operator-() const
				{
					return IntVector(_mm_sub_epi32(ALL_ZEROi, Data));
				}
				CLOAKENGINE_API IntVector& CLOAK_CALL_MATH IntVector::operator=(In const IntVector& v)
				{
					Data = v.Data;
					return *this;
				}
				CLOAKENGINE_API IntVector& CLOAK_CALL_MATH IntVector::operator=(In const IntPoint& v)
				{
					Data = _mm_set_epi32(v.W, v.Z, v.Y, v.X);
					return *this;
				}
				CLOAKENGINE_API IntVector& CLOAK_CALL_MATH IntVector::operator+=(In const IntVector& v)
				{
					Data = _mm_add_epi32(Data, v.Data);
					return *this;
				}
				CLOAKENGINE_API IntVector& CLOAK_CALL_MATH IntVector::operator-=(In const IntVector& v)
				{
					Data = _mm_sub_epi32(Data, v.Data);
					return *this;
				}
				CLOAKENGINE_API IntVector& CLOAK_CALL_MATH IntVector::operator*=(In const IntVector& v)
				{
					Data = _mm_mul_epi32(Data, v.Data);
					return *this;
				}
				CLOAKENGINE_API IntVector& CLOAK_CALL_MATH IntVector::operator*=(In int32_t v)
				{
					Data = _mm_mul_epi32(Data, _mm_set1_epi32(v));
					return *this;
				}
				CLOAKENGINE_API IntVector& CLOAK_CALL_MATH IntVector::operator/=(In const IntVector& v)
				{
					Data = _mm_cvtps_epi32(_mm_div_ps(_mm_cvtepi32_ps(Data), _mm_cvtepi32_ps(v.Data)));
					return *this;
				}
				CLOAKENGINE_API IntVector& CLOAK_CALL_MATH IntVector::operator/=(In int32_t v)
				{
					Data = _mm_cvtps_epi32(_mm_div_ps(_mm_cvtepi32_ps(Data), _mm_set1_ps(static_cast<float>(v))));
					return *this;
				}
				CLOAKENGINE_API bool CLOAK_CALL_MATH IntVector::operator==(In const IntVector& v) const
				{
					__m128i c = _mm_cmpeq_epi32(Data, v.Data);
					CLOAK_ALIGN float r[4];
					_mm_store_ps(r, _mm_castsi128_ps(c));
					uint32_t* i = reinterpret_cast<uint32_t*>(r);
					return (i[0] & i[1] & i[2]) != 0;
				}
				CLOAKENGINE_API bool CLOAK_CALL_MATH IntVector::operator!=(In const IntVector& v) const
				{
					return !operator==(v);
				}

				CLOAKENGINE_API float CLOAK_CALL_MATH IntVector::Length() const
				{
					__m128i d = _mm_mul_epi32(Data, Data);
					d = _mm_hadd_epi32(d, d);
					d = _mm_hadd_epi32(d, d);
					int32_t i = _mm_cvtsi128_si32(d);
					return sqrt(static_cast<float>(i));
				}
				CLOAKENGINE_API int32_t CLOAK_CALL_MATH IntVector::Dot(In const IntVector& v) const
				{
					__m128i d = _mm_mul_epi32(Data, v.Data);
					d = _mm_hadd_epi32(d, d);
					d = _mm_hadd_epi32(d, d);
					return _mm_cvtsi128_si32(d);
				}
				CLOAKENGINE_API IntVector CLOAK_CALL_MATH IntVector::Cross(In const IntVector& v) const
				{
					const __m128i s1 = _mm_shuffle_epi32(Data, Data, _MM_SHUFFLE(3, 0, 2, 1));
					const __m128i s2 = _mm_shuffle_epi32(v.Data, v.Data, _MM_SHUFFLE(3, 1, 0, 2));
					const __m128i s3 = _mm_shuffle_epi32(Data, Data, _MM_SHUFFLE(3, 1, 0, 2));
					const __m128i s4 = _mm_shuffle_epi32(v.Data, v.Data, _MM_SHUFFLE(3, 0, 2, 1));
					const __m128i d = _mm_sub_epi32(_mm_mul_epi32(s1, s2), _mm_mul_epi32(s3, s4));
					return IntVector(d);
				}
				CLOAKENGINE_API IntVector& CLOAK_CALL_MATH IntVector::Set(In int32_t X, In int32_t Y, In int32_t Z)
				{
					Data = _mm_set_epi32(0, Z, Y, X);
					return *this;
				}

				CLOAKENGINE_API float CLOAK_CALL_MATH IntVector::Length(In const IntVector& v)
				{
					return v.Length();
				}
				CLOAKENGINE_API int32_t CLOAK_CALL_MATH IntVector::Dot(In const IntVector& a, In const IntVector& b)
				{
					return a.Dot(b);
				}
				CLOAKENGINE_API IntVector CLOAK_CALL_MATH IntVector::Cross(In const IntVector& a, In const IntVector& b)
				{
					return a.Cross(b);
				}
				CLOAKENGINE_API IntVector CLOAK_CALL_MATH IntVector::Min(In const IntVector& a, In const IntVector& b)
				{
					return IntVector(_mm_min_epi32(a.Data, b.Data));
				}
				CLOAKENGINE_API IntVector CLOAK_CALL_MATH IntVector::Max(In const IntVector& a, In const IntVector& b)
				{
					return IntVector(_mm_max_epi32(a.Data, b.Data));
				}
				CLOAKENGINE_API IntVector CLOAK_CALL_MATH IntVector::Clamp(In const IntVector& a, In const IntVector& min, In const IntVector& max)
				{
					return IntVector(_mm_min_epi32(_mm_max_epi32(a.Data, min.Data), max.Data));
				}

				CLOAKENGINE_API void* CLOAK_CALL IntVector::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void* CLOAK_CALL IntVector::operator new[](In size_t s) {return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void CLOAK_CALL IntVector::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
				CLOAKENGINE_API void CLOAK_CALL IntVector::operator delete[](In void* s) {return API::Global::Memory::MemoryPool::Free(s); }

				CLOAKENGINE_API CLOAK_CALL Vector::Vector() : Vector(0, 0, 0) {}
				CLOAKENGINE_API CLOAK_CALL Vector::Vector(In float X, In float Y, In float Z)
				{
					Data = _mm_set_ps(0, Z, Y, X);
				}
				CLOAKENGINE_API CLOAK_CALL Vector::Vector(In float X, In float Y, In float Z, In float W)
				{
					Data = _mm_mul_ps(_mm_set_ps(0, Z, Y, X), _mm_set_ps1(1 / W));
				}
				CLOAKENGINE_API CLOAK_CALL Vector::Vector(In const Point& p) : Vector(p.X, p.Y, p.Z, p.W) {}
				CLOAKENGINE_API CLOAK_CALL Vector::Vector(In const Vector& v)
				{
					Data = v.Data;
				}
				CLOAKENGINE_API CLOAK_CALL Vector::Vector(In const __m128& d) : Data(d) {}
				CLOAKENGINE_API CLOAK_CALL Vector::Vector(In const IntVector& v) : Data(_mm_cvtepi32_ps(v.Data)) {}
				CLOAKENGINE_API CLOAK_CALL_MATH Vector::operator Point() const
				{
					Point r;
					_mm_store_ps(r.Data, Data);
					r.W = 1;
					return r;
				}
				CLOAKENGINE_API CLOAK_CALL_MATH Vector::operator Quaternion() const
				{
					return Quaternion(_mm_and_ps(Data, FLAG_SET_SSSN));
				}
				CLOAKENGINE_API CLOAK_CALL_MATH Vector::operator IntVector() const
				{
					return IntVector(_mm_cvtps_epi32(Data));
				}
				CLOAKENGINE_API CLOAK_CALL_MATH Vector::operator IntPoint() const
				{
					IntPoint r;
					_mm_store_ps(reinterpret_cast<float*>(r.Data), _mm_castsi128_ps(_mm_cvtps_epi32(Data)));
					return r;
				}
				CLOAKENGINE_API const Vector CLOAK_CALL_MATH Vector::operator+(In const Vector& v) const
				{
					return Vector(*this) += v;
				}
				CLOAKENGINE_API const Vector CLOAK_CALL_MATH Vector::operator-(In const Vector& v) const
				{
					return Vector(*this) -= v;
				}
				CLOAKENGINE_API Vector& CLOAK_CALL_MATH Vector::operator=(In const Vector& v)
				{
					Data = v.Data;
					return *this;
				}
				CLOAKENGINE_API Vector& CLOAK_CALL_MATH Vector::operator=(In const Point& p)
				{
					Data = _mm_mul_ps(_mm_set_ps(0, p.Z, p.Y, p.X), _mm_set_ps1(1 / p.W));
					return *this;
				}
				CLOAKENGINE_API Vector& CLOAK_CALL_MATH Vector::operator=(In const IntVector& v)
				{
					Data = _mm_cvtepi32_ps(v.Data);
					return *this;
				}
				CLOAKENGINE_API Vector& CLOAK_CALL_MATH Vector::operator+=(In const Vector& v)
				{
					Data = _mm_add_ps(Data, v.Data);
					return *this;
				}
				CLOAKENGINE_API Vector& CLOAK_CALL_MATH Vector::operator-=(In const Vector& v)
				{
					Data = _mm_sub_ps(Data, v.Data);
					return *this;
				}
				CLOAKENGINE_API const Vector CLOAK_CALL_MATH Vector::operator*(In const Vector& v) const
				{
					return Vector(*this) *= v;
				}
				CLOAKENGINE_API Vector& CLOAK_CALL_MATH Vector::operator*=(In const Vector& v)
				{
					Data = _mm_mul_ps(Data, v.Data);
					return *this;
				}
				CLOAKENGINE_API const Vector CLOAK_CALL_MATH Vector::operator*(In float f) const
				{
					return Vector(*this) *= f;
				}
				CLOAKENGINE_API Vector& CLOAK_CALL_MATH Vector::operator*=(In float f)
				{
					Data = _mm_mul_ps(Data, _mm_set_ps1(f));
					return *this;
				}
				CLOAKENGINE_API const Vector CLOAK_CALL_MATH Vector::operator/(In const Vector& v) const
				{
					return Vector(*this) /= v;
				}
				CLOAKENGINE_API Vector& CLOAK_CALL_MATH Vector::operator/=(In const Vector& v)
				{
					Data = _mm_div_ps(Data, v.Data);
					return *this;
				}
				CLOAKENGINE_API const Vector CLOAK_CALL_MATH Vector::operator/(In float f) const
				{
					return Vector(*this) /= f;
				}
				CLOAKENGINE_API Vector& CLOAK_CALL_MATH Vector::operator/=(In float f)
				{
					Data = _mm_mul_ps(Data, _mm_set_ps1(1 / f));
					return *this;
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::operator*(In const Matrix& m) const
				{
					return Vector(*this) *= m;
				}
				CLOAKENGINE_API Vector& CLOAK_CALL_MATH Vector::operator*=(In const Matrix& m)
				{
					__m128 o = _mm_mul_ps(_mm_shuffle_ps(Data, Data, _MM_SHUFFLE(0, 0, 0, 0)), m.Row[0]);
					o = _mm_add_ps(o, _mm_mul_ps(_mm_shuffle_ps(Data, Data, _MM_SHUFFLE(1, 1, 1, 1)), m.Row[1]));
					o = _mm_add_ps(o, _mm_mul_ps(_mm_shuffle_ps(Data, Data, _MM_SHUFFLE(2, 2, 2, 2)), m.Row[2]));
					o = _mm_add_ps(o, m.Row[3]);
					Data = _mm_div_ps(o, _mm_shuffle_ps(o, o, _MM_SHUFFLE(3, 3, 3, 3)));
					return *this;
				}
				CLOAKENGINE_API const Vector CLOAK_CALL_MATH Vector::operator-() const
				{
					return Vector(_mm_sub_ps(ALL_ZERO, Data));
				}
				CLOAKENGINE_API bool CLOAK_CALL_MATH Vector::operator==(In const Vector& v) const
				{
					__m128 c = _mm_and_ps(_mm_cmpge_ps(_mm_add_ps(Data, ALL_EPS), v.Data), _mm_cmple_ps(_mm_sub_ps(Data, ALL_EPS), v.Data));
					CLOAK_ALIGN float r[4];
					_mm_store_ps(r, c);
					uint32_t* i = reinterpret_cast<uint32_t*>(r);
					return (i[0] & i[1] & i[2]) != 0;
				}
				CLOAKENGINE_API bool CLOAK_CALL_MATH Vector::operator!=(In const Vector& v) const
				{
					return !operator==(v);
				}

				CLOAKENGINE_API float CLOAK_CALL_MATH Vector::Dot(In const Vector& v) const
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return VectorDot(Data, v.Data);
#else
					return _mm_cvtss_f32(_mm_dp_ps(Data, v.Data, 0x7F));
#endif
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::Cross(In const Vector& v) const
				{
					const __m128 s1 = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(3, 0, 2, 1));
					const __m128 s2 = _mm_shuffle_ps(v.Data, v.Data, _MM_SHUFFLE(3, 1, 0, 2));
					const __m128 s3 = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(3, 1, 0, 2));
					const __m128 s4 = _mm_shuffle_ps(v.Data, v.Data, _MM_SHUFFLE(3, 0, 2, 1));
					const __m128 d = _mm_sub_ps(_mm_mul_ps(s1, s2), _mm_mul_ps(s3, s4));
					return Vector(d);
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::Normalize() const
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return Vector(VectorNormalize(Data));
#else
					__m128 d = _mm_mul_ps(Data, _mm_rsqrt_ps(_mm_dp_ps(Data, Data, 0x7F)));
					return Vector(d);
#endif
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::Abs() const
				{
					return Vector(_mm_and_ps(Data, FLAG_ABS_SSSS));
				}
				CLOAKENGINE_API Vector& CLOAK_CALL_MATH Vector::Set(In float X, In float Y, In float Z)
				{
					Data = _mm_set_ps(0, Z, Y, X);
					return *this;
				}
				CLOAKENGINE_API Vector& CLOAK_CALL_THIS Vector::ProjectOnPlane(In const Plane& p)
				{
					Vector n = p.GetNormal();
					return *this -= n.Dot(*this) * n;
				}
				CLOAKENGINE_API std::pair<Vector, Vector> CLOAK_CALL_MATH Vector::GetOrthographicBasis() const
				{
					//Choose best of right, top and forward
					__m128 a = _mm_and_ps(Data, FLAG_ABS_SSSS);
					__m128 min = _mm_min_ps(a, _mm_min_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1)), _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 2, 2, 2))));
					a = _mm_cmple_ps(a, _mm_shuffle_ps(min, min, _MM_SHUFFLE(0, 0, 0, 0)));
					__m128 b = _mm_and_ps(a, ROW_OOOZ);

					//Calculate c = Data x b, then calculate d = Data x c
					__m128 s1 = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(3, 0, 2, 1));
					__m128 s2 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2));
					__m128 s3 = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(3, 1, 0, 2));
					__m128 s4 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
					__m128 c = _mm_sub_ps(_mm_mul_ps(s1, s2), _mm_mul_ps(s3, s4));
					s2 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 1, 0, 2));
					s4 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));
					__m128 d = _mm_sub_ps(_mm_mul_ps(s1, s2), _mm_mul_ps(s3, s4));

					//Normalize c and d
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					c = VectorNormalize(c);
					d = VectorNormalize(d);
#else
					c = _mm_mul_ps(c, _mm_rsqrt_ps(_mm_dp_ps(c, c, 0x7F)));
					d = _mm_mul_ps(d, _mm_rsqrt_ps(_mm_dp_ps(d, d, 0x7F)));
#endif
					return std::make_pair(Vector(c), Vector(d));
				}
				CLOAKENGINE_API float CLOAK_CALL_MATH Vector::Length() const
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return VectorLength(Data);
#else
					CLOAK_ALIGN float r[4];
					__m128 l = _mm_sqrt_ps(_mm_dp_ps(Data, Data, 0x7F));
					_mm_store_ps1(r, l);
					return r[0];
#endif
				}

				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::Abs(In const Vector& a)
				{
					return a.Abs();
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::Normalize(In const Vector& m)
				{
					return m.Normalize();
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::Cross(In const Vector& a, In const Vector& b)
				{
					return a.Cross(b);
				}
				CLOAKENGINE_API float CLOAK_CALL_MATH Vector::Dot(In const Vector& a, In const Vector& b)
				{
					return a.Dot(b);
				}
				CLOAKENGINE_API float CLOAK_CALL_MATH Vector::Length(In const Vector& a)
				{
					return a.Length();
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::Min(In const Vector& a, In const Vector& b)
				{
					return Vector(_mm_min_ps(a.Data, b.Data));
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::Max(In const Vector& a, In const Vector& b)
				{
					return Vector(_mm_max_ps(a.Data, b.Data));
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::Clamp(In const Vector& value, In const Vector& min, In const Vector& max)
				{
					return Vector(_mm_min_ps(_mm_max_ps(value.Data, min.Data), max.Data));
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::Lerp(In const Vector& a, In const Vector& b, In float c)
				{
					CLOAK_ASSUME(c >= 0 && c <= 1);
					return Vector(_mm_add_ps(a.Data, _mm_mul_ps(_mm_sub_ps(b.Data, a.Data), _mm_set1_ps(c))));
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::Floor(In const Vector& a)
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return Vector(VectorFloor(a.Data));
#else
					return Vector(_mm_floor_ps(a.Data));
#endif
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::Ceil(In const Vector& a)
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return Vector(VectorCeil(a.Data));
#else
					return Vector(_mm_ceil_ps(a.Data));
#endif
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Vector::Barycentric(In const Vector& a, In const Vector& b, In const Vector& c, In const Vector& p)
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return VectorBarycentric(a.Data, b.Data, c.Data, p.Data);
#else
					__m128 v0 = _mm_sub_ps(b.Data, a.Data);
					__m128 v1 = _mm_sub_ps(c.Data, a.Data);
					__m128 v2 = _mm_sub_ps(p.Data, a.Data);

					__m128 d00 = _mm_dp_ps(v0, v0, 0x7F);
					__m128 d01 = _mm_dp_ps(v0, v1, 0x7F);
					__m128 d11 = _mm_dp_ps(v1, v1, 0x7F);
					__m128 d20 = _mm_dp_ps(v2, v0, 0x7F);
					__m128 d21 = _mm_dp_ps(v2, v1, 0x7F);

					// d0 = 127 [ d11 | d11 | d00 | d00 ] 0
					__m128 d0 = _mm_shuffle_ps(d00, d11, _MM_SHUFFLE(0, 0, 0, 0));
					// d1 = 127 [ d21 | d21 | d20 | d20 ] 0
					__m128 d1 = _mm_shuffle_ps(d20, d21, _MM_SHUFFLE(0, 0, 0, 0));
					
					// m0 = 127 [ d00 | d00 | d00 | d11 ] 0
					__m128 m0 = _mm_shuffle_ps(d0, d0, _MM_SHUFFLE(0, 0, 0, 2));
					// m1 = 127 [ d11 | d11 | d21 | d20 ] 0
					__m128 m1 = _mm_shuffle_ps(d1, d11, _MM_SHUFFLE(0, 0, 2, 0));
					// m2 = 127 [ d01 | d01 | d20 | d21 ] 0
					__m128 m2 = _mm_shuffle_ps(d1, d01, _MM_SHUFFLE(0, 0, 0, 2));

					// vw = 127 [ denom | denom | w * denom | v * denom ] 0
					__m128 vw = _mm_sub_ps(_mm_mul_ps(m0, m1), _mm_mul_ps(d01, m2));

					// u = 127 [ 2 * denom | denom * (v + w) | 2 * w * denom | 2 * v * denom ] 0
					__m128 u = _mm_add_ps(_mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 0, 1, 0)), _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 1, 1, 0)));
					// r = 127 [ 2 | v+w | w | v ] 0
					__m128 r = _mm_div_ps(u, _mm_shuffle_ps(u, vw, _MM_SHUFFLE(3, 3, 3, 3)));
					
					__m128 res = _mm_xor_ps(FLAG_SIGN_NNPN, _mm_sub_ps(ROW_ZZOZ, r));
					return Vector(res);
#endif
				}

				CLOAKENGINE_API void* CLOAK_CALL Vector::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void* CLOAK_CALL Vector::operator new[](In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void CLOAK_CALL Vector::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
				CLOAKENGINE_API void CLOAK_CALL Vector::operator delete[](In void* s) { return API::Global::Memory::MemoryPool::Free(s); }

				CLOAKENGINE_API const Vector Vector::Forward = API::Global::Math::Vector(0, 0, 1);
				CLOAKENGINE_API const Vector Vector::Back = API::Global::Math::Vector(0, 0, -1);
				CLOAKENGINE_API const Vector Vector::Left = API::Global::Math::Vector(-1, 0, 0);
				CLOAKENGINE_API const Vector Vector::Right = API::Global::Math::Vector(1, 0, 0);
				CLOAKENGINE_API const Vector Vector::Top = API::Global::Math::Vector(0, 1, 0);
				CLOAKENGINE_API const Vector Vector::Bottom = API::Global::Math::Vector(0, -1, 0);
				CLOAKENGINE_API const Vector Vector::Zero = API::Global::Math::Vector(0, 0, 0);

				CLOAKENGINE_API CLOAK_CALL WorldPosition::WorldPosition() {}
				CLOAKENGINE_API CLOAK_CALL WorldPosition::WorldPosition(In float X, In float Y, In float Z) : Position(X, Y, Z)
				{
					AdjustWorldPos(&Position, &Node);
				}
				CLOAKENGINE_API CLOAK_CALL WorldPosition::WorldPosition(In float X, In float Y, In float Z, In int32_t nX, In int32_t nY, In int32_t nZ) : Position(X, Y, Z), Node(nX, nY, nZ)
				{
					AdjustWorldPos(&Position, &Node);
				}
				CLOAKENGINE_API CLOAK_CALL WorldPosition::WorldPosition(In const WorldPosition& v) : Position(v.Position), Node(v.Node) {}
				CLOAKENGINE_API CLOAK_CALL WorldPosition::WorldPosition(In const Vector& v) : Position(v)
				{
					AdjustWorldPos(&Position, &Node);
				}
				CLOAKENGINE_API CLOAK_CALL WorldPosition::WorldPosition(In const Point& p) : Position(p)
				{
					AdjustWorldPos(&Position, &Node);
				}
				CLOAKENGINE_API CLOAK_CALL WorldPosition::WorldPosition(In const __m128& p, In const __m128i& n) : Position(p), Node(n)
				{
					AdjustWorldPos(&Position, &Node);
				}
				CLOAKENGINE_API CLOAK_CALL WorldPosition::WorldPosition(In const Vector& v, In const IntVector& n) : WorldPosition(v.Data, n.Data) {}
				CLOAKENGINE_API CLOAK_CALL_MATH WorldPosition::operator Vector() const
				{
					return Vector(_mm_add_ps(Position.Data, _mm_mul_ps(_mm_cvtepi32_ps(Node.Data), WORLD_NODE_SIZE)));
				}
				CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH WorldPosition::operator+(In const WorldPosition& v) const
				{
					return WorldPosition(*this) += v;
				}
				CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH WorldPosition::operator-(In const WorldPosition& v) const
				{
					return WorldPosition(*this) -= v;
				}
				CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH WorldPosition::operator+(In const Vector& v) const
				{
					return WorldPosition(*this) += v;
				}
				CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH WorldPosition::operator-(In const Vector& v) const
				{
					return WorldPosition(*this) -= v;
				}
				CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH WorldPosition::operator*(In float v) const
				{
					return WorldPosition(*this) *= v;
				}
				CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH WorldPosition::operator/(In float v) const
				{
					return WorldPosition(*this) /= v;
				}
				CLOAKENGINE_API const WorldPosition CLOAK_CALL_MATH WorldPosition::operator-() const
				{
					return WorldPosition((-Position).Data, (-Node).Data);
				}
				CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH WorldPosition::operator=(In const WorldPosition& v)
				{
					Position = v.Position;
					Node = v.Node;
					return *this;
				}
				CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH WorldPosition::operator=(In const Vector& v)
				{
					Position = v;
					Node.Data = ALL_ZEROi;
					AdjustWorldPos(&Position, &Node);
					return *this;
				}
				CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH WorldPosition::operator=(In const Point& v)
				{
					return operator=(static_cast<Vector>(v));
				}
				CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH WorldPosition::operator+=(In const WorldPosition& v)
				{
					Position += v.Position;
					Node += v.Node;
					AdjustWorldPos(&Position, &Node);
					return *this;
				}
				CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH WorldPosition::operator-=(In const WorldPosition& v)
				{
					Position -= v.Position;
					Node -= v.Node;
					AdjustWorldPos(&Position, &Node);
					return *this;
				}
				CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH WorldPosition::operator+=(In const Vector& v)
				{
					Position += v;
					AdjustWorldPos(&Position, &Node);
					return *this;
				}
				CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH WorldPosition::operator-=(In const Vector& v)
				{
					Position -= v;
					AdjustWorldPos(&Position, &Node);
					return *this;
				}
				CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH WorldPosition::operator*=(In float v)
				{
					Position *= v;
					Vector n = Node * v;
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					__m128 fn = VectorFloor(n.Data);
#else
					__m128 fn = _mm_floor_ps(n.Data);
#endif
					Position += _mm_mul_ps(_mm_sub_ps(n.Data, fn), WORLD_NODE_SIZE);
					Node.Data = _mm_cvtps_epi32(fn);
					AdjustWorldPos(&Position, &Node);
					return *this;
				}
				CLOAKENGINE_API WorldPosition& CLOAK_CALL_MATH WorldPosition::operator/=(In float v)
				{
					Position /= v;
					Vector n = Node / v;
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					__m128 fn = VectorFloor(n.Data);
#else
					__m128 fn = _mm_floor_ps(n.Data);
#endif
					Position += _mm_mul_ps(_mm_sub_ps(n.Data, fn), WORLD_NODE_SIZE);
					Node.Data = _mm_cvtps_epi32(fn);
					AdjustWorldPos(&Position, &Node);
					return *this;
				}

				CLOAKENGINE_API bool CLOAK_CALL_MATH WorldPosition::operator==(In const WorldPosition& v) const
				{
					return Position == v.Position && Node == v.Node;
				}
				CLOAKENGINE_API bool CLOAK_CALL_MATH WorldPosition::operator!=(In const WorldPosition& v) const
				{
					return Position != v.Position || Node != v.Node;
				}

				CLOAKENGINE_API float CLOAK_CALL_MATH WorldPosition::Length() const
				{
					__m128 d = _mm_add_ps(Position.Data, _mm_mul_ps(_mm_cvtepi32_ps(Node.Data), WORLD_NODE_SIZE));
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return VectorLength(d);
#else
					CLOAK_ALIGN float r[4];
					__m128 l = _mm_sqrt_ps(_mm_dp_ps(d, d, 0x7F));
					_mm - store_ps1(r, l);
					return r[0];
#endif
				}
				CLOAKENGINE_API float CLOAK_CALL_MATH WorldPosition::Distance(In const WorldPosition& v) const
				{
					const IntVector n = Node - v.Node;
					const Vector p = Position - v.Position;
					__m128 d = _mm_add_ps(p.Data, _mm_mul_ps(_mm_cvtepi32_ps(n.Data), WORLD_NODE_SIZE));
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return VectorLength(d);
#else
					CLOAK_ALIGN float r[4];
					__m128 l = _mm_sqrt_ps(_mm_dp_ps(d, d, 0x7F));
					_mm - store_ps1(r, l);
					return r[0];
#endif
				}

				CLOAKENGINE_API float CLOAK_CALL_MATH WorldPosition::Length(In const WorldPosition& a)
				{
					return a.Length();
				}
				CLOAKENGINE_API float CLOAK_CALL_MATH WorldPosition::Distance(In const WorldPosition& a, In const WorldPosition& b)
				{
					return a.Distance(b);
				}

				CLOAKENGINE_API void* CLOAK_CALL WorldPosition::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void* CLOAK_CALL WorldPosition::operator new[](In size_t s) {return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void CLOAK_CALL WorldPosition::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
				CLOAKENGINE_API void CLOAK_CALL WorldPosition::operator delete[](In void* s) {return API::Global::Memory::MemoryPool::Free(s); }

				CLOAKENGINE_API CLOAK_CALL AABB::AABB() : AABB(0, 0, 0, 0, 0, 0) {}
				CLOAKENGINE_API CLOAK_CALL AABB::AABB(In float left, In float right, In float top, In float bottom, In_opt float depth) : AABB(left, right, top, bottom, depth, depth) {}
				CLOAKENGINE_API CLOAK_CALL AABB::AABB(In float left, In float right, In float top, In float bottom, In float minDepth, In float maxDepth) : Left(left), Right(right), Top(top), Bottom(bottom), MinDepth(minDepth), MaxDepth(maxDepth) {}
				CLOAKENGINE_API CLOAK_CALL AABB::AABB(In size_t size, In_reads(size) Vector* points) 
				{
					if (size > 0) 
					{
						__m128 mi = points[0].Data; 
						__m128 ma = points[0].Data;
						for (size_t a = 1; a < size; a++)
						{
							mi = _mm_min_ps(mi, points[a].Data);
							ma = _mm_max_ps(ma, points[a].Data);
						}
						Point pi = static_cast<Point>(mi);
						Point pa = static_cast<Point>(ma);
						Left = pi.X;
						Top = pi.Y;
						MinDepth = pi.Z;
						Right = pa.X;
						Bottom = pa.Y;
						MaxDepth = pa.Z;
					}
					else
					{
						Left = 0;
						Right = 0;
						Top = 0;
						Bottom = 0;
						MinDepth = 0;
						MaxDepth = 0;
					}
				}
				CLOAKENGINE_API CLOAK_CALL AABB::AABB(In size_t size, In_reads(size) const Vector* points) 
				{
					if (size > 0)
					{
						__m128 mi = points[0].Data;
						__m128 ma = points[0].Data;
						for (size_t a = 1; a < size; a++)
						{
							mi = _mm_min_ps(mi, points[a].Data);
							ma = _mm_max_ps(ma, points[a].Data);
						}
						Point pi = static_cast<Point>(mi);
						Point pa = static_cast<Point>(ma);
						Left = pi.X;
						Top = pi.Y;
						MinDepth = pi.Z;
						Right = pa.X;
						Bottom = pa.Y;
						MaxDepth = pa.Z;
					}
					else
					{
						Left = 0;
						Right = 0;
						Top = 0;
						Bottom = 0;
						MinDepth = 0;
						MaxDepth = 0;
					}
				}
				CLOAKENGINE_API CLOAK_CALL AABB::AABB(In size_t size, In_reads(size) Point* points) 
				{
					if (size > 0)
					{
						const Point& p = points[0];
						Left = p.X;
						Right = p.X;
						Top = p.Y;
						Bottom = p.Y;
						MinDepth = p.Z;
						MaxDepth = p.Z;
						for (size_t a = 1; a < size; a++)
						{
							const Point& q = points[a];
							Left = min(Left, q.X);
							Right = max(Right, q.X);
							Top = min(Top, q.Y);
							Bottom = max(Bottom, q.Y);
							MinDepth = min(MinDepth, q.Z);
							MaxDepth = max(MaxDepth, q.Z);
						}
					}
					else
					{
						Left = 0;
						Right = 0;
						Top = 0;
						Bottom = 0;
						MinDepth = 0;
						MaxDepth = 0;
					}
				}
				CLOAKENGINE_API CLOAK_CALL AABB::AABB(In size_t size, In_reads(size) const Point* points) 
				{
					if (size > 0)
					{
						const Point& p = points[0];
						Left = p.X;
						Right = p.X;
						Top = p.Y;
						Bottom = p.Y;
						MinDepth = p.Z;
						MaxDepth = p.Z;
						for (size_t a = 1; a < size; a++)
						{
							const Point& q = points[a];
							Left = min(Left, q.X);
							Right = max(Right, q.X);
							Top = min(Top, q.Y);
							Bottom = max(Bottom, q.Y);
							MinDepth = min(MinDepth, q.Z);
							MaxDepth = max(MaxDepth, q.Z);
						}
					}
					else
					{
						Left = 0;
						Right = 0;
						Top = 0;
						Bottom = 0;
						MinDepth = 0;
						MaxDepth = 0;
					}
				}

				CLOAKENGINE_API float CLOAK_CALL AABB::Volume() const
				{
					float dX = Right - Left;
					float dY = Bottom - Top;
					float dZ = MaxDepth - MinDepth;
					dX = max(0, dX);
					dZ = max(0, dY);
					dY = max(0, dZ);
					return dX * dY * dZ;
				}

				CLOAKENGINE_API CLOAK_CALL Matrix::Matrix() : Matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1) {}
				CLOAKENGINE_API CLOAK_CALL Matrix::Matrix(In float d[4][4])
				{
					for (size_t a = 0; a < 4; a++)
					{
						Row[a] = _mm_set_ps(d[a][3], d[a][2], d[a][1], d[a][0]);
					}
				}
				CLOAKENGINE_API CLOAK_CALL Matrix::Matrix(In float r00, In float r01, In float r02, In float r03, In float r10, In float r11, In float r12, In float r13, In float r20, In float r21, In float r22, In float r23, In float r30, In float r31, In float r32, In float r33)
				{
					Row[0] = _mm_set_ps(r03, r02, r01, r00);
					Row[1] = _mm_set_ps(r13, r12, r11, r10);
					Row[2] = _mm_set_ps(r23, r22, r21, r20);
					Row[3] = _mm_set_ps(r33, r32, r31, r30);
				}
				CLOAKENGINE_API CLOAK_CALL Matrix::Matrix(In const Matrix& m)
				{
					for (size_t a = 0; a < 4; a++) { Row[a] = m.Row[a]; }
				}
				CLOAKENGINE_API CLOAK_CALL Matrix::Matrix(In const __m128& r0, In const __m128& r1, In const __m128& r2, In const __m128& r3)
				{
					Row[0] = r0;
					Row[1] = r1;
					Row[2] = r2;
					Row[3] = r3;
				}
				CLOAKENGINE_API const Matrix CLOAK_CALL_MATH Matrix::operator*(In const Matrix& m) const
				{
					return Matrix(*this) *= m;
				}
				CLOAKENGINE_API Matrix& CLOAK_CALL_MATH Matrix::operator*=(In const Matrix& m)
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					MatrixMul(Row, m.Row);
#else
					__m128 o[4];
					for (size_t a = 0; a < 4; a++)
					{
						o[a] = _mm_mul_ps(_mm_shuffle_ps(Row[a], Row[a], _MM_SHUFFLE(0, 0, 0, 0)), m.Row[0]);
						o[a] = _mm_add_ps(o[a], _mm_mul_ps(_mm_shuffle_ps(Row[a], Row[a], _MM_SHUFFLE(1, 1, 1, 1)), m.Row[1]));
						o[a] = _mm_add_ps(o[a], _mm_mul_ps(_mm_shuffle_ps(Row[a], Row[a], _MM_SHUFFLE(2, 2, 2, 2)), m.Row[2]));
						o[a] = _mm_add_ps(o[a], _mm_mul_ps(_mm_shuffle_ps(Row[a], Row[a], _MM_SHUFFLE(3, 3, 3, 3)), m.Row[3]));
					}
					for (size_t a = 0; a < 4; a++) { Row[a] = o[a]; }
#endif
					return *this;
				}

				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::Inverse() const
				{
					__m128 A = _mm_movelh_ps(Row[0], Row[1]);
					__m128 B = _mm_movehl_ps(Row[1], Row[0]);
					__m128 C = _mm_movelh_ps(Row[2], Row[3]);
					__m128 D = _mm_movehl_ps(Row[3], Row[2]);

					__m128 AB = _mm_mul_ps(_mm_shuffle_ps(A, A, 0x0F), B);
					AB = _mm_sub_ps(AB, _mm_mul_ps(_mm_shuffle_ps(A, A, 0xA5), _mm_shuffle_ps(B, B, 0x4E)));
					__m128 DC = _mm_mul_ps(_mm_shuffle_ps(D, D, 0x0F), C);
					DC = _mm_sub_ps(DC, _mm_mul_ps(_mm_shuffle_ps(D, D, 0xA5), _mm_shuffle_ps(C, C, 0x4E)));

					__m128 dA = _mm_mul_ps(_mm_shuffle_ps(A, A, 0x5F), A);
					dA = _mm_sub_ss(dA, _mm_movehl_ps(dA, dA));
					__m128 dB = _mm_mul_ps(_mm_shuffle_ps(B, B, 0x5F), B);
					dB = _mm_sub_ss(dB, _mm_movehl_ps(dB, dB));
					__m128 dC = _mm_mul_ps(_mm_shuffle_ps(C, C, 0x5F), C);
					dC = _mm_sub_ss(dC, _mm_movehl_ps(dC, dC));
					__m128 dD = _mm_mul_ps(_mm_shuffle_ps(D, D, 0x5F), D);
					dD = _mm_sub_ss(dD, _mm_movehl_ps(dD, dD));

					__m128 d = _mm_mul_ps(_mm_shuffle_ps(DC, DC, 0xD8), AB);
					d = _mm_add_ps(d, _mm_movehl_ps(d, d));
					d = _mm_add_ss(d, _mm_shuffle_ps(d, d, 0x01));
					__m128 d1 = _mm_mul_ss(dA, dD);
					__m128 d2 = _mm_mul_ss(dB, dC);

					__m128 det = _mm_sub_ss(_mm_add_ss(d1, d2), d);
					__m128 rd = _mm_div_ss(_mm_set_ss(1.0f), det);
					rd = _mm_shuffle_ps(rd, rd, 0x00);
					rd = _mm_xor_ps(rd, FLAG_SIGN_PNNP);

					__m128 iD = _mm_mul_ps(_mm_shuffle_ps(C, C, 0xA0), _mm_movelh_ps(AB, AB));
					iD = _mm_add_ps(iD, _mm_mul_ps(_mm_shuffle_ps(C, C, 0xF5), _mm_movehl_ps(AB, AB)));
					iD = _mm_sub_ps(_mm_mul_ps(D, _mm_shuffle_ps(dA, dA, 0x00)), iD);
					__m128 iA = _mm_mul_ps(_mm_shuffle_ps(B, B, 0xA0), _mm_movelh_ps(DC, DC));
					iA = _mm_add_ps(iA, _mm_mul_ps(_mm_shuffle_ps(B, B, 0xF5), _mm_movehl_ps(DC, DC)));
					iA = _mm_sub_ps(_mm_mul_ps(A, _mm_shuffle_ps(dD, dD, 0x00)), iA);

					__m128 iB = _mm_mul_ps(D, _mm_shuffle_ps(AB, AB, 0x33));
					iB = _mm_sub_ps(iB, _mm_mul_ps(_mm_shuffle_ps(D, D, 0xB1), _mm_shuffle_ps(AB, AB, 0x66)));
					iB = _mm_sub_ps(_mm_mul_ps(C, _mm_shuffle_ps(dB, dB, 0x00)), iB);
					__m128 iC = _mm_mul_ps(A, _mm_shuffle_ps(DC, DC, 0x33));
					iC = _mm_sub_ps(iC, _mm_mul_ps(_mm_shuffle_ps(A, A, 0xB1), _mm_shuffle_ps(DC, DC, 0x66)));
					iC = _mm_sub_ps(_mm_mul_ps(B, _mm_shuffle_ps(dC, dC, 0x00)), iC);

					iA = _mm_mul_ps(rd, iA);
					iB = _mm_mul_ps(rd, iB);
					iC = _mm_mul_ps(rd, iC);
					iD = _mm_mul_ps(rd, iD);

					return Matrix(_mm_shuffle_ps(iA, iB, 0x77), _mm_shuffle_ps(iA, iB, 0x22), _mm_shuffle_ps(iC, iD, 0x77), _mm_shuffle_ps(iC, iD, 0x22));
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::Transpose() const
				{
					__m128 r[4] = { Row[0],Row[1],Row[2],Row[3] };
					_MM_TRANSPOSE4_PS(r[0], r[1], r[2], r[3]);
					return Matrix(r[0], r[1], r[2], r[3]);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::RemoveTranslation() const
				{
					//Row[3] set to 0 [ 0, 0, 0, W ] 128
					return Matrix(Row[0], Row[1], Row[2], _mm_and_ps(Row[3], FLAG_SET_NNNS));
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Matrix::GetTranslation() const
				{
					return Vector(Row[3]);
				}

				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::Inverse(In const Matrix& m)
				{
					return m.Inverse();
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::PerspectiveProjection(In float fov, In unsigned int screenWidth, In unsigned int screenHeight, In float nearPlane, In float farPlane)
				{
					const float h = 1 / tanf(fov / 2);
					const float w = (h*screenHeight) / screenWidth;
					const float q = farPlane / (farPlane - nearPlane);
					return Matrix(w, 0, 0, 0, 0, h, 0, 0, 0, 0, 1-q, 1, 0, 0, q * nearPlane, 0);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::OrthographicProjection(In unsigned int screenWidth, In unsigned int screenHeight, In float nearPlane, In float farPlane)
				{
					const float left = -(screenWidth / 2.0f);
					const float top = -(screenHeight / 2.0f);
					const float right = screenWidth + left;
					const float bottom = screenHeight + top;
					return Matrix::OrthographicProjection(left, right, top, bottom, nearPlane, farPlane);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::OrthographicProjection(In float left, In float right, In float top, In float bottom, In float nearPlane, In float farPlane)
				{
					const float lr = 1.0f / (right - left);
					const float tb = 1.0f / (top - bottom);
					const float nf = 1.0f / (farPlane - nearPlane);
					return Matrix(2 * lr, 0, 0, 0, 0, -2 * tb, 0, 0, 0, 0, -nf, 0, -(right + left)*lr, (top + bottom)*tb, 1 + nearPlane * nf, 1);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::LookAt(In const Vector& Position, In const Vector& Target, In const Vector& Up)
				{
					return LookTo(Position, Target - Position, Up);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::LookTo(In const Vector& Position, In const Vector& Direction, In const Vector& Up)
				{
					const Vector z = Vector::Normalize(Direction);
					const Vector x = Vector::Normalize(Up).Cross(z);
					const Vector y = z.Cross(x);
					const Point pz = (Point)z;
					const Point px = (Point)x;
					const Point py = (Point)y;
					return Matrix(px.X, py.X, pz.X, 0, px.Y, py.Y, pz.Y, 0, px.Z, py.Z, pz.Z, 0, -x.Dot(Position), -y.Dot(Position), -z.Dot(Position), 1);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::Translate(In float X, In float Y, In float Z)
				{
					return Matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, X, Y, Z, 1);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::Translate(In const Vector& v)
				{
					CLOAK_ALIGN float d[4];
					_mm_store_ps(d, v.Data);
					return Matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, d[0], d[1], d[2], 1);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::RotationX(In float angle)
				{
					const float s = std::sin(angle);
					const float c = std::cos(angle);
					return Matrix(1, 0, 0, 0, 0, c, -s, 0, 0, s, c, 0, 0, 0, 0, 1);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::RotationY(In float angle)
				{
					const float s = std::sin(angle);
					const float c = std::cos(angle);
					return Matrix(c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0, 0, 0, 0, 1);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::RotationZ(In float angle)
				{
					const float s = std::sin(angle);
					const float c = std::cos(angle);
					return Matrix(c, -s, 0, 0, s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::Rotation(In float angleX, In float angleY, In float angleZ, In RotationOrder order)
				{
					__m128 angles = _mm_xor_ps(_mm_set_ps(0, angleZ, angleY, angleX), FLAG_SIGN_NNNN);
					__m128 s, c;
					_mm_sincos_ps(angles, &s, &c);
					//s: 127 [ 0 | sz | sy | sx ] 0
					//c: 127 [ 1 | cz | cy | cx ] 0

					switch (order)
					{
						case CloakEngine::API::Global::Math::RotationOrder::XYZ:
						{
							//t0: 127 [ sx | sz | cx | cz ] 0
							__m128 t0 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(0, 2, 0, 2));
							//t1: 127 [ cx | sx | sx | cx ] 0
							__m128 t1 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(1, 3, 3, 1));
							//t2: 127 [ sz | sz | cz | cz ] 0
							__m128 t2 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(2, 2, 0, 0));
							//t3: 127 [ sy | sy | sy | sy ] 0
							__m128 t3 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 1, 1, 1));

							__m128 t4 = _mm_mul_ps(t1, t2);
							__m128 t5 = _mm_shuffle_ps(t4, t4, _MM_SHUFFLE(1, 0, 3, 2));

							//t6: 127 [ cy | cy | cy | cy ] 0
							__m128 t6 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(1, 1, 1, 1));

							//x1 127 [ r01 | r02 | r12 | r11 ] 0
							__m128 x1 = _mm_addsub_ps(t4, _mm_mul_ps(t3, t5));
							//x2 127 [ r21 | r10 | r22 | r00 ] 0
							__m128 x2 = _mm_xor_ps(FLAG_SIGN_PPNN, _mm_mul_ps(t6, t0));
							//x3 127 [ r00 | r10 | 0 | 0 ] 0
							__m128 x3 = _mm_shuffle_ps(ALL_ZERO, x2, _MM_SHUFFLE(0, 2, 0, 0));

							//r0 127 [ 0 | r00 | r02 | r01 ] 0
							__m128 r0 = _mm_shuffle_ps(x1, x3, _MM_SHUFFLE(0, 3, 2, 3));
							r0 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(3, 1, 0, 2));
							//r1 127 [ 0 | r10 | r11 | r12 ] 0
							__m128 r1 = _mm_shuffle_ps(x1, x3, _MM_SHUFFLE(0, 2, 0, 1));
							r1 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(3, 0, 1, 2));
							//r2 127 [ r21 | r22 | sy | 0 ] 0
							__m128 r2 = _mm_shuffle_ps(s, x2, _MM_SHUFFLE(3, 1, 1, 3));
							r2 = _mm_shuffle_ps(r2, r2, _MM_SHUFFLE(0, 2, 3, 1));

							//row0: 127 [ 0 | (sx*sz) - (cx*cz*sy)	|  (cx*sz) + (cz*sx*sy)	|  cy*cz	] 0
							//row1: 127 [ 0 | (cz*sx) + (cx*sy*sz)	|  (cx*cz) - (sx*sy*sz)	| -cy*sz	] 0
							//row2: 127 [ 0 | cx*cy					| -cy*sx				|  sy		] 0
							//row3: 127 [ 1 | 0						|  0					|  0		] 0
							return Matrix(r0, r1, r2, ROW_ZZZO);
						}
						case CloakEngine::API::Global::Math::RotationOrder::XZY:
						{
							//t0: 127 [ sx | sy | cx | cy ] 0
							__m128 t0 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(0, 1, 0, 1));
							//t1: 127 [ cx | sx | sx | cx ] 0
							__m128 t1 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(1, 3, 3, 1));
							//t2: 127 [ cy | cy | sy | sy ] 0
							__m128 t2 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(0, 0, 2, 2));
							//t3: 127 [ sz | sz | sz | sz ] 0
							__m128 t3 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 2, 2, 2));

							__m128 t4 = _mm_mul_ps(t1, t2);
							__m128 t5 = _mm_shuffle_ps(t4, t4, _MM_SHUFFLE(1, 0, 3, 2));

							//t6: 127 [ cz | cz | cz | cz ] 0
							__m128 t6 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(2, 2, 2, 2));

							//x1: 127 [ r01 | r02 | r22 | r21 ] 0
							__m128 x1 = _mm_addsub_ps(_mm_mul_ps(t4, t3), t5);
							//x2: 127 [ r12 | r20 | r11 | r00 ] 0
							__m128 x2 = _mm_mul_ps(t6, t0);
							//x3: 127 [ r00 | r20 | 0 | 0 ] 0
							__m128 x3 = _mm_shuffle_ps(ALL_ZERO, x2, _MM_SHUFFLE(0, 2, 0, 0));

							//r0: 127 [ 0 | r00 | r02 | r01 ] 0
							__m128 r0 = _mm_shuffle_ps(x1, x3, _MM_SHUFFLE(0, 3, 2, 3));
							r0 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(3, 1, 0, 2));
							//r1: 127 [ r11 | r12 | sz | 0 ] 0
							__m128 r1 = _mm_shuffle_ps(s, x2, _MM_SHUFFLE(1, 3, 2, 3));
							r1 = _mm_xor_ps(FLAG_SIGN_NPPP, _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0, 2, 3, 1)));
							//r2: 127 [ 0 | r20 | r22 | r21 ] 0
							__m128 r2 = _mm_shuffle_ps(x1, x3, _MM_SHUFFLE(0, 2, 1, 0));
							r2 = _mm_shuffle_ps(r2, r2, _MM_SHUFFLE(3, 1, 0, 2));

							//row0: 127 [ 0 | (cy*sx*sz) - (cx*sy)	| (sx*sy) + (cx*cy*sz)	| cz*cy		] 0
							//row1: 127 [ 0 | cz*sx					| cx*cz					| -sz		] 0
							//row2: 127 [ 0 | (cx*cy) + (sx*sz*sy)	| (cx*sz*sy) - (cy*sx)	| cz*sy		] 0
							//row3: 127 [ 1 | 0						| 0						| 0			] 0
							return Matrix(r0, r1, r2, ROW_ZZZO);
						}
						case CloakEngine::API::Global::Math::RotationOrder::YXZ:
						{
							//t0: 127 [ sz | sy | cz | cy ] 0
							__m128 t0 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(2, 1, 2, 1));
							//t1: 127 [ sy | cy | cy | sy ] 0
							__m128 t1 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(2, 0, 0, 2));
							//t2: 127 [ sz | sz | cz | cz ] 0
							__m128 t2 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(3, 3, 1, 1));
							//t3: 127 [ sx | sx | sx | sx ] 0
							__m128 t3 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(0, 0, 0, 0));

							__m128 t4 = _mm_mul_ps(t1, t2);
							__m128 t5 = _mm_shuffle_ps(t4, t4, _MM_SHUFFLE(1, 0, 3, 2));

							//t6: 127 [ cx | cx | cx | cx ] 0
							__m128 t6 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 0, 0, 0));

							//x1: 127 [ r00 | r02 | r12 | r10 ] 0
							__m128 x1 = _mm_addsub_ps(_mm_mul_ps(t4, t3), t5);
							//x2: 127 [ r01 | r20 | r11 | r22 ] 0
							__m128 x2 = _mm_mul_ps(t6, t0);
							//x3: 127 [ r01 | r11 | 0 | 0 ] 0
							__m128 x3 = _mm_shuffle_ps(ALL_ZERO, x2, _MM_SHUFFLE(3, 1, 0, 0));

							//r0: 127 [ r01 | 0 | r00 | r02 ] 0
							__m128 r0 = _mm_shuffle_ps(x1, x3, _MM_SHUFFLE(3, 0, 3, 2));
							r0 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(2, 0, 3, 1));
							//r1: 127 [ r11 | 0 | r12 | r10 ] 0
							__m128 r1 = _mm_shuffle_ps(x1, x3, _MM_SHUFFLE(2, 0, 1, 0));
							r1 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(2, 1, 3, 0));
							//r2: 127 [ r22 | r20 | sx | 0 ] 0
							__m128 r2 = _mm_shuffle_ps(s, x2, _MM_SHUFFLE(0, 2, 0, 3));
							r2 = _mm_xor_ps(FLAG_SIGN_PNPP, _mm_shuffle_ps(r2, r2, _MM_SHUFFLE(0, 3, 1, 2)));

							//row0: 127 [ 0 | (cy*sx*sz) - (cz*sy)	| cx*sz		| (cy*cz) + (sy*sx*sz)	] 0
							//row1: 127 [ 0 | (cy*cz*sx) + (sy*sz)	| cx*cz		| (cz*sy*sx) - (cy*sz)	] 0
							//row2: 127 [ 0 | cy*cx					| -sx		| cx*sy					] 0
							//row3: 127 [ 1 | 0						| 0			| 0						] 0
							return Matrix(r0, r1, r2, ROW_ZZZO);
						}
						case CloakEngine::API::Global::Math::RotationOrder::YZX:
						{
							//t0: 127 [ sx | sy | cx | cy ] 0
							__m128 t0 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(0, 1, 0, 1));
							//t1: 127 [ cy | sy | sy | cy ] 0
							__m128 t1 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(0, 2, 2, 0));
							//t2: 127 [ sx | sx | cx | cx ] 0
							__m128 t2 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(3, 3, 1, 1));
							//t3: 127 [ sz | sz | sz | sz ] 0
							__m128 t3 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 2, 2, 2));

							__m128 t4 = _mm_mul_ps(t1, t2);
							__m128 t5 = _mm_shuffle_ps(t4, t4, _MM_SHUFFLE(1, 0, 3, 2));

							//t6: 127 [ -cz | -cz | cz | cz ] 0
							__m128 t6 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(2, 2, 2, 2));

							//x1: 127 [ r12 | r10 | r20 | r22 ] 0
							__m128 x1 = _mm_addsub_ps(t4, _mm_mul_ps(t3, t5));
							//x2: 127 [ r21 | r02 | r11 | r00 ] 0
							__m128 x2 = _mm_xor_ps(FLAG_SIGN_PPNN, _mm_mul_ps(t6, t0));
							//x3: 127 [ r11 | r21 | 0 | 0 ] 0
							__m128 x3 = _mm_shuffle_ps(ALL_ZERO, x2, _MM_SHUFFLE(1, 3, 0, 0));

							//r0: 127 [ r00 | r02 | sz | 0 ] 0
							__m128 r0 = _mm_shuffle_ps(s, x2, _MM_SHUFFLE(0, 2, 2, 3));
							r0 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(0, 2, 1, 3));
							//r1: 127 [ 0 | r11 | r10 | r12 ] 0
							__m128 r1 = _mm_shuffle_ps(x1, x3, _MM_SHUFFLE(0, 3, 2, 3));
							r1 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(3, 0, 2, 1));
							//r2: 127 [ 0 | r21 | r22 | r20 ] 0
							__m128 r2 = _mm_shuffle_ps(x1, x3, _MM_SHUFFLE(0, 2, 0, 1));
							r2 = _mm_shuffle_ps(r2, r2, _MM_SHUFFLE(3, 1, 2, 0));

							//row0: 127 [ 0 | -cz*sy				| sz		| cy*cz					] 0
							//row1: 127 [ 0 | (cy*sx) + (cx*sy*sz)	| cz*cx		| (sy*sx) - (cy*cx*sz)	] 0
							//row2: 127 [ 0 | (cy*cx) - (sy*sz*sx)	| -cz*sx	| (cx*sy) + (cy*sz*sx)	] 0
							//row3: 127 [ 1 | 0						| 0			| 0						] 0
							return Matrix(r0, r1, r2, ROW_ZZZO);
						}
						case CloakEngine::API::Global::Math::RotationOrder::ZYX:
						{
							//t0: 127 [ sx | sz | cx | cz ] 0
							__m128 t0 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(0, 2, 0, 2));
							//t1: 127 [ sz | cz | cz | sz ] 0
							__m128 t1 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(2, 0, 0, 2));
							//t2: 127 [ sx | sx | cx | cx ] 0
							__m128 t2 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(3, 3, 1, 1));
							//t3: 127 [ sy | sy | sy | sy ] 0
							__m128 t3 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 1, 1, 1));

							__m128 t4 = _mm_mul_ps(t1, t2);
							__m128 t5 = _mm_shuffle_ps(t4, t4, _MM_SHUFFLE(1, 0, 3, 2));

							//t6: 127 [ cy | cy | cy | cy ] 0
							__m128 t6 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(1, 1, 1, 1));

							//x1: 127 [ r11 | r10 | r20 | r21 ] 0
							__m128 x1 = _mm_addsub_ps(_mm_mul_ps(t3, t4), t5);
							//x2: 127 [ r12 | r01 | r22 | r00 ] 0
							__m128 x2 = _mm_mul_ps(t6, t0);
							//x3: 127 [ r12 | r22 | 0 | 0 ] 0
							__m128 x3 = _mm_shuffle_ps(ALL_ZERO, x2, _MM_SHUFFLE(3, 1, 0, 0));


							//r0: 127 [ 0 | -sy | r01 | r00 ] 0
							__m128 r0 = _mm_xor_ps(FLAG_SIGN_PPNP, _mm_shuffle_ps(x2, s, _MM_SHUFFLE(3, 1, 2, 0)));
							//r1: 127 [ 0 | r12 | r11 | r10 ] 0
							__m128 r1 = _mm_shuffle_ps(x1, x3, _MM_SHUFFLE(0, 3, 3, 2));
							//r2: 127 [ 0 | r22 | r21 | r20 ] 0
							__m128 r2 = _mm_shuffle_ps(x1, x3, _MM_SHUFFLE(0, 2, 0, 1));
							
							//row0: 127 [ 0 | -sy		| cy*sz					| cz*cy					] 0
							//row1: 127 [ 0 | cy*sx		| (cz*cx) + (sz*sy*sx)	| (cz*sy*sx) - (cx*sz)	] 0
							//row2: 127 [ 0 | cy*cx		| (cx*sz*sy) - (cz*sx)	| (sz*sx) + (cz*cx*sy)	] 0
							//row3: 127 [ 1 | 0			| 0						| 0						] 0
							return Matrix(r0, r1, r2, ROW_ZZZO);
						}
						case CloakEngine::API::Global::Math::RotationOrder::ZXY:
						{
							//t0: 127 [ sz | sy | cz | cy ] 0
							__m128 t0 = _mm_shuffle_ps(c, s, _MM_SHUFFLE(2, 1, 2, 1));
							//t1: 127 [ sz | cz | cz | sz ] 0
							__m128 t1 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(3, 1, 1, 3));
							//t2: 127 [ cy | cy | sy | sy ] 0
							__m128 t2 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(0, 0, 2, 2));
							//t3: 127 [ sx | sx | sx | sx ] 0
							__m128 t3 = _mm_shuffle_ps(s, s, _MM_SHUFFLE(0, 0, 0, 0));

							__m128 t4 = _mm_mul_ps(t1, t2);
							__m128 t5 = _mm_shuffle_ps(t4, t4, _MM_SHUFFLE(1, 0, 3, 2));

							//t6: 127 [ cx | cx | cx | cx ] 0
							__m128 t6 = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 0, 0, 0));

							//x1: 127 [ r01 | r00 | r20 | r21 ] 0
							__m128 x1 = _mm_addsub_ps(t4, _mm_mul_ps(t3, t5));
							//x2: 127 [ r10 | r02 | r11 | r22 ] 0
							__m128 x2 = _mm_xor_ps(FLAG_SIGN_PPNN, _mm_mul_ps(t6, t0));
							//x3: 127 [ r22 | r02 | 0 | 0 ] 0
							__m128 x3 = _mm_shuffle_ps(ALL_ZERO, x2, _MM_SHUFFLE(0, 2, 0, 0));

							//r0: 127 [ 0 | r02 | r01 | r00 ] 0
							__m128 r0 = _mm_shuffle_ps(x1, x3, _MM_SHUFFLE(0, 2, 3, 2));
							//r1: 127 [ 0 | sx | r11 | r10 ] 0
							__m128 r1 = _mm_shuffle_ps(x2, s, _MM_SHUFFLE(3, 0, 1, 3));
							//r2: 127 [ 0 | r22 | r21 | r20 ] 0
							__m128 r2 = _mm_shuffle_ps(x1, x3, _MM_SHUFFLE(0, 3, 0, 1));

							//row0: 127 [ 0 | -cx*sy	| (cy*sz) + (cz*sx*sy)	| (cz*cy) - (sz*sx*sy)	] 0
							//row1: 127 [ 0 | sx		| cz*cx					| -cx*sz				] 0
							//row2: 127 [ 0 | cx*cy		| (sz*sy) - (cz*cy*sx)	| (cz*sy) + (cy*sz*sx)	] 0
							//row3: 127 [ 1 | 0			| 0						| 0						] 0
							return Matrix(r0, r1, r2, ROW_ZZZO);
						}
						default:
							break;
					}
					return Matrix();
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::Rotation(In const EulerAngles& e, In RotationOrder order) { return e.ToMatrix(order); }
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::Rotation(In const Vector& axis, In float angle)
				{
					const Point v = (Point)Vector::Normalize(axis);
					const float c = std::cos(angle);
					const float s = std::sin(angle);
					return Matrix(ROTATION_HELPER(v, X, X, c, c), ROTATION_HELPER(v, X, Y, c, -(v.Z*s)), ROTATION_HELPER(v, X, Z, c, v.Y*s), 0,
						ROTATION_HELPER(v, Y, X, c, v.Z*s), ROTATION_HELPER(v, Y, Y, c, c), ROTATION_HELPER(v, Y, Z, c, -(v.X*s)), 0,
						ROTATION_HELPER(v, Z, X, c, -(v.Y*s)), ROTATION_HELPER(v, Z, Y, c, v.X*s), ROTATION_HELPER(v, Z, Z, c, c), 0,
						0, 0, 0, 1);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::Scale(In float x, In float y, In float z)
				{
					return Matrix(x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::Scale(In const Vector& v)
				{
					const Point p = (Point)v;
					return Scale(p.X, p.Y, p.Z);
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::Transpose(In const Matrix& m)
				{
					return m.Transpose();
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Matrix::RemoveTranslation(In const Matrix& m)
				{
					return m.RemoveTranslation();
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Matrix::GetTranslation(In const Matrix& m)
				{
					return m.GetTranslation();
				}
				CLOAKENGINE_API void* CLOAK_CALL Matrix::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void* CLOAK_CALL Matrix::operator new[](In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void CLOAK_CALL Matrix::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
				CLOAKENGINE_API void CLOAK_CALL Matrix::operator delete[](In void* s) { return API::Global::Memory::MemoryPool::Free(s); }

				CLOAKENGINE_API const Matrix Matrix::Identity = API::Global::Math::Matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

				CLOAKENGINE_API CLOAK_CALL Quaternion::Quaternion()
				{
					Data = ALL_ZERO;
				}
				CLOAKENGINE_API CLOAK_CALL Quaternion::Quaternion(In float I, In float J, In float K, In float R)
				{
					Data = _mm_set_ps(R, K, J, I);
				}
				CLOAKENGINE_API CLOAK_CALL Quaternion::Quaternion(In const Vector& axis, In float angle)
				{
					__m128 a = axis.Normalize().Data;
					const float c = std::cos(angle / 2);
					const float s = std::sin(angle / 2);
					__m128 d = _mm_shuffle_ps(a, _mm_shuffle_ps(a, ALL_ONE, _MM_SHUFFLE(2, 2, 2, 2)), _MM_SHUFFLE(2, 1, 1, 0));
					Data = _mm_mul_ps(_mm_set_ps(c, s, s, s), d);
				}
				CLOAKENGINE_API CLOAK_CALL Quaternion::Quaternion(In const Quaternion& q)
				{
					Data = q.Data;
				}
				CLOAKENGINE_API CLOAK_CALL Quaternion::Quaternion(In const Point& p)
				{
					Data = _mm_load_ps(p.Data);
				}
				CLOAKENGINE_API CLOAK_CALL Quaternion::Quaternion(In const __m128& d) : Data(d) {}
				CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH Quaternion::operator*(In const Quaternion& q) const
				{
					return Quaternion(*this) *= q;
				}
				CLOAKENGINE_API Quaternion& CLOAK_CALL_MATH Quaternion::operator*=(In const Quaternion& q)
				{
					/*Data = 127 [ R | K | J | I ] 0 */
					__m128 a1123 = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(3, 2, 1, 1));
					__m128 a2231 = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(1, 3, 2, 2));
					__m128 b1000 = _mm_shuffle_ps(q.Data, q.Data, _MM_SHUFFLE(0, 0, 0, 1));
					__m128 b2312 = _mm_shuffle_ps(q.Data, q.Data, _MM_SHUFFLE(2, 1, 3, 2));
					__m128 t1 = _mm_mul_ps(a1123, b1000);
					__m128 t2 = _mm_mul_ps(a2231, b2312);
					__m128 t12 = _mm_xor_ps(_mm_add_ps(t1, t2), FLAG_SIGN_NPPP);
					__m128 a3312 = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(2, 1, 3, 3));
					__m128 b3231 = _mm_shuffle_ps(q.Data, q.Data, _MM_SHUFFLE(1, 3, 2, 3));
					__m128 a0000 = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(0, 0, 0, 0));
					__m128 t3 = _mm_mul_ps(a3312, b3231);
					__m128 t0 = _mm_mul_ps(a0000, q.Data);
					__m128 t03 = _mm_sub_ps(t0, t3);
					Data = _mm_add_ps(t03, t12);
					/*
					__m128 zwxy = _mm_shuffle_ps(q.Data, q.Data, _MM_SHUFFLE(1, 0, 3, 2));
					__m128 yxwz = _mm_shuffle_ps(q.Data, q.Data, _MM_SHUFFLE(2, 3, 0, 1));
					__m128 wzyx = _mm_shuffle_ps(zwxy, zwxy, _MM_SHUFFLE(2, 3, 0, 1));

					__m128 r = _mm_xor_ps(_mm_mul_ps(Data, q.Data), FLAG_SIGN_NNNP);
					__m128 i = _mm_xor_ps(_mm_mul_ps(Data, yxwz),	FLAG_SIGN_NPPP);
					__m128 j = _mm_xor_ps(_mm_mul_ps(Data, zwxy),	FLAG_SIGN_PPNP);
					__m128 k = _mm_xor_ps(_mm_mul_ps(Data, wzyx),	FLAG_SIGN_PNPP);
					Data = _mm_hadd_ps(_mm_hadd_ps(k, j), _mm_hadd_ps(i, r));
					*/
					return *this;
				}
				CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH Quaternion::operator*(In const Vector& q) const
				{
					return *this * static_cast<Quaternion>(q);
				}
				CLOAKENGINE_API Quaternion& CLOAK_CALL_MATH Quaternion::operator*=(In const Vector& q)
				{
					return *this *= static_cast<Quaternion>(q);
				}
				CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH Quaternion::operator*(In float a) const
				{
					return Quaternion(*this) *= a;
				}
				CLOAKENGINE_API Quaternion& CLOAK_CALL_MATH Quaternion::operator*=(In float a)
				{
					Data = _mm_mul_ps(Data, _mm_set_ps1(a));
					return *this;
				}
				CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH Quaternion::operator+(In const Quaternion& q) const
				{
					return Quaternion(*this) += q;
				}
				CLOAKENGINE_API Quaternion& CLOAK_CALL_MATH Quaternion::operator+=(In const Quaternion& q)
				{
					Data = _mm_add_ps(Data, q.Data);
					return *this;
				}
				CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH Quaternion::operator-(In const Quaternion& q) const
				{
					return Quaternion(*this) -= q;
				}
				CLOAKENGINE_API Quaternion& CLOAK_CALL_MATH Quaternion::operator-=(In const Quaternion& q)
				{
					Data = _mm_sub_ps(Data, q.Data);
					return *this;
				}
				CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH Quaternion::operator-() const
				{
					return Quaternion(_mm_sub_ps(ALL_ZERO, Data));
				}
				CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH Quaternion::operator*(In const DualQuaternion& q) const
				{
					return Quaternion(*this) *= q;
				}
				CLOAKENGINE_API Quaternion& CLOAK_CALL_MATH Quaternion::operator*=(In const DualQuaternion& q)
				{
					Quaternion cr = Quaternion::Conjugate(q.Real);
					Quaternion cd = Quaternion::Conjugate(q.Dual);
					Data = ((((q.Real*(*this)) + q.Dual)*cr) - (q.Real*cd)).Data;
					return *this;
				}
				CLOAKENGINE_API bool CLOAK_CALL_MATH Quaternion::operator==(In const Quaternion& q) const
				{
					__m128 c = _mm_and_ps(_mm_cmpge_ps(_mm_add_ps(Data, ALL_EPS), q.Data), _mm_cmple_ps(_mm_sub_ps(Data, ALL_EPS), q.Data));
					CLOAK_ALIGN float r[4];
					_mm_store_ps(r, c);
					uint32_t* i = reinterpret_cast<uint32_t*>(r);
					return (i[0] & i[1] & i[2] & i[3]) != 0;
				}
				CLOAKENGINE_API bool CLOAK_CALL_MATH Quaternion::operator!=(In const Quaternion& q) const
				{
					return !operator==(q);
				}
				CLOAKENGINE_API CLOAK_CALL_MATH Quaternion::operator Point() const
				{
					Point p;
					_mm_store_ps(p.Data, Data);
					return p;
				}
				CLOAKENGINE_API CLOAK_CALL_MATH Quaternion::operator Matrix() const
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					__m128 d[4];
					QMatrix(d, Data);
					return Matrix(d[0], d[1], d[2], d[3]);
#else
					__m128 l = _mm_rsqrt_ps(_mm_dp_ps(Data, Data, 0xFF));
					//r = 127 [RKJI] 0
					__m128 r = _mm_mul_ps(Data, l);
					//t = 2 * 127[R | -K | -J | -I]0
					__m128 tr = _mm_xor_ps(r, FLAG_SIGN_NNNP);
					__m128 t2 = _mm_shuffle_ps(tr, tr, _MM_SHUFFLE(0, 1, 0, 1));
					__m128 t3 = _mm_shuffle_ps(tr, tr, _MM_SHUFFLE(2, 3, 2, 3));

					__m128 q = _mm_mul_ps(r, r);
					__m128 di = _mm_sub_ps(_mm_add_ps(q, _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 3, 3, 3))), _mm_add_ps(_mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 1, 0, 2))));

					//127 [ RK | KJ | JI | IK ] 0
					__m128 kjik = _mm_mul_ps(r, _mm_shuffle_ps(r, r, _MM_SHUFFLE(2, 1, 0, 2)));
					//127 [RJ | KI | RJ | RI] 0
					__m128 jirr = _mm_mul_ps(r, _mm_shuffle_ps(r, r, _MM_SHUFFLE(1, 0, 3, 3)));
					__m128 dbl = _mm_set_ps1(2);

					__m128 p1 = _mm_mul_ps(dbl, _mm_addsub_ps(_mm_shuffle_ps(kjik, kjik, _MM_SHUFFLE(1, 1, 2, 2)), _mm_shuffle_ps(jirr, kjik, _MM_SHUFFLE(3, 3, 0, 0))));
					__m128 p2 = _mm_mul_ps(dbl, _mm_addsub_ps(_mm_shuffle_ps(jirr, jirr, _MM_SHUFFLE(2, 2, 2, 2)), _mm_shuffle_ps(jirr, jirr, _MM_SHUFFLE(1, 1, 2, 2))));
					/*
					p1:		127
					| JI + RK |
					| JI - RK |
					| JK + RI |
					| JK - RI |
					0

					p2:		127
					| IK + RJ |
					| IK - RJ |
					| IK + IK |
					|	 0	  |
					0

					di:		127
					|				0				|
					| (q[3] + q[2]) - (q[0] + q[1]) |
					| (q[3] + q[1]) - (q[2] + q[0]) |
					| (q[3] + q[0]) - (q[1] + q[2]) |
					0
					*/

					//row0 = 127 [     0 | p2[3] | p1[2] | di[0] ] 0
					//row1 = 127 [     0 | p1[0] | di[1] | p1[3] ] 0
					//row2 = 127 [     0 | di[2] | p1[1] | p2[2] ] 0
					//row3 = 127 [	   1 |  t[2] |  t[1] |  t[0] ] 0

					//p3 = 127 [ p2[3] | p2[2] | p1[2] | p1[1] ] 0
					__m128 p3 = _mm_shuffle_ps(p1, p2, _MM_SHUFFLE(3, 2, 2, 1));

					//row0 = 127 [ p1[2] | p2[3] | di[3] | di[0] ] 0
					__m128 row0 = _mm_shuffle_ps(di, p3, _MM_SHUFFLE(1, 3, 3, 0));
					row0 = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(1, 2, 3, 0));
					//row1 = 127 [ di[3] | di[1] | p1[0] | p1[3] ] 0
					__m128 row1 = _mm_shuffle_ps(p1, di, _MM_SHUFFLE(3, 1, 0, 3));
					row1 = _mm_shuffle_ps(row1, row1, _MM_SHUFFLE(3, 1, 2, 0));
					__m128 row2 = _mm_shuffle_ps(p3, di, _MM_SHUFFLE(3, 2, 0, 2));
					return Matrix(row0, row1, row2, ROW_ZZZO);
#endif
				}
				CLOAKENGINE_API CLOAK_CALL_MATH Quaternion::operator Vector() const
				{
					return Vector(Data);
				}

				CLOAKENGINE_API float CLOAK_CALL_MATH Quaternion::Length() const
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return QLength(Data);
#else
					__m128 l = _mm_sqrt_ps(_mm_dp_ps(Data, Data, 0xFF));
					return _mm_cvtss_f32(l);
#endif
				}
				CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH Quaternion::Normalize() const
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return Quaternion(QNormalize(Data));
#else
					return Quaternion(_mm_mul_ps(Data, _mm_rsqrt_ps(_mm_dp_ps(Data, Data, 0xFF))));
#endif
				}
				CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH Quaternion::Inverse() const
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return Quaternion(QInverse(Data));
#else
					return Quaternion(_mm_mul_ps(_mm_xor_ps(Data, FLAG_SIGN_NNNP), _mm_rsqrt_ps(_mm_dp_ps(Data, Data, 0xFF))));
#endif
				}
				CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH Quaternion::Conjugate() const
				{
					return Quaternion(_mm_xor_ps(Data, FLAG_SIGN_NNNP));
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Quaternion::ToAngularVelocity() const
				{
					/*Data = 127 [ R | K | J | I ] 0 */
					Vector n = Data;
					float d[4];
					_mm_store_ps(d, Data);
					float o = std::acos(d[3]);
					return n.Normalize() * o * 2;
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH Quaternion::Rotate(In const Vector& v) const 
				{
					/*Data		= 127 [ R | K | J | I ] 0	*/
					/*v.Data	= 127 [ ? | Z | Y | X ] 0	*/

#ifdef ALLOW_VARIANT_INSTRUCTIONS
					__m128 res = QVRot(Data, v.Data);
#else
					__m128 w = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(3, 3, 3, 3));
					__m128 r1 = _mm_mul_ps(Data, _mm_dp_ps(Data, v.Data, 0x7F));
					__m128 r2 = _mm_mul_ps(v.Data, _mm_sub_ps(_mm_mul_ps(w, w), _mm_dp_ps(Data, Data, 0x7F)));

					__m128 s1 = _mm_shuffle_ps(v.Data, v.Data, _MM_SHUFFLE(3, 0, 2, 1));
					__m128 s2 = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(3, 1, 0, 2));
					__m128 s3 = _mm_shuffle_ps(v.Data, v.Data, _MM_SHUFFLE(3, 1, 0, 2));
					__m128 s4 = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(3, 0, 2, 1));

					__m128 r3 = _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(s1, s2), _mm_mul_ps(s3, s4)), w);

					__m128 res = _mm_add_ps(_mm_mul_ps(ALL_TWO, _mm_add_ps(r1, r3)), r2);
#endif
					return Vector(res);
					
				}
				CLOAKENGINE_API EulerAngles CLOAK_CALL_MATH Quaternion::ToEulerAngles(In RotationOrder order) const
				{
					__m128 e1 = FLAG_SIGN_PPPP;
					__m128 e2 = FLAG_SIGN_PNPP;
					size_t apos[3];
					__m128 p = Data;
					switch (order)
					{
						case CloakEngine::API::Global::Math::RotationOrder::XYZ:
							apos[0] = 0;
							apos[1] = 1;
							apos[2] = 2;
							break;
						case CloakEngine::API::Global::Math::RotationOrder::XZY:
							p = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(3, 1, 2, 0));
							apos[0] = 0;
							apos[1] = 2;
							apos[2] = 1;
							e1 = FLAG_SIGN_NNPP;
							e2 = FLAG_SIGN_PPPP;
							break;
						case CloakEngine::API::Global::Math::RotationOrder::YXZ:
							p = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(3, 2, 0, 1));
							apos[0] = 1;
							apos[1] = 0;
							apos[2] = 2;
							e1 = FLAG_SIGN_NNPP;
							e2 = FLAG_SIGN_PPPP;
							break;
						case CloakEngine::API::Global::Math::RotationOrder::YZX:
							p = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(3, 1, 2, 0));
							apos[0] = 0;
							apos[1] = 2;
							apos[2] = 1;
							break;
						case CloakEngine::API::Global::Math::RotationOrder::ZXY:
							p = _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(3, 2, 0, 1));
							apos[0] = 1;
							apos[1] = 0;
							apos[2] = 2;
							break;
						case CloakEngine::API::Global::Math::RotationOrder::ZYX:
							apos[0] = 0;
							apos[1] = 1;
							apos[2] = 2;
							e1 = FLAG_SIGN_NNPP;
							e2 = FLAG_SIGN_PPPP;
							break;
						default:
							return EulerAngles();
					}

					__m128 t0 = _mm_shuffle_ps(p, p, _MM_SHUFFLE(3, 0, 0, 3));
					__m128 t1 = _mm_shuffle_ps(p, ALL_HALF, _MM_SHUFFLE(0, 0, 2, 1));
					t0 = _mm_xor_ps(_mm_mul_ps(_mm_mul_ps(t0, t1), ALL_TWO), e2);
					CLOAK_ALIGN float tf[4];
					_mm_store_ps(tf, t0);
					const float o2 = tf[0] + tf[1];
					float angle[3];
					angle[apos[1]] = std::asin(o2);
					if (o2 > QUATERNION_EULER_THRESHOLD || o2 < -QUATERNION_EULER_THRESHOLD)
					{
						angle[apos[0]] = std::atan2(tf[2], tf[3]);
						angle[apos[2]] = 0;
					}
					else
					{
						t0 = _mm_shuffle_ps(p, p, _MM_SHUFFLE(1, 0, 3, 3));
						t1 = _mm_shuffle_ps(p, p, _MM_SHUFFLE(1, 0, 2, 0));
						t0 = _mm_mul_ps(t0, t1);
						t1 = _mm_shuffle_ps(p, p, _MM_SHUFFLE(2, 1, 0, 1));
						__m128 t2 = _mm_shuffle_ps(p, p, _MM_SHUFFLE(2, 1, 1, 2));
						t1 = _mm_xor_ps(_mm_mul_ps(t1, t2), e1);
						t0 = _mm_add_ps(t0, t1);
						t0 = _mm_mul_ps(t0, ALL_TWO);
						_mm_store_ps(tf, t0);
						angle[apos[0]] = std::atan2(tf[0], 1 - tf[2]);
						angle[apos[2]] = std::atan2(tf[1], 1 - tf[3]);
					}
					return EulerAngles(angle[0], angle[1], angle[2]);
				}
				CLOAKENGINE_API Quaternion CLOAK_CALL_MATH Quaternion::RotationX(In float angle)
				{
					const float c = std::cos(angle / 2);
					const float s = std::sin(angle / 2);
					return Quaternion(_mm_set_ps(c, 0, 0, s));
				}
				CLOAKENGINE_API Quaternion CLOAK_CALL_MATH Quaternion::RotationY(In float angle)
				{
					const float c = std::cos(angle / 2);
					const float s = std::sin(angle / 2);
					return Quaternion(_mm_set_ps(c, 0, s, 0));
				}
				CLOAKENGINE_API Quaternion CLOAK_CALL_MATH Quaternion::RotationZ(In float angle)
				{
					const float c = std::cos(angle / 2);
					const float s = std::sin(angle / 2);
					return Quaternion(_mm_set_ps(c, s, 0, 0));
				}
				CLOAKENGINE_API Quaternion CLOAK_CALL_MATH Quaternion::Rotation(In float angleX, In float angleY, In float angleZ, In RotationOrder order)
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return Quaternion(QRotation2(angleX, angleY, angleZ, order));
#else
					const float axh = angleX / 2;
					const float ayh = angleY / 2;
					const float azh = angleZ / 2;
					const float sx = std::sin(axh);
					const float cx = std::cos(axh);
					const float sy = std::sin(ayh);
					const float cy = std::cos(ayh);
					const float sz = std::sin(azh);
					const float cz = std::cos(azh);
					switch (order)
					{
						case CloakEngine::API::Global::Math::RotationOrder::ZYX:
						{
							__m128 t0 = _mm_set_ps(cx, cz, cx, cy);
							__m128 t1 = _mm_set_ps(cy, sx, sy, sx);
							__m128 t2 = _mm_set_ps(cz, sy, cz, cz);
							__m128 t3 = _mm_set_ps(sx, cx, cy, cx);
							__m128 t4 = _mm_set_ps(sy, cy, sx, sy);
							__m128 t5 = _mm_set_ps(sz, sz, sz, sz);
							t0 = _mm_mul_ps(t0, _mm_mul_ps(t1, t2));
							t1 = _mm_mul_ps(t3, _mm_mul_ps(t4, t5));
							t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_PNPN));
							t0 = _mm_mul_ps(t0, _mm_rsqrt_ps(_mm_dp_ps(t0, t0, 0xFF)));
							return Quaternion(t0);
						}
						case CloakEngine::API::Global::Math::RotationOrder::YZX:
						{
							__m128 t0 = _mm_set_ps(cx, cy, cx, sx);
							__m128 t1 = _mm_set_ps(cy, sz, sy, cz);
							__m128 t2 = _mm_set_ps(cz, cx, cz, cy);
							__m128 t3 = _mm_set_ps(sx, sx, cy, cx);
							__m128 t4 = _mm_set_ps(sy, cz, sz, sy);
							__m128 t5 = _mm_set_ps(sz, sy, sx, sz);
							t0 = _mm_mul_ps(t0, _mm_mul_ps(t1, t2));
							t1 = _mm_mul_ps(t3, _mm_mul_ps(t4, t5));
							t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_NNPP));
							t0 = _mm_mul_ps(t0, _mm_rsqrt_ps(_mm_dp_ps(t0, t0, 0xFF)));
							return Quaternion(t0);
						}
						case CloakEngine::API::Global::Math::RotationOrder::ZXY:
						{
							__m128 t0 = _mm_set_ps(cx, cx, cx, cy);
							__m128 t1 = _mm_set_ps(cy, cy, sy, sx);
							__m128 t2 = _mm_set_ps(cz, sz, cz, cz);
							__m128 t3 = _mm_set_ps(sx, cz, cy, cx);
							__m128 t4 = _mm_set_ps(sy, sx, sx, sy);
							__m128 t5 = _mm_set_ps(sz, sy, sz, sz);
							t0 = _mm_mul_ps(t0, _mm_mul_ps(t1, t2));
							t1 = _mm_mul_ps(t3, _mm_mul_ps(t4, t5));
							t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_PNNP));
							t0 = _mm_mul_ps(t0, _mm_rsqrt_ps(_mm_dp_ps(t0, t0, 0xFF)));
							return Quaternion(t0);
						}
						case CloakEngine::API::Global::Math::RotationOrder::XZY:
						{
							__m128 t0 = _mm_set_ps(cx, cy, cz, cx);
							__m128 t1 = _mm_set_ps(cy, sz, sy, sy);
							__m128 t2 = _mm_set_ps(cz, cx, cx, sz);
							__m128 t3 = _mm_set_ps(sx, cz, cy, cy);
							__m128 t4 = _mm_set_ps(sy, sy, sz, cz);
							__m128 t5 = _mm_set_ps(sz, sx, sx, sx);
							t0 = _mm_mul_ps(t0, _mm_mul_ps(t1, t2));
							t1 = _mm_mul_ps(t3, _mm_mul_ps(t4, t5));
							t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_PPNN));
							t0 = _mm_mul_ps(t0, _mm_rsqrt_ps(_mm_dp_ps(t0, t0, 0xFF)));
							return Quaternion(t0);
						}
						case CloakEngine::API::Global::Math::RotationOrder::YXZ:
						{
							__m128 t0 = _mm_set_ps(cx, cz, cx, cy);
							__m128 t1 = _mm_set_ps(cy, sx, sy, sx);
							__m128 t2 = _mm_set_ps(cz, sy, cz, cz);
							__m128 t3 = _mm_set_ps(sx, cx, cy, cx);
							__m128 t4 = _mm_set_ps(sy, cy, sx, sy);
							__m128 t5 = _mm_set_ps(sz, sz, sz, sz);
							t0 = _mm_mul_ps(t0, _mm_mul_ps(t1, t2));
							t1 = _mm_mul_ps(t3, _mm_mul_ps(t4, t5));
							t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_NPPN));
							t0 = _mm_mul_ps(t0, _mm_rsqrt_ps(_mm_dp_ps(t0, t0, 0xFF)));
							return Quaternion(t0);
						}
						case CloakEngine::API::Global::Math::RotationOrder::XYZ:
						{
							__m128 t0 = _mm_set_ps(cx, cx, cx, cy);
							__m128 t1 = _mm_set_ps(cy, cy, sy, sx);
							__m128 t2 = _mm_set_ps(cz, sz, cz, cz);
							__m128 t3 = _mm_set_ps(sx, cz, cy, cx);
							__m128 t4 = _mm_set_ps(sy, sx, sx, sy);
							__m128 t5 = _mm_set_ps(sz, sy, sz, sz);
							t0 = _mm_mul_ps(t0, _mm_mul_ps(t1, t2));
							t1 = _mm_mul_ps(t3, _mm_mul_ps(t4, t5));
							t0 = _mm_add_ps(t0, _mm_xor_ps(t1, FLAG_SIGN_NPNP));
							t0 = _mm_mul_ps(t0, _mm_rsqrt_ps(_mm_dp_ps(t0, t0, 0xFF)));
							return Quaternion(t0);
						}
						default:
							break;
					}
					return Quaternion();
#endif
				}
				CLOAKENGINE_API Quaternion CLOAK_CALL_MATH Quaternion::Rotation(In const Vector& axis, In float angle)
				{
					return Quaternion(axis, angle);
				}
				CLOAKENGINE_API Quaternion CLOAK_CALL_MATH Quaternion::Rotation(In const EulerAngles& e, In RotationOrder order)
				{
					return Rotation(e.AngleX, e.AngleY, e.AngleZ, order);
				}
				CLOAKENGINE_API Quaternion CLOAK_CALL_MATH Quaternion::Rotation(In const Vector& begin, In const Vector& end)
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return Quaternion(QRotation1(begin.Data, end.Data));
#else
					const Vector a = Vector::Normalize(begin);
					const Vector b = Vector::Normalize(end);
					const float d = a.Dot(b);
					if (d > QUATERNION_REV_ROTATION_THRESHOLD) { return Quaternion(0, 0, 0, 1); }
					else if (d < -QUATERNION_REV_ROTATION_THRESHOLD) { return Quaternion(1, 0, 0, 0); }//Rotation about 180 degree
					const float s = sqrtf((1 + d) * 2);
					const float i = 1 / s;
					__m128 r = a.Cross(b).Data;
					r = _mm_or_ps(_mm_mul_ps(r, _mm_set_ps(0, i, i, i)), _mm_set_ps(s*0.5f, 0, 0, 0));
					r = _mm_mul_ps(r, _mm_rsqrt_ps(_mm_dp_ps(r, r, 0xFF)));//Normalize
					return r;
#endif
				}

				CLOAKENGINE_API float CLOAK_CALL_MATH Quaternion::Length(In const Quaternion& q)
				{
					return q.Length();
				}
				CLOAKENGINE_API Quaternion CLOAK_CALL_MATH Quaternion::Normalize(In const Quaternion& q)
				{
					return q.Normalize();
				}
				CLOAKENGINE_API Quaternion CLOAK_CALL_MATH Quaternion::Inverse(In const Quaternion& q)
				{
					return q.Inverse();
				}
				CLOAKENGINE_API Quaternion CLOAK_CALL_MATH Quaternion::Conjugate(In const Quaternion& q)
				{
					return q.Conjugate();
				}
				CLOAKENGINE_API Quaternion CLOAK_CALL_MATH Quaternion::NLerp(In const Quaternion& a, In const Quaternion& b, In float alpha)
				{
					alpha = min(1, max(0, alpha));
					Quaternion c = a;
					Quaternion d = b;
					c.Data = _mm_xor_ps(c.Data, _mm_and_ps(FLAG_SIGN_NNNN, _mm_shuffle_ps(c.Data, c.Data, _MM_SHUFFLE(3, 3, 3, 3))));
					d.Data = _mm_xor_ps(d.Data, _mm_and_ps(FLAG_SIGN_NNNN, _mm_shuffle_ps(d.Data, d.Data, _MM_SHUFFLE(3, 3, 3, 3))));
					return ((c*(1 - alpha)) + (b*alpha)).Normalize();
				}
				CLOAKENGINE_API Quaternion CLOAK_CALL_MATH Quaternion::SLerp(In const Quaternion& a, In const Quaternion& b, In float alpha)
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					return Quaternion(QSLerp(a.Data, b.Data, alpha));
#else
					alpha = min(1, max(0, alpha));
					__m128 dot = _mm_dp_ps(a.Data, b.Data, 0xFF);
					float d = _mm_cvtss_f32(dot);
					if (d > 0.995f) { return NLerp(a, b, alpha); }
					d = min(1, max(-1, d));
					float beta = alpha*std::acos(d);
					Quaternion v = (b - (a*d)).Normalize();
					return (a*cos(beta)) + (v*sin(beta));
#endif
				}
				CLOAKENGINE_API Matrix CLOAK_CALL_MATH Quaternion::ToMatrix(In const Quaternion& qat)
				{
					return static_cast<Matrix>(qat);
				}
				CLOAKENGINE_API EulerAngles CLOAK_CALL_MATH Quaternion::ToEulerAngles(In const Quaternion& q, In RotationOrder order) { return q.ToEulerAngles(order); }
				CLOAKENGINE_API void* CLOAK_CALL Quaternion::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void* CLOAK_CALL Quaternion::operator new[](In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void CLOAK_CALL Quaternion::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
				CLOAKENGINE_API void CLOAK_CALL Quaternion::operator delete[](In void* s) { return API::Global::Memory::MemoryPool::Free(s); }

				CLOAKENGINE_API CLOAK_CALL DualQuaternion::DualQuaternion(In const Quaternion& r, In const Quaternion& d) : Real(r), Dual(d) {}
				CLOAKENGINE_API CLOAK_CALL DualQuaternion::DualQuaternion(In const Quaternion& r) : Real(r), Dual(0, 0, 0, 0) {}
				CLOAKENGINE_API CLOAK_CALL DualQuaternion::DualQuaternion() : Real(0, 0, 0, 1), Dual(0, 0, 0, 0) {}
				CLOAKENGINE_API CLOAK_CALL DualQuaternion::DualQuaternion(In const DualQuaternion& q) : Real(q.Real), Dual(q.Dual) {}

				CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH DualQuaternion::operator*(In float s) const
				{
					return DualQuaternion(*this) *= s;
				}
				CLOAKENGINE_API DualQuaternion& CLOAK_CALL_MATH DualQuaternion::operator*=(In float s)
				{
					Real *= s;
					Dual *= s;
					return *this;
				}
				CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH DualQuaternion::operator*(In const DualQuaternion& q) const
				{
					return DualQuaternion(*this) *= q;
				}
				CLOAKENGINE_API DualQuaternion& CLOAK_CALL_MATH DualQuaternion::operator*=(In const DualQuaternion& q)
				{
					Dual = (q.Real*Dual) + (q.Dual*Real);
					Real = q.Real*Real;
					return *this;
				}
				CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH DualQuaternion::operator+(In const DualQuaternion& q) const
				{
					return DualQuaternion(*this) += q;
				}
				CLOAKENGINE_API DualQuaternion& CLOAK_CALL_MATH DualQuaternion::operator+=(In const DualQuaternion& q)
				{
					Real += q.Real;
					Dual += q.Dual;
					return *this;
				}
				CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH DualQuaternion::operator-(In const DualQuaternion& q) const
				{
					return DualQuaternion(*this) -= q;
				}
				CLOAKENGINE_API DualQuaternion& CLOAK_CALL_MATH DualQuaternion::operator-=(In const DualQuaternion& q)
				{
					Real -= q.Real;
					Dual -= q.Dual;
					return *this;
				}
				CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH DualQuaternion::operator-() const
				{
					return DualQuaternion(-Real, -Dual);
				}
				CLOAKENGINE_API CLOAK_CALL_MATH DualQuaternion::operator Matrix() const
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					__m128 d[4];
					DQMatrix(d, Real.Data, Dual.Data);
					return Matrix(d[0], d[1], d[2], d[3]);
#else
					__m128 l = _mm_rsqrt_ps(_mm_dp_ps(Real.Data, Real.Data, 0xFF));
					//r = 127 [RKJI] 0
					__m128 r = _mm_mul_ps(Real.Data, l);
					__m128 d = _mm_mul_ps(Dual.Data, l);

					//t = 2 * 127[R | -K | -J | -I]0
					__m128 td = _mm_mul_ps(d, ALL_TWO);
					__m128 tr = _mm_xor_ps(r, FLAG_SIGN_NNNP);
					__m128 t1 = _mm_shuffle_ps(td, td, _MM_SHUFFLE(0, 1, 2, 3));
					__m128 t2 = _mm_shuffle_ps(tr, tr, _MM_SHUFFLE(0, 1, 0, 1));
					__m128 t3 = _mm_shuffle_ps(tr, tr, _MM_SHUFFLE(2, 3, 2, 3));
					__m128 t4 = _mm_hsub_ps(_mm_mul_ps(td, t2), _mm_mul_ps(t1, t3));
					__m128 t5 = _mm_hadd_ps(_mm_mul_ps(td, t3), _mm_mul_ps(t1, t2));
					t1 = _mm_addsub_ps(_mm_shuffle_ps(t5, t4, _MM_SHUFFLE(3, 2, 1, 0)), _mm_shuffle_ps(t4, t5, _MM_SHUFFLE(2, 3, 0, 1)));
					__m128 t = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(2, 1, 3, 0));
					
					__m128 q = _mm_mul_ps(r, r);
					__m128 di = _mm_sub_ps(_mm_add_ps(q, _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 3, 3, 3))), _mm_add_ps(_mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 1, 0, 2))));

					//127 [ RK | KJ | JI | IK ] 0
					__m128 kjik = _mm_mul_ps(r, _mm_shuffle_ps(r, r, _MM_SHUFFLE(2, 1, 0, 2)));
					//127 [RJ | KI | RJ | RI] 0
					__m128 jirr = _mm_mul_ps(r, _mm_shuffle_ps(r, r, _MM_SHUFFLE(1, 0, 3, 3)));

					__m128 dbl = _mm_set_ps1(2);

					__m128 p1 = _mm_mul_ps(dbl, _mm_addsub_ps(_mm_shuffle_ps(kjik, kjik, _MM_SHUFFLE(1, 1, 2, 2)), _mm_shuffle_ps(jirr, kjik, _MM_SHUFFLE(3, 3, 0, 0))));
					__m128 p2 = _mm_mul_ps(dbl, _mm_addsub_ps(_mm_shuffle_ps(jirr, jirr, _MM_SHUFFLE(2, 2, 2, 2)), _mm_shuffle_ps(jirr, jirr, _MM_SHUFFLE(1, 1, 2, 2))));
					/*
					p1:		127
						| JI + RK |
						| JI - RK |
						| JK + RI |
						| JK - RI |
							0

					p2:		127
						| IK + RJ |
						| IK - RJ |
						| IK + IK |
						|	 0	  |
							0

					di:		127
						|				0				|
						| (q[3] + q[2]) - (q[0] + q[1]) |
						| (q[3] + q[1]) - (q[2] + q[0]) |
						| (q[3] + q[0]) - (q[1] + q[2]) |
							0
					*/

					//row0 = 127 [     0 | p2[3] | p1[2] | di[0] ] 0
					//row1 = 127 [     0 | p1[0] | di[1] | p1[3] ] 0
					//row2 = 127 [     0 | di[2] | p1[1] | p2[2] ] 0
					//row3 = 127 [	   1 |  t[2] |  t[1] |  t[0] ] 0

					//p3 = 127 [ p2[3] | p2[2] | p1[2] | p1[1] ] 0
					__m128 p3 = _mm_shuffle_ps(p1, p2, _MM_SHUFFLE(3, 2, 2, 1));

					//row0 = 127 [ p1[2] | p2[3] | di[3] | di[0] ] 0
					__m128 row0 = _mm_shuffle_ps(di, p3, _MM_SHUFFLE(1, 3, 3, 0));
					row0 = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(1, 2, 3, 0));
					//row1 = 127 [ di[3] | di[1] | p1[0] | p1[3] ] 0
					__m128 row1 = _mm_shuffle_ps(p1, di, _MM_SHUFFLE(3, 1, 0, 3));
					row1 = _mm_shuffle_ps(row1, row1, _MM_SHUFFLE(3, 1, 2, 0));
					__m128 row2 = _mm_shuffle_ps(p3, di, _MM_SHUFFLE(3, 2, 0, 2));
					__m128 row3 = _mm_shuffle_ps(t, _mm_shuffle_ps(t, ALL_ONE, _MM_SHUFFLE(0, 0, 2, 2)), _MM_SHUFFLE(2, 0, 1, 0));
					return Matrix(row0, row1, row2, row3);
#endif
				}
				CLOAKENGINE_API const Quaternion CLOAK_CALL_MATH DualQuaternion::operator*(In const Quaternion& q) const
				{
					return Quaternion(q)*(*this);
				}

				CLOAKENGINE_API float CLOAK_CALL_MATH DualQuaternion::Length() const
				{
					return Real.Length();
				}
				CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH DualQuaternion::Normalize() const
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					__m128 d[2];
					DQNormalize(d, Real.Data, Dual.Data);
					return DualQuaternion(d[0], d[1]);
#else
					__m128 l = _mm_rsqrt_ps(_mm_dp_ps(Real.Data, Real.Data, 0xFF));
					return DualQuaternion(_mm_mul_ps(Real.Data, l), _mm_mul_ps(Dual.Data, l));
#endif
				}
				CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH DualQuaternion::Inverse() const
				{
					Quaternion r = Real.Inverse();
					return DualQuaternion(r, -r * Dual * r);
				}
				CLOAKENGINE_API const DualQuaternion CLOAK_CALL_MATH DualQuaternion::Conjugate() const
				{
					return DualQuaternion(Real.Conjugate(), -Dual.Conjugate());
				}

				CLOAKENGINE_API Quaternion CLOAK_CALL_MATH DualQuaternion::GetRotationPart() const
				{
					return Real;
				}
				CLOAKENGINE_API Vector CLOAK_CALL_MATH DualQuaternion::GetTranslationPart() const
				{
					return Vector(_mm_mul_ps(Dual.Data, ALL_TWO));
				}
				
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::Inverse(In const DualQuaternion& q)
				{
					return q.Inverse();
				}
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::Normalize(In const DualQuaternion& q)
				{
					return q.Normalize();
				}
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::Conjugate(In const DualQuaternion& q)
				{
					return q.Conjugate();
				}
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::Translate(In float X, In float Y, In float Z)
				{
					return DualQuaternion(Quaternion(0, 0, 0, 1), Quaternion(_mm_mul_ps(_mm_set_ps(0, Z, Y, X), ALL_HALF)));
				}
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::Translate(In const Vector& v)
				{
					__m128 d = _mm_mul_ps(v.Data, ALL_HALF);
					d = _mm_shuffle_ps(d, _mm_shuffle_ps(d, ALL_ZERO, _MM_SHUFFLE(2, 2, 2, 2)), _MM_SHUFFLE(2, 1, 1, 0));
					return DualQuaternion(Quaternion(0, 0, 0, 1), Quaternion(d));
				}
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::RotationX(In float angle)
				{
					return Rotation(Vector(-1, 0, 0), angle);
				}
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::RotationY(In float angle)
				{
					return Rotation(Vector(0, -1, 0), angle);
				}
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::RotationZ(In float angle)
				{
					return Rotation(Vector(0, 0, -1), angle);
				}
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::Rotation(In float angleX, In float angleY, In float angleZ, In RotationOrder order)
				{
					return DualQuaternion(Quaternion::Rotation(angleX, angleY, angleZ, order));					
				}
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::Rotation(In const Vector& axis, In float angle)
				{
					return DualQuaternion(Quaternion(axis, angle));
				}
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::Rotation(In const EulerAngles& e, In RotationOrder order)
				{
					return DualQuaternion(Quaternion::Rotation(e.AngleX, e.AngleY, e.AngleZ, order));
				}
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::Rotation(In const Vector& begin, In const Vector& end)
				{
					return DualQuaternion(Quaternion::Rotation(begin, end));
				}
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::NLerp(In const DualQuaternion& a, In const DualQuaternion& b, In float alpha)
				{
					alpha = min(1, max(0, alpha));
					return DualQuaternion(Quaternion::NLerp(a.Real, b.Real, alpha), (a.Dual*(1 - alpha)) + (b.Dual*alpha));
				}
				CLOAKENGINE_API DualQuaternion CLOAK_CALL_MATH DualQuaternion::SLerp(In const DualQuaternion& a, In const DualQuaternion& b, In float alpha)
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					__m128 d[2];
					DQSLerp(d, a.Real.Data, a.Dual.Data, b.Real.Data, b.Dual.Data, alpha);
					return DualQuaternion(d[0], d[1]);
#else
					alpha = min(1, max(0, alpha));
					__m128 dot = _mm_dp_ps(a.Real.Data, b.Real.Data, 0xFF);
					float d = _mm_cvtss_f32(dot);
					DualQuaternion t = d < 0 ? -b : b;
					DualQuaternion diff = DualQuaternion::Conjugate(a)*t;
					CLOAK_ALIGN float real[4];
					_mm_store_ps(real, diff.Real.Data);
					__m128 inv = _mm_rsqrt_ps(_mm_dp_ps(diff.Real.Data, diff.Real.Data, 0x7F));
					__m128 dir = _mm_mul_ps(diff.Real.Data, inv);
					__m128 pitch = _mm_xor_ps(_mm_mul_ps(ALL_TWO, _mm_mul_ps(_mm_shuffle_ps(diff.Dual.Data, diff.Dual.Data, _MM_SHUFFLE(3, 3, 3, 3)), inv)), FLAG_SIGN_NNNN);
					__m128 dr = _mm_mul_ps(_mm_mul_ps(dir, pitch), _mm_mul_ps(_mm_shuffle_ps(diff.Real.Data, diff.Real.Data, _MM_SHUFFLE(3, 3, 3, 3)), ALL_HALF));
					__m128 mom = _mm_mul_ps(_mm_sub_ps(diff.Dual.Data, dr), inv);
					
					pitch = _mm_mul_ps(pitch, _mm_set_ps1(alpha));
					const float angle = std::acos(real[3])*alpha;

					__m128 sinA = _mm_set_ps1(std::sin(angle));
					__m128 cosA = _mm_set_ps1(std::cos(angle));
					__m128 sinD = _mm_mul_ps(dir, sinA);
					__m128 cosD = _mm_add_ps(_mm_mul_ps(sinA, mom), _mm_mul_ps(_mm_mul_ps(pitch, ALL_HALF), _mm_mul_ps(cosA, dir)));
					cosD = _mm_shuffle_ps(cosD, _mm_shuffle_ps(cosD, _mm_mul_ps(_mm_xor_ps(pitch, FLAG_SIGN_NNNN), _mm_mul_ps(sinA, ALL_HALF)), _MM_SHUFFLE(0, 0, 2, 2)), _MM_SHUFFLE(3, 1, 1, 0));
					sinD = _mm_shuffle_ps(sinD, _mm_shuffle_ps(sinD, cosA, _MM_SHUFFLE(0, 0, 2, 2)), _MM_SHUFFLE(3, 1, 1, 0));
					return a*DualQuaternion(Quaternion(sinD), Quaternion(cosD));
					//return a*DualQuaternion(Quaternion(sinD.X,sinD.Y,sinD.Z,cosA),Quaternion(cosD.X,cosD.Y,cosD.Z,-pitch*0.5f*sinA)); (Order I J K R)
					/*
					const float dot = (begin.Real.R*end.Real.R) + (begin.Real.I*end.Real.I) + (begin.Real.J*end.Real.J) + (begin.Real.K*end.Real.K);
					DualQuaternion target = dot < 0 ? -end : end;
					DualQuaternion diff = begin.GetConjugate()*target;

					Space3D::Vector vr(diff.Real.I, diff.Real.J, diff.Real.K);
					Space3D::Vector vd(diff.Dual.I, diff.Dual.J, diff.Dual.K);
					const float inv = 1 / vr.Length();

					float angle = 2 * astd::cos(diff.Real.R);
					float pitch = -2 * diff.Dual.R * inv;
					Space3D::Vector dir = vr*inv;
					Space3D::Vector mom = (vd - (dir*pitch*diff.Real.R*0.5f))*inv;

					angle *= t;
					pitch *= t;

					const float sinA = std::sin(0.5f*angle);
					const float cosA = std::cos(0.5f*angle);
					Space3D::Vector sinD = dir*sinA;
					Space3D::Vector cosD = (sinA*mom) + (pitch*0.5f*cosA*dir);
					return begin*DualQuaternion(Quaternion(sinD.X, sinD.Y, sinD.Z, cosA), Quaternion(cosD.X, cosD.Y, cosD.Z, -pitch*0.5f*sinA));
					*/
#endif
				}
				CLOAKENGINE_API void* CLOAK_CALL DualQuaternion::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void* CLOAK_CALL DualQuaternion::operator new[](In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
				CLOAKENGINE_API void CLOAK_CALL DualQuaternion::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
				CLOAKENGINE_API void CLOAK_CALL DualQuaternion::operator delete[](In void* s) { return API::Global::Memory::MemoryPool::Free(s); }

				const __m128 g_Neg3Mask = _mm_castsi128_ps(_mm_set_epi32(0x80000000, 0, 0, 0));
				const __m128 g_Neg2Mask = _mm_castsi128_ps(_mm_set_epi32(0, 0x80000000, 0, 0));
				namespace Space2D {
					CLOAKENGINE_API CLOAK_CALL Point::Point() { X = 0; Y = 0; }
					CLOAKENGINE_API CLOAK_CALL Point::Point(In float x, In float y)
					{
						X = x;
						Y = y;
					}
					CLOAKENGINE_API CLOAK_CALL_MATH Point::operator Vector() const { return Vector(X, Y); }
					CLOAKENGINE_API inline const Space2D::Point CLOAK_CALL_MATH Space2D::Point::operator+(const Space2D::Vector& b) const { return Space2D::Point(X + b.X, Y + b.Y); }
					CLOAKENGINE_API inline const Space2D::Point CLOAK_CALL_MATH Space2D::Point::operator-(const Space2D::Vector& b) const { return Space2D::Point(X - b.X, Y - b.Y); }

					CLOAKENGINE_API CLOAK_CALL Vector::Vector() { X = 0; Y = 0; A = 1.0f; }
					CLOAKENGINE_API CLOAK_CALL Vector::Vector(In float x, In float y)
					{
						X = x;
						Y = y;
						A = 1.0f;
					}
					CLOAKENGINE_API CLOAK_CALL Vector::Vector(In float x, In float y, In float a)
					{
						X = x;
						Y = y;
						A = a;
					}
					CLOAKENGINE_API CLOAK_CALL_MATH Vector::operator Point() const { return Point(X, Y); }
					CLOAKENGINE_API inline float CLOAK_CALL_MATH Space2D::Vector::operator*(In const Space2D::Vector& b) const { return (X*b.Y) - (Y*b.X); }
					CLOAKENGINE_API inline const Space2D::Vector CLOAK_CALL_MATH Space2D::Vector::operator*(In const float& b) const { return b*(*this); }
					CLOAKENGINE_API inline const Space2D::Vector CLOAK_CALL_MATH Space2D::Vector::operator+(In const Space2D::Vector& b) const { return Space2D::Vector(X + b.X, Y + b.Y, 1.0f); }
					CLOAKENGINE_API inline const Space2D::Vector CLOAK_CALL_MATH Space2D::Vector::operator-(In const Space2D::Vector& b) const { return Space2D::Vector(X - b.X, Y - b.Y, 1.0f); }
					CLOAKENGINE_API inline float CLOAK_CALL_MATH Space2D::Vector::Length() const { return sqrtf((X*X) + (Y*Y)); }
					CLOAKENGINE_API inline const Space2D::Vector CLOAK_CALL_MATH Space2D::Vector::Normalize() const { float l = Length(); return Space2D::Vector(X / l, Y / l, A); }
					CLOAKENGINE_API float CLOAK_CALL DotProduct(In const Vector& a, In const Vector& b) { return (a.X*b.X) + (a.Y*b.Y); }
				}
				CLOAKENGINE_API inline const Space2D::Vector CLOAK_CALL operator*(In const float& a, In const Space2D::Vector& b) { return Space2D::Vector(b.X*a, b.Y*a, b.A); }
			}
		}
	}
	namespace Impl {
		namespace Global {
			namespace Math {
				void CLOAK_CALL Initialize(In bool sse41, In bool avx)
				{
#ifdef ALLOW_VARIANT_INSTRUCTIONS
					API::Global::Math::VectorFloor				= API::Global::Math::SSE3::Floor;
					API::Global::Math::VectorCeil				= API::Global::Math::SSE3::Ceil;
					API::Global::Math::PlaneInit				= API::Global::Math::SSE3::PlaneInit;
					API::Global::Math::PlaneSignedDistance		= API::Global::Math::SSE3::PlaneSDistance;
					API::Global::Math::MatrixFrustrum			= API::Global::Math::SSE3::MatrixFrustrum;
					API::Global::Math::VectorDot				= API::Global::Math::SSE3::VectorDot;
					API::Global::Math::VectorLength				= API::Global::Math::SSE3::VectorLength;
					API::Global::Math::VectorNormalize			= API::Global::Math::SSE3::VectorNormalize;
					API::Global::Math::VectorBarycentric		= API::Global::Math::SSE3::VectorBarycentric;
					API::Global::Math::DQSLerp					= API::Global::Math::SSE3::DQSLerp;
					API::Global::Math::DQNormalize				= API::Global::Math::SSE3::DQNormalize;
					API::Global::Math::DQMatrix					= API::Global::Math::SSE3::DQMatrix;
					API::Global::Math::QSLerp					= API::Global::Math::SSE3::QSLerp;
					API::Global::Math::QVRot					= API::Global::Math::SSE3::QVRot;
					API::Global::Math::QRotation1				= API::Global::Math::SSE3::QRotation1;
					API::Global::Math::QRotation2				= API::Global::Math::SSE3::QRotation2;
					API::Global::Math::QLength					= API::Global::Math::SSE3::QLength;
					API::Global::Math::QNormalize				= API::Global::Math::SSE3::QNormalize;
					API::Global::Math::QInverse					= API::Global::Math::SSE3::QInverse;
					API::Global::Math::QMatrix					= API::Global::Math::SSE3::QMatrix;
					API::Global::Math::MatrixMul				= API::Global::Math::SSE3::MatrixMul;

					if (sse41)
					{
						API::Global::Math::VectorFloor			= API::Global::Math::SSE4::Floor;
						API::Global::Math::VectorCeil			= API::Global::Math::SSE4::Ceil;
						API::Global::Math::PlaneInit			= API::Global::Math::SSE4::PlaneInit;
						API::Global::Math::PlaneSignedDistance	= API::Global::Math::SSE4::PlaneSDistance;
						API::Global::Math::MatrixFrustrum		= API::Global::Math::SSE4::MatrixFrustrum;
						API::Global::Math::VectorDot			= API::Global::Math::SSE4::VectorDot;
						API::Global::Math::VectorLength			= API::Global::Math::SSE4::VectorLength;
						API::Global::Math::VectorNormalize		= API::Global::Math::SSE4::VectorNormalize;
						API::Global::Math::VectorBarycentric	= API::Global::Math::SSE4::VectorBarycentric;
						API::Global::Math::DQSLerp				= API::Global::Math::SSE4::DQSLerp;
						API::Global::Math::DQNormalize			= API::Global::Math::SSE4::DQNormalize;
						API::Global::Math::DQMatrix				= API::Global::Math::SSE4::DQMatrix;
						API::Global::Math::QSLerp				= API::Global::Math::SSE4::QSLerp;
						//API::Global::Math::QVRot				= API::Global::Math::SSE4::QVRot;
						API::Global::Math::QRotation1			= API::Global::Math::SSE4::QRotation1;
						API::Global::Math::QRotation2			= API::Global::Math::SSE4::QRotation2;
						API::Global::Math::QLength				= API::Global::Math::SSE4::QLength;
						API::Global::Math::QNormalize			= API::Global::Math::SSE4::QNormalize;
						API::Global::Math::QInverse				= API::Global::Math::SSE4::QInverse;
						API::Global::Math::QMatrix				= API::Global::Math::SSE4::QMatrix;
					}
					if (avx)
					{
						API::Global::Math::MatrixMul			= API::Global::Math::AVX::MatrixMul;
					}
#endif
				}
			}
		}
	}
}
