/*
 * SkTrimGeometry.h — conic path declaration (project code, NOT Skia code).
 * Mirrors src/core/SkGeometry.h for SkConic::BuildUnitArc.
 *
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef SKTRIM_GEOMETRY_H
#define SKTRIM_GEOMETRY_H

#include "SkTrimMatrix.h"
#include "SkTrimPathTypes.h"
#include "SkTrimPoint.h"

// kMaxConicsForArc: Skia sizes dst[] for a full circle (4 quadrants + 1).
// Callers here only request <=90-degree spans (<=2 conics written), so 5 is safe.
#define kMaxConicsForArc 5

struct SkConic {
    SkPoint fPts[3];
    SkScalar fW;
    SkConic() {}
    SkConic(const SkPoint& p0, const SkPoint& p1, const SkPoint& p2, SkScalar w) {
        this->set(p0, p1, p2, w);
    }
    SkConic(const SkPoint pts[3], SkScalar w) {
        this->set(pts, w);
    }
    void set(const SkPoint pts[3], SkScalar w) {
        fPts[0] = pts[0];
        fPts[1] = pts[1];
        fPts[2] = pts[2];
        this->setW(w);
    }
    void set(const SkPoint& p0, const SkPoint& q, const SkPoint& p1, SkScalar w) {
        fPts[0] = p0;
        fPts[1] = q;
        fPts[2] = p1;
        this->setW(w);
    }
    void setW(SkScalar w) {
        fW = w > 0 && SkIsFinite(w) ? w : 1;
    }
    static int BuildUnitArc(const SkVector& uStart, const SkVector& uStop,
                            SkPathDirection dir, const SkMatrix* userMatrix,
                            SkConic dst[kMaxConicsForArc]);
};

#endif /* SKTRIM_GEOMETRY_H */
