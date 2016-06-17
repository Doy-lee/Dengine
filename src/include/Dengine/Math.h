#ifndef DENGINE_MATH_H
#define DENGINE_MATH_H

#include <Dengine/Common.h>

namespace Dengine
{
class Math
{
public:
	static inline f32 squared(const f32 a)
	{
		f32 result = a * a;
		return result;
	}
};
}
#endif

