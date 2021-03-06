#ifndef DENGINE_MATH_H
#define DENGINE_MATH_H

#include <math.h>

#include "Dengine/Common.h"

#define MATH_PI 3.14159265359f
#define ABS(x) ((x) > 0 ? (x) : -(x))
#define DEGREES_TO_RADIANS(x) ((x * (MATH_PI / 180.0f)))
#define RADIANS_TO_DEGREES(x) ((x * (180.0f / MATH_PI)))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SQUARED(x) ((x) * (x))
#define SQRT(x) (sqrtf(x))

typedef f32 Radians;
typedef f32 Degrees;

INTERNAL inline f32 math_acosf(f32 a)
{
	f32 result = acosf(a);
	return result;
}

INTERNAL inline Radians math_cosf(Radians a)
{
	Radians result = cosf(a);
	return result;
}

INTERNAL inline Radians math_sinf(Radians a)
{
	Radians result = sinf(a);
	return result;
}

INTERNAL inline f32 math_atan2f(f32 y, f32 x)
{
	f32 result = atan2f(y, x);
	return result;
}

INTERNAL inline f32 math_tanf(Radians angle)
{
	f32 result = tanf(angle);
	return result;
}


/* VECTORS */
typedef union v2
{
	struct { f32 x, y; };
	struct { f32 w, h; };
	struct { f32 min, max; };
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
	v2 vec2[2];
} v4;

typedef struct Rect
{
	v2 min;
	v2 max;
} Rect;

