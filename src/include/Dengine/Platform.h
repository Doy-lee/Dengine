#ifndef DENGINE_PLATFORM_H
#define DENGINE_PLATFORM_H

#include "Dengine/Common.h"
#include "Dengine/Math.h"

/* Forward Declaration */
typedef struct MemoryArena MemoryArena_;

// TODO(doyle): Revise key code system -- should we store the extraneous keys or
// have a mapping function to map it back to ascii?
enum KeyCode
{
	keycode_space,
	keycode_exclamation,
	keycode_dbl_quotes,
	keycode_hash,
	keycode_dollar,
	keycode_percent,
	keycode_ampersand,
	keycode_single_quote,
	keycode_left_parenthesis,
	keycode_right_parenthesis,
	keycode_star,
	keycode_plus,
	keycode_comma,
	keycode_hyphen,
	keycode_dot,
	keycode_forward_slash,
	keycode_0,
	keycode_1,
	keycode_2,
	keycode_3,
	keycode_4,
	keycode_5,
	keycode_6,
	keycode_7,
	keycode_8,
	keycode_9,
	keycode_colon,
	keycode_semi_colon,
	keycode_left_angle_bracket,
	keycode_equal,
	keycode_right_angle_bracket,
	keycode_question_mark,
	keycode_at,
	keycode_A,
	keycode_B,
	keycode_C,
	keycode_D,
	keycode_E,
	keycode_F,
	keycode_G,
	keycode_H,
	keycode_I,
	keycode_J,
	keycode_K,
	keycode_L,
	keycode_M,
	keycode_N,
	keycode_O,
	keycode_P,
	keycode_Q,
	keycode_R,
	keycode_S,
	keycode_T,
	keycode_U,
	keycode_V,
	keycode_W,
	keycode_X,
	keycode_Y,
	keycode_Z,
	keycode_left_square_bracket,
	keycode_back_slash,
	keycode_right_square_bracket,
	keycode_up_hat,
	keycode_underscore,
	keycode_alt_tilda,
	keycode_a,
	keycode_b,
	keycode_c,
	keycode_d,
	keycode_e,
	keycode_f,
	keycode_g,
	keycode_h,
	keycode_i,
	keycode_j,
	keycode_k,
	keycode_l,
	keycode_m,
	keycode_n,
	keycode_o,
	keycode_p,
	keycode_q,
	keycode_r,
	keycode_s,
	keycode_t,
	keycode_u,
	keycode_v,
	keycode_w,
	keycode_x,
	keycode_y,
	keycode_z,
	keycode_left_squiggly_bracket,
	keycode_pipe,
	keycode_right_squiggly_bracket,
	keycode_tilda,
	keycode_up,
	keycode_down,
	keycode_left,
	keycode_right,
	keycode_leftShift,
	keycode_mouseLeft,
	keycode_enter,
	keycode_backspace,
	keycode_tab,
	keycode_count,
	keycode_null,
};

typedef struct KeyState
{
	f32 delayInterval;
	u32 oldHalfTransitionCount;
	u32 newHalfTransitionCount;
	b32 endedDown;
} KeyState;

typedef struct KeyInput
{
	v2 mouseP;
	KeyState keys[keycode_count];
} KeyInput;

typedef struct PlatformFileRead
{
	void *buffer;
	i32 size;
} PlatformFileRead;

// TODO(doyle): Create own custom memory allocator
#define PLATFORM_MEM_FREE_(arena, ptr, bytes) platform_memoryFree(arena, CAST(void *) ptr, bytes)
// TODO(doyle): numBytes in mem free is temporary until we create custom
// allocator since we haven't put in a system to track memory usage per
// allocation
void platform_memoryFree(MemoryArena_ *arena, void *data, i32 numBytes);

#define PLATFORM_MEM_ALLOC_(arena, num, type) CAST(type *) platform_memoryAlloc(arena, num * sizeof(type))
void *platform_memoryAlloc(MemoryArena_ *arena, i32 numBytes);

void platform_closeFileRead(MemoryArena_ *arena, PlatformFileRead *file);
i32 platform_readFileToBuffer(MemoryArena_ *arena, const char *const filePath,
                              PlatformFileRead *file);

#endif
