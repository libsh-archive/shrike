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
#include <iostream>
#include <string>
#include "ShObjFile.hpp"

namespace {

struct Triple {
  int idx[3];
};

void readFace(std::istream& in, std::vector<Triple>& triples);

}

namespace SH {

ShObjFile::ShObjFile(const ShObjFile& other)
  : vertices(other.vertices),
    texcoords(other.texcoords),
    normals(other.normals),
    tangents(other.tangents),
    faces(other.faces)
{
}

ShObjFile::~ShObjFile()
{
}

ShObjFile& ShObjFile::operator=(const ShObjFile& other)
{
  vertices = other.vertices;
  texcoords = other.texcoords;
  normals = other.normals;
  tangents = other.tangents;
  faces = other.faces;
  return *this;
}

ShObjFile::ShObjFile()
{
}

std::istream& operator>>(std::istream& in, ShObjFile& obj)
{
  obj = ShObjFile();

  char ch = 0;
  while (in) {
    in >> std::ws >> ch;
    if (!in) break; // TODO: Check for error conditions.
    switch (ch) {
    case 'v': {
      ch = in.get();
      switch (ch) {
      case ' ': {
        // Vertex
        float x, y, z;
        in >> x >> y >> z;
        obj.vertices.push_back(ShPoint3f(x, y, z));
        break;
      }
      case 't': {
        // Texture coordinate
        float u, v;
        in >> u >> v;
        obj.texcoords.push_back(ShTexCoord2f(u, v));
        break;
      }
      case 'n': {
        // Normal
        float x, y, z;
        in >> x >> y >> z;
        obj.normals.push_back(ShNormal3f(x, y, z));
        break;
      }
      }
      while (in && in.get() != '\n') ; // Ignore rest of line
      break;
    }
    case 'r': { // Tangent
      float x, y, z;
      in >> x >> y >> z;
      obj.tangents.push_back(ShVector3f(x, y, z));
      while (in && in.get() != '\n') ; // Ignore rest of line
      break;
    }
    case 'f': { // Face
      std::vector<Triple> triples;
      readFace(in, triples);
      
      if (triples.size() == 3) {
        ShObjFace face; 
        // Simple triangle.
        for (int i = 0; i < 3; i++) face.points[i] = triples[i].idx[0] - 1;
        for (int i = 0; i < 3; i++) face.texcoords[i] = triples[i].idx[1] - 1;
        for (int i = 0; i < 3; i++) face.normals[i] = triples[i].idx[2] - 1;
        for (int i = 0; i < 3; i++) face.tangents[i] = triples[i].idx[0] - 1;
        obj.faces.push_back(face);
      } else {
        // Not a triangle. Assume simple polygon, triangulate by ear
        // cutting.
        for (std::size_t i = 2; i < triples.size(); i++) {
          ShObjFace face; 
          face.points[0] = triples[0].idx[0] - 1;
          face.points[1] = triples[i - 1].idx[0] - 1;
          face.points[2] = triples[i].idx[0] - 1;
          
          face.texcoords[0] = triples[0].idx[1] - 1;
          face.texcoords[1] = triples[i - 1].idx[1] - 1;
          face.texcoords[2] = triples[i].idx[1] - 1;
          
          face.normals[0] = triples[0].idx[2] - 1;
          face.normals[1] = triples[i - 1].idx[2] - 1;
          face.normals[2] = triples[i].idx[2] - 1;

          face.tangents[0] = triples[0].idx[2] - 1;
          face.tangents[1] = triples[i - 1].idx[2] - 1;
          face.tangents[2] = triples[i].idx[2] - 1;
          obj.faces.push_back(face);
        }
      }
      break;
    }
    case '#': // Comment
    case '$': // Apparently this is also used for comments sometimes
    case 'l': // Line, ignore
    case 'p': // Point, ignore
    case 'm': // material library, ignore
    case 'g': // group, ignore
    case 's': // smoothing group, ignore
    case 'u': // material, ignore
    default: // anything else we ignore as well
      while (in && in.get() != '\n') ; // Ignore rest of line
      break;
    }
  }
  return in;
}

}

namespace  {

void readFace(std::istream& in, std::vector<Triple>& triples)
{
  char ch;

  while (in) {
    ch = in.get();
    if (ch >= '0' && ch <= '9') {
      // Triple coming up
      int i = 0;
      Triple entry;
      entry.idx[0] = ch - '0';
      entry.idx[1] = 0;
      entry.idx[2] = 0;

      bool done = false;
      while (!done) {
        ch = in.get();
        if (ch >= '0' && ch <= '9') {
          entry.idx[i] = entry.idx[i]*10 + (ch - '0');
        } else if (ch == '/') {
          i++;
        } else if (ch == '\n') {
          triples.push_back(entry);
          return;
        } else if (isspace(ch)) {
          triples.push_back(entry);
          done = true;
        } else {
          if (!in) return;
          // Bad face input
        }
      }
    } else if (!isspace(ch)) {
      if (!in) return;
      // Bad face input
    } else if (ch == '\n') {
      return;
    }
  }
}

}
