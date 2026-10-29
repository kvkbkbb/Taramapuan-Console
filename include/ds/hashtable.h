#ifndef DS_HASHTABLE_H
#define DS_HASHTABLE_H

#include <string>

template <typename V>
class HashTable {
    struct Node {
        std::string key;
        V value;
        Node* next;
        Node(const std::string& k, const V& v) : key(k), value(v), next(nullptr) {}
    };

    Node** buckets_;
    int bucketCount_;
    int size_;

    unsigned hash(const std::string& key) const {
        unsigned h = 5381;
        for (size_t i = 0; i < key.size(); ++i) {
            h = ((h << 5) + h) + static_cast<unsigned char>(key[i]);
        }
        return h % static_cast<unsigned>(bucketCount_);
    }

    Node* findNode(const std::string& key) const {
        Node* current = buckets_[hash(key)];
        while (current != nullptr) {
            if (current->key == key) {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }

public:
    explicit HashTable(int bucketCount = 64)
        : bucketCount_(bucketCount), size_(0) {
        buckets_ = new Node*[bucketCount_];
        for (int i = 0; i < bucketCount_; ++i) {
            buckets_[i] = nullptr;
        }
    }

    ~HashTable() {
        clear();
        delete[] buckets_;
    }

    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    void put(const std::string& key, const V& value) {
        Node* existing = findNode(key);
        if (existing != nullptr) {
            existing->value = value;
            return;
        }
        const unsigned index = hash(key);
        Node* node = new Node(key, value);
        node->next = buckets_[index];
        buckets_[index] = node;
        ++size_;
    }

    V* get(const std::string& key) {
        Node* node = findNode(key);
        return node == nullptr ? nullptr : &node->value;
    }

    const V* get(const std::string& key) const {
        const Node* node = findNode(key);
        return node == nullptr ? nullptr : &node->value;
    }

    bool contains(const std::string& key) const {
        return findNode(key) != nullptr;
    }

    void remove(const std::string& key) {
        const unsigned index = hash(key);
        Node* current = buckets_[index];
        Node* previous = nullptr;
        while (current != nullptr) {
            if (current->key == key) {
                if (previous == nullptr) {
                    buckets_[index] = current->next;
                } else {
                    previous->next = current->next;
                }
                delete current;
                --size_;
                return;
            }
            previous = current;
            current = current->next;
        }
    }

    void clear() {
        for (int i = 0; i < bucketCount_; ++i) {
            Node* current = buckets_[i];
            while (current != nullptr) {
                Node* next = current->next;
                delete current;
                current = next;
            }
            buckets_[i] = nullptr;
        }
        size_ = 0;
    }

    int size() const {
        return size_;
    }
};

#endif
