#include <sh/sh.hpp>
#include <sh/shutil.hpp>

using namespace SH;
using namespace ShUtil;

#include "dist_util.hpp"

ShAttrib4f //< square distance, sign (plane equation), unnorm gradient vector
segdist (
    ShAttrib4f L,  //< line segment (0,1 point 0, 2,3 point 1)
    ShPoint2f  x   //< test point
) {
    // compute 2D vector from first endpoint to x 
    ShAttrib2f v = x - L(0,1);
    // compute tangent (should really precompute, but...) 
    ShAttrib2f d = L(2,3) - L(0,1);
    // compute squared length of tangent
    ShAttrib1f d2 = (d|d);
    // compute t value of closest point on line
    ShAttrib1f t = (v|d)/d2;
    // clamp to range (0,1) 
    t = pos(t);
    t = sat(t);
    // compute point on line 
    ShAttrib2f p = L(0,1) + t*d;

    // configure return value
    ShAttrib4f r;
    // compute vector from p to x (is gradient)
    r(2,3) = x - p;
    // compute squared distance
    r(0) = (r(2,3)|r(2,3));
    // compute sign using plane equa+epsilontion; normal is (-d(1),d(0))
    // r(1) = v(1)*d(0) - v(0)*d(1); (works fine for sign, bad pseudodistance)
    r(1) = (v(1)*d(0) - v(0)*d(1))*rsqrt(d2);

    return r;
}

ShAttrib4f //< square distance, sign (plane equation), unnorm gradient vector
segdist_d (
    ShAttrib4f L,  //< line segment (0,1 point 0, 2,3 vector to point 1)
    ShPoint2f  x   //< test point
) {
    // compute 2D vector from first endpoint to x 
    ShAttrib2f v = x - L(0,1);
    // compute squared length of tangent
    ShAttrib1f d2 = (L(2,3)|L(2,3));
    // compute t value of closest point on line
    ShAttrib1f t = (v|L(2,3))/d2;
    // clamp to range (0,1) 
    t = pos(t);
    t = sat(t);
    // compute point on line 
    ShAttrib2f p = L(0,1) + t*L(2,3);

    // configure return value
    ShAttrib4f r;
    // compute vector from p to x (is gradient)
    r(2,3) = x - p;
    // compute squared distance
    r(0) = (r(2,3)|r(2,3));
    // compute sign using plane equation
    r(1) = (v(1)*L(2) - v(0)*L(3))*rsqrt(d2);

    return r;
}

ShAttrib4f //< signed distance, signed value (dist from line), gradient vector
segdists (
    ShAttrib4f L[],  //< line segments (0,1 point 0, 2,3 point 1)
    int N,           // number of line segments
    ShPoint2f X      //< test point
) {
    static float epsilon = 1e-6;
    ShAttrib4f r = segdist(L[0],X);
    ShAttrib4f ra, rb, nr;
    for (int i=1; i<N; i++) {
       ra = segdist(L[i],X);
       rb = r;
       // merge distances, using pseudodistance to resolve ambiguities at vertices
       nr = cond(abs(ra(1)) > abs(rb(1)),ra,rb);
       r = cond(ra(0) < rb(0) - epsilon,ra,nr);
       r = cond(rb(0) < ra(0) - epsilon,rb,r);
    }
    // compute true distance from squared distance
    r(0) = sqrt(r(0));  // is also length of gradient
    // transfer sign from pseudodistance
    r(0) = cond(r(1) < 0.0, -r(0), r(0));
    // normalize gradient (and transfer sign)
    r(2,3) = r(2,3)/r(0);
    return r;
}

ShAttrib4f //< signed distance, signed value (dist from line), gradient vector
segdists_d (
    ShAttrib4f L[],  //< line segments (0,1 point 0, 2,3 vector to point 1)
    int N,           // number of line segments
    ShPoint2f X      //< test point
) {
    static float epsilon = 1e-6;
    ShAttrib4f r = segdist_d(L[0],X);
    ShAttrib4f ra, rb, nr;
    for (int i=1; i<N; i++) {
       ra = segdist_d(L[i],X);
       rb = r;
       // merge distances, using pseudodistance to resolve ambiguities at vertices
       nr = cond(abs(ra(1)) > abs(rb(1)),ra,rb);
       r = cond(ra(0) < rb(0) - epsilon,ra,nr);
       r = cond(rb(0) < ra(0) - epsilon,rb,r);
    }
    // compute true distance from squared distance
    r(0) = sqrt(r(0));  // is also length of gradient
    // transfer sign from pseudodistance
    r(0) = cond(r(1) < 0.0, -r(0), r(0));
    // normalize gradient (and transfer sign)
    r(2,3) = r(2,3)/r(0);
    return r;
}

