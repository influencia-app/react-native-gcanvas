/*
 * SkTrimPoint.h — SkPoint/SkVector types (project code, NOT Skia code).
 * Mirrors include/core/SkPoint.h for the symbols used here.
 */
#ifndef SKTRIM_POINT_H
#define SKTRIM_POINT_H

#include <cmath>

typedef float SkScalar;

#define SK_Scalar1          1.0f
#define SK_ScalarNearlyZero 1e-6f
#define SK_ScalarHalf       0.5f

inline float SkScalarAbs(float x) { return x >= 0 ? x : -x; }
inline float SkScalarSqrt(float x) { return sqrtf(x); }
inline float SkScalarInvert(float x) { return 1.0f / x; }
inline bool SkIsNaN(float x) { return x != x; }
inline bool SkIsFinite(float x) { return x == x; }
#define SkASSERT(cond) ((void)0)
#define SkDEBUGCODE(...) ((void)0)

struct SkPoint {
    float fX;
    float fY;
    static SkPoint Make(float x, float y) {
        SkPoint p;
        p.fX = x;
        p.fY = y;
        return p;
    }
    float x() const { return fX; }
    float y() const { return fY; }
    bool isZero() const { return (0 == fX) & (0 == fY); }
    void set(float x, float y) {
        fX = x;
        fY = y;
    }
    void offset(float dx, float dy) {
        fX += dx;
        fY += dy;
    }
    float length() const { return sqrtf(fX * fX + fY * fY); }
    bool setLength(float length) {
        float d = sqrtf(fX * fX + fY * fY);
        if (d > 1e-6f) {
            fX = fX / d * length;
            fY = fY / d * length;
            return true;
        }
        fX = 0;
        fY = 0;
        return false;
    }
    void negate() {
        fX = -fX;
        fY = -fY;
    }
    bool isFinite() const { return SkIsFinite(fX) && SkIsFinite(fY); }
    bool equals(float x, float y) const { return fX == x && fY == y; }
    float cross(const SkPoint& vec) const { return CrossProduct(*this, vec); }
    float dot(const SkPoint& vec) const { return DotProduct(*this, vec); }
    static float DotProduct(const SkPoint& a, const SkPoint& b) {
        return a.fX * b.fX + a.fY * b.fY;
    }
    static float CrossProduct(const SkPoint& a, const SkPoint& b) {
        return a.fX * b.fY - a.fY * b.fX;
    }
    friend bool operator==(const SkPoint& a, const SkPoint& b) {
        return a.fX == b.fX && a.fY == b.fY;
    }
    friend bool operator!=(const SkPoint& a, const SkPoint& b) {
        return a.fX != b.fX || a.fY != b.fY;
    }
    friend SkPoint operator-(const SkPoint& a, const SkPoint& b);
};

typedef SkPoint SkVector;

inline SkVector operator-(const SkPoint& a, const SkPoint& b) {
    SkVector v;
    v.fX = a.fX - b.fX;
    v.fY = a.fY - b.fY;
    return v;
}

struct SkPointPriv {
    static bool EqualsWithinTolerance(const SkPoint& a, const SkPoint& b) {
        return SkScalarAbs(a.fX - b.fX) <= 1e-6f && SkScalarAbs(a.fY - b.fY) <= 1e-6f;
    }
};

#endif /* SKTRIM_POINT_H */
