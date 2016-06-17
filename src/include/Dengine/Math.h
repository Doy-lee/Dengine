#ifndef DENGINE_MATH_H
#define DENGINE_MATH_H

#include <Dengine/Common.h>

#define squared(x) (x * x)
#define abs(x) ((x) > 0 ? (x) : -(x))

union v2
{
	struct
	{
		f32 x, y;
	};
	struct
	{
		f32 w, h;
	};
	f32 data[2];
};

union v3
{
	struct
	{
		f32 x, y, z;
	};
	struct
	{
		f32 r, g, b;
	};
	f32 data[3];
};

union v4
{
	struct
	{
		f32 x, y, z, w;
	};
	struct
	{
		f32 r, g, b, a;
	};
	f32 data[4];
};

INTERNAL inline v2 V2(const f32 x, const f32 y)
{
	v2 result = v2{x, y};
	return result;
}
INTERNAL inline v3 V3(const f32 x, const f32 y, const f32 z)
{
	v3 result = v3{x, y, z};
	return result;
}
INTERNAL inline v4 V4(const f32 x, const f32 y, const f32 z, const f32 w)
{
	v4 result = v4{x, y, z, w};
	return result;
}

#define DEFINE_VECTOR_MATH(num) \
INTERNAL inline v##num v##num##_add(const v##num a, const v##num b) \
{ \
	v##num result; \
	for (i32 i = 0; i < ##num; i++) { result.data[i] = a.data[i] + b.data[i]; } \
	return result; \
} \
INTERNAL inline v##num v##num##_sub(const v##num a, const v##num b) \
{ \
	v##num result; \
	for (i32 i = 0; i < ##num; i++) { result.data[i] = a.data[i] - b.data[i]; } \
	return result; \
} \
INTERNAL inline v##num v##num##_scale(const v##num a, const f32 b) \
{ \
	v##num result; \
	for (i32 i = 0; i < ##num; i++) { result.data[i] = a.data[i] * b; } \
	return result; \
} \
INTERNAL inline v##num v##num##_mul(const v##num a, const v##num b) \
{ \
	v##num result; \
	for (i32 i = 0; i < ##num; i++) { result.data[i] = a.data[i] * b.data[i]; } \
	return result; \
} \

DEFINE_VECTOR_MATH(2);
DEFINE_VECTOR_MATH(3);
DEFINE_VECTOR_MATH(4);

#endif
