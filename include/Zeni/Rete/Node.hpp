#ifndef ZENI_RETE_NODE_H
#define ZENI_RETE_NODE_H

#include "Zeni/Concurrency/Maester.hpp"
#include "Custom_Data.hpp"
#include "Token.hpp"

//#include <algorithm>
//#include <cassert>
//#include <functional>
//#include <list>
//#include <unordered_set>
#include <deque>

namespace Zeni {

  namespace Rete {

    class Agenda;
    class Custom_Data;
    class Network;
    class Node;
    class Node_Action;
    class Node_Existential;
    class Node_Filter;
    class Node_Join;
    class Node_Join_Existential;
    class Node_Join_Negation;
    class Node_Negation;
    class Node_Predicate;

    class Node : public Concurrency::Maester
    {
      Node(const Node &) = delete;
      Node & operator=(const Node &) = delete;

      friend ZENI_RETE_LINKAGE void bind_to_action(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Action> &action, const std::shared_ptr<Node> &out);
      friend ZENI_RETE_LINKAGE void bind_to_existential(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Existential> &existential, const std::shared_ptr<Node> &out);
      friend ZENI_RETE_LINKAGE void bind_to_join_existential(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Join_Existential> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);
      friend ZENI_RETE_LINKAGE void bind_to_filter(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Filter> &filter);
      friend ZENI_RETE_LINKAGE void bind_to_join(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Join> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);
      friend ZENI_RETE_LINKAGE void bind_to_negation(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Negation> &negation, const std::shared_ptr<Node> &out);
      friend ZENI_RETE_LINKAGE void bind_to_join_negation(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Join_Negation> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);
      friend ZENI_RETE_LINKAGE void bind_to_predicate(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Predicate> &predicate, const std::shared_ptr<Node> &out);

    public:
      typedef std::list<std::shared_ptr<Node_Filter>> Filters;
      typedef std::list<std::shared_ptr<Node>> Output_Ptrs;

      typedef std::unordered_set<std::shared_ptr<Node>> Outputs;
      typedef std::unordered_set<std::shared_ptr<const Token>, Zeni::hash_deref<Token>, Zeni::compare_deref_eq> Tokens;

      ZENI_RETE_LINKAGE std::shared_ptr<const Node> shared_from_this() const { return std::static_pointer_cast<const Node>(Concurrency::Maester::shared_from_this()); }
      ZENI_RETE_LINKAGE std::shared_ptr<Node> shared_from_this() { return std::static_pointer_cast<Node>(Concurrency::Maester::shared_from_this()); }

    protected:
      ZENI_RETE_LINKAGE Node() {}

    public:
      ZENI_RETE_LINKAGE virtual void Destroy(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) = 0;

      ZENI_RETE_LINKAGE void suppress_destruction(const bool &suppress) {
        destruction_suppressed = suppress;
      }

      //std::shared_ptr<const Node> shared() const {
      //  return shared_from_this();
      //}
      //std::shared_ptr<Node> shared() {
      //  return shared_from_this();
      //}

      ZENI_RETE_LINKAGE const Output_Ptrs & get_outputs_all() const {
        return outputs_all;
      }

      ZENI_RETE_LINKAGE virtual std::shared_ptr<const Node> parent_left() const = 0;
      ZENI_RETE_LINKAGE virtual std::shared_ptr<const Node> parent_right() const = 0;
      ZENI_RETE_LINKAGE virtual std::shared_ptr<Node> parent_left() = 0;
      ZENI_RETE_LINKAGE virtual std::shared_ptr<Node> parent_right() = 0;

      ZENI_RETE_LINKAGE int64_t get_height() const { return height; }
      ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_token_owner() const { return token_owner.lock(); }
      ZENI_RETE_LINKAGE int64_t get_size() const { return size; }
      ZENI_RETE_LINKAGE int64_t get_token_size() const { return token_size; }

      ZENI_RETE_LINKAGE virtual std::shared_ptr<const Node_Filter> get_filter(const int64_t &index) const = 0;

      ZENI_RETE_LINKAGE virtual const Tokens & get_output_tokens() const = 0;
      ZENI_RETE_LINKAGE virtual bool has_output_tokens() const = 0;

      ZENI_RETE_LINKAGE void receive(Concurrency::Job_Queue &job_queue, const Concurrency::Raven &raven) override;

