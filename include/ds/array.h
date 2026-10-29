#ifndef DS_ARRAY_H
#define DS_ARRAY_H

template <typename T>
class Array {
    T* data_;
    int size_;
    int cap_;

    void ensure(int needed) {
        if (needed <= cap_) {
            return;
        }
        int newCap = cap_ == 0 ? 8 : cap_ * 2;
        while (newCap < needed) {
            newCap *= 2;
        }
        T* fresh = new T[newCap];
        for (int i = 0; i < size_; ++i) {
            fresh[i] = data_[i];
        }
        delete[] data_;
        data_ = fresh;
        cap_ = newCap;
    }

public:
    Array() : data_(nullptr), size_(0), cap_(0) {}

    Array(const Array& other) : data_(nullptr), size_(0), cap_(0) {
        ensure(other.size_);
        size_ = other.size_;
        for (int i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
    }

    ~Array() {
        delete[] data_;
    }

    Array& operator=(const Array& other) {
        if (this == &other) {
            return *this;
        }
        Array copy(other);
        swap(copy);
        return *this;
    }

    void swap(Array& other) {
        T* d = data_;
        data_ = other.data_;
        other.data_ = d;
        int s = size_;
        size_ = other.size_;
        other.size_ = s;
        int c = cap_;
        cap_ = other.cap_;
        other.cap_ = c;
    }

    void push(const T& value) {
        ensure(size_ + 1);
        data_[size_++] = value;
    }

    void pop() {
        if (size_ > 0) {
            --size_;
        }
    }

    void clear() {
        size_ = 0;
    }

    void removeAt(int index) {
        for (int i = index; i < size_ - 1; ++i) {
            data_[i] = data_[i + 1];
        }
        if (size_ > 0) {
            --size_;
        }
    }

    T& operator[](int index) {
        return data_[index];
    }

    const T& operator[](int index) const {
        return data_[index];
    }

    T& back() {
        return data_[size_ - 1];
    }

    const T& back() const {
        return data_[size_ - 1];
    }

    int size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    T* begin() {
        return data_;
    }

    T* end() {
        return data_ + size_;
    }

    const T* begin() const {
        return data_;
    }

    const T* end() const {
        return data_ + size_;
    }
};

#endif
