#ifndef GRPORT_HPP
#define GRPORT_HPP

#include "sh/ShVariableNode.hpp"
#include "GrNode.hpp"

struct GrEdge {
  GrEdge(GrPort* from, GrPort* to)
    : from(from), to(to)
  {
  }
  
  GrPort* from;
  GrPort* to;
};

class GrPort {
public:
  GrPort(GrNode* parent, const SH::ShVariableNodePtr& var,
         double x, double y, // bottom-left coordinates, rel. to parent node
         bool in);
  ~GrPort();
  
  void draw();

  void draw_edges();
  
  friend void join(GrPort* a, GrPort* b);
  friend void unjoin(GrPort* a, GrPort* b);

  static void draw_edge(double x_from, double y_from,
                        double x_to, double y_to);

  double global_x() const;
  double global_y() const;
  
  bool in() const { return m_in; }
  
private:
  GrNode* m_parent;
  SH::ShVariableNodePtr m_var;
  double m_x, m_y;
  unsigned int m_gl_name;
  bool m_in;

  typedef std::list<GrEdge> EdgeList;
  EdgeList m_edges;
};


#endif
