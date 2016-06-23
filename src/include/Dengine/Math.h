#ifndef DENGINE_MATH_H
#define DENGINE_MATH_H

#include <Dengine/Common.h>
#include <math.h>

#define squared(x) (x * x)
#define absolute(x) ((x) > 0 ? (x) : -(x))

/* VECTORS */
typedef union v2
{
	struct { f32 x, y; };
	struct { f32 w, h; };
	f32 e[2];
} v2;

typedef union v3
{
	struct { f32 x, y, z; };
    struct { f32 r, g, b; };
	f32 e[3];
} v3;

typedef union v4
{
	struct { f32 x, y, z, w; };
	struct { f32 r, g, b, a; };
	f32 e[4];
} v4;

INTERNAL inline v2 V2(const f32 x, const f32 y)
{
	v2 result = {x, y};
	return result;
}
INTERNAL inline v3 V3(const f32 x, const f32 y, const f32 z)
{
	v3 result = {x, y, z};
	return result;
}
INTERNAL inline v4 V4(const f32 x, const f32 y, const f32 z, const f32 w)
{
	v4 result = {x, y, z, w};
	return result;
}

/*
	DEFINE_VECTOR_MATH
	Automagically generate basic vector function using the C preprocessor
	where ##num is replaced by the valued in DEFINE_VECTOR_MATH(num) when it is
	called, i.e. DEFINE_VECTOR_MATH(2)

	An example of this generates,
	INTERNAL inline v2 v2_add(const v2 a, const v2 b)
	{
		v2 result;
		for (i32 i = 0; i < 2; i++) { result.data[i] = a.data[i] + b.data[i]; }
		return result;
	}

	This is repeated for v3 and v4 for the basic math operations
 */

