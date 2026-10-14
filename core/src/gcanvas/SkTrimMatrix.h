/*
 * SkTrimMatrix.h — 2D affine matrix (project code, NOT Skia code).
 * Mirrors include/core/SkMatrix.h for the symbols used here.
 */
#ifndef SKTRIM_MATRIX_H
#define SKTRIM_MATRIX_H

#include "SkTrimPathTypes.h"

struct SkMatrix {
    float a, b, c, d, tx, ty;
    void setSinCos(float sinV, float cosV) {
        a = cosV;
        b = sinV;
        c = -sinV;
        d = cosV;
        tx = 0;
        ty = 0;
    }
    void preScale(float sx, float sy) {
        a *= sx;
        b *= sx;
        c *= sy;
        d *= sy;
    }
    void postConcat(const SkMatrix& m) {
        float na = m.a * a + m.c * b, nb = m.b * a + m.d * b;
        float nc = m.a * c + m.c * d, nd = m.b * c + m.d * d;
        float ntx = m.a * tx + m.c * ty + m.tx, nty = m.b * tx + m.d * ty + m.ty;
        a = na;
        b = nb;
        c = nc;
        d = nd;
        tx = ntx;
        ty = nty;
    }
    void mapPoints(SkSpan<SkPoint> dst, SkSpan<const SkPoint> src) const {
        size_t n = dst.size() < src.size() ? dst.size() : src.size();
        for (size_t i = 0; i < n; ++i) {
            float x = src[i].fX, y = src[i].fY;
            dst[i].fX = a * x + c * y + tx;
            dst[i].fY = b * x + d * y + ty;
        }
    }
    void mapPoints(SkSpan<SkPoint> pts) const { mapPoints(pts, pts); }
};

#endif /* SKTRIM_MATRIX_H */
