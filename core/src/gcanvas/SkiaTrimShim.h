/*
 * SkiaTrimShim.h — minimal type shim (project code, NOT Skia code) so the
 * trimmed Skia reference units (SkGeometry.cpp: SkConic::BuildUnitArc)
 * compile inside react-native-gcanvas without the Skia tree.
 *
 * Only the symbols used by the kept ellipse/arc code are provided.
 * 2D-affine only; no perspective support (GCanvas 2D has none either).
 */
#ifndef GSKIA_TRIM_SHIM_H
#define GSKIA_TRIM_SHIM_H

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
    float fX, fY;
    bool operator==(const SkPoint& o) const { return fX == o.fX && fY == o.fY; }
    static float DotProduct(const SkPoint& a, const SkPoint& b) { return a.fX * b.fX + a.fY * b.fY; }
    static float CrossProduct(const SkPoint& a, const SkPoint& b) { return a.fX * b.fY - a.fY * b.fX; }
};

struct SkVector {
    float fX, fY;
    float x() const { return fX; }
    float y() const { return fY; }
    void setLength(float len) {
        float d = sqrtf(fX * fX + fY * fY);
        if (d > 1e-6f) { fX = fX / d * len; fY = fY / d * len; }
    }
    static float DotProduct(const SkVector& a, const SkVector& b) { return a.fX * b.fX + a.fY * b.fY; }
};

struct SkMatrix {
    // 2D affine: [a c tx] [b d ty] (no perspective).
    float a, b, c, d, tx, ty;
    void setSinCos(float sinV, float cosV) { a = cosV; b = sinV; c = -sinV; d = cosV; tx = 0; ty = 0; }
    void preScale(float sx, float sy) { a *= sx; b *= sx; c *= sy; d *= sy; }
    void postConcat(const SkMatrix& m) {
        float na = a * m.a + c * m.b, nb = b * m.a + d * m.b;
        float nc = a * m.c + c * m.d, nd = b * m.c + d * m.d;
        float ntx = a * m.tx + c * m.ty + tx, nty = b * m.tx + d * m.ty + ty;
        a = na; b = nb; c = nc; d = nd; tx = ntx; ty = nty;
    }
    void mapPoints(SkPoint pts[], int count) const {
        for (int i = 0; i < count; ++i) {
            float x = pts[i].fX, y = pts[i].fY;
            pts[i].fX = a * x + c * y + tx;
            pts[i].fY = b * x + d * y + ty;
        }
    }
};

enum SkPathDirection { kCW = 0, kCCW = 1 };

struct SkPointPriv {
    static bool EqualsWithinTolerance(const SkPoint& a, const SkPoint& b) {
        return SkScalarAbs(a.fX - b.fX) <= 1e-6f && SkScalarAbs(a.fY - b.fY) <= 1e-6f;
    }
};

// kMaxConicsForArc: Skia sizes dst[] for a full circle (4 quadrants + 1).
// Callers here only request <=90-degree spans (<=2 conics written), so 5 is safe.
#define kMaxConicsForArc 5

struct SkConic {
    SkPoint fPts[3];
    SkScalar fW;
    void set(const SkPoint* pts, SkScalar w) {
        fPts[0] = pts[0]; fPts[1] = pts[1]; fPts[2] = pts[2]; fW = w;
    }
    void set(const SkPoint& p0, const SkPoint& q, const SkPoint& p1, SkScalar w) {
        fPts[0] = p0; fPts[1] = q; fPts[2] = p1; fW = w;
    }
    static int BuildUnitArc(const SkVector& uStart, const SkVector& uStop,
                            SkPathDirection dir, const SkMatrix* userMatrix,
                            SkConic dst[kMaxConicsForArc]);
};

#endif /* GSKIA_TRIM_SHIM_H */
