// Sh: A GPU metaprogramming language.
//
// Copyright (c) 2003 University of Waterloo Computer Graphics Laboratory
// Project administrator: Michael D. McCool
// Authors: Zheng Qin, Stefanus Du Toit, Kevin Moule, Tiberiu S. Popa,
//          Michael D. McCool
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must
// not claim that you wrote the original software. If you use this
// software in a product, an acknowledgment in the product documentation
// would be appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must
// not be misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
//////////////////////////////////////////////////////////////////////////////

// Evaluate cubic Bernstein (Bezier) basis functions.
template <int N, ShValueType V>
ShAttrib4f
bernstein_basis_4 (
   const ShGeneric<N,V>& t
) {
   ShAttrib4f r;
   ShAttrib<N,SH_TEMP,V> it = fillcast<N>(ShAttrib<1,SH_TEMP,V>(1.0f)) - t;
   r(0) = it*it*it;
   r(1) = 3.0*it*it*t;
   r(2) = 3.0*it*t*t;
   r(3) = t*t*t;
   return r;
}

// Evaluate cubic Bezier spline
template <int N, ShValueType V>
ShGeneric<N,V>
bezier (
   const ShGeneric<N,V>& t, 
   const ShAttrib4f& p
) {
   ShAttrib4f B = bernstein_basis_4(t);
   ShGeneric<N,V> r = B[0] * p[0];
   for (int i=1; i<4; i++) {
      r += B[i] * p[i];  
   }
   return r;
}

// linear smooth step
template <int N, ShValueType V>
ShGeneric<N,V>
sstep (
   const ShGeneric<N,V>& t, 
   const ShGeneric<N,V>& c, 
   const ShGeneric<N,V>& w
) {
   return clamp((t - c)/w + 0.5f, 0.0f, 1.0f);
}

// linear smooth step
template <int N, ShValueType V>
ShGeneric<N,V>
sstep (
   const ShGeneric<N,V>& t, 
   const ShGeneric<N,V>& w
) {
   return clamp(t/w + 0.5f, 0, 1);
}

// linear smooth pulse 
template <int N, ShValueType V>
ShGeneric<N,V>
spulse (
   const ShGeneric<N,V>& t, 
   const ShGeneric<N,V>& r0, 
   const ShGeneric<N,V>& r1, 
   const ShGeneric<N,V>& w
) {
   return sstep(t,r0,w) - sstep(t,r1,w);
}

// cubic smooth step 
template <int N, ShValueType V>
ShGeneric<N,V>
csstep (
   const ShGeneric<N,V>& t, 
   const ShGeneric<N,V>& w
) {
   ShAttrib4f p(0.0,0.0,1.0,1.0);
   return bezier(sstep(t,w),p);
}

// cubic smooth step 
template <int N, ShValueType V>
ShGeneric<N,V>
csstep (
   const ShGeneric<N,V>& t, 
   const ShGeneric<N,V>& r, 
   const ShGeneric<N,V>& w
) {
   ShAttrib4f p(0.0,0.0,1.0,1.0);
   return bezier(sstep(t,r,w),p);
}

// cubic smooth pulse 
template <int N, ShValueType V>
ShGeneric<N,V>
cspulse (
   const ShGeneric<N,V>& t, 
   const ShGeneric<N,V>& r0, 
   const ShGeneric<N,V>& r1, 
   const ShGeneric<N,V>& w
) {
   return csstep(t,r0,w) - csstep(t,r1,w);
}

