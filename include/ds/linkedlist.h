#ifndef DS_LINKEDLIST_H
#define DS_LINKEDLIST_H

template <typename T>
class LinkedList {
public:
    struct Node {
        T value;
        Node* previous;
        Node* next;

        explicit Node(const T& item)
            : value(item), previous(nullptr), next(nullptr) {}
    };

    LinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}

    ~LinkedList() {
        clear();
    }

    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;

    T* append(const T& item) {
        Node* node = new Node(item);
        node->previous = tail_;
        if (tail_ != nullptr) {
            tail_->next = node;
        } else {
            head_ = node;
        }
        tail_ = node;
        ++size_;
        return &node->value;
    }

    void clear() {
        Node* current = head_;
        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }
        head_ = nullptr;
        tail_ = nullptr;
        size_ = 0;
    }

    int size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    Node* head() {
        return head_;
    }

    const Node* head() const {
        return head_;
    }

private:
    Node* head_;
    Node* tail_;
    int size_;
};

#endif
