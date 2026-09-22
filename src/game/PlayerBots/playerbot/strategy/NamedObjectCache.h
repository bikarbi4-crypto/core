#pragma once

#include <algorithm>
#include <string>
#include <utility>

namespace ai
{
    // An ordered, owning key store; object ownership remains with NamedObjectContext.
    // V9 needs a three-way lookup that stops at equality. std::map's public API
    // cannot request that traversal, and accessing MSVC's private tree is not portable.
    // AVL rotations preserve node addresses and ordered iteration, including when
    // an Update callback inserts another value into the context.
    template <class T>
    class NamedObjectCache
    {
        using Value = std::pair<const std::string, T*>;

        struct Node
        {
            explicit Node(const std::string& key, Node* parent) : value(key, nullptr), parent(parent) {}
            Value value;
            Node* left = nullptr;
            Node* right = nullptr;
            Node* parent;
            int height = 1;
        };

        static Node* First(Node* node)
        {
            if (node)
                while (node->left)
                    node = node->left;
            return node;
        }

        static Node* Next(Node* node)
        {
            if (node->right)
                return First(node->right);
            Node* parent = node->parent;
            while (parent && node == parent->right)
            {
                node = parent;
                parent = parent->parent;
            }
            return parent;
        }

    public:
        class iterator
        {
        public:
            explicit iterator(Node* node = nullptr) : node(node) {}
            Value& operator*() const { return node->value; }
            Value* operator->() const { return &node->value; }
            iterator& operator++() { node = Next(node); return *this; }
            iterator operator++(int) { iterator old(*this); ++*this; return old; }
            bool operator==(iterator other) const { return node == other.node; }
            bool operator!=(iterator other) const { return node != other.node; }
        private:
            Node* node;
        };

        NamedObjectCache() = default;
        NamedObjectCache(const NamedObjectCache&) = delete;
        NamedObjectCache& operator=(const NamedObjectCache&) = delete;
        ~NamedObjectCache() { clear(); }

        iterator begin() { return iterator(First(root)); }
        iterator end() { return iterator(); }
        iterator find(const std::string& key) { return iterator(Find(key)); }

        T*& operator[](const std::string& key)
        {
            Node* parent = nullptr;
            Node* node = root;
            int order = 0;
            while (node)
            {
                order = key.compare(node->value.first);
                if (order == 0)
                    return node->value.second;
                parent = node;
                node = order < 0 ? node->left : node->right;
            }

            // Finish allocation/key copying before linking anything into the tree.
            Node* inserted = new Node(key, parent);
            if (!parent)
                root = inserted;
            else if (order < 0)
                parent->left = inserted;
            else
                parent->right = inserted;
            Rebalance(parent);
            return inserted->value.second;
        }

        void erase(const std::string& key)
        {
            Node* node = Find(key);
            if (!node)
                return;

            Node* balanceFrom = node->parent;
            if (node->left && node->right)
            {
                Node* successor = First(node->right);
                if (successor->parent == node)
                    balanceFrom = successor;
                else
                {
                    balanceFrom = successor->parent;
                    Replace(successor, successor->right);
                    successor->right = node->right;
                    successor->right->parent = successor;
                }
                // Transplant the node itself: other iterators/keys stay valid.
                Replace(node, successor);
                successor->left = node->left;
                successor->left->parent = successor;
                successor->height = node->height;
            }
            else
                Replace(node, node->left ? node->left : node->right);

            delete node;
            Rebalance(balanceFrom);
        }

        void clear()
        {
            DeleteNodes(root);
            root = nullptr;
        }

    private:
        Node* Find(const std::string& key) const
        {
            Node* node = root;
            while (node)
            {
                const int order = key.compare(node->value.first);
                if (order == 0)
                    return node; // Also a hit when the stored object is nullptr.
                node = order < 0 ? node->left : node->right;
            }
            return nullptr;
        }

        static int Height(Node* node) { return node ? node->height : 0; }
        static void UpdateHeight(Node* node)
        {
            node->height = 1 + (std::max)(Height(node->left), Height(node->right));
        }

        void Replace(Node* node, Node* replacement)
        {
            if (!node->parent)
                root = replacement;
            else if (node->parent->left == node)
                node->parent->left = replacement;
            else
                node->parent->right = replacement;
            if (replacement)
                replacement->parent = node->parent;
        }

        Node* RotateLeft(Node* node)
        {
            Node* top = node->right;
            Replace(node, top);
            node->right = top->left;
            if (node->right)
                node->right->parent = node;
            top->left = node;
            node->parent = top;
            UpdateHeight(node);
            UpdateHeight(top);
            return top;
        }

        Node* RotateRight(Node* node)
        {
            Node* top = node->left;
            Replace(node, top);
            node->left = top->right;
            if (node->left)
                node->left->parent = node;
            top->right = node;
            node->parent = top;
            UpdateHeight(node);
            UpdateHeight(top);
            return top;
        }

        void Rebalance(Node* node)
        {
            while (node)
            {
                UpdateHeight(node);
                const int balance = Height(node->left) - Height(node->right);
                if (balance > 1)
                {
                    if (Height(node->left->left) < Height(node->left->right))
                        RotateLeft(node->left);
                    node = RotateRight(node);
                }
                else if (balance < -1)
                {
                    if (Height(node->right->right) < Height(node->right->left))
                        RotateRight(node->right);
                    node = RotateLeft(node);
                }
                node = node->parent;
            }
        }

        static void DeleteNodes(Node* node)
        {
            if (!node)
                return;
            DeleteNodes(node->left);
            DeleteNodes(node->right);
            delete node;
        }

        Node* root = nullptr;
    };
}
