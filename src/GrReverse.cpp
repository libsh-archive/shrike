#include "GrReverse.hpp"
#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <cstdlib>
#include "GrView.hpp"
#include "GrNode.hpp"
#include "GrPort.hpp"

using namespace SH;
using namespace ShUtil;

void decompose(GrNode* source)
{
  if (!source) {
    std::cerr << "Can't decompose a null pointer!" << std::endl;
    return;
  }
  GrView* view = source->view();
  
  ShProgram program = source->program();

  ShCtrlGraphNodePtr entry = program->ctrlGraph->entry();

  if (!entry) {
    std::cerr << "Could not obtain entry node from control graph." << std::endl;
    return;
  }
  ShCtrlGraphNodePtr n = entry->follower;

  if (!n) {
    std::cerr << "Could not obtain body node from control graph." << std::endl;
    return;
  }

  ShBasicBlockPtr block = n->block;
  
  if (!block) {
    std::cerr << "Could not obtain basic block from control graph." << std::endl;
    return;
  }

  std::map<ShVariableNodePtr, GrPort*> varmap;

  ShProgram inp = ShKernelLib::inputPass(program);

  GrNode* inn = view->addProgram(inp, 0, 0);

  ShProgramNode::VarList::iterator I = program->inputs.begin();
  for (GrNode::PortList::iterator P = inn->outputs_begin();
       P != inn->outputs_end();
       ++P, ++I) {
    varmap[*I] = *P;
  }

//   ShProgram outp = ShKernelLib::outputPass(program);
//   GrNode* outn = view->addProgram(outp, 0, 0);
  
  for (ShBasicBlock::ShStmtList::iterator I = block->begin(); I != block->end(); ++I) {
    const ShStatement& stmt = *I;

    ShProgram nibble = SH_BEGIN_PROGRAM() {
      ShVariable dest(new ShVariableNode(SH_OUTPUT, stmt.dest.node()->size(),
                                         stmt.dest.node()->specialType()),
                      stmt.dest.swizzle(), stmt.dest.neg());
      if (stmt.dest.node()->hasName()) dest.name(stmt.dest.name());
      switch (opInfo[stmt.op].arity) {
        case 0:
        {
          ShStatement ns(dest, stmt.op);
          nibble->tokenizer.blockList()->addStatement(ns);
          break;
        }
        case 1:
        {
          ShVariableNodePtr node0;
          if (!(stmt.src[0].hasValues() || stmt.src[0].node()->kind() == SH_TEXTURE)) {
            node0 = new ShVariableNode(SH_INPUT, stmt.src[0].node()->size(),
                                       stmt.src[0].node()->specialType());

            if (stmt.src[0].node()->hasName()) node0->name(stmt.src[0].name());
          } else {
            node0 = stmt.src[0].node();
          }
          ShVariable src0(node0, stmt.src[0].swizzle(), stmt.src[0].neg());

          ShStatement ns(dest, stmt.op, src0);
          nibble->tokenizer.blockList()->addStatement(ns);
          break;
        }
        case 2:
        {
          ShVariableNodePtr node0;
          if (!(stmt.src[0].hasValues() || stmt.src[0].node()->kind() == SH_TEXTURE)) {
            node0 = new ShVariableNode(SH_INPUT, stmt.src[0].node()->size(),
                                       stmt.src[0].node()->specialType());
            if (stmt.src[0].node()->hasName()) node0->name(stmt.src[0].name());
          } else {
            node0 = stmt.src[0].node();
          }
          ShVariable src0(node0, stmt.src[0].swizzle(), stmt.src[0].neg());

          ShVariableNodePtr node1;
          if (!(stmt.src[1].hasValues() || stmt.src[1].node()->kind() == SH_TEXTURE)) {
            node1 = new ShVariableNode(SH_INPUT, stmt.src[1].node()->size(),
                                       stmt.src[1].node()->specialType());
            if (stmt.src[1].node()->hasName()) node1->name(stmt.src[1].name());
          } else {
            node1 = stmt.src[1].node();
          }
          ShVariable src1(node1, stmt.src[1].swizzle(), stmt.src[1].neg());
          
          ShStatement ns(dest, src0, stmt.op, src1);
          nibble->tokenizer.blockList()->addStatement(ns);
          break;
        }
        case 3:
        {
          ShVariableNodePtr node0;
          if (!(stmt.src[0].hasValues() || stmt.src[0].node()->kind() == SH_TEXTURE)) {
            node0 = new ShVariableNode(SH_INPUT, stmt.src[0].node()->size(),
                                       stmt.src[0].node()->specialType());
            if (stmt.src[0].node()->hasName()) node0->name(stmt.src[0].name());
          } else {
            node0 = stmt.src[0].node();
          }
          ShVariable src0(node0, stmt.src[0].swizzle(), stmt.src[0].neg());

          ShVariableNodePtr node1;
          if (!(stmt.src[1].hasValues() || stmt.src[1].node()->kind() == SH_TEXTURE)) {
            node1 = new ShVariableNode(SH_INPUT, stmt.src[1].node()->size(),
                                       stmt.src[1].node()->specialType());
            if (stmt.src[1].node()->hasName()) node1->name(stmt.src[1].name());
          } else {
            node1 = stmt.src[1].node();
          }
          ShVariable src1(node1, stmt.src[1].swizzle(), stmt.src[1].neg());

          ShVariableNodePtr node2;
          if (!(stmt.src[2].hasValues() || stmt.src[2].node()->kind() == SH_TEXTURE)) {
            node2 = new ShVariableNode(SH_INPUT, stmt.src[2].node()->size(),
                                       stmt.src[2].node()->specialType());
            if (stmt.src[2].node()->hasName()) node2->name(stmt.src[2].name());
          } else {
            node2 = stmt.src[2].node();
          }
          ShVariable src2(node2, stmt.src[2].swizzle(), stmt.src[2].neg());
          
          ShStatement ns(dest, stmt.op, src0, src1, src2);
          nibble->tokenizer.blockList()->addStatement(ns);
          break;
        }
      }
    } SH_END;

    nibble->name(opInfo[stmt.op].name);
    
    GrNode* nn = view->addProgram(nibble, 0, 0);
    
    GrNode::PortList::iterator P = nn->inputs_begin();
    for (int i = 0; i < opInfo[stmt.op].arity; i++) {
      if (!(stmt.src[i].hasValues() || stmt.src[i].node()->kind() == SH_TEXTURE)) {
        GrPort* port = varmap[stmt.src[i].node()];
        if (!port) {
          std::cerr << "No source for " << stmt.src[i].name() << std::endl;
          std::cerr << "(src[" << i << "] in " << opInfo[stmt.op].name << std::endl;
          abort();
        }
        join(port, *P);
        ++P;
      }
    }

    varmap[stmt.dest.node()] = *nn->outputs_begin();
  }

  // TODO: Make output node.

  view->layout();
}
