#include "GrGen.hpp"
#include <map>
#include <list>
#include <algorithm>
#include <iterator>
#include <sh/sh.hpp>
#include "GrNode.hpp"
#include "GrPort.hpp"
#include "GrMonitor.hpp"

using namespace SH;

struct VarInfo {
  VarInfo(GrPort* port) : marked(false), port(port) {}
  bool marked;
  GrPort* port;
};

struct OutVarInfo : public VarInfo {
  OutVarInfo(const ShProgram& target, GrPort* port, GrNode* target_node)
    : VarInfo(port), target(target), target_node(target_node)
  {
  }
  ShProgram target;
  GrNode* target_node;
};

struct InVarInfo : public VarInfo {
  InVarInfo(GrPort* source, GrPort* port)
    : VarInfo(port), source(source)
  {
  }
  GrPort* source;
};

std::map<ShVariableNodePtr, InVarInfo*> info_in_map;
std::map<ShVariableNodePtr, OutVarInfo*> info_out_map;

void makeVariableInfoNode(GrNode* node)
{
  if (node->marked()) {
    std::cerr << "Already marked " << node->program()->name() << std::endl;
    return;
  }
  node->mark(true);

  std::cerr << "Making info for " << node->program()->name() << std::endl;
  
  for (GrNode::PortList::iterator I = node->inputs_begin(); I != node->inputs_end(); ++I) {
    GrPort* port = *I;
    for (GrPort::EdgeList::iterator J = port->begin_edges(); J != port->end_edges(); ++J) {
      GrEdge* edge = *J;
      if (edge->to == port) {
        info_in_map[port->var()] = new InVarInfo(edge->from, port);
        break;
      }
    }
  }

  for (GrNode::PortList::iterator I = node->outputs_begin(); I != node->outputs_end(); ++I) {
    GrPort* port = *I;
    for (GrPort::EdgeList::iterator J = port->begin_edges(); J != port->end_edges(); ++J) {
      GrEdge* edge = *J;
      // TODO: Handle multiple out edges
      if (edge->from == port) {
        std::cerr << "Making out info for " << port->var()->name() << "[" << port->var().object() << "]"  << std::endl;
        info_out_map[port->var()] = new OutVarInfo(edge->to->parent()->program(), port, edge->to->parent());
        break;
      } else {
        std::cerr << "Edge mismatch for " << port->var()->name() << std::endl;
      }
    }
  }

  
  for (GrNode::PortList::iterator I = node->inputs_begin(); I != node->inputs_end(); ++I) {
    GrPort* port = *I;
    for (GrPort::EdgeList::iterator J = port->begin_edges(); J != port->end_edges(); ++J) {
      GrEdge* edge = *J;
      if (edge->to != port) continue;
      makeVariableInfoNode(edge->from->parent());
    }
  }
}

void makeVariableInfo(GrNode* final_node)
{
  info_in_map.clear();
  info_out_map.clear();

  final_node->clearMarked();

  makeVariableInfoNode(final_node);

  final_node->clearMarked();
}

OutVarInfo* out_info(const ShVariableNodePtr& var)
{
  if (!info_out_map[var]) {
    std::cerr << "Did not have info for output " << var->name() << "[" << var.object() << "]" << std::endl;
    info_out_map[var] = new OutVarInfo(0, 0, 0);
  }
  return info_out_map[var];
}

InVarInfo* in_info(const ShVariableNodePtr& var)
{
  if (!info_in_map[var]) {
    std::cerr << "Did not have info for input " << var->name() << std::endl;
    info_in_map[var] = new InVarInfo(0, 0);
  }
  return info_in_map[var];
}

