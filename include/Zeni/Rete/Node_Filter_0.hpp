#ifndef ZENI_RETE_NODE_FILTER_0_HPP
#define ZENI_RETE_NODE_FILTER_0_HPP

#include "Zeni/Concurrency/Container/Hash_Trie_S2.hpp"
#include "Zeni/Concurrency/Container/Super_Hash_Trie.hpp"
#include "Internal/Node_Unary.hpp"

namespace Zeni::Rete {

  class Node_Filter_0 : public Node_Unary {
    Node_Filter_0(const Node_Filter_0 &) = delete;
    Node_Filter_0 & operator=(const Node_Filter_0 &) = delete;

    Node_Filter_0(const std::shared_ptr<Network> network, const std::shared_ptr<const Symbol> &symbol);

  public:
    typedef Concurrency::Hash_Trie_S2<std::shared_ptr<const Symbol>, Concurrency::Super_Hash_Trie<Token_Trie, Node_Trie>, hash_deref<Symbol>, compare_deref_eq> Symbol_Trie;
    typedef Concurrency::Super_Hash_Trie<Symbol_Trie, Node_Trie> Filter_Layer_1_Trie;
    typedef Filter_Layer_1_Trie::Snapshot Filter_Layer_1_Snapshot;
    enum Filter_Layer_1 {
      FILTER_LAYER_1_SYMBOL = 0,
      FILTER_LAYER_1_VARIABLE_OUTPUTS = 1
    };
    enum Filter_Layer_1_Symbol {
      FILTER_LAYER_1_SYMBOL_TOKENS = 0,
      FILTER_LAYER_1_SYMBOL_OUTPUTS = 1
    };

    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Filter_0> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Symbol> &symbol);

    ZENI_RETE_LINKAGE std::shared_ptr<const Symbol> get_symbol() const;

    ZENI_RETE_LINKAGE std::pair<Node_Trie::Result, std::shared_ptr<Node>> connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child) override;

    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Connect_Filter_0 &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Disconnect_Output &message) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

  private:
    const std::shared_ptr<const Symbol> m_symbol;

    Filter_Layer_1_Trie m_filter_layer_1_trie;
  };

}

#endif
