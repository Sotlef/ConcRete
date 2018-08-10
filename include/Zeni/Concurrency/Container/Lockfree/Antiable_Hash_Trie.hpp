#ifndef ZENI_CONCURRENCY_ANTIABLE_HASH_TRIE_HPP
#define ZENI_CONCURRENCY_ANTIABLE_HASH_TRIE_HPP

#include "Intrusive_Shared_Ptr.hpp"

#include <array>
#include <cstring>
#include <functional>
#include <stack>

namespace Zeni::Concurrency {

  namespace Antiable_Hash_Trie_Internal {

    template <typename INT_TYPE>
    constexpr static INT_TYPE hamming(const INT_TYPE value = std::numeric_limits<INT_TYPE>::max()) {
      return (value & 0x1) + ((value >> 1) ? hamming(value >> 1) : 0);
    }

    template <typename INT_TYPE>
    constexpr static INT_TYPE unhamming(const INT_TYPE value) {
      INT_TYPE rv = 0;
      for (INT_TYPE i = 0; i != value; ++i)
        rv |= INT_TYPE(0x1) << i;
      return rv;
    }

    template <typename SUMMABLE_TYPE>
    constexpr static SUMMABLE_TYPE sum(const SUMMABLE_TYPE lhs, const SUMMABLE_TYPE rhs) {
      return lhs + rhs;
    }

    template <typename INT_TYPE>
    constexpr static INT_TYPE log2(const INT_TYPE value) {
      return value > 0x1 ? 1 + log2(value >> 1) : 0;
    }

    struct Main_Node : public Enable_Intrusive_Sharing<Main_Node> {
    private:
      Main_Node(const Main_Node &) = delete;
      Main_Node & operator=(const Main_Node &) = delete;

    public:
      Main_Node() = default;
    };

    template <typename KEY>
    struct ZENI_CONCURRENCY_CACHE_ALIGN Singleton_Node : public Main_Node {
    private:
      Singleton_Node(const Singleton_Node &) = delete;
      Singleton_Node & operator=(const Singleton_Node &) = delete;

      template <typename KEY_TYPE>
      Singleton_Node(KEY_TYPE &&key_, const bool insertion_, const int64_t count_) : key(std::forward<KEY_TYPE>(key_)), inserted(insertion_), count(count_) {}

    public:
      template <typename KEY_TYPE>
      Singleton_Node(KEY_TYPE &&key_, const bool insertion_) : key(std::forward<KEY_TYPE>(key_)), inserted(insertion_), count(insertion_ ? 1 : -1) {}

      const Singleton_Node * updated_count(const bool insertion) const {
        assert(count);
        const int64_t new_count = count + (insertion ? 1 : -1);
        return new_count != 0 ? new Singleton_Node(key, inserted, new_count) : nullptr;
      }

      const KEY key;
      const bool inserted;
      const int64_t count;
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE>
    struct ICtrie_Node : public Main_Node {
    private:
      ICtrie_Node(const ICtrie_Node &) = delete;
      ICtrie_Node & operator=(const ICtrie_Node &) = delete;

      static const FLAG_TYPE hamming_max = hamming<FLAG_TYPE>();

    protected:
      ICtrie_Node(const FLAG_TYPE bmp)
        : m_bmp(bmp)
      {
      }

    public:
      static const FLAG_TYPE W = log2(hamming_max);

      static const ICtrie_Node * Create(const FLAG_TYPE bmp, const size_t hamming_value, const std::array<const Main_Node *, hamming_max> &branches) {
        static Factory factory;
        return factory.create(bmp, hamming_value, branches);
      }

