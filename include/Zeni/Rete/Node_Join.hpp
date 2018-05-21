#ifndef ZENI_RETE_NODE_JOIN_H
#define ZENI_RETE_NODE_JOIN_H

#include "Node.hpp"

#include <functional>
#include <unordered_map>

namespace Zeni {

  namespace Rete {

    class Node_Join : public Node {
      Node_Join(const Node_Join &);
      Node_Join & operator=(const Node_Join &);

      friend ZENI_RETE_LINKAGE void bind_to_join(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Join> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);

    public:
      ZENI_RETE_LINKAGE Node_Join(Variable_Bindings bindings_);

      ZENI_RETE_LINKAGE void destroy(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) override;

      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_left() const override { return input0.lock(); }
      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_right() const override { return input1.lock(); }
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_left() override { return input0.lock(); }
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_right() override { return input1.lock(); }

      ZENI_RETE_LINKAGE std::shared_ptr<const Node_Filter> get_filter(const int64_t &index) const override;

      ZENI_RETE_LINKAGE const Tokens & get_output_tokens() const override;
      ZENI_RETE_LINKAGE bool has_output_tokens() const override;

      ZENI_RETE_LINKAGE void insert_token(const std::shared_ptr<Network> &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;
      ZENI_RETE_LINKAGE bool remove_token(const std::shared_ptr<Network> &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;

      ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

      ZENI_RETE_LINKAGE bool disabled_input(const std::shared_ptr<Node> &input) override;

      ZENI_RETE_LINKAGE void print_details(std::ostream &os) const override; ///< Formatted for dot: http://www.graphviz.org/content/dot-language

      ZENI_RETE_LINKAGE void print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const override;

      ZENI_RETE_LINKAGE void output_name(std::ostream &os, const int64_t &depth) const override;

      ZENI_RETE_LINKAGE bool is_active() const override;

      ZENI_RETE_LINKAGE std::vector<WME> get_filter_wmes() const override;

      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Join> find_existing(const Variable_Bindings &bindings, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);

      ZENI_RETE_LINKAGE virtual const Variable_Bindings * get_bindings() const override { return &bindings; }

    private:
      void join_tokens(const std::shared_ptr<Network> &network, const std::shared_ptr<const Token> &lhs, const std::shared_ptr<const Token> &rhs);

      std::shared_ptr<const Token> join_tokens(const std::shared_ptr<const Token> lhs, const std::shared_ptr<const Token> &rhs) const;

      void disconnect(const std::shared_ptr<Network> &network, const std::shared_ptr<const Node> &from) override;

      void pass_tokens(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) override;
      void unpass_tokens(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) override;

      Variable_Bindings bindings;
      std::weak_ptr<Node> input0;
      std::weak_ptr<Node> input1;
      int64_t input0_count = 0;
      int64_t input1_count = 0;

      std::unordered_map<std::pair<std::shared_ptr<const Token>, bool>, std::pair<Tokens, Tokens>, std::function<size_t(const std::pair<std::shared_ptr<const Token>, bool> &)>, std::function<bool(const std::pair<std::shared_ptr<const Token>, bool> &, const std::pair<std::shared_ptr<const Token>, bool> &)>> matching
        = std::unordered_map<std::pair<std::shared_ptr<const Token>, bool>, std::pair<Tokens, Tokens>, std::function<size_t(const std::pair<std::shared_ptr<const Token>, bool> &)>, std::function<bool(const std::pair<std::shared_ptr<const Token>, bool> &, const std::pair<std::shared_ptr<const Token>, bool> &)>>(0, [this](const std::pair<std::shared_ptr<const Token>, bool> &itoken)->size_t {
        return itoken.first->hash_bindings(itoken.second, this->bindings);
      }, [this](const std::pair<std::shared_ptr<const Token>, bool> &lhs, const std::pair<std::shared_ptr<const Token>, bool> &rhs)->bool {
        return lhs.first->eval_bindings(lhs.second, this->bindings, *rhs.first, rhs.second);
      });
      Tokens output_tokens;

      struct Connected {
        Connected() : connected0(true), connected1(true) {}

        bool connected0 : 1;
        bool connected1 : 1;
      } data;
    };

    ZENI_RETE_LINKAGE void bind_to_join(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Join> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);

  }

}

#endif
