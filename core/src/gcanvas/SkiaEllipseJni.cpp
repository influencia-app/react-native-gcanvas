/*
 * SkiaEllipseJni.cpp — JNI bridge (project code, NOT Skia code).
 *
 * Tessellation calls SkConic::BuildUnitArc from SkGeometry.cpp (trimmed
 * Skia unit in this folder): 1 conic per quadrant-or-less arc, weight
 * w = cos(phi/2). Each conic (P0, Q, P1, w) becomes one cubic bezier
 * with k = 2*w/3. No Skia code is copied here.
 */
#include <jni.h>

#include <cmath>
#include <cstdio>

#include "SkTrimGeometry.h"

namespace {

static const int kMaxOutPathOps = 4096;

struct Out {
    char* p;
    char* end;
    void put(const char* s) {
        while (*s && p + 1 < end) {
            *p++ = *s++;
        }
        *p = 0;
    }
    void num(double v) {
        char b[32];
        snprintf(b, sizeof(b), "%.6g", v);
        put(b);
    }
    void moveTo(double x, double y) {
        put("g");
        num(x);
        put(",");
        num(y);
        put(";");
    }
    void lineTo(double x, double y) {
        put("i");
        num(x);
        put(",");
        num(y);
        put(";");
    }
    void closePath() { put("o;"); }
    // One conic as cubic bezier 'z', k = 2*w/3.
    void conicAsCubic(const SkPoint& p0, const SkPoint& q, const SkPoint& p1, float w) {
        float k = (2.0f * w) / 3.0f;
        put("z");
        num(p0.fX + k * (q.fX - p0.fX));
        put(",");
        num(p0.fY + k * (q.fY - p0.fY));
        put(",");
        num(p1.fX + k * (q.fX - p1.fX));
        put(",");
        num(p1.fY + k * (q.fY - p1.fY));
        put(",");
        num(p1.fX);
        put(",");
        num(p1.fY);
        put(";");
    }
};

// Unit->ellipse map: translate(cx,cy) * rotate(rot) * scale(rx,ry).
static SkMatrix ellipseMatrix(double cx, double cy, double rx, double ry, double rot) {
    double c = cos(rot);
    double s = sin(rot);
    SkMatrix m;
    m.a = (float)(c * rx);
    m.b = (float)(s * rx);
    m.c = (float)(-s * ry);
    m.d = (float)(c * ry);
    m.tx = (float)cx;
    m.ty = (float)cy;
    return m;
}

// One directed range [a0, a1] (|range| <= 90 degrees) as cubic beziers.
// firstMove emits the leading moveTo exactly once per path.
static void emitArcRange(Out& o, const SkMatrix& m, double a0, double a1,
                         SkPathDirection dir, bool& firstMove) {
    SkVector u0;
    u0.fX = (float)cos(a0);
    u0.fY = (float)sin(a0);
    SkVector u1;
    u1.fX = (float)cos(a1);
    u1.fY = (float)sin(a1);
    SkConic dst[kMaxConicsForArc];
    int n = SkConic::BuildUnitArc(u0, u1, dir, &m, dst);
    for (int i = 0; i < n; ++i) {
        if (firstMove) {
            o.moveTo(dst[i].fPts[0].fX, dst[i].fPts[0].fY);
            firstMove = false;
        }
        o.conicAsCubic(dst[i].fPts[0], dst[i].fPts[1], dst[i].fPts[2], dst[i].fW);
    }
}

}  // namespace

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_influaap_droidnative_1p5js_P5Bridge_ellipseOpsNative(
    JNIEnv* env, jobject /*thiz*/, jdouble cx, jdouble cy, jdouble rx, jdouble ry,
    jdouble rot, jdouble start, jdouble end, jboolean ccw) {
    char out[kMaxOutPathOps];
    Out o;
    o.p = out;
    o.end = out + sizeof(out);
    *o.p = 0;
    if (rx > 0 && ry > 0) {
        double sweep = end - start;
        if (ccw) {
            sweep = -sweep;
        }
        double a = sweep >= 0 ? sweep : -sweep;
        const double kTau = 3.141592653589793 * 2.0;
        const double kEps = 1e-4;
        if (a >= kEps) {
            if (a >= kTau - 1e-4) {
                a = kTau;
            }
            int n = (int)ceil(a / (kTau / 4.0));
            if (n < 1) {
                n = 1;
            }
            if (n > 4) {
                n = 4;
            }
            double phi = (sweep < 0 ? -a : a) / n;
            SkMatrix m = ellipseMatrix(cx, cy, rx, ry, rot);
            SkPathDirection dir = ccw ? kCCW : kCW;
            bool firstMove = true;
            double t = start;
            for (int i = 0; i < n; ++i) {
                emitArcRange(o, m, t, t + phi, dir, firstMove);
                t += phi;
            }
        }
    }
    return env->NewStringUTF(out);
}

JNIEXPORT jstring JNICALL
Java_com_influaap_droidnative_1p5js_P5Bridge_roundRectOpsNative(
    JNIEnv* env, jobject /*thiz*/, jdouble x, jdouble y, jdouble w, jdouble h,
    jdouble tl, jdouble tr, jdouble br, jdouble bl) {
    char out[kMaxOutPathOps];
    Out o;
    o.p = out;
    o.end = out + sizeof(out);
    *o.p = 0;
    double hw = (w >= 0 ? w : -w) / 2.0;
    double hh = (h >= 0 ? h : -h) / 2.0;
    if (tl < 0) tl = 0; if (tr < 0) tr = 0; if (br < 0) br = 0; if (bl < 0) bl = 0;
    if (tl > hw) tl = hw; if (tl > hh) tl = hh;
    if (tr > hw) tr = hw; if (tr > hh) tr = hh;
    if (br > hw) br = hw; if (br > hh) br = hh;
    if (bl > hw) bl = hw; if (bl > hh) bl = hh;
    const double kHalfPi = 3.141592653589793 / 2.0;
    o.moveTo(x + tl, y);
    o.lineTo(x + w - tr, y);
    if (tr > 0) {
        SkMatrix m = ellipseMatrix(x + w - tr, y + tr, tr, tr, 0);
        bool firstMove = false;
        emitArcRange(o, m, -kHalfPi, 0.0, kCW, firstMove);
    }
    o.lineTo(x + w, y + h - br);
    if (br > 0) {
        SkMatrix m = ellipseMatrix(x + w - br, y + h - br, br, br, 0);
        bool firstMove = false;
        emitArcRange(o, m, 0.0, kHalfPi, kCW, firstMove);
    }
    o.lineTo(x + bl, y + h);
    if (bl > 0) {
        SkMatrix m = ellipseMatrix(x + bl, y + h - bl, bl, bl, 0);
        bool firstMove = false;
        emitArcRange(o, m, kHalfPi, 2.0 * kHalfPi, kCW, firstMove);
    }
    o.lineTo(x, y + tl);
    if (tl > 0) {
        SkMatrix m = ellipseMatrix(x + tl, y + tl, tl, tl, 0);
        bool firstMove = false;
        emitArcRange(o, m, 2.0 * kHalfPi, 3.0 * kHalfPi, kCW, firstMove);
    }
    o.closePath();
    return env->NewStringUTF(out);
}

}  // extern "C"
