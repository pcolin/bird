#ifndef LIKELY_H
#define LIKELY_H

inline bool likely(bool expr) {
  return __builtin_expect((expr), true);
}

inline bool unlikely(bool expr) {
  return __builtin_expect((expr), false);
}

#endif