#include "Zeni/Rete/Raven_Connect_Output.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Raven_Connect_Output::Raven_Connect_Output(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output)
    : Raven(recipient, network, output)
  {
  }

  void Raven_Connect_Output::receive() const {
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}