ShAttrib4f //< signed distance, signed value (dist from line), gradient vector
segdists_a (
    ShAttrib4f L[],  //< line segments (0,1 point 0, 2,3 point 1)
    int N,           // number of line segments
    ShPoint2f X      //< test point
) {
    ShAttrib4f nr;
    ShAttrib4f r = segdist(L[0],X);
    for (int i=1; i<N; i++) {
       nr = segdist(L[i],X);
       r = cond(r(0) < nr(0),r,nr);
    }
    // compute true distance from squared distance
    r(0) = sqrt(r(0));  // is also length of gradient
    // transfer sign from pseudodistance
    r(0) = cond(r(1) < 0.0, -r(0), r(0));
    // normalize gradient (and transfer sign)
    r(2,3) = r(2,3)/r(0);
    return r;
}

ShAttrib4f //< signed distance, signed value (dist from line), gradient vector
segdists_da (
    ShAttrib4f L[],  //< line segments (0,1 point 0, 2,3 vector to point 1)
    int N,           // number of line segments
    ShPoint2f X      //< test point
) {
    ShAttrib4f nr;
    ShAttrib4f r = segdist_d(L[0],X);
    for (int i=1; i<N; i++) {
       nr = segdist_d(L[i],X);
       r = cond(r(0) < nr(0),r,nr);
    }
    // compute true distance from squared distance
    r(0) = sqrt(r(0));  // is also length of gradient
    // transfer sign from pseudodistance
    r(0) = cond(r(1) < 0.0, -r(0), r(0));
    // normalize gradient (and transfer sign)
    r(2,3) = r(2,3)/r(0);
    return r;
}

ShAttrib4f //< square distance, sign (plane equation), unnorm gradient vector
cornerdist (
    ShAttrib4f c,  // vertex position and normal of separating plane   
    ShAttrib4f d, // direction vectors of line segment
    ShPoint2f  X  // test point 
) {
    // compute 2D vector from vertex to X 
    ShAttrib2f v = X - c(0,1);
    // figure out which side of the separating plane we are on
    ShAttrib1f ns = c(2,3)|v;

    // set up line parameters
    ShAttrib4f L;
    ShAttrib4f e = c(0,1,0,1) + d;
    L(0,1) = cond(ns < 0,e(0,1),c(0,1));
    L(2,3) = cond(ns < 0,c(0,1),e(2,3));

    // solve rest using distance to line
    return segdist(L,X);
}

ShAttrib4f //< square distance, sign (plane equation), unnorm gradient vector
cornerdist_d (
    ShAttrib4f c, // vertex position and normal of separating plane   
    ShAttrib4f d, // direction vectors of line segment
    ShPoint2f  X  // test point 
) {
    // compute 2D vector from vertex to X 
    ShAttrib2f v = X - c(0,1);
    // figure out which side of the separating plane we are on
    ShAttrib1f ns = c(2,3)|v;

    // set up line parameters
    ShAttrib4f L;
    L(0,1) = cond(ns < 0,c(0,1)+d(0,1),c(0,1));
    L(2,3) = cond(ns < 0,-d(0,1),d(2,3));

    // solve rest using distance to line
    return segdist_d(L,X);
}

ShAttrib4f //< signed distance, signed value (dist from line), gradient vector
cornerdists (
    ShAttrib4f c[],
    ShAttrib4f d[], 
    int K,         
    ShPoint2f X   
) {
    ShAttrib4f r = cornerdist(c[0],d[0],X);
    ShAttrib4f nr;
    for (int i=1; i<K; i++) {
       nr = cornerdist(c[i],d[i],X);
       r = cond(r(0) < nr(0),r,nr);
    }
    // compute true distance from squared distance
    r(0) = sqrt(r(0));  // is also length of gradient
    // transfer sign from pseudodistance
    r(0) = cond(r(1) < 0.0, -r(0), r(0));
    // normalize gradient (and transfer sign)
    r(2,3) = r(2,3)/r(0);
    return r;
}

ShAttrib4f //< signed distance, signed value (dist from line), gradient vector
cornerdists_d (
    ShAttrib4f c[],
    ShAttrib4f d[], 
    int K,         
    ShPoint2f X   
) {
    ShAttrib4f r = cornerdist_d(c[0],d[0],X);
    ShAttrib4f nr;
    for (int i=1; i<K; i++) {
       nr = cornerdist_d(c[i],d[i],X);
       r = cond(r(0) < nr(0),r,nr);
    }
    // compute true distance from squared distance
    r(0) = sqrt(r(0));  // is also length of gradient
    // transfer sign from pseudodistance
    r(0) = cond(r(1) < 0.0, -r(0), r(0));
    // normalize gradient (and transfer sign)
    r(2,3) = r(2,3)/r(0);
    return r;
}
