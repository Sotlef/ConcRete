#include "Zeni/Rete/Node_Filter.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Raven_Disconnect_Output.hpp"
#include "Zeni/Rete/Raven_Token_Insert.hpp"
#include "Zeni/Rete/Raven_Token_Remove.hpp"
#include "Zeni/Rete/Token_Alpha.hpp"

#include <cassert>

namespace Zeni {

  namespace Rete {

    Node_Filter::Node_Filter(const WME &wme_)
      : Node_Unary(1, 1, 1, nullptr),
      m_wme(wme_),
      m_variable(std::make_tuple(std::dynamic_pointer_cast<const Symbol_Variable>(std::get<0>(m_wme.get_symbols())),
        std::dynamic_pointer_cast<const Symbol_Variable>(std::get<1>(m_wme.get_symbols())),
        std::dynamic_pointer_cast<const Symbol_Variable>(std::get<2>(m_wme.get_symbols()))))
    {
    }

    const WME & Node_Filter::get_wme() const {
      return m_wme;
    }

    std::shared_ptr<Node_Filter> Node_Filter::Create_Or_Increment_Output_Count(const std::shared_ptr<Network> &network, const WME &wme) {
      class Friendly_Node_Filter : public Node_Filter {
      public:
        Friendly_Node_Filter(const WME &wme_) : Node_Filter(wme_) {}
      };

      auto filter = std::make_shared<Friendly_Node_Filter>(wme);

      if (network->get_Node_Sharing() == Network::Node_Sharing::Enabled) {
        const auto existing_filter = network->find_filter_and_increment_output_count(filter);
        if (existing_filter)
          return existing_filter;
      }

      network->source_filter(filter);

      return filter;
    }

    void Node_Filter::send_disconnect_from_parents(const std::shared_ptr<Network> &network, class Locked_Node_Data &locked_node_data) {
      // Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      network->get_Job_Queue()->give(std::make_shared<Raven_Disconnect_Output>(network, network, shared_from_this()));
      // locked_node_unary_data.modify_input().reset();
    }

    bool Node_Filter::receive(const Raven_Token_Insert &raven) {
      const auto token = std::dynamic_pointer_cast<const Token_Alpha>(raven.get_Token());
      assert(token);
      const auto &wme = token->get_wme();

      if (!std::get<0>(m_variable) && *std::get<0>(m_wme.get_symbols()) != *std::get<0>(wme->get_symbols()))
        return false;
      if (!std::get<1>(m_variable) && *std::get<1>(m_wme.get_symbols()) != *std::get<1>(wme->get_symbols()))
        return false;
      if (!std::get<2>(m_variable) && *std::get<2>(m_wme.get_symbols()) != *std::get<2>(wme->get_symbols()))
        return false;

      if (std::get<0>(m_variable) && std::get<1>(m_variable) && *std::get<0>(m_variable) == *std::get<1>(m_variable) && *std::get<0>(wme->get_symbols()) != *std::get<1>(wme->get_symbols()))
        return false;
      if (std::get<0>(m_variable) && std::get<2>(m_variable) && *std::get<0>(m_variable) == *std::get<2>(m_variable) && *std::get<0>(wme->get_symbols()) != *std::get<2>(wme->get_symbols()))
        return false;
      if (std::get<1>(m_variable) && std::get<2>(m_variable) && *std::get<1>(m_variable) == *std::get<2>(m_variable) && *std::get<1>(wme->get_symbols()) != *std::get<2>(wme->get_symbols()))
        return false;

      Outputs outputs;
      
      {
        Locked_Node_Data locked_node_data(this);
        Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

        outputs = locked_node_data.get_outputs();
        auto found = locked_node_unary_data.get_input_antitokens().find(token);
        if (found == locked_node_unary_data.get_input_antitokens().end()) {
          locked_node_unary_data.modify_input_tokens().insert(token);
          locked_node_data.modify_output_tokens().insert(token);
        }
        else {
          locked_node_unary_data.modify_input_antitokens().erase(found);
          return false;
        }
      }

      const auto sft = shared_from_this();
      for (auto &output : outputs)
        raven.get_Network()->get_Job_Queue()->give(std::make_shared<Raven_Token_Insert>(output, raven.get_Network(), sft, token));
      return true;
    }