    private:
      ZENI_RETE_LINKAGE virtual void insert_token(const std::shared_ptr<Network> &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) = 0;
      ZENI_RETE_LINKAGE virtual void remove_token(const std::shared_ptr<Network> &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) = 0; ///< Returns true if removed the last

    public:
      ZENI_RETE_LINKAGE virtual void disconnect(const std::shared_ptr<Network> &/*network*/, const std::shared_ptr<const Node> &/*from*/) {}

      ZENI_RETE_LINKAGE virtual void pass_tokens(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) = 0;
      ZENI_RETE_LINKAGE virtual void unpass_tokens(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) = 0;

      ZENI_RETE_LINKAGE virtual bool operator==(const Node &rhs) const = 0;

      ZENI_RETE_LINKAGE virtual bool disabled_input(const std::shared_ptr<Node> &);

      /// Disable an enabled output
      ZENI_RETE_LINKAGE void disable_output(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output);
      /// Enable a disabled output
      ZENI_RETE_LINKAGE void enable_output(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output);

      /// Add a new enabled output
      ZENI_RETE_LINKAGE void insert_output_enabled(const std::shared_ptr<Node> &output);
      /// Add a new disabled output
      ZENI_RETE_LINKAGE void insert_output_disabled(const std::shared_ptr<Node> &output);

      ZENI_RETE_LINKAGE void erase_output(const std::shared_ptr<Node> &output);
      ZENI_RETE_LINKAGE void erase_output_enabled(const std::shared_ptr<Node> &output);
      ZENI_RETE_LINKAGE void erase_output_disabled(const std::shared_ptr<Node> &output);

      ZENI_RETE_LINKAGE virtual void print_details(std::ostream &os) const = 0; ///< Formatted for dot: http://www.graphviz.org/content/dot-language

      ZENI_RETE_LINKAGE virtual void print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const = 0;

      ZENI_RETE_LINKAGE virtual void output_name(std::ostream &os, const int64_t &depth) const = 0;

      ZENI_RETE_LINKAGE virtual bool is_active() const = 0; ///< Has the node matched and forwarded at least one token?

      ZENI_RETE_LINKAGE virtual std::vector<WME> get_filter_wmes() const = 0;

      ZENI_RETE_LINKAGE virtual const Variable_Bindings * get_bindings() const { return nullptr; }

      template <typename VISITOR>
      VISITOR visit_preorder(VISITOR visitor, const bool &strict) {
        return visit_preorder(visitor, strict, (intptr_t(this) & ~intptr_t(3)) | ((visitor_value & 3) != 1 ? 1 : 2));
      }

      template <typename VISITOR>
      VISITOR visit_preorder(VISITOR visitor, const bool &strict, const intptr_t &visitor_value_);

      std::shared_ptr<Custom_Data> custom_data;

    protected:
      bool destruction_suppressed = false;
      Output_Ptrs outputs_all;
      Outputs outputs_enabled;
      Outputs outputs_disabled;

      int64_t height = 0;
      std::weak_ptr<const Node> token_owner;
      int64_t size = -1;
      int64_t token_size = -1;

    private:
      intptr_t visitor_value = 0;
    };

  }

}

#endif
#include "Node_Filter.hpp"
#if !defined(ZENI_RETE_NODE_H_PART2) && defined(ZENI_RETE_FILTER_H_DONE)
#define ZENI_RETE_NODE_H_PART2

namespace Zeni {

  namespace Rete {

    template <typename VISITOR>
    VISITOR Node::visit_preorder(VISITOR visitor, const bool &strict, const intptr_t &visitor_value_) {
      std::deque<std::shared_ptr<Node>> nodes;
      nodes.push_back(shared_from_this());

      while(!nodes.empty()) {
        const std::shared_ptr<Node> node = nodes.front();
        nodes.pop_front();

        if(node->visitor_value == visitor_value_)
          continue;

        if(!strict && !std::dynamic_pointer_cast<Node_Filter>(node)) {
          if(node->parent_left()->visitor_value != visitor_value_) {
            nodes.push_front(node->parent_left());
            continue;
          }
          else if(node->parent_right()->visitor_value != visitor_value_) {
            nodes.push_front(node->parent_right());
            continue;
          }
        }

        node->visitor_value = visitor_value_;
        visitor(node);
        for(auto &o : node->outputs_all)
          nodes.push_back(o);
      }

      return visitor;
    }

  }

}

#endif