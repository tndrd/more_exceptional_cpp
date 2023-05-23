#ifndef MYSTACK_HPP_INCLUDE
#define MYSTACK_HPP_INCLUDE

#include <cassert>
#include <stdexcept>

namespace tndrd {

template<typename T> void construct_at (T* ptr, T&& rhs) {
    new (ptr) T {std::move(rhs)};
}

template<typename T> void construct_at (T* ptr, const T& rhs) {
    new (ptr) T {rhs};
}

template<typename T> void destroy_at (T* ptr) { ptr->~T(); }

template<typename FwdIter> void destroy(FwdIter first, FwdIter last) {
    for (; first != last; ++first)
        destroy_at(std::addressof(*first));
}

}

template<typename T>
class MyStackBuf {
protected:
    T* buf_;
    size_t size_;
    size_t used_;

protected:
    MyStackBuf(size_t sz):
        buf_{ !sz ? nullptr : static_cast<T*>(::operator new (sz * sizeof(T)))},
        size_{sz},
        used_{0}
    { }

    ~MyStackBuf() {
        tndrd::destroy(buf_, buf_ + used_);
        ::operator delete (buf_);
    }

    MyStackBuf(MyStackBuf&& rhs) noexcept:
        buf_{rhs.buf_},
        size_{rhs.size_},
        used_{rhs.used_}
    {
        rhs.buf_ = nullptr;
        rhs.size_ = 0;
        rhs.used_ = 0;        
    }

    MyStackBuf& operator= (MyStackBuf&& rhs) noexcept {
        std::swap(buf_, rhs.buf_);
        std::swap(used_, rhs.used_);
        std::swap(size_, rhs.size_);
        return *this;
    }

    MyStackBuf(const MyStackBuf& rhs) = delete;
    MyStackBuf& operator= (const MyStackBuf& rhs) = delete;
};

template<typename T>
class MyStack final: private MyStackBuf<T> {
    using MyStackBuf<T>::buf_;
    using MyStackBuf<T>::size_;
    using MyStackBuf<T>::used_;

public:
    explicit MyStack(size_t size = 0): MyStackBuf<T>{size} { }

    MyStack(MyStack&& rhs) = default;
    MyStack& operator= (MyStack&& rhs) = default;

    MyStack(const MyStack& rhs): MyStackBuf<T>{rhs.used_} {
        for (; used_ < rhs.used_; ++used_)
            tndrd::construct_at(buf_ + used_, rhs.buf_[used_]);
    }

    MyStack& operator= (const MyStack& rhs) { 
        MyStack tmp {rhs};
        std::swap(*this, tmp);
        return *this;
    }

    void push(const T& x) noexcept {
        T tmp {x};
        push(std::move(tmp));
    }

    void push(T&& x)
    {
        assert(used_ <= size_);
        static_assert(std::is_nothrow_move_constructible<T>::value);
        static_assert(std::is_nothrow_move_assignable<T>::value);
        
        if (used_ == size_) {
            MyStack tmp {size_ * 2 + 1};
            
            tmp.append(std::move(*this));
            tmp.push(std::move(x));

            std::swap(*this, tmp); 
        } else {
            tndrd::construct_at(buf_ + used_, std::move(x));
            ++used_;
        }
    }

    T top() const
    {
        if (empty())
            throw std::runtime_error("Attempt to top empty stack");
        return buf_[used_ - 1];
    }

    void pop()
    {
        if (empty())
            throw std::runtime_error("Attempt to pop empty stack");

        if (size_ && used_ < size_ / 2) {
            MyStack tmp {size_ / 2};
            tmp.append(std::move(*this));
            std::swap(*this, tmp); 
        }

        --used_;
        tndrd::destroy_at(buf_ + used_);
    }

    void append(MyStack&& rhs) noexcept {
        for (int i = 0; i < used_; ++i)
            push(std::move(rhs.buf_[i]));
    }

    size_t size() const { return used_; }
    size_t capacity() const { return size_; }
    bool empty() const { return used_ == 0; }

};

#endif
