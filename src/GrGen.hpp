#ifndef GRGEN_HPP
#define GRGEN_HPP

class Shader;
class GrNode;

#include <sh/ShProgram.hpp>

SH::ShProgram generateShader(GrNode* root, GrNode* final);

#endif
