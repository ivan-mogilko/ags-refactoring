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

#ifdef _DEBUG

#include "util/ptr.h"
#include "debug/assert.h"

using AGS::Common::UniquePtr;

struct A
{
    int a;

    A()
    {
        a_ctor_counter++;
    }
    virtual ~A()
    {
        a_dtor_counter++;
    }

    static size_t a_ctor_counter;
    static size_t a_dtor_counter;
};

size_t A::a_ctor_counter = 0;
size_t A::a_dtor_counter = 0;

struct B : public A
{
    int b;

    B()
    {
        b_ctor_counter++;
    }
    ~B()
    {
        b_dtor_counter++;
    }

    static size_t b_ctor_counter;
    static size_t b_dtor_counter;
};

size_t B::b_ctor_counter = 0;
size_t B::b_dtor_counter = 0;

void Test_UniquePtr()
{
    typedef UniquePtr<A> AUPtr;
    A::a_ctor_counter = 0;
    A::a_dtor_counter = 0;
    B::b_ctor_counter = 0;
    B::b_dtor_counter = 0;
    {
        A *a = new A();
        AUPtr a1(a);
        assert(a1.Get() == a);
        AUPtr a2 = a1;
        assert(a1.Get() == NULL);
        assert(a2.Get() == a);
        assert(a1 == NULL);
        assert(a2 == a);
    }
    assert(A::a_ctor_counter == 1);
    assert(A::a_dtor_counter == 1);
    {
        AUPtr a1(new A());
        a1->a = 1;
        (*a1).a++;
        AUPtr a2(new A());
        a2->a = 5;
        a1 = a2;
        assert(a1->a == 5);
        a2 = AUPtr(new A());
    }
    assert(A::a_ctor_counter == 4);
    assert(A::a_dtor_counter == 4);
    {
        AUPtr a1(new B());
        AUPtr a2(a1);
    }
    assert(A::a_ctor_counter == 5);
    assert(A::a_dtor_counter == 5);
    assert(B::b_ctor_counter == 1);
    assert(B::b_dtor_counter == 1);
}

void Test_Ptr()
{
    Test_UniquePtr();
}

#endif // _DEBUG