ShProgram generateShader(GrNode* root_node, GrNode* final_node)
{
  ShEnvironment::useExceptions = true;
  
  // TODO: Make all variable info
  // E.g. backward DFS/BFS from the final node.
  makeVariableInfo(final_node);
  
  ShProgram p = root_node->program();
  ShProgram final = final_node->program();
  
  // TODO
  // First pass, find all uniforms/constants, make multiple-output
  // programs, add them on to P with combine

  // No outputs in the root?
  if (p->outputs.empty()) {
    std::cerr << "Root has no outputs. What's going on?" << std::endl;
    return 0;
  }

  if (final->target() == "gpu:fragment") {
    int count = 0;
    for (GrNode::PortList::iterator TN = root_node->outputs_begin(); TN != root_node->outputs_end(); ++TN) {
      GrPort* port = *TN;
      if (port->monitor()) {
        ShProgram adapt = SH_BEGIN_PROGRAM("gpu:fragment") {
          ShVariable v(new ShVariableNode(SH_INOUT, port->var()->size(), SH_COLOR));
        } SH_END;
        ShProgram monprog = adapt << (shSwizzle(count) << p);
        port->monitor()->setFragmentProgram(monprog);
      }
      count++;
    }
  } 
  
  bool done = false;
  
  ShProgramNode::VarList::iterator OI = p->outputs.begin();
  while (!done) {
    ShVariableNodePtr output = *OI;

    std::cerr << "Considering output " << output->name() << std::endl;

    // First, clear the marked bits of all our outputs, we will use
    // this later.
    for (ShProgramNode::VarList::iterator I = p->outputs.begin();
         I != p->outputs.end(); ++I) {
      out_info(*I)->marked = false;
    }

    // We will build up an output permutation for the current program
    // based on this output's target's input requirements
    ShManipulator<int> perm;
    ShProgram target = out_info(output)->target;
    GrNode* target_node = out_info(output)->target_node;
    
    // If we run into a problem setting up this target program, we'll
    // skip it and assume we can deal with it later.
    bool skip = false;
    if (target) {
      // Consider all the inputs of the target program
      for (ShProgramNode::VarList::iterator I = target->inputs.begin();
           I != target->inputs.end(); ++I) {
        ShVariableNodePtr var = *I;
        
        // See if we have the output required by this input
        /* = find(p->outputs.begin(), p->outputs.end(),
           in_info(var)->source_var);*/
        ShProgramNode::VarList::iterator J;
        bool invalid = false;
        for (J = p->outputs.begin(); J != p->outputs.end(); ++J) {
          if (in_info(var)->source == 0) { invalid = true; break; }
          if (out_info(*J)->port == in_info(var)->source) break;
        }
        if (J == p->outputs.end() || invalid) {
          // Don't have that output yet. Skip "output" entirely.
          skip = true;
          break;
        } else {
          // Set the required output as the next item in the permutation
          // that is being built, and mark it so we know we don't have
          // to add it later.
          perm = perm(distance(p->outputs.begin(), J));
          out_info(*J)->marked = true;
        }
      }
    } else {
      std::cerr << "Skipping output " << output->name() << ", no target" << std::endl;
      skip = true;
    }

    if (!skip) {
      try {
        std::cerr << "Doing some smashing" << std::endl;
        // Add the rest of the outputs to the permutation
        // (Thereby making it an actual permutation)
        int i = 0;
        ShProgramNode::VarList leftovers;
        for (ShProgramNode::VarList::iterator I = p->outputs.begin();
             I != p->outputs.end(); ++I, ++i) {
          if (!out_info(*I)->marked) {
            leftovers.push_back(*I);
            perm = perm(i);
          }
        }

        // Apply the permutation to the program outputs
        p = perm << p;

        // Connect the target program for the permuted outputs
        p = target << p;

        ShProgramNode::VarList::iterator I = p->outputs.begin();
        //std::advance(I, target->outputs.size());
        ShProgramNode::VarList::iterator L = leftovers.begin();
        ShProgramNode::VarList::iterator T = target->outputs.begin();
        GrNode::PortList::iterator TN = target_node->outputs_begin();
        for (int count = 0; I != p->outputs.end(); ++I, ++count) {
          if (count < target->outputs.size()) {
            info_out_map[*I] = out_info(*T);

            GrPort* port = *TN;
            if (port->monitor()) {
              if (final->target() == "gpu:fragment") {
                std::cerr << "Setting up monitor fragment program" << std::endl;
                ShProgram adapt = SH_BEGIN_PROGRAM("gpu:fragment") {
                  ShVariable v(new ShVariableNode(SH_INOUT, (*T)->size(), SH_COLOR));
                } SH_END;
                ShProgram monprog = adapt << (shSwizzle(count) << p);
                port->monitor()->setFragmentProgram(monprog);
              } 
            }
            ++T;
            ++TN;
          } else {
            info_out_map[*I] = out_info(*L);
            ++L;
          }
        }
      
        // If we have reached the final program, we are done
        if (target == final) done = true;
        // Move on to the next output otherwise
        OI = p->outputs.begin();
      } catch (const ShAlgebraException& e) {
        std::cerr << e.message() << std::endl;
        return 0;
      } catch (const ShException& e) {
        std::cerr << "Misc Error: " << e.message() << std::endl;
        return 0;
      }
    } else {
      // Skip this output, will handle it at some other point
      OI++;

      // If we can't handle any outputs, and we're not done yet,
      // there's a problem (e.g. a cycle).
      if (OI == p->outputs.end()) {
        // TODO: Report cycle
        std::cerr << "None of these outputs have worked. Aborting" << std::endl;
        return 0;
      }
    }
  }

  // TODO: Set up monitors for discarded outputs.
  // Requires more traversal. Hrmpf.
  
  p = shRange(0, (int)final->outputs.size() - 1) << p;

  
//   while (!consumed_output_program) {
//     o = next_unassigned_output;
//     if (!changed and seen o before) {
//       complain, cycle!
//     }
//     for (i = each o->target->inputs) {
//       if (i->source not in P->outputs) {
//         leave o for later;
//       } else {
//         setup i->source to be nth output;
//         outrange = outrange(location of i->source);
//         mark i->source
//       }
//     }
//     if (o is not left for later) {
//       for (each s in P->outputs) {
//         if (s not marked) outrange = outrange(s);
//       }
//       P = outrange << P;
//       P = o->target << P;
//       if (o->target = output_program) consumed_output_program = true;
//       changed = true;
//     } else {
//       changed = false;
//     }
//   }

  // Discard all the remaining outputs (using an empty range)
  //p = ShManipulator<int>() << p;

  return p;
}
