/** SegDist.
 *
 * Compute distance and gradient from a point to a line segment.
 *
 * This functions returns the squared distance, an evaluation of the
 * plane equation (whose sign lets us know which side of the edge
 * we are on), and the gradient of the distance to a line segment.  
 * These are packed into the components of a 4-tuple.   
 *
 * We depend on dead-code elimination (which is supposed to work 
 * on components of tuples) to get rid of computations that are not 
 * needed.   
 */
ShAttrib4f //< square distance, sign (plane equation), unnorm gradient vector
segdist (
    ShAttrib4f L,  //< line segment (0,1 point 0, 2,3 point 1)
    ShPoint2f X    //< test point
);

ShAttrib4f //< square distance, sign (plane equation), unnorm gradient vector
segdist_d (
    ShAttrib4f L,  //< line segment (0,1 point 0, 2,3 vector to point 1)
    ShPoint2f X    //< test point
);

/** SegDists.
 *
 * Compute signed distance and gradient from a point to a contour
 * represented as a sequence of line segments.   Resolves ambiguity at
 * shared endpoints using pseudodistance test.
 *
 * Computes the signed distance, pseudodistance, and gradient vector
 * of the signed distance.
 */
ShAttrib4f //< signed distance, signed value (dist from line), gradient vector
segdists (
    ShAttrib4f L[],  //< line segments (0,1 point 0, 2,3 point 1)
    int N,           // number of line segments
    ShPoint2f X      //< test point
); 
ShAttrib4f //< signed distance, signed value (dist from line), gradient vector
segdists_d (
    ShAttrib4f L[],  //< line segments (0,1 point 0, 2,3 vector to point 1)
    int N,           // number of line segments
    ShPoint2f X      //< test point
); 

/** SegDists.
 *
 * Compute signed distance and gradient from a point to a contour
 * represented as a sequence of line segments.   Does not attempt to
 * resolve ambiguity.
 *
 * Computes the signed distance, pseudodistance, and gradient vector
 * of the signed distance.
 */
ShAttrib4f //< signed distance, signed value (dist from line), gradient vector
segdists_a (
    ShAttrib4f L[],  //< line segments (0,1 point 0, 2,3 point 1)
    int N,           // number of line segments
    ShPoint2f X      //< test point
); 
ShAttrib4f //< signed distance, signed value (dist from line), gradient vector
segdists_da (
    ShAttrib4f L[],  //< line segments (0,1 point 0, 2,3 vector to point 1)
    int N,           // number of line segments
    ShPoint2f X      //< test point
); 

/** CornerDist.
 *
 * Compute distance and gradient from a point to a `corner'.   A corner
 * consists of two line segments joined at a common vertex.
 *
 * This functions returns the squared distance, an evaluation of the
 * pseudodistance (whose sign lets us know which side of the corner
 * we are on), and the gradient of the distance to the corner.  
 * These are packed into the components of a 4-tuple.   
 *
 * We depend on dead-code elimination (which is supposed to work 
 * on components of tuples) to get rid of computations that are not 
 * needed.   
 */
ShAttrib4f //< square distance, sign (plane equation), unnorm gradient vector
cornerdist (
    ShAttrib4f c,  //< corner point and normal of separating plane
    ShAttrib4f d,  //< direction vectors away from corner
    ShPoint2f X    //< test point
);

ShAttrib4f //< square distance, sign (plane equation), unnorm gradient vector
cornerdist_d (
    ShAttrib4f c,  //< corner point and normal of separating plane
    ShAttrib4f d,  //< direction vectors away from corner
    ShPoint2f X    //< test point
);

/** CornerDists.
 *
 * Compute signed distance and gradient from a point to a contour
 * represented as a sequence of corners.  
 *
 * Computes the signed distance, pseudodistance, and gradient vector
 * of the signed distance.
 */
ShAttrib4f //< signed distance, signed value (dist from line), gradient vector
cornerdists (
    ShAttrib4f c[],  //< corner points and normals of separating planes
    ShAttrib4f d[],  //< direction vectors away from corner
    int N,           // number of corners
    ShPoint2f X      //< test point
); 
ShAttrib4f //< signed distance, signed value (dist from line), gradient vector
cornerdists_d (
    ShAttrib4f c[],  //< corner points and normals of separating planes
    ShAttrib4f d[],  //< direction vectors away from corner
    int N,           // number of corners
    ShPoint2f X      //< test point
); 
