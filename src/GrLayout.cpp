#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include "GrView.hpp"
#include "GrNode.hpp"
#include "GrPort.hpp"

void GrView::layout()
{
  char filename[] = {'/','t','m','p','/','s','h','t','u','d','i','o','_','X','X','X','X','X','X', 0};
  if (!mktemp(filename)) {
    std::cerr << "Could not make temp file name" << std::endl;
    return;
  }

  std::ofstream out(filename);

  out << "digraph shtudio {" << std::endl;
  out << "rankdir=LR;" << std::endl;
  //out << "nodesep=20;" << std::endl;
  out << "ordering=out;" << std::endl;
  out << "center=1;" << std::endl;
  out << "edge [minlen=20];" << std::endl;
  for (NodeList::iterator I = m_nodes.begin(); I != m_nodes.end(); ++I) {
    GrNode* node = *I;
    out << "n" << node->pickid() << " [height=" << node->height()
        << ",width=" << node->width() << ",fixedsize=true,shape=box,"
        << "label=\"" << (I - m_nodes.begin()) << "\"];" << std::endl;
  }
  for (NodeList::iterator I = m_nodes.begin(); I != m_nodes.end(); ++I) {
    GrNode* node = *I;

    for (GrNode::PortList::iterator J = node->outputs_begin(); J != node->outputs_end(); ++J) {
      GrPort* port = *J;

      for (GrPort::EdgeList::iterator K = port->begin_edges(); K != port->end_edges(); ++K) {
        GrEdge* edge = *K;
        if (port == edge->from) {
          out << "n" << edge->from->parent()->pickid() << " -> n" << edge->to->parent()->pickid()
              <<";" << std::endl;
        }
      }
    }
  }

  out << "}" << std::endl;

  out.close();

  char resfilename[] = {'/','t','m','p','/','s','h','t','u','d','i','o','_','X','X','X','X','X','X', 0};
  if (!mktemp(resfilename)) {
    std::cerr << "Could not make temp file name for output" << std::endl;
    return;
  }

  
  std::string cmdline;

  cmdline = "dot -Tplain ";
  cmdline += filename;
  cmdline += " > ";
  cmdline += resfilename;
  
  system(cmdline.c_str());

  std::ifstream in(resfilename);

  while (!in.eof()) {
    std::string type;
    in >> std::ws >> type;
    if (in.eof()) break;
    if (type != "node") { while (in.get() != '\n'); continue; }

    std::string name;
    double x, y, width, height;

    in >> std::ws >> name;
    in >> std::ws >> x;
    in >> std::ws >> y;
    in >> std::ws >> width;
    in >> std::ws >> height;
    int index = -1;
    in >> std::ws >> index;

    while (in.get() != '\n');

    if (index < 0) continue;

//     std::cerr << index << " at " << x << ", " << y << std::endl;

    GrNode* node = *(m_nodes.begin() + index);
    node->moveTo(x - node->width()/2.0,
                 y - node->height()/2.0);
  }

  
  SetCurrent();
  setupView();
  paint();

  unlink(filename);
  unlink(resfilename);
  
}
