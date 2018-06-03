#include "Zeni/Rete/Raven_Decrement_Output_Count.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Raven_Decrement_Output_Count::Raven_Decrement_Output_Count(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output)
    : Raven(recipient, network, output)
  {
  }

  void Raven_Decrement_Output_Count::receive() const {
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}