
// Clamp to a given range 
template <int N, typename T>
ShGeneric<N,T>
clamp (
   const ShGeneric<N,T>& s, 
   const ShGeneric<N,T>& lower, 
   const ShGeneric<N,T>& upper
) {
   return min(max(s,lower),upper);
}

// Clamp to a given range 
template <int N, typename T>
ShGeneric<N,T>
clamp (
   const ShGeneric<N,T>& s, 
   const ShGeneric<1,T>& lower, 
   const ShGeneric<1,T>& upper
) {
   return min(max(s,fillcast<N>(lower)),fillcast<N>(upper));
}

// Clamp to a given range 
template <int N, typename T>
ShGeneric<N,T>
clamp (
   const ShGeneric<N,T>& s, 
   float lower, 
   float upper
) {
   return min(max(s,fillcast<N>(ShAttrib<1,SH_TEMP,T>(lower))),
		    fillcast<N>(ShAttrib<1,SH_TEMP,T>(upper)));
}

// Evaluate cubic Bernstein (Bezier) basis functions.
template <int N, typename T>
ShAttrib4f
bernstein_basis_4 (
   const ShGeneric<N,T>& t
) {
   ShAttrib4f r;
   ShAttrib<N,SH_TEMP,T> it = fillcast<N>(ShAttrib<1,SH_TEMP,T>(1.0f)) - t;
   r(0) = it*it*it;
   r(1) = it*it*t;
   r(2) = it*t*t;
   r(3) = t*t*t;
   return r;
}

// Evaluate cubic Bezier spline
template <int N, typename T>
ShGeneric<N,T>
bezier (
   const ShGeneric<N,T>& t, 
   const ShAttrib4f& p
) {
   ShAttrib4f B = bernstein_basis_4(t);
   ShGeneric<N,T> r = B[0] * p[0];
   for (int i=1; i<4; i++) {
      r += B[i] * p[i];  
   }
   return r;
}

// linear smooth step
template <int N, typename T>
ShGeneric<N,T>
sstep (
   const ShGeneric<N,T>& t, 
   const ShGeneric<N,T>& c, 
   const ShGeneric<N,T>& w
) {
   return clamp((t - c)/w + 0.5, 0.0, 1.0);
}

// linear smooth step
template <int N, typename T>
ShGeneric<N,T>
sstep (
   const ShGeneric<N,T>& t, 
   const ShGeneric<N,T>& w
) {
   return clamp(t/w + 0.5, 0.0, 1.0);
}

// linear smooth pulse 
template <int N, typename T>
ShGeneric<N,T>
spulse (
   const ShGeneric<N,T>& t, 
   const ShGeneric<N,T>& r0, 
   const ShGeneric<N,T>& r1, 
   const ShGeneric<N,T>& w
) {
   return sstep(t,r0,w) - sstep(t,r1,w);
}

// cubic smooth step 
template <int N, typename T>
ShGeneric<N,T>
csstep (
   const ShGeneric<N,T>& t, 
   const ShGeneric<N,T>& w
) {
   ShAttrib4f p(0.0,0.0,1.0,1.0);
   return bezier(sstep(t,w),p);
}

// cubic smooth step 
template <int N, typename T>
ShGeneric<N,T>
csstep (
   const ShGeneric<N,T>& t, 
   const ShGeneric<N,T>& r, 
   const ShGeneric<N,T>& w
) {
   ShAttrib4f p(0.0,0.0,1.0,1.0);
   return bezier(sstep(t,r,w),p);
}

// cubic smooth pulse 
template <int N, typename T>
ShGeneric<N,T>
cspulse (
   const ShGeneric<N,T>& t, 
   const ShGeneric<N,T>& r0, 
   const ShGeneric<N,T>& r1, 
   const ShGeneric<N,T>& w
) {
   return csstep(t,r0,w) - csstep(t,r1,w);
}

