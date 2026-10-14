/*
 * Copyright 2025 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "SkTrimPathTypes.h"

const SkPathFillType kDefFillType = SkPathFillType::kWinding;

const SkPathVerb gOvalVerbs[] = {
    SkPathVerb::kMove,
    SkPathVerb::kConic,
    SkPathVerb::kConic,
    SkPathVerb::kConic,
    SkPathVerb::kConic,
    SkPathVerb::kClose
};

const uint8_t gOvalSegMask = kConic_SkPathSegmentMask;

const float gFourQuarterCircleConics[] = {
    SK_ScalarRoot2Over2,
    SK_ScalarRoot2Over2,
    SK_ScalarRoot2Over2,
    SK_ScalarRoot2Over2,
};

static void set_as_oval(SkPathRaw* raw, SkSpan<SkPoint> storage,
                        const SkRect& r, SkPathDirection dir, unsigned index) {
    SkASSERT(storage.size() >= 9);

    raw->fPoints = { storage.data(), 9 };
    raw->fVerbs = gOvalVerbs;
    raw->fConics = gFourQuarterCircleConics;
    raw->fBounds = r;
    raw->fFillType = kDefFillType;
    raw->fConvexity = SkPathDirection_ToConvexity(dir);
    raw->fSegmentMask = gOvalSegMask;

    SkPath_OvalPointIterator ovalIter(r, dir, index);
    SkPath_RectPointIterator rectIter(r, dir, index + (dir == SkPathDirection::kCW ? 0 : 1));

    storage[0] = ovalIter.current();
    for (unsigned i = 0; i < 4; ++i) {
        storage[i*2 + 1] = rectIter.next();
        storage[i*2 + 2] = ovalIter.next();
    }
}

/////////////////////////////

const SkPathVerb gRRectVerbs_LineStart[] = {
    SkPathVerb::kMove,
    SkPathVerb::kLine, SkPathVerb::kConic,
    SkPathVerb::kLine, SkPathVerb::kConic,
    SkPathVerb::kLine, SkPathVerb::kConic,
    SkPathVerb::kLine, SkPathVerb::kConic,
    SkPathVerb::kClose
};

const SkPathVerb gRRectVerbs_ConicStart[] = {
    SkPathVerb::kMove,
    SkPathVerb::kConic, SkPathVerb::kLine,
    SkPathVerb::kConic, SkPathVerb::kLine,
    SkPathVerb::kConic, SkPathVerb::kLine,
    SkPathVerb::kConic, // we can skip the last line
    SkPathVerb::kClose
};

const uint8_t gRRectSegMask = kLine_SkPathSegmentMask | kConic_SkPathSegmentMask;

static void set_as_rrect(SkPathRaw* raw, SkSpan<SkPoint> storage,
                         const SkRRect& rrect, SkPathDirection dir, unsigned index) {
    // we start with a conic on odd indices when moving CW vs. even indices when moving CCW
    const bool startsWithConic = ((index & 1) == (dir == SkPathDirection::kCW));
    // if we start with a conic, we end with a line, which we can skip (relying on close())
    const size_t npoints = 13 - startsWithConic;
    const SkRect& bounds = rrect.getBounds();

    SkASSERT(storage.size() >= npoints);

    raw->fPoints = { storage.data(), npoints };
    if (startsWithConic) {
        raw->fVerbs = gRRectVerbs_ConicStart;
    } else {
        raw->fVerbs = gRRectVerbs_LineStart;
    }
    raw->fConics = gFourQuarterCircleConics;
    raw->fBounds = bounds;
    raw->fFillType = kDefFillType;
    raw->fConvexity = SkPathDirection_ToConvexity(dir);
    raw->fSegmentMask = gRRectSegMask;

    SkPath_RRectPointIterator rrectIter(rrect, dir, index);
    // Corner iterator indices follow the collapsed radii model,
    // adjusted such that the start pt is "behind" the radii start pt.
    const unsigned rectStartIndex = index / 2 + (dir == SkPathDirection::kCW ? 0 : 1);
    SkPath_RectPointIterator rectIter(bounds, dir, rectStartIndex);

    storage[0] = rrectIter.current();
    if (startsWithConic) {
        for (unsigned i = 0; i < 3; ++i) {
            // conic points
            storage[i*3 + 1] = rectIter.next();
            storage[i*3 + 2] = rrectIter.next();
            // line point
            storage[i*3 + 3] = rrectIter.next();
        }
        // last conic points
        storage[10] = rectIter.next();
        storage[11] = rrectIter.next();
        // the final line is accomplished by close()
    } else {
        for (unsigned i = 0; i < 4; ++i) {
            // line point
            storage[i*3 + 1] = rrectIter.next();
            // conic points
            storage[i*3 + 2] = rectIter.next();
            storage[i*3 + 3] = rrectIter.next();
        }
    }
    // close
}

/////////////////////////////

SkPathRawShapes::Oval::Oval(const SkRect& r, SkPathDirection dir, unsigned index) {
    set_as_oval(this, fStorage, r, dir, index);
}

SkPathRawShapes::RRect::RRect(const SkRRect& rrect, SkPathDirection dir, unsigned index) {
    const SkRect& bounds = rrect.getBounds();

    if (rrect.isOval()) {
        // degenerate(oval) => line points are collapsing
        set_as_oval(this, fStorage, bounds, dir, index / 2);
    } else {
        set_as_rrect(this, fStorage, rrect, dir, index);
    }
}

/////////////////////////////

const SkPathVerb gTriangle_Verbs[] = {
    SkPathVerb::kMove,
    SkPathVerb::kLine,
    SkPathVerb::kLine,
    SkPathVerb::kClose
};

static SkPathConvexity tri_to_convexity(SkSpan<const SkPoint> pts) {
    SkVector u = pts[1] - pts[0],
             v = pts[2] - pts[1];
    float cross = u.fX * v.fY - u.fY * v.fX;

    return cross > 0 ? SkPathConvexity::kConvex_CW
                     : (cross < 0) ? SkPathConvexity::kConvex_CCW
                                   : SkPathConvexity::kConvex_Degenerate;
}

SkPathRawShapes::Triangle::Triangle(SkSpan<const SkPoint> threePoints, const SkRect& bounds)
    : SkPathRaw{threePoints, gTriangle_Verbs, {}, bounds,
                SkPathFillType::kDefault, tri_to_convexity(threePoints), kLine_SkPathSegmentMask}
{
    SkASSERT(threePoints.size() == 3);
    SkASSERT(SkRect::Bounds(threePoints).value() == bounds);
}
