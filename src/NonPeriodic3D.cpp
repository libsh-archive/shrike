#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <iostream>
#include <sstream>
#include "Shader.hpp"
#include "Globals.hpp"

using namespace SH;
using namespace ShUtil;

const double DIFFR = 0.05;

class NonPeriodic3D : public Shader {
public:
  NonPeriodic3D();
  ~NonPeriodic3D();

  bool init();

  ShProgram vertex() { return vsh;}
  ShProgram fragment() { return fsh;}

  ShProgram vsh, fsh;

  static NonPeriodic3D instance;
};

NonPeriodic3D::NonPeriodic3D()
  : Shader("Tiling: 3D Nonperiodic")
{
}

NonPeriodic3D::~NonPeriodic3D()
{
}

ShColor3f defcolor = ShColor3f(1.0f, 0.0f, 1.0f);
ShColor3f color[3] = {ShColor3f(1.0f, 0.0f, 0.0f), ShColor3f(0.0f, 1.0f, 0.0f), ShColor3f(0.0f, 0.0f, 1.0f)};
 
ShPoint3f modPoint(ShPoint3f p)
{
  return p - floor( p + ShAttrib3f(0.5,0.5,0.5) );
}

void testPlane(
   ShVector3f body,
   ShAttrib4f planecoord,
   ShVector3f pt0,
   ShPoint3f pt1,
   ShColor3f col,
   ShColor3f& C,
   ShAttrib1f& best
)
{
  ShVector3f plane = ShVector3f(planecoord(0), planecoord(1), planecoord(2));
  ShPoint4f pt1p;
  pt1p(0,1,2) = pt1;
  pt1p(3) = 1.0;
  ShAttrib1f t = -((planecoord | pt1p)) / (body | plane);
  ShColor3f previous = C;
  
  ShPoint3f p = pt0 + t*body;

  ShAttrib1f xmin = floor(p(1)+0.5)-0.5;
  ShAttrib1f ymin = floor(p(2)+0.5)-0.5;
  
  ShAttrib1f dmax = body | ShVector3f(planecoord(3), -xmin, -ymin);
  ShAttrib1f dmin = body | ShVector3f(planecoord(3), -xmin-1.0, -ymin-1.0);
  ShAttrib1f my_d = -(body | pt0);

  ShAttrib1f diff = min( fmod(p(1)+0.5, 1.0), fmod(p(2)+0.5, 1.0));
  C = cond( diff<0.05, diff/0.05*col, col);
 
  ShAttrib1f ignore = max(max(p(1)<-0.5, p(1)>1.5), max(p(2)<-0.5, p(2)>1.5));
  ignore = max(ignore,min(my_d>(dmin-1e-7), my_d<(dmax+1e-7)));
  ignore = max(ignore, t > best);
  
  C = cond( ignore, previous, C);
  best = cond(ignore, best, t);
}

ShColor3f testCube(
  ShVector3f body,
  ShPoint3f p
)
{
  ShColor3f result;
  ShAttrib1f best = 100000.0;

  result = ShColor3f(1.0, 1.0, 0.0);
 
  for (int i = 0; i < 3; i++) {
    ShAttrib4f plane = ShAttrib4f(1.0, 0.0, 0.0, -0.5);
    testPlane( body, plane, (ShVector3f)p, p, color[i], result, best );
    plane = ShAttrib4f(1.0, 0.0, 0.0, -1.5);
    testPlane( body, plane, (ShVector3f)p, p, color[i], result, best );
    body = body(1,2,0);
    p = p(1,2,0);
  }
  
  return result;
}

bool NonPeriodic3D::init()
{
  vsh = SH_BEGIN_PROGRAM("gpu:vertex") {
    ShInputPosition4f ipos;
    
    ShOutputPosition4f opos; // Position in NDC
    ShInOutTexCoord2f tc; // pass through tex coords

    opos = Globals::mvp | ipos; // Compute NDC position
  } SH_END;
  
  ShAttrib2f SH_DECL(scale) = ShAttrib2f(10.0, 10.0);
  scale.range(1.0, 100.0);

  ShVector3f SH_DECL(body) = ShVector3f(1.0, 1.0, 1.0);
  body.name("plane orientation");
  body.range(0.0, 20.0);

  for (int i = 0; i < 3; i++) {
    std::ostringstream os;
    os << "Colour " << i;
    color[i].name(os.str());
  }

  // This is a 3D nonperiodic tiling from Craig Kaplan.
  // It should be possible to extend this to make Penrose tiles.
  fsh = SH_BEGIN_PROGRAM("gpu:fragment") {
    ShInputTexCoord2f tc; // ignore texcoords
    
    ShOutputColor3f result;
    
    body = normalize(body);

    tc = scale*tc;
  
    ShVector3f v1;
    v1[0] = 1.0/body(0);
    v1[1] = -1.0/body(1);
    v1[2] = 0.0;
    v1 = normalize(v1);

    ShVector3f v2 = cross(body,v1);
    v2 = normalize(v2);
    
    ShPoint3f p = tc(0)*v1 + tc(1)*v2;
    p = modPoint(p);
    
    result = testCube(body, p);
    
  } SH_END;
  return true;
}

NonPeriodic3D NonPeriodic3D::instance = NonPeriodic3D();






