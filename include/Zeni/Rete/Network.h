#ifndef Network_H
#define Network_H

#include "Agenda.h"
#include "Node_Action.h"
#include "Node_Predicate.h"
#include "Working_Memory.h"

#include <chrono>
#include <unordered_map>

namespace Zeni {

  namespace Rete {

    class ZENI_RETE_LINKAGE Network {
      Network(Network &);
      Network & operator=(Network &);

    public:
      class ZENI_RETE_LINKAGE CPU_Accumulator {
        CPU_Accumulator(const CPU_Accumulator &);
        CPU_Accumulator operator=(const CPU_Accumulator &);

      public:
        CPU_Accumulator(Network &network_);
        ~CPU_Accumulator();

      private:
        Network & network;
      };

      enum class Node_Sharing { Enabled, Disabled };
      enum class Printed_Output { Normal, None };

      Network(const Printed_Output &printed_output = Printed_Output::Normal);
      ~Network();

      std::shared_ptr<Node_Action> make_action(const std::string &name, const bool &user_action, const Node_Action::Action &action, const std::shared_ptr<Node> &out, const std::shared_ptr<const Variable_Indices> &variables);
      std::shared_ptr<Node_Action> make_action_retraction(const std::string &name, const bool &user_action, const Node_Action::Action &action, const Node_Action::Action &retraction, const std::shared_ptr<Node> &out, const std::shared_ptr<const Variable_Indices> &variables);
      std::shared_ptr<Node_Existential> make_existential(const std::shared_ptr<Node> &out);
      std::shared_ptr<Node_Join_Existential> make_existential_join(const Variable_Bindings &bindings, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);
      std::shared_ptr<Node_Filter> make_filter(const WME &wme);
      std::shared_ptr<Node_Join> make_join(const Variable_Bindings &bindings, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);
      std::shared_ptr<Node_Negation> make_negation(const std::shared_ptr<Node> &out);
      std::shared_ptr<Node_Join_Negation> make_negation_join(const Variable_Bindings &bindings, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);
      std::shared_ptr<Node_Predicate> make_predicate_vc(const Node_Predicate::Predicate &pred, const Token_Index &lhs_index, const std::shared_ptr<const Symbol> &rhs, const std::shared_ptr<Node> &out);
      std::shared_ptr<Node_Predicate> make_predicate_vv(const Node_Predicate::Predicate &pred, const Token_Index &lhs_index, const Token_Index &rhs_index, const std::shared_ptr<Node> &out);

      Agenda & get_agenda() { return agenda; }
      std::shared_ptr<Node_Action> get_rule(const std::string &name);
      std::set<std::string> get_rule_names() const;
      int64_t get_rule_name_index() const { return rule_name_index; }
      void set_rule_name_index(const int64_t &rule_name_index_) { rule_name_index = rule_name_index_; }
      Printed_Output get_printed_output() const { return m_printed_output;  }

      void excise_all();
      void excise_filter(const std::shared_ptr<Node_Filter> &filter);
      void excise_rule(const std::string &name, const bool &user_command);
      std::string next_rule_name(const std::string &prefix);
      std::shared_ptr<Node_Action> unname_rule(const std::string &name, const bool &user_command);

      void insert_wme(const std::shared_ptr<const WME> &wme);
      void remove_wme(const std::shared_ptr<const WME> &wme);
      void clear_wmes();

      double rete_cpu_time() const { return m_cpu_time; }
      size_t rete_size() const;
      void rete_print(std::ostream &os) const; ///< Formatted for dot: http://www.graphviz.org/content/dot-language
      void rete_print_rules(std::ostream &os) const;
      void rete_print_firing_counts(std::ostream &os) const;
      void rete_print_matches(std::ostream &os) const;

      template <typename VISITOR>
      VISITOR visit_preorder(VISITOR visitor, const bool &strict) {
        visitor_value = visitor_value != 1 ? 1 : 2;

        for (auto &o : filters)
          visitor = o->visit_preorder(visitor, strict, visitor_value);
        return visitor;
      }

    protected:
      Agenda agenda;

    private:
      void source_rule(const std::shared_ptr<Node_Action> &action, const bool &user_command);

      Node::Filters filters;
      std::unordered_map<std::string, std::shared_ptr<Node_Action>> rules;
      int64_t rule_name_index = 0;
      Working_Memory working_memory;
      intptr_t visitor_value = 0;

      double m_cpu_time = 0.0;
      size_t m_rete_depth = 0;
      std::chrono::high_resolution_clock::time_point m_start;

      // Options

      Node_Sharing m_node_sharing = Node_Sharing::Enabled;
      Printed_Output m_printed_output;
    };

  }

}

#endif
