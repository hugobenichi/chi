#ifndef __chi_geometry_
#define __chi_geometry_

#include <stdint.h>
#include <stdio.h>

struct vec {
	int32_t x;
	int32_t y;
};
typedef struct vec vec;

struct rec {
	union {
		struct {
			vec min;
			vec max;
		};
		struct {
			int32_t x0;
			int32_t y0;
			int32_t x1;
			int32_t y1;
		};
	};
};
typedef struct rec rec;

static inline vec v(int32_t x ,int32_t y)
{
	return (vec){
		.x = x,
		.y = y,
	};
}

static inline rec r(vec min, vec max)
{
	return (rec){
		.min = min,
		.max = max,
	};
}

static inline vec sub_vec_vec(vec v0, vec v1)
{
	return v(v0.x - v1.x, v0.y - v1.y);
}

static inline vec rec_diag(rec r)
{
	return sub_vec_vec(r.max, r.min);
}

static inline int32_t rec_w(rec r)
{
	return r.x1 - r.x0;
}

static inline int32_t rec_h(rec r)
{
	return r.y1 - r.y0;
}

static inline vec add_vec_vec(vec v0, vec v1)
{
	return v(v0.x + v1.x, v0.y + v1.y);
}

static inline rec add_vec_rec(vec v0, rec r1)
{
	return r(add_vec_vec(v0, r1.min), add_vec_vec(v0, r1.max));
}

static inline rec add_rec_vec(rec r0, vec v1)
{
	return add_vec_rec(v1, r0);
}

static inline rec add_rec_rec(rec r0, rec r1)
{
	return r(add_vec_vec(r0.min, r1.min), add_vec_vec(r0.max, r1.max));
}

static inline rec sub_rec_vec(rec r0, vec v1)
{
	return r(sub_vec_vec(r0.min, v1), sub_vec_vec(r0.max, v1));
}

// Generic adder for vec and rec.
#define add(a,b) _Generic((a),          \
	rec:	add_rec_other(b),       \
	vec:	add_vec_other(b))((a),(b))

#define add_rec_other(b) _Generic((b),  \
	rec:	add_rec_rec,            \
	vec:	add_rec_vec)

#define add_vec_other(b) _Generic((b),  \
	rec:	add_vec_rec,            \
	vec:	add_vec_vec)

#define sub(a,b) _Generic((a),          \
	rec:	sub_rec_vec,            \
	vec:	sub_vec_vec)((a),(b))


static char* vec_print(char *dst, size_t len, vec v)
{
	int n = snprintf(dst, len, "{.x=%d, .y=%d}", v.x, v.y);
	return dst + n; //_min(n, len);
}

static char* rec_print(char *dst, size_t len, rec r)
{
	int32_t w = rec_w(r);
	int32_t h = rec_h(r);
	int n = snprintf(dst, len, "{.x0=%d, .y0=%d, .x1=%d, .y1=%d, w=%d, .h=%d}", r.x0, r.y0, r.x1, r.y1, w, h);
	return dst + n; //_min(n, len);
}

#endif
