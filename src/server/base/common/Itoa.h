/* Itoa.h
** Effecient interger to string vonversions by Matthew Wilson.
*/

#ifndef BASE_ITOA_H
#define BASE_ITOA_H

#include <assert.h>
#include <cstddef>
#include <algorithm>

namespace base {
namespace detail {

const char digits[] = "9876543210123456789";
static const char* zero = digits + 9;
static_assert(sizeof(digits) == 20, "");

const char digitsHex[] = "0123456789ABCDEF";
static_assert(sizeof digitsHex == 17, "");

} // namespace detail

template<typename T> size_t convert(char buf[], T value) {
  T i = value;
  char* p = buf;

  do {
    int lsd = static_cast<int>(i % 10);
    i /= 10;
    *p++ = detail::zero[lsd];
  } while (i != 0);

  if (value < 0) {
    *p++ = '-';
  }
  *p = '\0';
  std::reverse(buf, p);

  return p - buf;
}

static size_t convertHex(char buf[], uintptr_t value) {
  uintptr_t i = value;
  char* p = buf;

  do {
    int lsd = static_cast<int>(i % 16);
    i /= 16;
    *p++ = detail::digitsHex[lsd];
  } while (i != 0);

  *p = '\0';
  std::reverse(buf, p);

  return p - buf;
}

} // namespace base

#endif // BASE_ITOA_H
