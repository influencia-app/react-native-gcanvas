/*
 * SkTrimPathTypes.h — path/shape types (project code, NOT Skia code).
 * Mirrors include/core/SkPathTypes.h, SkRRect.h (predicates) and
 * src/core/SkPathMakers.h (iterators) for the symbols used here.
 *
 * Copyright 2012 Google Inc.
 * Copyright 2019 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef SKTRIM_PATHTYPES_H
#define SKTRIM_PATHTYPES_H

#include <cstddef>

#include "SkTrimPoint.h"

enum SkPathFillType { kWinding = 0, kEvenOdd = 1, kInverseWinding = 2, kInverseEvenOdd = 3 };
enum SkPathVerb { kMove = 0, kLine = 1, kQuad = 2, kConic = 3, kCubic = 4, kClose = 5 };
enum SkPathConvexity { kConvex_CW = 0, kConvex_CCW = 1, kConvex_Degenerate = 2 };
enum SkPathDirection { kCW = 0, kCCW = 1 };

static const unsigned char kLine_SkPathSegmentMask = 1 << 0;
static const unsigned char kConic_SkPathSegmentMask = 1 << 2;

inline SkPathConvexity SkPathDirection_ToConvexity(SkPathDirection dir) {
    return dir == kCW ? kConvex_CW : kConvex_CCW;
}

template <typename T>
struct SkSpan {
    T* fPtr;
    size_t fSize;
    SkSpan() : fPtr(0), fSize(0) {}
    SkSpan(T* p, size_t n) : fPtr(p), fSize(n) {}
    template <size_t N>
    SkSpan(T (&arr)[N]) : fPtr(arr), fSize(N) {}
    template <typename U>
    SkSpan(const SkSpan<U>& o) : fPtr(o.data()), fSize(o.size()) {}
    T* data() const { return fPtr; }
    size_t size() const { return fSize; }
    T& operator[](size_t i) const { return fPtr[i]; }
};

struct SkRect {
    float fLeft, fTop, fRight, fBottom;
    float width() const { return fRight - fLeft; }
    float height() const { return fBottom - fTop; }
    float centerX() const { return (fLeft + fRight) * 0.5f; }
    float centerY() const { return (fTop + fBottom) * 0.5f; }
};

struct SkRRect {
    enum Corner { kUpperLeft_Corner = 0, kUpperRight_Corner = 1,
                  kLowerRight_Corner = 2, kLowerLeft_Corner = 3 };
    SkRect fBounds;
    SkVector fRadii[4];
    const SkRect& getBounds() const { return fBounds; }
    SkVector radii(Corner corner) const { return fRadii[corner]; }
    bool isEmpty() const {
        return !(fBounds.fLeft < fBounds.fRight && fBounds.fTop < fBounds.fBottom);
    }
    bool isRect() const {
        if (isEmpty()) {
            return false;
        }
        for (int i = 0; i < 4; ++i) {
            if (fRadii[i].fX != 0 || fRadii[i].fY != 0) {
                return false;
            }
        }
        return true;
    }
    bool isOval() const {
        float w = fBounds.width();
        float h = fBounds.height();
        if (w <= 0 || h <= 0) {
            return false;
        }
        for (int i = 0; i < 4; ++i) {
            if (fRadii[i].fX != w * 0.5f || fRadii[i].fY != h * 0.5f) {
                return false;
            }
        }
        return true;
    }
};

struct SkPathRaw {
    SkPoint fStorage[16];
    float fConicStorage[8];
    SkSpan<SkPoint> fPoints;
    SkSpan<const SkPathVerb> fVerbs;
    SkSpan<const float> fConics;
    SkRect fBounds;
    SkPathFillType fFillType;
    SkPathConvexity fConvexity;
    unsigned char fSegmentMask;
};

template <unsigned N>
class SkPath_PointIterator {
public:
    SkPath_PointIterator(SkPathDirection dir, unsigned startIndex)
        : fCurrent(startIndex % N)
        , fAdvance(dir == SkPathDirection::kCW ? 1 : N - 1) { }
    const SkPoint& current() const { return fPts[fCurrent]; }
    const SkPoint& next() {
        fCurrent = (fCurrent + fAdvance) % N;
        return this->current();
    }
protected:
    SkPoint fPts[N];
private:
    unsigned fCurrent;
    unsigned fAdvance;
};

class SkPath_RectPointIterator : public SkPath_PointIterator<4> {
public:
    SkPath_RectPointIterator(const SkRect& rect, SkPathDirection dir, unsigned startIndex)
        : SkPath_PointIterator(dir, startIndex) {
        fPts[0] = SkPoint::Make(rect.fLeft, rect.fTop);
        fPts[1] = SkPoint::Make(rect.fRight, rect.fTop);
        fPts[2] = SkPoint::Make(rect.fRight, rect.fBottom);
        fPts[3] = SkPoint::Make(rect.fLeft, rect.fBottom);
    }
};

class SkPath_OvalPointIterator : public SkPath_PointIterator<4> {
public:
    SkPath_OvalPointIterator(const SkRect& oval, SkPathDirection dir, unsigned startIndex)
        : SkPath_PointIterator(dir, startIndex) {
        const SkScalar cx = oval.centerX();
        const SkScalar cy = oval.centerY();
        fPts[0] = SkPoint::Make(cx, oval.fTop);
        fPts[1] = SkPoint::Make(oval.fRight, cy);
        fPts[2] = SkPoint::Make(cx, oval.fBottom);
        fPts[3] = SkPoint::Make(oval.fLeft, cy);
    }
};

class SkPath_RRectPointIterator : public SkPath_PointIterator<8> {
public:
    SkPath_RRectPointIterator(const SkRRect& rrect, SkPathDirection dir, unsigned startIndex)
        : SkPath_PointIterator(dir, startIndex) {
        const SkRect& bounds = rrect.getBounds();
        const SkScalar L = bounds.fLeft;
        const SkScalar T = bounds.fTop;
        const SkScalar R = bounds.fRight;
        const SkScalar B = bounds.fBottom;
        fPts[0] = SkPoint::Make(L + rrect.radii(SkRRect::kUpperLeft_Corner).fX, T);
        fPts[1] = SkPoint::Make(R - rrect.radii(SkRRect::kUpperRight_Corner).fX, T);
        fPts[2] = SkPoint::Make(R, T + rrect.radii(SkRRect::kUpperRight_Corner).fY);
        fPts[3] = SkPoint::Make(R, B - rrect.radii(SkRRect::kLowerRight_Corner).fY);
        fPts[4] = SkPoint::Make(R - rrect.radii(SkRRect::kLowerRight_Corner).fX, B);
        fPts[5] = SkPoint::Make(L + rrect.radii(SkRRect::kLowerLeft_Corner).fX, B);
        fPts[6] = SkPoint::Make(L, B - rrect.radii(SkRRect::kLowerLeft_Corner).fY);
        fPts[7] = SkPoint::Make(L, T + rrect.radii(SkRRect::kUpperLeft_Corner).fY);
    }
};

#endif /* SKTRIM_PATHTYPES_H */
