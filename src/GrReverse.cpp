#include "GrReverse.hpp"
#include <sh/sh.hpp>
#include <sh/shutil.hpp>
#include <cstdlib>
#include "GrView.hpp"
#include "GrNode.hpp"
#include "GrPort.hpp"

using namespace SH;
using namespace ShUtil;

GrPort* makeSwizzle(const ShVariable& var,
                    GrPort* source,
                    GrView* view)
{
  ShProgram splitp = SH_BEGIN_PROGRAM() {
    ShVariable in(new ShVariableNode(SH_INPUT, var.node()->size(),
                                     var.node()->specialType()));
    if (var.node()->hasName()) in.name(var.name());
    for (int i = 0; i < var.node()->size(); i++) {
      ShVariable out(new ShVariableNode(SH_OUTPUT, 1, var.node()->specialType()));
      // TODO out.name()
      ShVariable ini(in(i));
      shASN(out, ini);
    }
  } SH_END;
  splitp->name("split");

  GrNode* split = view->addProgram(splitp, 0, 0);

  join(source, *split->inputs_begin());

  ShProgram joinp = SH_BEGIN_PROGRAM() {
    ShVariable out(new ShVariableNode(SH_OUTPUT, var.size(),
                                     var.node()->specialType()));
    if (var.node()->hasName()) out.name(var.name());
    for (int i = 0; i < var.size(); i++) {
      ShVariable in(new ShVariableNode(SH_INPUT, 1, var.node()->specialType()));
      // TODO in.name()
      ShVariable outi(out(i));
      shASN(outi, in);
    }
  } SH_END;
  joinp->name("join");

  GrNode* joinn = view->addProgram(joinp, 0, 0);
  for (int i = 0; i < var.size(); i++) {
    join(*(split->outputs_begin() + var.swizzle()[i]), *(joinn->inputs_begin() + i));
  }

  return *joinn->outputs_begin();
}

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

    GrPort* sources[opInfo[stmt.op].arity];
    
    for (int i = 0; i < opInfo[stmt.op].arity; i++) {
      if (!(stmt.src[i].hasValues() || stmt.src[i].node()->kind() == SH_TEXTURE)) {
        sources[i] = varmap[stmt.src[i].node()];
        if (!sources[i]) {
          std::cerr << "No source for " << stmt.src[i].name() << std::endl;
          std::cerr << "(src[" << i << "] in " << opInfo[stmt.op].name << std::endl;
          abort();
        }

        if (!stmt.src[i].swizzle().identity()) {
          sources[i] = makeSwizzle(stmt.src[i], sources[i], view);
        }
        
      } else {
        sources[i] = 0;
      }
    }
    
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
      if (!sources[i]) continue;
      join(sources[i], *P);
      ++P;
    }

    varmap[stmt.dest.node()] = *nn->outputs_begin();
  }

  // TODO: Make output node.

  view->layout();
}
