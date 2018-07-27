#ifndef ZENI_CONCURRENCY_CTRIE_HPP
#define ZENI_CONCURRENCY_CTRIE_HPP

#include "Intrusive_Shared_Ptr.hpp"

#include <array>
#include <functional>

namespace Zeni::Concurrency {

  namespace Ctrie_Internal {
    template <typename INT_TYPE>
    constexpr static INT_TYPE hamming(const INT_TYPE value = std::numeric_limits<INT_TYPE>::max()) {
      return (value & 0x1) + ((value >> 1) ? hamming(value >> 1) : 0);
    }

    template <typename SUMMABLE_TYPE>
    constexpr static SUMMABLE_TYPE sum(const SUMMABLE_TYPE lhs, const SUMMABLE_TYPE rhs) {
      return lhs + rhs;
    }

    template <typename INT_TYPE>
    constexpr static INT_TYPE log2(const INT_TYPE value) {
      return value > 0x1 ? 1 + log2(value >> 1) : 0;
    }

    struct MainNode : public Enable_Intrusive_Sharing<MainNode> {
    private:
      MainNode(const MainNode &) = delete;
      MainNode & operator=(const MainNode &) = delete;

    public:
      MainNode() = default;
    };

    struct Branch : public Enable_Intrusive_Sharing<Branch> {
    private:
      Branch(const Branch &) = delete;
      Branch & operator=(const Branch &) = delete;

    public:
      Branch() = default;
    };

    template <typename HASH_VALUE_TYPE>
    struct ICtrie_Node : public MainNode {
    private:
      ICtrie_Node(const ICtrie_Node &) = delete;
      ICtrie_Node & operator=(const ICtrie_Node &) = delete;

      static const HASH_VALUE_TYPE hamming_max = hamming<HASH_VALUE_TYPE>();
      static const HASH_VALUE_TYPE W = log2(hamming_max);

    protected:
      ICtrie_Node(const HASH_VALUE_TYPE bmp)
        : m_bmp(bmp)
      {
      }

    public:
      static const size_t npos = size_t(-1);

      static const ICtrie_Node * Create(const HASH_VALUE_TYPE bmp, const std::array<const Branch *, hamming_max> &branches) {
        static Factory factory;
        return factory.create(bmp, branches);
      }

      HASH_VALUE_TYPE get_bmp() const { return m_bmp; }

      virtual size_t get_hamming_value() const = 0;

      inline const Branch * match(const HASH_VALUE_TYPE hash_value, const size_t level) const;

    private:
      HASH_VALUE_TYPE m_bmp;

      class Factory {
        Factory(const Factory &) = delete;
        Factory & operator=(const Factory &) = delete;

      public:
        inline Factory();

        const ICtrie_Node<HASH_VALUE_TYPE> * create(const HASH_VALUE_TYPE bmp, const std::array<const Branch *, hamming_max> &branches) {
          return m_generator[bmp](bmp, branches);
        }

      private:
        std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE> *(const HASH_VALUE_TYPE, const std::array<const Branch *, hamming_max> &)>, sum(hamming_max, HASH_VALUE_TYPE(1))> m_generator;
      };

      size_t flagpos(const HASH_VALUE_TYPE hash_value, const size_t level) const {
        const HASH_VALUE_TYPE rotation = (level * 5u) % W;
        const HASH_VALUE_TYPE rotated = (hash_value << rotation) | (hash_value >> rotation);
        const HASH_VALUE_TYPE desired_bit_index = rotated & W;
        const HASH_VALUE_TYPE desired_bit = HASH_VALUE_TYPE(1u) << desired_bit_index;
        if (!(desired_bit & m_bmp))
          return npos;
        size_t array_index = 0u;
        HASH_VALUE_TYPE bmp = m_bmp;
        HASH_VALUE_TYPE current_bit = desired_bit;
        while (current_bit != 0x1) {
          if (bmp & 0x1)
            ++array_index;
          bmp >>= 1;
          current_bit >>= 1;
        }
        return array_index;
      }
    };

    template <typename HASH_VALUE_TYPE, size_t HAMMING_VALUE>
    struct Ctrie_Node : public ICtrie_Node<HASH_VALUE_TYPE> {
    private:
      Ctrie_Node(const Ctrie_Node &) = delete;
      Ctrie_Node & operator=(const Ctrie_Node &) = delete;