      static const ICtrie_Node * Create(const Main_Node * const first, const HASH_VALUE_TYPE first_hash, const Main_Node * const second, const HASH_VALUE_TYPE second_hash, const size_t level) {
        assert(first_hash != second_hash);
        assert(level < hamming_max);
        const auto first_flag = flag(first_hash, level);
        const auto second_flag = flag(second_hash, level);
        if (first_flag == second_flag) {
          std::array<const Main_Node *, hamming_max> branches;
          branches[0] = Create(first, first_hash, second, second_hash, level + W);
          return Create(first_flag, 1, branches);
        }
        else {
          std::array<const Main_Node *, hamming_max> branches;
          branches[0] = first_flag < second_flag ? first : second;
          branches[1] = first_flag > second_flag ? first : second;
          return Create(first_flag | second_flag, 2, branches);
        }
      }

      FLAG_TYPE get_bmp() const { return m_bmp; }

      virtual size_t get_hamming_value() const = 0;

      std::pair<FLAG_TYPE, size_t> flagpos(const HASH_VALUE_TYPE hash_value, const size_t level) const {
        const FLAG_TYPE desired_bit = flag(hash_value, level);
        const size_t array_index = hamming(m_bmp & (desired_bit - 1));
        return std::make_pair(desired_bit, array_index);
      }

      virtual const Main_Node * at(const size_t i) const = 0;

      virtual const ICtrie_Node * inserted(const size_t pos, const FLAG_TYPE flag, const Main_Node * const new_branch) const = 0;

      virtual const ICtrie_Node * updated(const size_t pos, const FLAG_TYPE flag, const Main_Node * const new_branch) const = 0;

      virtual const ICtrie_Node * erased(const size_t pos, const FLAG_TYPE flag) const = 0;

    private:
      static const HASH_VALUE_TYPE unhamming_filter = HASH_VALUE_TYPE(unhamming(W));
      FLAG_TYPE m_bmp;

      class Factory {
        Factory(const Factory &) = delete;
        Factory & operator=(const Factory &) = delete;

      public:
        inline Factory();

        const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> * create(const FLAG_TYPE bmp, const size_t hamming_value, const std::array<const Main_Node *, hamming_max> &branches) {
          return m_generator[hamming_value](bmp, branches);
        }

      private:
        std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> *(const FLAG_TYPE, const std::array<const Main_Node *, hamming_max> &)>, sum(hamming_max, FLAG_TYPE(1))> m_generator;
      };

      static FLAG_TYPE flag(const HASH_VALUE_TYPE hash_value, const size_t level) {
        const HASH_VALUE_TYPE shifted_hash = hash_value >> level;
        const HASH_VALUE_TYPE desired_bit_index = shifted_hash & unhamming_filter;
        const FLAG_TYPE desired_bit = FLAG_TYPE(1u) << desired_bit_index;
        return desired_bit;
      }
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE, size_t HAMMING_VALUE>
    struct Ctrie_Node : public ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> {
    private:
      Ctrie_Node(const Ctrie_Node &) = delete;
      Ctrie_Node & operator=(const Ctrie_Node &) = delete;

    public:
      static const size_t hamming_value = HAMMING_VALUE;

      Ctrie_Node(const FLAG_TYPE bmp, const std::array<const Main_Node *, hamming<FLAG_TYPE>()> &branches)
        : ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE>(bmp),
        m_branches(reinterpret_cast<const std::array<const Main_Node *, hamming_value> &>(branches)) //< Should always be smaller, safe to copy subset
      {
      }

      ~Ctrie_Node() {
        for (auto branch : m_branches)
          branch->decrement_refs();
      }

      size_t get_hamming_value() const override {
        return hamming_value;
      };

      const Main_Node * at(const size_t i) const override {
        assert(i >= 0 && i < m_branches.size());
        return m_branches[i];
      }

      const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> * inserted(const size_t pos, const FLAG_TYPE flag, const Main_Node * const new_branch) const override {
        assert(!(get_bmp() & flag));
        std::array<const Main_Node *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(const Main_Node *));
          std::memcpy(new_branches.data() + (pos + 1), m_branches.data() + pos, (hamming_value - pos) * sizeof(const Main_Node *));
          for (size_t i = 0; i != hamming_value; ++i)
            m_branches[i]->increment_refs();
        }
        new_branches[pos] = new_branch;
        return ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE>::Create(this->get_bmp() | flag, hamming_value + 1, new_branches);
      }

