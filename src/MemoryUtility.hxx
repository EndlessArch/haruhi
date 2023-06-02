#ifndef HARUHI_MEMORYUTILITY_HXX
#define HARUHI_MEMORYUTILITY_HXX

#include <memory>

#include <AppKit/AppKit.hpp>

typedef struct __nod {
    void operator()(NS::Object*p) const {
      p->release();
    }
  } _ns_object_deleter;

template <typename T>
using nsp_unique = std::unique_ptr<T, _ns_object_deleter>;

#endif