#define DEFINE_VECTOR_MATH(num) \
INTERNAL inline v##num v##num##_add(const v##num a, const v##num b) \
{ \
	v##num result; \
	for (i32 i = 0; i < ##num; i++) { result.e[i] = a.e[i] + b.e[i]; } \
	return result; \
} \
INTERNAL inline v##num v##num##_sub(const v##num a, const v##num b) \
{ \
	v##num result; \
	for (i32 i = 0; i < ##num; i++) { result.e[i] = a.e[i] - b.e[i]; } \
	return result; \
} \
INTERNAL inline v##num v##num##_scale(const v##num a, const f32 b) \
{ \
	v##num result; \
	for (i32 i = 0; i < ##num; i++) { result.e[i] = a.e[i] * b; } \
	return result; \
} \
INTERNAL inline v##num v##num##_mul(const v##num a, const v##num b) \
{ \
	v##num result; \
	for (i32 i = 0; i < ##num; i++) { result.e[i] = a.e[i] * b.e[i]; } \
	return result; \
} \
INTERNAL inline f32 v##num##_dot(const v##num a, const v##num b) \
{ \
	/* \
	   DOT PRODUCT \
	   Two vectors with dot product equals |a||b|cos(theta) \
	   |a|   |d| \
	   |b| . |e| = (ad + be + cf) \
	   |c|   |f| \
	 */ \
	f32 result = 0; \
	for (i32 i = 0; i < ##num; i++) { result += (a.e[i] * b.e[i]); } \
	return result; \
} \
INTERNAL inline b32 v##num##_equals(const v##num a, const v##num b) \
{ \
	b32 result = TRUE; \
	for (i32 i = 0; i < ##num; i++) \
		if (a.e[i] != b.e[i]) result = FALSE; \
	return result; \
} \

DEFINE_VECTOR_MATH(2);
DEFINE_VECTOR_MATH(3);
DEFINE_VECTOR_MATH(4);

INTERNAL inline v3 v3_cross(const v3 a, const v3 b)
{
	/*
	   CROSS PRODUCT
	   Generate a perpendicular vector to the 2 vectors
	   |a|   |d|   |bf - ce|
	   |b| x |e| = |cd - af|
	   |c|   |f|   |ae - be|
	 */
	v3 result;
	result.e[0] = (a.e[1] * b.e[2]) - (a.e[2] * b.e[1]);
	result.e[1] = (a.e[2] * b.e[0]) - (a.e[0] * b.e[2]);
	result.e[2] = (a.e[0] * b.e[1]) - (a.e[1] * b.e[0]);
	return result;
}

/* MATRICES */
typedef union mat4
{
    v4 col[4];
    f32 e[4][4];
} mat4;

INTERNAL inline mat4 mat4_identity()
{
	mat4 result    = {0};
	result.e[0][0] = 1;
	result.e[1][1] = 1;
	result.e[2][2] = 1;
	result.e[3][3] = 1;
	return result;
}

INTERNAL inline mat4 mat4_ortho(const f32 left, const f32 right,
                                const f32 bottom, const f32 top,
                                const f32 zNear, const f32 zFar)
{
	mat4 result    = mat4_identity();
	result.e[0][0] = +2.0f   / (right - left);
	result.e[1][1] = +2.0f   / (top   - bottom);
	result.e[2][2] = -2.0f   / (zFar  - zNear);

	result.e[3][0] = -(right + left)   / (right - left);
	result.e[3][1] = -(top   + bottom) / (top   - bottom);
	result.e[3][2] = -(zFar  + zNear)  / (zFar  - zNear);

	return result;
}

INTERNAL inline mat4 mat4_translate(const f32 x, const f32 y, const f32 z)
{
	mat4 result = mat4_identity();
	result.e[3][0] = x;
	result.e[3][1] = y;
	result.e[3][2] = z;
	return result;
}

INTERNAL inline mat4 mat4_rotate(const f32 radians, const f32 x, const f32 y,
                                 const f32 z)
{
	mat4 result = mat4_identity();
	f32 sinVal = sinf(radians);
	f32 cosVal = cosf(radians);

	result.e[0][0] = (cosVal + (squared(x) * (1.0f - cosVal)));
	result.e[0][1] = ((y * z * (1.0f - cosVal)) + (z * sinVal));
	result.e[0][2] = ((z * x * (1.0f - cosVal)) - (y * sinVal));

	result.e[1][0] = ((x * y * (1.0f - cosVal)) - (z * sinVal));
	result.e[1][1] = (cosVal + (squared(y) * (1.0f - cosVal)));
	result.e[1][2] = ((z * y * (1.0f - cosVal)) + (x * sinVal));

	result.e[2][0] = ((x * z * (1.0f - cosVal)) + (y * sinVal));
	result.e[2][1] = ((y * z * (1.0f - cosVal)) - (x * sinVal));
	result.e[2][2] = (cosVal + (squared(z) * (1.0f - cosVal)));

	result.e[3][3] = 1;

	return result;
}

INTERNAL inline mat4 mat4_scale(const f32 x, const f32 y, const f32 z)
{
	mat4 result    = {0};
	result.e[0][0] = x;
	result.e[1][1] = y;
	result.e[2][2] = z;
	result.e[3][3] = 1;
	return result;
}

INTERNAL inline mat4 mat4_mul(const mat4 a, const mat4 b)
{
	mat4 result = {0};
	for (i32 j = 0; j < 4; j++) {
		for (i32 i = 0; i < 4; i++) {
			result.e[j][i] = a.e[0][i] * b.e[j][0]
			               + a.e[1][i] * b.e[j][1]
			               + a.e[2][i] * b.e[j][2]
			               + a.e[3][i] * b.e[j][3];
		}
	}

	return result;
}


INTERNAL inline v4 mat4_mul_v4(const mat4 a, const v4 b)
{
	v4 result = {0};
	result.x =
	    a.e[0][0] * b.x + a.e[0][1] * b.y + a.e[0][2] * b.z + a.e[0][3] * b.w;
	result.y =
	    a.e[1][0] * b.x + a.e[1][1] * b.y + a.e[1][2] * b.z + a.e[1][3] * b.w;
	result.z =
	    a.e[2][0] * b.x + a.e[2][1] * b.y + a.e[2][2] * b.z + a.e[2][3] * b.w;
	result.w =
	    a.e[3][0] * b.x + a.e[3][1] * b.y + a.e[3][2] * b.z + a.e[3][3] * b.w;

	return result;
}

#endif