INTERNAL inline v2 V2i(const i32 x, const i32 y)
{
	v2 result = {CAST(f32)x, CAST(f32)y};
	return result;
}
INTERNAL inline v2 V2(const f32 x, const f32 y)
{
	v2 result = {x, y};
	return result;
}
INTERNAL inline v3 V3i(const i32 x, const i32 y, const i32 z)
{
	v3 result = {CAST(f32)x, CAST(f32)y, CAST(f32)z};
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

#define DEFINE_VECTOR_FLOAT_MATH(num) \
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
INTERNAL inline v##num v##num##_hadamard(const v##num a, const v##num b) \
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

DEFINE_VECTOR_FLOAT_MATH(2);
DEFINE_VECTOR_FLOAT_MATH(3);
DEFINE_VECTOR_FLOAT_MATH(4);

INTERNAL inline f32 v2_lengthSq(const v2 a, const v2 b)
{
	f32 x      = b.x - a.x;
	f32 y      = b.y - a.y;
	f32 result = (SQUARED(x) + SQUARED(y));
	return result;
}

INTERNAL inline f32 v2_magnitude(const v2 a, const v2 b)
{
	f32 lengthSq = v2_lengthSq(a, b);
	f32 result = SQRT(lengthSq);
	return result;
}

INTERNAL inline v2 v2_normalise(const v2 a)
{
	f32 magnitude = v2_magnitude(V2(0, 0), a);
	v2 result     = V2(a.x, a.y);
	result        = v2_scale(a, 1 / magnitude);
	return result;
}

INTERNAL inline b32 v2_intervalsOverlap(v2 a, v2 b)
{
	b32 result = FALSE;
	
	f32 lenOfA = a.max - a.min;
	f32 lenOfB = b.max - b.min;

	if (lenOfA > lenOfB)
	{
		v2 tmp = a;
		a      = b;
		b      = tmp;
	}

	if ((a.min >= b.min && a.min <= b.max) ||
	    (a.max >= b.min && a.max <= b.max))
	{
		result = TRUE;
	}

	return result;
}

INTERNAL inline v2 v2_perpendicular(const v2 a) {
	v2 result = {a.y, -a.x};
	return result;
}

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

	// Column/row
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

	result.e[0][0] = (cosVal + (SQUARED(x) * (1.0f - cosVal)));
	result.e[0][1] = ((y * z * (1.0f - cosVal)) + (z * sinVal));
	result.e[0][2] = ((z * x * (1.0f - cosVal)) - (y * sinVal));

	result.e[1][0] = ((x * y * (1.0f - cosVal)) - (z * sinVal));
	result.e[1][1] = (cosVal + (SQUARED(y) * (1.0f - cosVal)));
	result.e[1][2] = ((z * y * (1.0f - cosVal)) + (x * sinVal));

	result.e[2][0] = ((x * z * (1.0f - cosVal)) + (y * sinVal));
	result.e[2][1] = ((y * z * (1.0f - cosVal)) - (x * sinVal));
	result.e[2][2] = (cosVal + (SQUARED(z) * (1.0f - cosVal)));

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


INTERNAL inline v4 mat4_mulV4(const mat4 a, const v4 b)
{
	v4 result = {0};

	result.x = (a.e[0][0] * b.x) + (a.e[1][0] * b.y) + (a.e[2][0] * b.z) +
	           (a.e[3][0] * b.w);
	result.y = (a.e[0][1] * b.x) + (a.e[1][1] * b.y) + (a.e[2][1] * b.z) +
	           (a.e[3][1] * b.w);
	result.z = (a.e[0][2] * b.x) + (a.e[1][2] * b.y) + (a.e[2][2] * b.z) +
	           (a.e[3][2] * b.w);
	result.w = (a.e[0][3] * b.x) + (a.e[1][3] * b.y) + (a.e[2][3] * b.z) +
	           (a.e[3][3] * b.w);

	return result;
}

INTERNAL inline b32 math_rectContainsP(Rect rect, v2 p)
{
	b32 outsideOfRectX = FALSE;
	if (p.x < rect.min.x || p.x > rect.max.w)
		outsideOfRectX = TRUE;

	b32 outsideOfRectY = FALSE;
	if (p.y < rect.min.y || p.y > rect.max.h)
		outsideOfRectY = TRUE;

	if (outsideOfRectX || outsideOfRectY) return FALSE;
	else return TRUE;
}


INTERNAL inline Rect math_rectCreate(v2 origin, v2 size)
{
	Rect result = {0};
	result.min  = origin;
	result.max  = v2_add(origin, size);

	return result;
}

INTERNAL inline v2 math_rectGetSize(Rect rect)
{
	f32 width  = ABS(rect.max.x - rect.min.x);
	f32 height = ABS(rect.max.y - rect.min.y);

	v2 result = V2(width, height);

	return result;
}

INTERNAL inline v2 math_rectGetCentre(Rect rect)
{
	f32 sumX  = rect.min.x + rect.max.x;
	f32 sumY  = rect.min.y + rect.max.y;
	v2 result = v2_scale(V2(sumX, sumY), 0.5f);

	return result;
}

INTERNAL inline Rect math_rectShift(Rect rect, v2 shift)
{
	Rect result = {0};
	result.min = v2_add(rect.min, shift);
	result.max = v2_add(rect.max, shift);

	return result;
}

INTERNAL inline void math_applyRotationToVertexes(v2 pos, v2 pivotPoint,
                                                  Radians rotate,
                                                  v2 *vertexList,
                                                  i32 vertexListSize)
{
	if (rotate == 0) return;
	// NOTE(doyle): Move the world origin to the base position of the object.
	// Then move the origin to the pivot point (e.g. center of object) and
	// rotate from that point.
	v2 pointOfRotation = v2_add(pivotPoint, pos);

	mat4 rotateMat = mat4_translate(pointOfRotation.x, pointOfRotation.y, 0.0f);
	rotateMat      = mat4_mul(rotateMat, mat4_rotate(rotate, 0.0f, 0.0f, 1.0f));
	rotateMat      = mat4_mul(rotateMat, mat4_translate(-pointOfRotation.x,
	                                               -pointOfRotation.y, 0.0f));

	// TODO(doyle): Make it nice for 2d vector, stop using mat4
	for (i32 i = 0; i < vertexListSize; i++)
	{
		// NOTE(doyle): Manual matrix multiplication since vertex pos is 2D and
		// matrix is 4D
		v2 oldP = vertexList[i];
		v2 newP = {0};

		newP.x = (oldP.x * rotateMat.e[0][0]) + (oldP.y * rotateMat.e[1][0]) +
		         (rotateMat.e[3][0]);
		newP.y = (oldP.x * rotateMat.e[0][1]) + (oldP.y * rotateMat.e[1][1]) +
		         (rotateMat.e[3][1]);

		vertexList[i] = newP;
	}
}
INTERNAL inline f32 math_lerp(f32 a, f32 t, f32 b)
{
	/*
	    Linear blend between two values. We having a starting point "a", and
	    the distance to "b" is defined as (b - a). Then we can say

	    a + t(b - a)

	    As our linear blend fn. We start from "a" and choosing a t from 0->1
	    will vary the value of (b - a) towards b. If we expand this, this
	    becomes

	    a + (t * b) - (a * t) == (1 - t)a + t*b
	*/
	f32 result = ((1 - t) * a) + (t * b);

	return result;
}

#endif
