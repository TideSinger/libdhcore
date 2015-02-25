/***********************************************************************************
 * Copyright (c) 2012, Sepehr Taghdisian
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 ***********************************************************************************/


#ifndef __COLOR_H__
#define __COLOR_H__

#include <math.h>

#include "types.h"
#include "mem-mgr.h"
#include "numeric.h"
#include "core-api.h"

struct CORE_CPP_API ALIGN16 color
{
    union   {
        struct {
            float r;
            float g;
            float b;
            float a;
        };

        float f[4];
    };
};

/* globals */
#if defined(_GNUC_) && !defined(__cplusplus)
static const struct color g_color_blue = {.r=0.0f, .g=0.0f, .b=1.0f, .a=1.0f};
static const struct color g_color_red = {.r=1.0f, .g=0.0f, .b=0.0f, .a=1.0f};
static const struct color g_color_green = {.r=0.0f, .g=1.0f, .b=0.0f, .a=1.0f};
static const struct color g_color_white = {.r=1.0f, .g=1.0f, .b=1.0f, .a=1.0f};
static const struct color g_color_black = {.r=0.0f, .g=0.0f, .b=0.0f, .a=1.0f};
static const struct color g_color_grey = {.r=0.3f, .g=0.3f, .b=0.3f, .a=1.0f};
static const struct color g_color_yellow = {.r=1.0f, .g=1.0f, .b=0.0f, .a=1.0f};
#else
static const struct color g_color_blue = {0.0f, 0.0f, 1.0f, 1.0f};
static const struct color g_color_red = {1.0f, 0.0f, 0.0f, 1.0f};
static const struct color g_color_green = {0.0f, 1.0f, 0.0f, 1.0f};
static const struct color g_color_white = {1.0f, 1.0f, 1.0f, 1.0f};
static const struct color g_color_black = {0.0f, 0.0f, 0.0f, 1.0f};
static const struct color g_color_grey = {0.3f, 0.3f, 0.3f, 1.0f};
static const struct color g_color_yellow = {1.0f, 1.0f, 0.0f, 1.0f};
#endif

INLINE int color_isequal(const struct color* c1, const struct color* c2)
{
    return (c1->r == c2->r) && (c1->g == c2->g) && (c1->b == c2->b) && (c1->a == c2->a);
}

INLINE struct color* color_setf(struct color* c, float r, float g, float b, float a)
{
    c->r = r;
    c->g = g;
    c->b = b;
    c->a = a;
    return c;
}

INLINE struct color* color_setfv(struct color* c, const float* f)
{
    c->r = f[0];
    c->g = f[1];
    c->b = f[2];
    c->a = f[3];
    return c;
}

/* rgba values are between 0~255 range */
INLINE struct color* color_seti(struct color* c, uint8 r, uint8 g, uint8 b, uint8 a)
{
    c->r = ((float)r)/255.0f;
    c->g = ((float)g)/255.0f;
    c->b = ((float)b)/255.0f;
    c->a = ((float)a)/255.0f;
    return c;
}

INLINE struct color* color_setc(struct color* r, const struct color* c)
{
    r->r = c->r;
    r->g = c->g;
    r->b = c->b;
    r->a = c->a;
    return r;
}

INLINE uint color_rgba_uint(const struct color* c)
{
    uint8 r = (uint8)(c->r*255.0f);
    uint8 g = (uint8)(c->g*255.0f);
    uint8 b = (uint8)(c->b*255.0f);
    uint8 a = (uint8)(c->a*255.0f);
    return (uint)((((r)&0xff)<<24) | (((g)&0xff)<<16) | (((b)&0xff)<<8) | ((a)&0xff));
}

INLINE uint color_rgba_uint_swizzle(const struct color* c)
{
    uint8 r = (uint8)(c->r*255.0f);
    uint8 g = (uint8)(c->g*255.0f);
    uint8 b = (uint8)(c->b*255.0f);
    uint8 a = (uint8)(c->a*255.0f);
    return (uint)((((a)&0xff)<<24) | (((b)&0xff)<<16) | (((g)&0xff)<<8) | ((r)&0xff));
}


INLINE uint color_rgb_uint(const struct color* c)
{
    uint8 r = (uint8)(c->r*255.0f);
    uint8 g = (uint8)(c->g*255.0f);
    uint8 b = (uint8)(c->b*255.0f);
    return (uint)((((r)&0xff)<<24) | (((g)&0xff)<<16) | (((b)&0xff)<<8) | 0xff);
}

INLINE struct color* color_tolinear(struct color* r, const struct color* c)
{
    r->r = c->r*c->r;
    r->g = c->g*c->g;
    r->b = c->b*c->b;
    r->a = c->a;
    return r;
}

INLINE struct color* color_togamma(struct color* r, const struct color* c)
{
    r->r = sqrtf(c->r);
    r->g = sqrtf(c->g);
    r->b = sqrtf(c->b);
    r->a = c->a;
    return r;
}

