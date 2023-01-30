#include "math.h"
#include <math.h> // sin/cos

static float math_floorf(float x);
static float math_ceilf(float x);
static float math_cosf(float x);
static float math_sinf(float x);

core_math_api* math_api = &(core_math_api){
	.floorf = math_floorf,
	.ceilf = math_ceilf,
	.cosf = math_cosf,
	.sinf = math_sinf
};

static float math_floorf(float x)
{
	const int32_t ix = (int32_t)x;
	const float fix = (float)ix;
	return fix - ((fix > x) ? 1.0f : 0.0f);
}

static float math_ceilf(float x)
{
	const int32_t ix = (int32_t)x;
	const float fix = (float)ix;
	return fix + ((fix < x) ? 1.0f : 0.0f);
}

static float math_cosf(float x)
{
	// TODO: Replace?
	return cosf(x);
}

static float math_sinf(float x)
{
	// TODO: Replace?
	return sinf(x);
}
