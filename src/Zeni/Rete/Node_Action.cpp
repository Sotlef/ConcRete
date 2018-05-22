#include "Zeni/Rete/Node_Action.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Raven_Token_Insert.hpp"
#include "Zeni/Rete/Raven_Token_Remove.hpp"

#include <cassert>

namespace Zeni {

  namespace Rete {

    Node_Action::Node_Action(const std::string &name_, const std::shared_ptr<Node> &input, const std::shared_ptr<const Variable_Indices> &variables,
      const Action &action_,
      const Action &retraction_)
      : Node_Unary(input->get_height() + 1, input->get_size(), input->get_token_size(), input),
      m_variables(variables),
      m_name(name_),
      m_action(action_),
      m_retraction(retraction_)
    {
      assert(!m_name.empty());
    }

    std::shared_ptr<Node_Action> Node_Action::Create(const std::shared_ptr<Network> &network, const std::string &name, const bool &user_action, const std::shared_ptr<Node> &out, const std::shared_ptr<const Variable_Indices> &variables, const Node_Action::Action &action, const Node_Action::Action &retraction) {
      class Friendly_Node_Action : public Node_Action {
      public:
        Friendly_Node_Action(const std::string &name_, const std::shared_ptr<Node> &input, const std::shared_ptr<const Variable_Indices> &variables,
          const Action &action_,
          const Action &retraction_) : Node_Action(name_, input, variables, action_, retraction_) {}
      };

      const auto action_fun = std::make_shared<Friendly_Node_Action>(name, out, variables, action, retraction);

      network->source_rule(action_fun, user_action);

      out->connect_output(network, action_fun);

      return action_fun;
    }

    Node_Action::~Node_Action() {
      //for (auto &token : get_input_tokens())
      //  m_retraction(*this, *token);
    }

    void Node_Action::receive(const Raven_Token_Insert &raven) {
      Node_Unary::receive(raven);
      m_action(*this, *raven.get_Token());
    }

    void Node_Action::receive(const Raven_Token_Remove &raven) {
      Node_Unary::receive(raven);
      m_retraction(*this, *raven.get_Token());
    }

    bool Node_Action::operator==(const Node &) const {
      return false;
    }

  }

}
