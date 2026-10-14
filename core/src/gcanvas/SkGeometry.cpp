/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkTrimGeometry.h"
#include "SkTrimPoint.h"

#ifndef SK_ScalarRoot2Over2
#define SK_ScalarRoot2Over2 0.707106781f
#endif

#include <cmath>

/////////////////////////////////////////////////////////////////////////////
//
// NURB representation for conics.  Helpful explanations at:
//
// http://citeseerx.ist.psu.edu/viewdoc/
//   download?doi=10.1.1.44.5740&rep=rep1&type=ps
// and
// http://www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/spline/NURBS/RB-conics.html
//
// F = (A (1 - t)^2 + C t^2 + 2 B (1 - t) t w)
//     ------------------------------------------
//         ((1 - t)^2 + t^2 + 2 (1 - t) t w)
//
//   = {t^2 (P0 + P2 - 2 P1 w), t (-2 P0 + 2 P1 w), P0}
//     ------------------------------------------------
//             {t^2 (2 - 2 w), t (-2 + 2 w), 1}
//

// F' = 2 (C t (1 + t (-1 + w)) - A (-1 + t) (t (-1 + w) - w) + B (1 - 2 t) w)
//
//  t^2 : (2 P0 - 2 P2 - 2 P0 w + 2 P2 w)
//  t^1 : (-2 P0 + 2 P2 + 4 P0 w - 4 P1 w)
//  t^0 : -2 P0 w + 2 P1 w
//
//  We disregard magnitude, so we can freely ignore the denominator of F', and
//  divide the numerator by 2
//
//    coeff[0] for t^2
//    coeff[1] for t^1
//    coeff[2] for t^0
//



























int SkConic::BuildUnitArc(const SkVector& uStart, const SkVector& uStop, SkPathDirection dir,
                          const SkMatrix* userMatrix, SkConic dst[kMaxConicsForArc]) {
    // rotate by x,y so that uStart is (1.0)
    SkScalar x = SkPoint::DotProduct(uStart, uStop);
    SkScalar y = SkPoint::CrossProduct(uStart, uStop);

    SkScalar absY = SkScalarAbs(y);

    // check for (effectively) coincident vectors
    // this can happen if our angle is nearly 0 or nearly 180 (y == 0)
    // ... we use the dot-prod to distinguish between 0 and 180 (x > 0)
    if (absY <= SK_ScalarNearlyZero && x > 0 && ((y >= 0 && SkPathDirection::kCW == dir) ||
                                                 (y <= 0 && SkPathDirection::kCCW == dir))) {
        return 0;
    }

    if (dir == SkPathDirection::kCCW) {
        y = -y;
    }

    // We decide to use 1-conic per quadrant of a circle. What quadrant does [xy] lie in?
    //      0 == [0  .. 90)
    //      1 == [90 ..180)
    //      2 == [180..270)
    //      3 == [270..360)
    //
    int quadrant = 0;
    if (0 == y) {
        quadrant = 2;        // 180
        SkASSERT(SkScalarAbs(x + SK_Scalar1) <= SK_ScalarNearlyZero);
    } else if (0 == x) {
        SkASSERT(absY - SK_Scalar1 <= SK_ScalarNearlyZero);
        quadrant = y > 0 ? 1 : 3; // 90 : 270
    } else {
        if (y < 0) {
            quadrant += 2;
        }
        if ((x < 0) != (y < 0)) {
            quadrant += 1;
        }
    }

    const SkPoint quadrantPts[] = {
        { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 }
    };
    const SkScalar quadrantWeight = SK_ScalarRoot2Over2;

    int conicCount = quadrant;
    for (int i = 0; i < conicCount; ++i) {
        dst[i].set(&quadrantPts[i * 2], quadrantWeight);
    }

    // Now compute any remaing (sub-90-degree) arc for the last conic
    const SkPoint finalP = { x, y };
    const SkPoint& lastQ = quadrantPts[quadrant * 2];  // will already be a unit-vector
    const SkScalar dot = SkVector::DotProduct(lastQ, finalP);
    if (SkIsNaN(dot)) {
        return 0;
    }
    SkASSERT(0 <= dot && dot <= SK_Scalar1 + SK_ScalarNearlyZero);

    if (dot < 1) {
        SkVector offCurve = { lastQ.x() + x, lastQ.y() + y };
        // compute the bisector vector, and then rescale to be the off-curve point.
        // we compute its length from cos(theta/2) = length / 1, using half-angle identity we get
        // length = sqrt(2 / (1 + cos(theta)). We already have cos() when to computed the dot.
        // This is nice, since our computed weight is cos(theta/2) as well!
        //
        const SkScalar cosThetaOver2 = SkScalarSqrt((1 + dot) / 2);
        offCurve.setLength(SkScalarInvert(cosThetaOver2));
        if (!SkPointPriv::EqualsWithinTolerance(lastQ, offCurve)) {
            dst[conicCount].set(lastQ, offCurve, finalP, cosThetaOver2);
            conicCount += 1;
        }
    }

    // now handle counter-clockwise and the initial unitStart rotation
    SkMatrix    matrix;
    matrix.setSinCos(uStart.fY, uStart.fX);
    if (dir == SkPathDirection::kCCW) {
        matrix.preScale(SK_Scalar1, -SK_Scalar1);
    }
    if (userMatrix) {
        matrix.postConcat(*userMatrix);
    }
    for (int i = 0; i < conicCount; ++i) {
        matrix.mapPoints(dst[i].fPts);
    }
    return conicCount;
}
