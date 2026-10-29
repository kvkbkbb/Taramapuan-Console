#ifndef DS_QUEUE_H
#define DS_QUEUE_H

template <typename T>
class Queue {
    struct Node {
        T value;
        Node* next;
        explicit Node(const T& item) : value(item), next(nullptr) {}
    };

    Node* head_;
    Node* tail_;
    int size_;

public:
    Queue() : head_(nullptr), tail_(nullptr), size_(0) {}

    Queue(const Queue& other) : head_(nullptr), tail_(nullptr), size_(0) {
        for (const Node* current = other.head_; current != nullptr; current = current->next) {
            enqueue(current->value);
        }
    }

    ~Queue() {
        clear();
    }

    Queue& operator=(const Queue& other) {
        if (this == &other) {
            return *this;
        }
        clear();
        for (const Node* current = other.head_; current != nullptr; current = current->next) {
            enqueue(current->value);
        }
        return *this;
    }

    void enqueue(const T& item) {
        Node* node = new Node(item);
        if (tail_ != nullptr) {
            tail_->next = node;
        } else {
            head_ = node;
        }
        tail_ = node;
        ++size_;
    }

    void dequeue() {
        if (head_ == nullptr) {
            return;
        }
        Node* node = head_;
        head_ = head_->next;
        if (head_ == nullptr) {
            tail_ = nullptr;
        }
        delete node;
        --size_;
    }

    T& front() {
        return head_->value;
    }

    const T& front() const {
        return head_->value;
    }

    bool empty() const {
        return size_ == 0;
    }

    int size() const {
        return size_;
    }

    void clear() {
        while (!empty()) {
            dequeue();
        }
    }

    template <typename Fn>
    void forEach(Fn fn) const {
        for (const Node* current = head_; current != nullptr; current = current->next) {
            fn(current->value);
        }
    }
};

#endif
