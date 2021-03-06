#include "Zeni/Rete/Internal/Message_Relink_Output.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Message_Relink_Output::Message_Relink_Output(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node_Key> key_, const std::shared_ptr<Node> child_)
    : Message(recipient, network),
    key(key_),
    child(child_)
  {
  }

  void Message_Relink_Output::receive() const {
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
