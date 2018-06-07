
#ifndef GAP_BUFFER_ITERATOR_H_
#define GAP_BUFFER_ITERATOR_H_

#include "gap_buffer_fwd.h"
#include <cassert>
#include <iterator>
#include <type_traits>

template <class Ch, class Tr, bool IsConst>
class gap_buffer_iterator {
    using traits_type = typename std::iterator<std::random_access_iterator_tag, Ch>;
    using buffer_type = typename std::conditional<IsConst, const gap_buffer<Ch, Tr>, gap_buffer<Ch, Tr>>::type;
    using size_type   = typename buffer_type::size_type;

    template <class CharT, class Traits, bool Const>
    friend class gap_buffer_iterator;

public:
    using difference_type   = typename traits_type::difference_type;
    using iterator_category = typename traits_type::iterator_category;
    using pointer           = typename traits_type::pointer;
    using reference         = typename traits_type::reference;
    using value_type        = typename traits_type::value_type;

public:
    gap_buffer_iterator()                                : gap_buffer_iterator(nullptr, 0) {}
    gap_buffer_iterator(buffer_type *buf, size_type pos) : buf_(buf), pos_(pos) {}

public:
    // These only exist for the const version
    template <bool Const = IsConst, typename std::enable_if<Const>::type* = nullptr>
    gap_buffer_iterator(const gap_buffer_iterator<Ch, Tr, false> &other) : buf_(other.buf_), pos_(other.pos_) {}

    template <bool Const = IsConst, typename std::enable_if<Const>::type* = nullptr>
    gap_buffer_iterator& operator=(const gap_buffer_iterator<Ch, Tr, false> &rhs)  {
        gap_buffer_iterator(rhs).swap(*this);
        return *this;
    }

public:
    gap_buffer_iterator(const gap_buffer_iterator &rhs)         = default;
    gap_buffer_iterator& operator=(const gap_buffer_iterator &) = default;

public:
    gap_buffer_iterator& operator+=(difference_type rhs) { pos_ += rhs; return *this; }
    gap_buffer_iterator& operator-=(difference_type rhs) { pos_ -= rhs; return *this; }

public:
    gap_buffer_iterator& operator++()    { ++pos_; return *this; }
    gap_buffer_iterator& operator--()    { --pos_; return *this; }
    gap_buffer_iterator operator++(int)  { gap_buffer_iterator tmp(*this); ++pos_; return tmp; }
    gap_buffer_iterator operator--(int)  { gap_buffer_iterator tmp(*this); --pos_; return tmp; }

public:
    gap_buffer_iterator operator+(difference_type rhs) const { return gap_buffer_iterator(pos_ + rhs); }
    gap_buffer_iterator operator-(difference_type rhs) const { return gap_buffer_iterator(pos_ - rhs); }

public:
    difference_type operator-(const gap_buffer_iterator& rhs) const                           { assert(buf_ == rhs.buf_); return pos_ - rhs.pos_; }
    friend gap_buffer_iterator operator+(difference_type lhs, const gap_buffer_iterator& rhs) { return gap_buffer_iterator(lhs + rhs.pos_); }
    friend gap_buffer_iterator operator-(difference_type lhs, const gap_buffer_iterator& rhs) { return gap_buffer_iterator(lhs - rhs.pos_); }

public:
    reference operator*() const                        { return buf_[pos_];          }
    reference operator[](difference_type offset) const { return buf_[pos_ + offset]; }
    pointer operator->() const                         { return &buf_[pos_];         }

public:
    template <class CharT, class Traits, bool Const> bool operator==(const gap_buffer_iterator<CharT, Traits, Const> &rhs) const { assert(buf_ == rhs.buf_); return pos_ == rhs.pos_; }
    template <class CharT, class Traits, bool Const> bool operator!=(const gap_buffer_iterator<CharT, Traits, Const> &rhs) const { assert(buf_ == rhs.buf_); return pos_ != rhs.pos_; }
    template <class CharT, class Traits, bool Const> bool operator>(const gap_buffer_iterator<CharT, Traits, Const> &rhs) const  { assert(buf_ == rhs.buf_); return pos_ > rhs.pos_;  }
    template <class CharT, class Traits, bool Const> bool operator<(const gap_buffer_iterator<CharT, Traits, Const> &rhs) const  { assert(buf_ == rhs.buf_); return pos_ < rhs.pos_;  }
    template <class CharT, class Traits, bool Const> bool operator>=(const gap_buffer_iterator<CharT, Traits, Const> &rhs) const { assert(buf_ == rhs.buf_); return pos_ >= rhs.pos_; }
    template <class CharT, class Traits, bool Const> bool operator<=(const gap_buffer_iterator<CharT, Traits, Const> &rhs) const { assert(buf_ == rhs.buf_); return pos_ <= rhs.pos_; }

private:
    buffer_type *buf_;
    size_type    pos_;
};

#endif