      const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> * updated(const size_t pos, const FLAG_TYPE flag, const Main_Node * const new_branch) const override {
        assert(get_bmp() & flag);
        std::array<const Main_Node *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(const Main_Node *));
          std::memcpy(new_branches.data() + (pos + 1), m_branches.data() + (pos + 1), (hamming_value - pos - 1) * sizeof(const Main_Node *));
          for (size_t i = 0; i != pos; ++i)
            new_branches[i]->increment_refs();
          for (size_t i = pos + 1; i != hamming_value; ++i)
            new_branches[i]->increment_refs();
        }
        new_branches[pos] = new_branch;
        return ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE>::Create(this->get_bmp(), hamming_value, new_branches);
      }

      const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> * erased(const size_t pos, const FLAG_TYPE flag) const override {
        assert(get_bmp() & flag);
        std::array<const Main_Node *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(const Main_Node *));
          std::memcpy(new_branches.data() + pos, m_branches.data() + (pos + 1), (hamming_value - pos - 1) * sizeof(const Main_Node *));
          for (size_t i = 0; i != hamming_value - 1; ++i)
            new_branches[i]->increment_refs();
        }
        return ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE>::Create(this->get_bmp() & ~flag, hamming_value - 1, new_branches);
      }

    private:
      const std::array<const Main_Node *, hamming_value> m_branches;
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE, size_t IN = hamming<FLAG_TYPE>()>
    struct Ctrie_Node_Generator {
      static void Create(std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> *(const FLAG_TYPE, const std::array<const Main_Node *, hamming<FLAG_TYPE>()> &)>, sum(hamming<FLAG_TYPE>(), FLAG_TYPE(1))> &generator) {
        Ctrie_Node_Generator<HASH_VALUE_TYPE, FLAG_TYPE, IN - 1>::Create(generator);

        generator[IN] = [](const FLAG_TYPE bmp, const std::array<const Main_Node *, hamming<FLAG_TYPE>()> &branches)->ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> * {
          return new Ctrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, IN>(bmp, branches);
        };
      }
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE>
    struct Ctrie_Node_Generator<HASH_VALUE_TYPE, FLAG_TYPE, 0> {
      static void Create(std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> *(const FLAG_TYPE, const std::array<const Main_Node *, hamming<FLAG_TYPE>()> &)>, sum(hamming<FLAG_TYPE>(), FLAG_TYPE(1))> &generator) {
        generator[0] = [](const FLAG_TYPE /*bmp*/, const std::array<const Main_Node *, hamming<FLAG_TYPE>()> &/*branches*/)->ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> * {
          return nullptr; // new Ctrie_Node<HASH_VALUE_TYPE, 0>(bmp, branches);
        };
      }
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE>
    ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE>::Factory::Factory() {
      Ctrie_Node_Generator<HASH_VALUE_TYPE, FLAG_TYPE>::Create(m_generator);
    }

    template <typename KEY>
    struct List_Node : public Main_Node {
    private:
      List_Node(const List_Node &) = delete;
      List_Node & operator=(const List_Node &) = delete;

    public:
      List_Node(const Singleton_Node<KEY> * const snode_, List_Node * const next_ = nullptr) : snode(snode_), next(next_) {}

      ~List_Node() {
        snode->decrement_refs();
        while (next) {
          const int64_t refs = next->decrement_refs();
          if (refs > 1)
            break;
          List_Node * next_next = next->next;
          next->next = nullptr;
          next = next_next;
        }
      }

      std::pair<bool, List_Node *> updated(const KEY &key, const bool insertion) const {
        const Singleton_Node<KEY> * found = nullptr;
        List_Node * new_head = nullptr;
        List_Node * new_tail = nullptr;
        List_Node * old_head = const_cast<List_Node *>(this);
        for (; old_head; old_head = old_head->next) {
          if (old_head->snode->key == key) {
            found = old_head->snode;
            if (old_head->next) {
              old_head->next->increment_refs();
              if (new_head)
                new_tail->next = old_head->next;
              else {
                new_head = old_head->next;
                new_tail = new_head;
              }
              break;
            }
          }
          else {
            old_head->snode->increment_refs();
            if (new_head)
              new_head = new List_Node(old_head->snode, new_head);
            else {
              new_head = new List_Node(old_head->snode, nullptr);
              new_tail = new_head;
            }
          }
        }
        if (found) {
          const auto new_snode = found->updated_count(insertion);
          return new_snode ? std::make_pair(false, new List_Node(new_snode, new_head)) : std::make_pair(!insertion, new_head);
        }
        else
          return std::make_pair(insertion, new List_Node(new Singleton_Node<KEY>(key, insertion), new_head));
      }

    public:
      const Singleton_Node<KEY> * const snode;
      List_Node * next = nullptr;
    };
  }

  template <typename KEY, typename HASH = std::hash<KEY>, typename FLAG_TYPE = uint32_t>
  class Antiable_Hash_Trie {
    Antiable_Hash_Trie & operator=(const Antiable_Hash_Trie &) = delete;

  public:
    typedef KEY Key;
    typedef HASH Hash;

    typedef decltype(Hash()(Key())) Hash_Value;
    typedef FLAG_TYPE Flag_Type;

    typedef Antiable_Hash_Trie_Internal::Main_Node MNode;
    typedef Antiable_Hash_Trie_Internal::Singleton_Node<Key> SNode;
    typedef Antiable_Hash_Trie_Internal::ICtrie_Node<Hash_Value, Flag_Type> CNode;
    typedef Antiable_Hash_Trie_Internal::List_Node<Key> LNode;

    typedef Antiable_Hash_Trie<Key, Hash, Flag_Type> Snapshot;

    class const_iterator {
      struct Level {
        const MNode * mnode = nullptr;
        size_t pos = -1;

        bool operator==(const Level &rhs) const {
          return mnode == rhs.mnode && pos == rhs.pos;
        }
        bool operator!=(const Level &rhs) const {
          return mnode != rhs.mnode || pos != rhs.pos;
        }
      };

    public:
      typedef std::forward_iterator_tag iterator_category;
      typedef const SNode value_type;
      typedef const SNode & reference;

      const_iterator() = default;

      const_iterator(const MNode * root)
        : m_root(root)
      {
        for (;;) {
          if (const auto cnode = dynamic_cast<const CNode *>(root)) {
            assert(cnode->get_hamming_value() != 0);
            if (cnode->get_hamming_value() != 1)
              m_level_stack.push({ cnode, size_t(1) });
            root = cnode->at(0);
          }
          else if (const auto lnode = dynamic_cast<const LNode *>(root)) {
            if (lnode->next)
              m_level_stack.push({ lnode->next, size_t(-1) });
            m_level_stack.push({ lnode->snode, size_t(-1) });
            if (!lnode->snode->inserted)
              ++*this;
            break;
          }
          else if (const auto snode = dynamic_cast<const SNode *>(root)) {
            m_level_stack.push({ snode, size_t(-1) });
            if (!snode->inserted)
              ++*this;
            break;
          }
          else
            break;
        }
      }

      reference operator*() const {
        return *static_cast<const SNode *>(m_level_stack.top().mnode);
      }

      const_iterator next() const {
        return ++const_iterator(*this);
      }

      const_iterator & operator++() {
        m_level_stack.pop();
        while(!m_level_stack.empty()) {
          const auto top = m_level_stack.top();
          m_level_stack.pop();
          if (const auto cnode = dynamic_cast<const CNode *>(top.mnode)) {
            if (top.pos + 1 != cnode->get_hamming_value())
              m_level_stack.push({ cnode, top.pos + 1 });
            const auto mnode_next = cnode->at(top.pos);
            if (const auto snode_next = dynamic_cast<const SNode *>(mnode_next)) {
              if (snode_next->inserted) {
                m_level_stack.push({ snode_next, size_t(0) });
                break;
              }
            }
            else
              m_level_stack.push({ mnode_next, size_t(0) });
          }
          else if (const auto lnode = dynamic_cast<const LNode *>(top.mnode)) {
            if (lnode->next)
              m_level_stack.push({ lnode->next, size_t(-1) });
            if (lnode->snode->inserted) {
              m_level_stack.push({ lnode->snode, size_t(-1) });
              break;
            }
          }
          else if (const auto snode = dynamic_cast<const SNode *>(top.mnode)) {
            abort();
          }
          else
            abort();
        }
        return *this;
      }

      const_iterator operator++(int) {
        const_iterator rv(*this);
        ++*this;
        return rv;
      }

      bool operator==(const const_iterator &rhs) const {
        return m_level_stack == rhs.m_level_stack;
      }
      bool operator!=(const const_iterator &rhs) const {
        return m_level_stack != rhs.m_level_stack;
      }

    private:
      Intrusive_Shared_Ptr<const MNode> m_root;
      std::stack<Level> m_level_stack;
    };

    const_iterator cbegin() const {
      return const_iterator(isnapshot());
    }

    const_iterator cend() const {
      return const_iterator();
    }

    const_iterator begin() const {
      return cbegin();
    }

    const_iterator end() const {
      return cend();
    }

    Antiable_Hash_Trie() = default;

    Antiable_Hash_Trie(const Antiable_Hash_Trie &rhs)
      : m_root(rhs.isnapshot())
    {
    }

    Antiable_Hash_Trie(Antiable_Hash_Trie &&rhs)
      : m_root(rhs.m_root.load(std::memory_order_relaxed))
    {
      rhs.m_root.store(nullptr, std::memory_order_relaxed);
    }

    ~Antiable_Hash_Trie() {
      const MNode * const mnode = m_root.load(std::memory_order_acquire);
      if(mnode)
        mnode->decrement_refs();
    }

    std::pair<Intrusive_Shared_Ptr<const SNode>, Snapshot> lookup(const Key &key) const {
      const Hash_Value hash_value = Hash()(key);
      const MNode * const root = isnapshot();
      const SNode * const found = ilookup(root, key, hash_value, 0);
      if (found && found->insertion) {
        found->increment_refs();
        return std::make_pair(found, Snapshot(root));
      }
      else
        return std::make_pair(nullptr, Snapshot(root));
    }

    std::pair<bool, Snapshot> insert(const Key &key) {
      return insert_or_erase(key, true);
    }

    std::pair<bool, Snapshot> erase(const Key &key) {
      return insert_or_erase(key, false);
    }

    Snapshot shapshot() const {
      return isnapshot();
    }

  private:
    std::pair<bool, Snapshot> insert_or_erase(const Key &key, const bool insertion) {
      const Hash_Value hash_value = Hash()(key);
      const MNode * root = isnapshot();
      for (;;) {
        const auto[first_or_last, new_root] = iinsert(root, key, hash_value, insertion, 0);
        if (new_root)
          new_root->increment_refs();
        if (root)
          root->decrement_refs();
        if (CAS(m_root, root, new_root, std::memory_order_release, std::memory_order_acquire))
          return std::make_pair(first_or_last, Snapshot(new_root));
        else {
          if (new_root)
            new_root->decrement_refs();
          enforce_snapshot(root);
        }
      }
    }

    Antiable_Hash_Trie(const MNode * const mnode)
      : m_root(mnode)
    {
    }

    const MNode * isnapshot() const {
      const MNode * root = m_root.load(std::memory_order_acquire);
      enforce_snapshot(root);
      return root;
    }

    void enforce_snapshot(const MNode * &root) const {
      while (root && !root->increment_refs())
        root = m_root.load(std::memory_order_acquire);
    }

    const SNode * ilookup(
      const MNode * mnode,
      const Key &key,
      const Hash_Value &hash_value,
      size_t level) const
    {
      for (;;) {
        if (const auto cnode = dynamic_cast<const CNode *>(mnode)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          const MNode * const next = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!next)
            return nullptr;
          mnode = next;
          level += CNode::W;
          continue;
        }
        else if (auto lnode = dynamic_cast<const LNode *>(mnode)) {
          do {
            if (lnode->snode->key == key)
              return lnode->snode;
            else
              lnode = lnode->next;
          } while (lnode);
          return nullptr;
        }
        else if (auto snode = dynamic_cast<const SNode *>(mnode))
          return snode->insertion && snode->key == key ? snode : nullptr;
        else
          return nullptr;
      }
    }

    std::pair<bool, const MNode *> iinsert(
      const MNode * const mnode,
      const Key &key,
      const Hash_Value &hash_value,
      const bool insertion,
      const size_t level)
    {
      if (const auto cnode = dynamic_cast<const CNode *>(mnode)) {
        const auto[flag, pos] = cnode->flagpos(hash_value, level);
        const MNode * const next = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
        if (!next)
          return std::make_pair(insertion ? true : false, cnode->inserted(pos, flag, new SNode(key, insertion)));
        const auto[first_or_last, new_next] = iinsert(next, key, hash_value, insertion, level + CNode::W);
        if (!new_next) {
          const auto new_cnode = cnode->erased(pos, flag);
          if (!new_cnode)
            return std::make_pair(first_or_last, nullptr);
          else if (new_cnode->get_hamming_value() != 1)
            return std::make_pair(first_or_last, new_cnode);
          else {
            const auto new_mnode = new_cnode->at(0);
            if (dynamic_cast<const CNode *>(new_mnode))
              return std::make_pair(first_or_last, new_cnode);
            else {
              new_mnode->increment_refs();
              delete new_cnode;
              return std::make_pair(first_or_last, new_mnode);
            }
          }
        }
        else
          return std::make_pair(first_or_last, cnode->updated(pos, flag, new_next));
      }
      else if (auto lnode = dynamic_cast<const LNode *>(mnode)) {
        const Hash_Value lnode_hash = Hash()(lnode->snode->key);
        if (lnode_hash != hash_value) {
          lnode->increment_refs();
          return std::make_pair(insertion ? true : false, CNode::Create(new SNode(key, insertion), hash_value, lnode, lnode_hash, level));
        }
        else {
          const auto [first_or_last, new_lnode] = lnode->updated(key, insertion);
          if (!new_lnode->next) {
            const auto new_snode = new_lnode->snode;
            new_snode->increment_refs();
            new_lnode->decrement_refs();
            return std::make_pair(first_or_last, new_snode);
          }
          return std::make_pair(first_or_last, new_lnode);
        }
      }
      else if (auto snode = dynamic_cast<const SNode *>(mnode)) {
        const Hash_Value snode_hash = Hash()(snode->key);
        if (snode_hash != hash_value) {
          snode->increment_refs();
          return std::make_pair(insertion ? true : false, CNode::Create(new SNode(key, insertion), hash_value, snode, snode_hash, level));
        }
        else if (snode->key != key) {
          snode->increment_refs();
          return std::make_pair(insertion ? true : false, new LNode(new SNode(key, insertion), new LNode(snode)));
        }
        else {
          const auto new_snode = snode->updated_count(insertion);
          return std::make_pair(!insertion && !new_snode, new_snode);
        }
      }
      else
        return std::make_pair(insertion ? true : false, new SNode(key, insertion));
    }

    template <typename VALUE_TYPE, typename DESIRED>
    static bool CAS(std::atomic<VALUE_TYPE *> &atomic_value, VALUE_TYPE * &expected, const DESIRED * desired, const std::memory_order success = std::memory_order_seq_cst, const std::memory_order failure = std::memory_order_seq_cst) {
      if (atomic_value.compare_exchange_strong(expected, desired, success, failure)) {
        if (expected)
          expected->decrement_refs();
        return true;
      }
      else {
        if (desired)
          desired->decrement_refs();
        return false;
      }
    }

    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<const MNode *> m_root = nullptr;
  };

}

#endif