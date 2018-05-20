#ifndef RETE_NEGATION_JOIN_H
#define RETE_NEGATION_JOIN_H

#include "Node.h"

#include <functional>
#include <unordered_map>

namespace Zeni {

  namespace Rete {

    class Node_Join_Negation : public Node {
      Node_Join_Negation(const Node_Join_Negation &);
      Node_Join_Negation & operator=(const Node_Join_Negation &);

      friend ZENI_RETE_LINKAGE void bind_to_negation_join(Network &network, const std::shared_ptr<Node_Join_Negation> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);

    public:
      ZENI_RETE_LINKAGE Node_Join_Negation(Variable_Bindings bindings_);

      ZENI_RETE_LINKAGE void destroy(Network &network, const std::shared_ptr<Node> &output) override;

      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_left() const override { return input0.lock(); }
      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_right() const override { return input1.lock(); }
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_left() override { return input0.lock(); }
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_right() override { return input1.lock(); }

      ZENI_RETE_LINKAGE std::shared_ptr<const Node_Filter> get_filter(const int64_t &index) const override;

      ZENI_RETE_LINKAGE const Tokens & get_output_tokens() const override;
      ZENI_RETE_LINKAGE bool has_output_tokens() const override;

      ZENI_RETE_LINKAGE void insert_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;
      ZENI_RETE_LINKAGE bool remove_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;

      ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

      ZENI_RETE_LINKAGE void print_details(std::ostream &os) const override; ///< Formatted for dot: http://www.graphviz.org/content/dot-language

      ZENI_RETE_LINKAGE void print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const override;

      ZENI_RETE_LINKAGE void output_name(std::ostream &os, const int64_t &depth) const override;

      ZENI_RETE_LINKAGE bool is_active() const override;

      ZENI_RETE_LINKAGE std::vector<WME> get_filter_wmes() const override;

      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Join_Negation> find_existing(const Variable_Bindings &bindings, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);

      ZENI_RETE_LINKAGE virtual const Variable_Bindings * get_bindings() const override { return &bindings; }

    private:
      void join_tokens(Network &network, const std::shared_ptr<const Token> &lhs);
      void unjoin_tokens(Network &network, const std::shared_ptr<const Token> &lhs);

      void pass_tokens(Network &network, const std::shared_ptr<Node> &output) override;
      void unpass_tokens(Network &network, const std::shared_ptr<Node> &output) override;

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
    };

    ZENI_RETE_LINKAGE void bind_to_negation_join(Network &network, const std::shared_ptr<Node_Join_Negation> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);

  }

}

#endif
