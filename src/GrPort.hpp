#ifndef GRPORT_HPP
#define GRPORT_HPP

#include <list>
#include "sh/ShVariableNode.hpp"

class wxMenu;

class GrNode;
class GrMonitor;
class GrPort;

struct GrEdge {
  GrEdge(GrPort* from, GrPort* to)
    : from(from), to(to)
  {
  }

  int pickid;
  
  GrPort* from;
  GrPort* to;
};

class GrPort {
public:
  GrPort(GrNode* parent, const SH::ShVariableNodePtr& var,
         double x, double y, // bottom-left coordinates, rel. to parent node
         bool in);
  ~GrPort();

  GrMonitor* monitor() const { return m_monitor; }

  void monitor(GrMonitor* monitor) { m_monitor = monitor; }
  
  void draw();

  void draw_edges();
  
  friend void join(GrPort* a, GrPort* b);
  friend void unjoin(GrPort* a, GrPort* b);

  static void draw_edge(double x_from, double y_from,
                        double x_to, double y_to);

  void move(double x, double y);
  
  double global_x() const;
  double global_y() const;
  
  bool in() const { return m_in; }

  typedef std::list<GrEdge*> EdgeList;
  EdgeList::iterator begin_edges() { return m_edges.begin(); }
  EdgeList::iterator end_edges() { return m_edges.end(); }

  GrNode* parent() { return m_parent; }

  SH::ShVariableNodePtr var() { return m_var; }

  wxMenu* contextMenu();
  
private:
  GrNode* m_parent;
  SH::ShVariableNodePtr m_var;
  double m_x, m_y;
  unsigned int m_gl_name;
  bool m_in;
  GrMonitor* m_monitor;

  EdgeList m_edges;
};


#endif
