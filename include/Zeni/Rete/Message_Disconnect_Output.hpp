#ifndef ZENI_RETE_MESSAGE_DISCONNECT_OUTPUT_HPP
#define ZENI_RETE_MESSAGE_DISCONNECT_OUTPUT_HPP

#include "Message.hpp"

namespace Zeni::Rete {

  class Message_Disconnect_Output : public Message {
    Message_Disconnect_Output(const Message_Disconnect_Output &) = delete;
    Message_Disconnect_Output & operator=(const Message_Disconnect_Output &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Disconnect_Output(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<Node> child, const bool decrement_output_count_);

    void receive() const override;

    const std::shared_ptr<Node> child;
    const bool decrement_output_count;
  };

}

#endif
