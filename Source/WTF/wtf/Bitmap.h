/*
 *  Copyright (C) 2010-2017 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#ifndef Bitmap_h
#define Bitmap_h

#include <array>
#include <wtf/Atomics.h>
#include <wtf/StdLibExtras.h>
#include <stdint.h>
#include <string.h>
#include "HashFunctions.h"

namespace WTF {

template<size_t bitmapSize, typename WordType = uint32_t>
class Bitmap {
    WTF_MAKE_FAST_ALLOCATED;
    
    static_assert(sizeof(WordType) <= sizeof(unsigned), "WordType must not be bigger than unsigned");
public:
    Bitmap();

    static constexpr size_t size()
    {
        return bitmapSize;
    }

    bool get(size_t, Dependency = Dependency()) const;
    void set(size_t);
    void set(size_t, bool);
    bool testAndSet(size_t);
    bool testAndClear(size_t);
    bool concurrentTestAndSet(size_t, Dependency = Dependency());
    bool concurrentTestAndClear(size_t, Dependency = Dependency());
    size_t nextPossiblyUnset(size_t) const;
    void clear(size_t);
    void clearAll();
    int64_t findRunOfZeros(size_t runLength) const;
    size_t count(size_t start = 0) const;
    size_t isEmpty() const;
    size_t isFull() const;
    
    void merge(const Bitmap&);
    void filter(const Bitmap&);
    void exclude(const Bitmap&);
    
    bool subsumes(const Bitmap&) const;
    
    template<typename Func>
    void forEachSetBit(const Func&) const;
    
    size_t findBit(size_t startIndex, bool value) const;
    
    class iterator {
    public:
        iterator()
            : m_bitmap(nullptr)
            , m_index(0)
        {
        }
        
        iterator(const Bitmap& bitmap, size_t index)
            : m_bitmap(&bitmap)
            , m_index(index)
        {
        }
        
        size_t operator*() const { return m_index; }
        
        iterator& operator++()
        {
            m_index = m_bitmap->findBit(m_index + 1, true);
            return *this;
        }
        
        bool operator==(const iterator& other) const
        {
            return m_index == other.m_index;
        }
        
        bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }

    private:
        const Bitmap* m_bitmap;
        size_t m_index;
    };
    
    // Use this to iterate over set bits.
    iterator begin() const { return iterator(*this, findBit(0, true)); }
    iterator end() const { return iterator(*this, bitmapSize); }
    
    void mergeAndClear(Bitmap&);
    void setAndClear(Bitmap&);
    
    bool operator==(const Bitmap&) const;
    bool operator!=(const Bitmap&) const;
    
    unsigned hash() const;

private:
    static const unsigned wordSize = sizeof(WordType) * 8;
    static const unsigned words = (bitmapSize + wordSize - 1) / wordSize;

    // the literal '1' is of type signed int.  We want to use an unsigned
    // version of the correct size when doing the calculations because if
    // WordType is larger than int, '1 << 31' will first be sign extended
    // and then casted to unsigned, meaning that set(31) when WordType is
    // a 64 bit unsigned int would give 0xffff8000
    static const WordType one = 1;

    std::array<WordType, words> bits;
};

template<size_t bitmapSize, typename WordType>
inline Bitmap<bitmapSize, WordType>::Bitmap()
{
    clearAll();
}

template<size_t bitmapSize, typename WordType>
inline bool Bitmap<bitmapSize, WordType>::get(size_t n, Dependency dependency) const
{
    return !!(dependency.consume(this)->bits[n / wordSize] & (one << (n % wordSize)));
}

template<size_t bitmapSize, typename WordType>
inline void Bitmap<bitmapSize, WordType>::set(size_t n)
{
    bits[n / wordSize] |= (one << (n % wordSize));
}

template<size_t bitmapSize, typename WordType>
inline void Bitmap<bitmapSize, WordType>::set(size_t n, bool value)
{
    if (value)
        set(n);
    else
        clear(n);
}

template<size_t bitmapSize, typename WordType>
inline bool Bitmap<bitmapSize, WordType>::testAndSet(size_t n)
{
    WordType mask = one << (n % wordSize);
    size_t index = n / wordSize;
    bool result = bits[index] & mask;
    bits[index] |= mask;
    return result;
}

template<size_t bitmapSize, typename WordType>
inline bool Bitmap<bitmapSize, WordType>::testAndClear(size_t n)
{
    WordType mask = one << (n % wordSize);
    size_t index = n / wordSize;
    bool result = bits[index] & mask;
    bits[index] &= ~mask;
    return result;
}

template<size_t bitmapSize, typename WordType>
ALWAYS_INLINE bool Bitmap<bitmapSize, WordType>::concurrentTestAndSet(size_t n, Dependency dependency)
{
    WordType mask = one << (n % wordSize);
    size_t index = n / wordSize;
    WordType* data = dependency.consume(bits.data()) + index;
    return !bitwise_cast<Atomic<WordType>*>(data)->transactionRelaxed(
        [&] (WordType& value) -> bool {
            if (value & mask)
                return false;
            
            value |= mask;
            return true;
        });
}

template<size_t bitmapSize, typename WordType>
ALWAYS_INLINE bool Bitmap<bitmapSize, WordType>::concurrentTestAndClear(size_t n, Dependency dependency)
{
    WordType mask = one << (n % wordSize);
    size_t index = n / wordSize;
    WordType* data = dependency.consume(bits.data()) + index;
    return !bitwise_cast<Atomic<WordType>*>(data)->transactionRelaxed(
        [&] (WordType& value) -> bool {
            if (!(value & mask))
                return false;
            
            value &= ~mask;
            return true;
        });
}

template<size_t bitmapSize, typename WordType>
inline void Bitmap<bitmapSize, WordType>::clear(size_t n)
{
    bits[n / wordSize] &= ~(one << (n % wordSize));
}

template<size_t bitmapSize, typename WordType>
inline void Bitmap<bitmapSize, WordType>::clearAll()
{
    memset(bits.data(), 0, sizeof(bits));
}

template<size_t bitmapSize, typename WordType>
inline size_t Bitmap<bitmapSize, WordType>::nextPossiblyUnset(size_t start) const
{
    if (!~bits[start / wordSize])
        return ((start / wordSize) + 1) * wordSize;
    return start + 1;
}

template<size_t bitmapSize, typename WordType>
inline int64_t Bitmap<bitmapSize, WordType>::findRunOfZeros(size_t runLength) const
{
    if (!runLength) 
        runLength = 1; 
     
    for (size_t i = 0; i <= (bitmapSize - runLength) ; i++) {
        bool found = true; 
        for (size_t j = i; j <= (i + runLength - 1) ; j++) { 
            if (get(j)) {
                found = false; 
                break;
            }
        }
        if (found)  
            return i; 
    }
    return -1;
}

template<size_t bitmapSize, typename WordType>
inline size_t Bitmap<bitmapSize, WordType>::count(size_t start) const
{
    size_t result = 0;
    for ( ; (start % wordSize); ++start) {
        if (get(start))
            ++result;
    }
    for (size_t i = start / wordSize; i < words; ++i)
        result += WTF::bitCount(static_cast<unsigned>(bits[i]));
    return result;
}

template<size_t bitmapSize, typename WordType>
inline size_t Bitmap<bitmapSize, WordType>::isEmpty() const
{
    for (size_t i = 0; i < words; ++i)
        if (bits[i])
            return false;
    return true;
}

template<size_t bitmapSize, typename WordType>
inline size_t Bitmap<bitmapSize, WordType>::isFull() const
{
    for (size_t i = 0; i < words; ++i)
        if (~bits[i])
            return false;
    return true;
}

template<size_t bitmapSize, typename WordType>
inline void Bitmap<bitmapSize, WordType>::merge(const Bitmap& other)
{
    for (size_t i = 0; i < words; ++i)
        bits[i] |= other.bits[i];
}

template<size_t bitmapSize, typename WordType>
inline void Bitmap<bitmapSize, WordType>::filter(const Bitmap& other)
{
    for (size_t i = 0; i < words; ++i)
        bits[i] &= other.bits[i];
}

template<size_t bitmapSize, typename WordType>
inline void Bitmap<bitmapSize, WordType>::exclude(const Bitmap& other)
{
    for (size_t i = 0; i < words; ++i)
        bits[i] &= ~other.bits[i];
}

template<size_t bitmapSize, typename WordType>
inline bool Bitmap<bitmapSize, WordType>::subsumes(const Bitmap& other) const
{
    for (size_t i = 0; i < words; ++i) {
        WordType myBits = bits[i];
        WordType otherBits = other.bits[i];
        if ((myBits | otherBits) != myBits)
            return false;
    }
    return true;
}

template<size_t bitmapSize, typename WordType>
template<typename Func>
inline void Bitmap<bitmapSize, WordType>::forEachSetBit(const Func& func) const
{
    for (size_t i = 0; i < words; ++i) {
        WordType word = bits[i];
        if (!word)
            continue;
        size_t base = i * wordSize;
        for (size_t j = 0; j < wordSize; ++j) {
            if (word & 1)
                func(base + j);
            word >>= 1;
        }
    }
}

template<size_t bitmapSize, typename WordType>
inline size_t Bitmap<bitmapSize, WordType>::findBit(size_t startIndex, bool value) const
{
    WordType skipValue = -(static_cast<WordType>(value) ^ 1);
    size_t wordIndex = startIndex / wordSize;
    size_t startIndexInWord = startIndex - wordIndex * wordSize;
    
    while (wordIndex < words) {
        WordType word = bits[wordIndex];
        if (word != skipValue) {
            size_t index = startIndexInWord;
            if (findBitInWord(word, index, wordSize, value))
                return wordIndex * wordSize + index;
        }
        
        wordIndex++;
        startIndexInWord = 0;
    }
    
    return bitmapSize;
}

template<size_t bitmapSize, typename WordType>
inline void Bitmap<bitmapSize, WordType>::mergeAndClear(Bitmap& other)
{
    for (size_t i = 0; i < words; ++i) {
        bits[i] |= other.bits[i];
        other.bits[i] = 0;
    }
}

template<size_t bitmapSize, typename WordType>
inline void Bitmap<bitmapSize, WordType>::setAndClear(Bitmap& other)
{
    for (size_t i = 0; i < words; ++i) {
        bits[i] = other.bits[i];
        other.bits[i] = 0;
    }
}

template<size_t bitmapSize, typename WordType>
inline bool Bitmap<bitmapSize, WordType>::operator==(const Bitmap& other) const
{
    for (size_t i = 0; i < words; ++i) {
        if (bits[i] != other.bits[i])
            return false;
    }
    return true;
}

template<size_t bitmapSize, typename WordType>
inline bool Bitmap<bitmapSize, WordType>::operator!=(const Bitmap& other) const
{
    return !(*this == other);
}

template<size_t bitmapSize, typename WordType>
inline unsigned Bitmap<bitmapSize, WordType>::hash() const
{
    unsigned result = 0;
    for (size_t i = 0; i < words; ++i)
        result ^= IntHash<WordType>::hash(bits[i]);
    return result;
}

} // namespace WTF

using WTF::Bitmap;

#endif
