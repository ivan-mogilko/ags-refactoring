//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// Helper pointer wrappers.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__PTR_H
#define __AGS_CN_UTIL__PTR_H

#include "core/types.h"
#include "debug/assert.h"

namespace AGS
{
namespace Common
{

//=============================================================================
//
// UniquePtr - a wrapper around object pointer that enforces sole ownership
// of an object and provides automatic deletion.
// When assigned, UniquePtr does not copy pointer, but "moves" it, giving
// away ownership.
//
// NOTE: this implementation of unique ptr is not "authentic", because it
// does not have moving ctor only supported by C++11 standart.
//
//=============================================================================
template <class T>
class UniquePtr
{
public:
    UniquePtr()
        : _p(NULL)
    {
    }

    UniquePtr(UniquePtr<T> &ptr)
        : _p(NULL)
    {
        *this = ptr;
    }

    UniquePtr(T *p)
        : _p(p)
    {
    }

    ~UniquePtr()
    {
        delete _p;
    }

    // Returns raw pointer without loosing ownership
    inline T *Get() const
    {
        return _p;
    }

    // Returns raw pointer, giving away ownership
    T *Release()
    {
        T *p = _p;
        _p = NULL;
        return p;
    }

    // Replaces owned pointer
    void Reset(T *p = NULL)
    {
        delete _p;
        _p = p;
    }

    // Exchanges pointers with another UniquePtr
    inline void Swap(UniquePtr<T> &ptr)
    {
        T *p = ptr._p;
        ptr._p = _p;
        _p = p;
    }

    // Assignment operator transfers object ownership from another UniquePtr;
    // the previous object (if any) is deleted
    inline UniquePtr<T> &operator=(UniquePtr<T> &ptr)
    {
        Reset(ptr.Release());
        return *this;
    } 

    inline T *operator->() const
    {
        assert(_p != NULL);
        return _p;
    }

    inline T &operator*() const
    {
        assert(_p != NULL);
        return *_p;
    }

    inline bool operator==(const UniquePtr<T> &ptr) const { return _p == ptr._p; }
    inline bool operator==(const T *p) const { return _p == p; }
    inline bool operator!=(const UniquePtr<T> &ptr) const { return _p != ptr._p; }
    inline bool operator!=(const T *p) const { return _p != p; }
    // Helping compiler to compare with 0 (int)
    inline bool operator == (int addr) const { return _p == (void*)addr; }
    inline bool operator != (int addr) const { return _p != (void*)addr; }

    inline operator bool() const
    {
        return _p != NULL;
    }

private:
    T      *_p;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__PTR_H
