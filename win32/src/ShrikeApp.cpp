#include "ShrikeApp.hpp"
#include "ShrikeFrame.hpp"

IMPLEMENT_APP(ShrikeApp)

ShrikeApp::ShrikeApp()
{
}
  
bool ShrikeApp::OnInit()
{
  ShrikeFrame* frame = new ShrikeFrame;
  frame->Show(true);
  
  return true;
}