INLINE struct color* color_muls(struct color* r, const struct color* c, float k)
{
    r->r = c->r * k;
    r->g = c->g * k;
    r->b = c->b * k;
    r->a = c->a;
    return r;
}

INLINE struct color* color_mul(struct color* r, const struct color* c1, const struct color* c2)
{
    r->r = c1->r * c2->r;
    r->g = c1->g * c2->g;
    r->b = c1->b * c2->b;
    r->a = minf(c1->a, c2->a);
    return r;
}

INLINE struct color* color_add(struct color* r, const struct color* c1, const struct color* c2)
{
    r->r = c1->r + c2->r;
    r->g = c1->g + c2->g;
    r->b = c1->b + c2->b;
    r->a = maxf(c1->a, c2->a);
    return r;
}

INLINE struct color* color_swizzle_abgr(struct color* r, const struct color* c)
{
    return color_setf(r, c->a, c->b, c->g, c->r);
}

INLINE struct color* color_lerp(struct color* r, const struct color* c1, const struct color* c2,
                                float t)
{
    float tinv  = 1.0f - t;
    return color_setf(r,
        c1->r*t + c2->r*tinv,
        c1->g*t + c2->g*tinv,
        c1->b*t + c2->b*tinv,
        c1->a*t + c2->a*tinv);
}

#ifdef __cplusplus
#include "err.h"

namespace dh {

class ALIGN16 CORE_CPP_API Color
{
private:
    color m_clr;

public:
    static const Color Black;
    static const Color White;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Purple;
    static const Color Grey;

public:
    Color()
    {
    }

    Color(float r, float g, float b, float a = 1.0f)
    {
        color_setf(&m_clr, r, g, b, a);
    }

    Color(uint8 r, uint8 g, uint8 b, uint8 a = 255)
    {
        color_seti(&m_clr, r, g, b, a);
    }

    Color(const color &c)
    {
        m_clr = c;
    }
    
    Color& operator=(const Color &c)
    {
        color_setc(&m_clr, &c.m_clr);
        return *this;
    }

    float r() const
    {
        return m_clr.r;
    }

    float g() const
    {
        return m_clr.g;
    }

    float b() const
    {
        return m_clr.b;
    }

    float a() const
    {
        return m_clr.a;
    }

    Color& set(float r, float g, float b, float a = 1.0f)
    {
        color_setf(&m_clr, r, g, b, a);
        return *this;
    }

    Color& set(uint8 r, uint8 g, uint8 b, uint8 a = 255)
    {
        color_seti(&m_clr, r, g, b, a);
        return *this;
    }

    Color& set(const color &c)
    {
        m_clr = c;
        return *this;
    }

    bool operator==(const Color &c)
    {
        return color_isequal(&m_clr, &c.m_clr);
    }

    Color linear() const
    {
        Color c;
        color_tolinear(&c.m_clr, &m_clr);
        return c;
    }

    Color gamma() const
    {
        Color c;
        color_togamma(&c.m_clr, &m_clr);
        return c;
    }

    Color operator+(const Color& c) const
    {
        Color r;
        color_add(&r.m_clr, &m_clr, &c.m_clr);
        return r;
    }

    Color& operator+=(const Color& c)
    {
        color_add(&m_clr, &m_clr, &c.m_clr);
        return *this;
    }

    Color operator*(const Color& c) const
    {
        Color r;
        color_mul(&r.m_clr, &m_clr, &c.m_clr);
        return r;
    }

    Color& operator*=(const Color& c)
    {
        color_mul(&m_clr, &m_clr, &c.m_clr);
        return *this;
    }

    Color operator*(float k) const
    {
        Color r;
        color_muls(&r.m_clr, &m_clr, k);
        return r;
    }

    Color& operator*=(float k)
    {
        color_muls(&m_clr, &m_clr, k);
        return *this;
    }

    uint to_uint() const
    {
        return color_rgba_uint(&m_clr);
    }

    uint to_uint_swizzle() const
    {
        return color_rgba_uint_swizzle(&m_clr);
    }

    Color swizzle() const
    {
        Color r;
        color_swizzle_abgr(&r.m_clr, &m_clr);
        return r;
    }

    static Color lerp(const Color& c0, const Color& c1, float t)
    {
        Color r;
        color_lerp(&r.m_clr, &c0.m_clr, &c1.m_clr, t);
        return r;
    }

    Color& alpha_mul(float alpha)
    {
        m_clr.a *= alpha;
        return *this;
    }

    operator color*()   {   return &m_clr;  }
    operator const color*() const   {   return &m_clr;  }
    operator float*()   {   return m_clr.f; }
    operator const float*() const   {   return m_clr.f; }
    float operator[](uint idx) const    {   return m_clr.f[idx];    }
    float& operator[](uint idx) {   return m_clr.f[idx];    }
};

}   /* dh */
#endif

#endif /* COLOR_H */
