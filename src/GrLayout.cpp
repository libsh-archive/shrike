#include <iostream>
#include <fstream>
#include <sstream>
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
  out << "nodesep=20;" << std::endl;
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

  cmdline = "dot ";
  cmdline += filename;
  cmdline += " > ";
  cmdline += resfilename;

  
  system(cmdline.c_str());

  std::cerr << "Result in " << resfilename << std::endl;
  std::ifstream in(resfilename);

  while (!in.eof()) {
    std::string line;
    std::getline(in, line);
    if (in.eof()) break;
    if (line.find("shape=box") == std::string::npos) continue;
    std::string::size_type l = line.find("label=");
    if (l == std::string::npos) continue;
    std::string label = line.substr(l + 6);
    std::string::size_type le = label.find(',');
    if (le == std::string::npos) continue;
    label = label.substr(0, le);
    int index = -1;
    {
      std::istringstream is(label);
      is >> index;
    }
    if (index < 0) continue;
    
    std::string::size_type p = line.find("pos=\"");
    if (p == std::string::npos) continue;
    line = line.substr(p + 5);
    std::string::size_type e = line.find('"');
    if (e == std::string::npos) continue;
    line = line.substr(0, e);
    std::string::size_type c = line.find(',');
    if (c == std::string::npos) continue;
    double x, y;
    {
      std::istringstream is(line.substr(0, c));
      is >> x;
    }
    {
      std::istringstream is(line.substr(c+1));
      is >> y;
    }
    y/=72.0;
    x/=72.0;
    std::cerr << index << " at " << x << ", " << y << std::endl;

    GrNode* node = *(m_nodes.begin() + index);
    node->moveTo(x - node->width()/2.0,
                 y + node->height()/2.0);
  }

  SetCurrent();
  setupView();
  paint();
  
}