    public:
      static const size_t hamming_value = HAMMING_VALUE;

      Ctrie_Node(const HASH_VALUE_TYPE bmp, const std::array<const Branch *, hamming<HASH_VALUE_TYPE>()> &branches_)
        : ICtrie_Node<HASH_VALUE_TYPE>(bmp),
        branches(reinterpret_cast<const std::array<const Branch *, hamming_value> &>(branches_)) //< Should always be smaller, safe to copy subset
      {
      }

      ~Ctrie_Node() {
        for (auto branch : branches)
          branch->decrement_refs();
      }

      size_t get_hamming_value() const override {
        return hamming_value;
      };

      template <typename INDEX>
      const Branch * at(INDEX &&i) const {
        assert(i >= 0 && i < branches.size());
        return branches[i];
      }

    private:
      const std::array<const Branch *, hamming_value> branches;
    };

    template <typename HASH_VALUE_TYPE, size_t IN = hamming<HASH_VALUE_TYPE>()>
    struct Ctrie_Node_Generator {
      static void Create(std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE> *(const HASH_VALUE_TYPE, const std::array<const Branch *, hamming<HASH_VALUE_TYPE>()> &)>, sum(hamming<HASH_VALUE_TYPE>(), HASH_VALUE_TYPE(1))> &generator) {
        Ctrie_Node_Generator<HASH_VALUE_TYPE, IN - 1>::Create(generator);

        generator[IN] = [](const HASH_VALUE_TYPE bmp, const std::array<const Branch *, hamming<HASH_VALUE_TYPE>()> &branches)->ICtrie_Node<HASH_VALUE_TYPE> * {
          return new Ctrie_Node<HASH_VALUE_TYPE, IN>(bmp, branches);
        };
      }
    };

    template <typename HASH_VALUE_TYPE>
    struct Ctrie_Node_Generator<HASH_VALUE_TYPE, 0> {
      static void Create(std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE> *(const HASH_VALUE_TYPE, const std::array<const Branch *, hamming<HASH_VALUE_TYPE>()> &)>, sum(hamming<HASH_VALUE_TYPE>(), HASH_VALUE_TYPE(1))> &generator) {
        generator[0] = [](const HASH_VALUE_TYPE bmp, const std::array<const Branch *, hamming<HASH_VALUE_TYPE>()> &branches)->ICtrie_Node<HASH_VALUE_TYPE> * {
          return new Ctrie_Node<HASH_VALUE_TYPE, 0>(bmp, branches);
        };
      }
    };

    template <typename HASH_VALUE_TYPE>
    const Branch * ICtrie_Node<HASH_VALUE_TYPE>::match(const HASH_VALUE_TYPE hash_value, const size_t level) const {
      const size_t array_index = flagpos(hash_value, level);
      return array_index != npos ?
        static_cast<const Ctrie_Node<HASH_VALUE_TYPE, hamming_max> *>(this)->at(array_index) :
        nullptr;
    }

    template <typename HASH_VALUE_TYPE>
    ICtrie_Node<HASH_VALUE_TYPE>::Factory::Factory() {
      Ctrie_Node_Generator<HASH_VALUE_TYPE>::Create(m_generator);
    }

    template <typename KEY, typename TYPE>
    struct Singleton_Node : public Branch {
    private:
      Singleton_Node(const Singleton_Node &) = delete;
      Singleton_Node & operator=(const Singleton_Node &) = delete;

    public:
      Singleton_Node() : key(KEY()), value(TYPE()) {}

      template <typename KEY_TYPE, typename TYPE_TYPE>
      Singleton_Node(KEY_TYPE &&key_, TYPE_TYPE &&value_) : key(std::forward<KEY_TYPE>(key_)), value(std::forward<TYPE_TYPE>(value_)) {}

      const KEY key;
      const TYPE value;
    };

    template <typename KEY, typename TYPE>
    struct Tomb_Node : public MainNode {
    private:
      Tomb_Node(const Tomb_Node &) = delete;
      Tomb_Node  &operator=(const Tomb_Node &) = delete;

    public:
      Tomb_Node() = default;

      template <typename SINGLETON_NODE>
      Tomb_Node(SINGLETON_NODE &&snode_) : snode(std::forward<SINGLETON_NODE>(snode_)) {}

