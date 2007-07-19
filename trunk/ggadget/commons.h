/*
  Copyright 2007 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef GGADGET_COMMONS_H__
#define GGADGET_COMMONS_H__

#include <cassert>
#include <stdint.h>         // Integer types and macros.

namespace ggadget {

#ifdef __GNUC__
/**
 * Tell the compiler to do printf format string checking if the
 * compiler supports it.
 */
#define PRINTF_ATTRIBUTE(arg1,arg2) \
  __attribute__((__format__ (__printf__, arg1, arg2)))

/**
 * Tell the compiler to do scanf format string checking if the
 * compiler supports it.
 */
#define SCANF_ATTRIBUTE(arg1,arg2) \
    __attribute__((__format__ (__scanf__, arg1, arg2)))
    
#else // __GNUC__

#define PRINTF_ATTRIBUTE(arg1, arg2)
#define SCANF_ATTRIBUTE(arg1, arg2)

#endif // else __GNUC__

/** A macro to turn a symbol into a string. */
#define AS_STRING(x)   AS_STRING_INTERNAL(x)
#define AS_STRING_INTERNAL(x)   #x

/**
 * A macro to disallow the evil copy constructor and @c operator= methods.
 * This should be used in the @c private: declarations for a class.
 */
#define DISALLOW_EVIL_CONSTRUCTORS(TypeName)    \
  TypeName(const TypeName&);                    \
  void operator=(const TypeName&)

/**
 * A macro to disallow all the implicit constructors, namely the
 * default constructor, copy constructor and @c operator= methods.
 * This should be used in the private: declarations for a class
 * that wants to prevent anyone from instantiating it. This is
 * especially useful for classes containing only static methods.
 */
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName();                                    \
  DISALLOW_EVIL_CONSTRUCTORS(TypeName)

/** Use @c implicit_cast as a safe version of @c static_cast or @c const_cast
 * for upcasting in the type hierarchy.
 * It's used to cast a pointer to @c Foo to a pointer to @c SuperclassOfFoo
 * or casting a pointer to @c Foo to a const pointer to @c Foo).<br>
 * When you use @c implicit_cast, the compiler checks that the cast is safe.
 * Such explicit <code>implicit_cast</code>s are necessary in surprisingly
 * many situations where C++ demands an exact type match instead of an
 * argument type convertable to a target type.
 *
 * <p>The From type can be inferred, so the preferred syntax for using
 * implicit_cast is the same as for static_cast etc.:</p>
 * <code>
 *   implicit_cast<ToType>(expr)
 * </code>
 *
 * <p>@c implicit_cast would have been part of the C++ standard library,
 * but the proposal was submitted too late.  It will probably make
 * its way into the language in the future.
 */
template<typename To, typename From>
inline To implicit_cast(From const &f) {
  return f;
}

/**
 * Downcast a pointer.
 * When you upcast (that is, cast a pointer from type @c Foo to type
 * @c SuperclassOfFoo), it's fine to use @c implicit_cast, since upcasts
 * always succeed.  When you downcast (that is, cast a pointer from
 * type @c Foo to type @c SubclassOfFoo), @c static_cast isn't safe, because
 * how do you know the pointer is really of type @c SubclassOfFoo?  It
 * could be a bare @c Foo, or of type @c DifferentSubclassOfFoo.  Thus,
 * when you downcast, you should use this template.  In debug mode, we
 * use @c dynamic_cast to double-check the downcast is legal (we die
 * if it's not).  In normal mode, we do the efficient @c static_cast
 * instead.
 * <p>Use like this: <code>down_cast<T*>(foo)</code>.
 */
template<typename To, typename From>
inline To down_cast(From* f) {          // so we only accept pointers
  // Ensures that To is a sub-type of From *.  This test is here only
  // for compile-time type checking, and has no overhead in an
  // optimized build at run-time, as it will be optimized away
  // completely.
  if (false) {
    implicit_cast<From*, To>(0);
  }

  ASSERT(f == NULL || dynamic_cast<To>(f) != NULL); // RTTI: debug mode only!
  return static_cast<To>(f);
}

/**
 * This template function declaration is used in defining arraysize.
 * Note that the function doesn't need an implementation, as we only
 * use its type.
 */
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
template <typename T, size_t N>
char (&ArraySizeHelper(const T (&array)[N]))[N];

/**
 * The @c arraysize(arr) macro returns the # of elements in an array arr.
 * The expression is a compile-time constant, and therefore can be
 * used in defining new arrays, for example.  If you use arraysize on
 * a pointer by mistake, you will get a compile-time error.
 *
 * One caveat is that @c arraysize() doesn't accept any array of an
 * anonymous type or a type defined inside a function.  This is
 * due to a limitation in C++'s template system.  The limitation might
 * eventually be removed, but it hasn't happened yet.
 */
#define arraysize(array) (sizeof(ArraySizeHelper(array)))


#undef LOG
#undef ASSERT
#undef ASSERT_M
#undef VERIFY
#undef VERIFY_M
#undef DLOG

/**
 * Print log with printf format parameters.
 * It works in both debug and release versions.
 */
// TODO: Let log information go into debug console.
#define LOG  printf

#ifdef NDEBUG
#define ASSERT(x)
#define ASSERT_M(x, y)
#define VERIFY(x)
#define VERIFY_M(x, y)
#define DLOG(x)
#else // NDEBUG

/**
 * Assert an expression and abort if it is not true.
 * Normally it only works in debug versions.
 */
#define ASSERT(x) assert(x)

/**
 * Assert an expression with a message in printf format.
 * It only works in debug versions.
 * Sample usage: <code>ASSERT_M(a==b, ("%d==%d failed\n", a, b));</code>
 */
#define ASSERT_M(x, y) do { if (!(x)) { DLOG y; assert(x); } } while (0)

/**
 * Verify an expression and print a message if the expression is not true.
 * It only works in debug versions.
 */ 
#define VERIFY(x) do { if (!(x)) \
    DLOG("%s:%d: VERIFY FAILED", __FILE__, __LINE__); } while (0)

/**
 * Verify an expression with a message in printf format.
 * It only works in debug versions.
 * Sample usage: <code>VERIFY_M(a==b, ("%d==%d failed\n", a, b));</code>
 */ 
#define VERIFY_M(x, y) do { if (!(x)) { DLOG y; VERIFY(x); } } while (0)

/**
 * Print debug log with printf format parameters.
 * It only works in debug versions.
 */
// TODO: Let log information go into debug console.
#define DLOG  printf

#endif // else NDEBUG

} // namespace ggadget

#endif  // GGADGET_COMMONS_H__
