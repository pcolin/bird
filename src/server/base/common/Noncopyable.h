#ifndef BASE_NONCOPYABLE_H
#define BASE_NONCOPYABLE_H

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  void operator=(const TypeName&) = delete;\
  void operator=(TypeName&&) = delete;     \
  TypeName(const TypeName&) = delete;      \
  TypeName(TypeName&&) = delete

#endif // BASE_NONCOPYABLE_H