      ~Tomb_Node() {
        snode->decrement_refs();
      }

      const Singleton_Node<KEY, TYPE> * const snode;
    };

    template <typename KEY, typename TYPE>
    struct List_Node : public MainNode {
    private:
      List_Node(const List_Node &) = delete;
      List_Node & operator=(const List_Node &) = delete;

    public:
      template <typename SINGLETON_NODE, typename LIST_NODE>
      List_Node(SINGLETON_NODE &&snode_, LIST_NODE &&next_) : snode(std::forward<SINGLETON_NODE>(snode_)), next(std::forward<LIST_NODE>(next_)) {}

      ~List_Node() {
        snode->decrement_refs();
        next->decrement_refs();
      }

      const Singleton_Node<KEY, TYPE> * const snode;
      const List_Node * const next;
    };

    template <typename GENERATION = std::uint64_t>
    struct Indirection_Node : public Branch {
    private:
      Indirection_Node(const Indirection_Node &) = delete;
      Indirection_Node & operator=(const Indirection_Node &) = delete;

    public:
      template <typename MAIN_NODE>
      Indirection_Node(MAIN_NODE &&main_, GENERATION &&gen_) : main(std::forward<MAIN_NODE>(main_)), gen(std::forward<GENERATION>(gen_)) {}

      ~Indirection_Node() {
        main->decrement_refs();
      }

      const MainNode * const main;
      const GENERATION gen;
    };
  }

  template <typename KEY, typename TYPE, typename HASH = std::hash<KEY>, typename GENERATION = std::uint64_t>
  class Ctrie {
    Ctrie(const Ctrie &) = delete;
    Ctrie & operator=(const Ctrie &) = delete;

  public:
    typedef KEY Key;
    typedef TYPE Type;
    typedef HASH Hash;
    typedef GENERATION Generation;

    typedef decltype(Hash()(Key())) Hash_Value;

    typedef Ctrie_Internal::MainNode MainNode;
    typedef Ctrie_Internal::Branch Branch;
    typedef Ctrie_Internal::ICtrie_Node<Hash_Value> CNode;
    typedef Ctrie_Internal::Singleton_Node<Key, Type> SNode;
    typedef Ctrie_Internal::Tomb_Node<Key, Type> TNode;
    typedef Ctrie_Internal::List_Node<Key, Type> LNode;
    typedef Ctrie_Internal::Indirection_Node<Generation> INode;

    Ctrie() = default;

    ~Ctrie() {
      m_root.load(std::memory_order_acquire)->decrement_refs();
    }

    Intrusive_Shared_Ptr<const SNode> lookup(const Key &key) {
      const SNode * found = nullptr;
      for(;;) {
        const INode * root = m_root.load(std::memory_order_acquire);
        if (!ilookup(found, root, key, Hash()(key), 0, nullptr))
          break;
      }
      if (found)
        found->increment_refs();
      return found;
    }

  private:
    bool ilookup(const SNode * &found,
      const INode * inode,
      const Key &key,
      const Hash_Value &hash_value,
      size_t level,
      const INode * parent) const
    {
      for (;;) {
        if (const auto cnode = dynamic_cast<const CNode *>(inode->main)) {
          const Branch * branch = cnode->match(hash_value, level);
          if (branch) {
            if (const auto inode_next = dynamic_cast<const INode *>(branch)) {
              parent = inode;
              inode = inode_next;
              ++level;
              continue;
            }
            else if (const auto snode = dynamic_cast<const SNode *>(branch)) {
              if (snode->key == key)
                found = snode;
              return false;
            }
            else
              abort();
          }
          return false;
        }
        else if (const auto tnode = dynamic_cast<const TNode *>(inode->main)) {
          // clean(parent, lev - W) // Only pseudocode for now, irrelevant until removal implemented
          return true;
        }
        else if (auto lnode = dynamic_cast<const LNode *>(inode->main)) {
          do {
            if (lnode->snode->key == key) {
              found = lnode->snode;
              return false;
            }
            else
              lnode = lnode->next;
          } while (lnode);
          return false;
        }
        else
          abort();
      }
    }

    std::atomic<const INode *> m_root = new INode(CNode::Create(0x0, {}), 0);
  };

}

#endif