    bool Node_Filter::receive(const Raven_Token_Remove &raven) {
      const auto token = std::dynamic_pointer_cast<const Token_Alpha>(raven.get_Token());
      assert(token);
      const auto &wme = token->get_wme();

      if (!std::get<0>(m_variable) && *std::get<0>(m_wme.get_symbols()) != *std::get<0>(wme->get_symbols()))
        return false;
      if (!std::get<1>(m_variable) && *std::get<1>(m_wme.get_symbols()) != *std::get<1>(wme->get_symbols()))
        return false;
      if (!std::get<2>(m_variable) && *std::get<2>(m_wme.get_symbols()) != *std::get<2>(wme->get_symbols()))
        return false;

      if (std::get<0>(m_variable) && std::get<1>(m_variable) && *std::get<0>(m_variable) == *std::get<1>(m_variable) && *std::get<0>(wme->get_symbols()) != *std::get<1>(wme->get_symbols()))
        return false;
      if (std::get<0>(m_variable) && std::get<2>(m_variable) && *std::get<0>(m_variable) == *std::get<2>(m_variable) && *std::get<0>(wme->get_symbols()) != *std::get<2>(wme->get_symbols()))
        return false;
      if (std::get<1>(m_variable) && std::get<2>(m_variable) && *std::get<1>(m_variable) == *std::get<2>(m_variable) && *std::get<1>(wme->get_symbols()) != *std::get<2>(wme->get_symbols()))
        return false;

      Outputs outputs;

      {
        Locked_Node_Data locked_node_data(this);
        Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

        outputs = locked_node_data.get_outputs();
        const auto token = std::make_shared<Token_Alpha>(wme);
        auto found = locked_node_unary_data.get_input_tokens().find(token);
        if (found != locked_node_unary_data.get_input_tokens().end()) {
          locked_node_unary_data.modify_input_tokens().erase(found);
          locked_node_data.modify_output_tokens().erase(locked_node_data.get_output_tokens().find(token));
        }
        else {
          locked_node_unary_data.modify_input_antitokens().insert(token);
          return false;
        }
      }

      const auto sft = shared_from_this();
      for (auto &output : outputs)
        raven.get_Network()->get_Job_Queue()->give(std::make_shared<Raven_Token_Remove>(output, raven.get_Network(), sft, token));
      return true;
    }

    bool Node_Filter::operator==(const Node &rhs) const {
      if (auto filter = dynamic_cast<const Node_Filter *>(&rhs)) {
        if ((std::get<0>(m_variable) != nullptr) ^ (std::get<0>(filter->m_variable) != nullptr))
          return false;
        if (!std::get<0>(m_variable) && *std::get<0>(m_wme.get_symbols()) != *std::get<0>(filter->m_wme.get_symbols()))
          return false;
        if ((std::get<1>(m_variable) != nullptr) ^ (std::get<1>(filter->m_variable) != nullptr))
          return false;
        if (!std::get<1>(m_variable) && *std::get<1>(m_wme.get_symbols()) != *std::get<1>(filter->m_wme.get_symbols()))
          return false;
        if ((std::get<2>(m_variable) != nullptr) ^ (std::get<2>(filter->m_variable) != nullptr))
          return false;
        if (!std::get<2>(m_variable) && *std::get<2>(m_wme.get_symbols()) != *std::get<2>(filter->m_wme.get_symbols()))
          return false;

        if (std::get<0>(m_variable) && std::get<1>(m_variable) && ((*std::get<0>(m_variable) == *std::get<1>(m_variable)) ^ (*std::get<0>(filter->m_variable) == *std::get<1>(filter->m_variable))))
          return false;
        if (std::get<0>(m_variable) && std::get<2>(m_variable) && ((*std::get<0>(m_variable) == *std::get<2>(m_variable)) ^ (*std::get<0>(filter->m_variable) == *std::get<2>(filter->m_variable))))
          return false;
        if (std::get<1>(m_variable) && std::get<2>(m_variable) && ((*std::get<1>(m_variable) == *std::get<2>(m_variable)) ^ (*std::get<1>(filter->m_variable) == *std::get<2>(filter->m_variable))))
          return false;

        return true;
      }
      return false;
    }

  }

}
