#pragma once

#include <stddef.h>

// TODO: These are profiling helpers, remove before release
#ifdef _MSC_VER
#define NANOSORT_INLINE __forceinline
#define NANOSORT_NOINLINE __declspec(noinline)
#else
#define NANOSORT_INLINE __attribute__((always_inline))
#define NANOSORT_NOINLINE __attribute__((noinline))
#endif

namespace nanosort_detail {

struct Less {
  template <typename T>
  bool operator()(const T& l, const T& r) const {
    return l < r;
  }
};

template <typename It>
struct IteratorTraits {
  typedef typename It::value_type value_type;
};

template <typename T>
struct IteratorTraits<T*> {
  typedef T value_type;
};

template <typename T>
NANOSORT_INLINE void swap(T& l, T& r) {
#if __cplusplus > 199711L
  T t(static_cast<T&&>(l));
  l = static_cast<T&&>(r);
  r = static_cast<T&&>(t);
#else
  T t(l);
  l = r;
  r = t;
#endif
}

// Return median of 5 elements in the array
template <typename T, typename It, typename Compare>
NANOSORT_NOINLINE T median5(It first, It last, Compare comp) {
  size_t n = last - first;

  T e0 = first[(n >> 2) * 0];
  T e1 = first[(n >> 2) * 1];
  T e2 = first[(n >> 2) * 2];
  T e3 = first[(n >> 2) * 3];
  T e4 = first[n - 1];

  if (comp(e1, e0)) swap(e1, e0);
  if (comp(e4, e3)) swap(e4, e3);
  if (comp(e3, e0)) swap(e3, e0);

  if (comp(e1, e4)) swap(e1, e4);
  if (comp(e2, e1)) swap(e2, e1);
  if (comp(e3, e2)) swap(e2, e3);

  if (comp(e2, e1)) swap(e2, e1);

  return e2;
}

// Split array into x<pivot and x>=pivot
template <typename T, typename It, typename Compare>
NANOSORT_NOINLINE It partition(T pivot, It first, It last, Compare comp) {
  It res = first;
  for (It it = first; it != last; ++it) {
    bool r = comp(*it, pivot);
    swap(*res, *it);
    res += r;
  }
  return res;
}

// Splits array into x<=pivot and x>pivot
template <typename T, typename It, typename Compare>
NANOSORT_NOINLINE It partition_rev(T pivot, It first, It last, Compare comp) {
  It res = first;
  for (It it = first; it != last; ++it) {
    bool r = comp(pivot, *it);
    swap(*res, *it);
    res += !r;
  }
  return res;
}

// TODO: do we need this?
template <typename T, typename It, typename Compare>
NANOSORT_NOINLINE void insertion_sort(It begin, It end, Compare comp) {
  if (begin == end) return;

  for (It it = begin + 1; it != end; ++it) {
    T val = *it;
    It hole = it;

    // move hole backwards
    while (hole > begin && comp(val, *(hole - 1))) {
      *hole = *(hole - 1);
      hole--;
    }

    // fill hole with element
    *hole = val;
  }
}

// BubbleSort works better it has N(N-1)/2 stores, but x is updated in the
// inner loop. This is cmp/cmov sequence making the inner loop 2 cycles.
// TODO: auto, moves
template <typename It, typename Compare>
void BubbleSort(It first, It last, Compare comp) {
  auto n = last - first;
  for (auto i = n; i > 1; i--) {
    auto x = first[0];
    for (decltype(n) j = 1; j < i; j++) {
      auto y = first[j];
      bool is_smaller = comp(y, x);
      first[j - 1] = is_smaller ? y : x;
      x = is_smaller ? x : y;
    }
    first[i - 1] = x;
  }
}

// BubbleSort2 bubbles two elements at a time. This means it's doing N(N+1)/4
// iterations and therefore much less stores. Correctly ordering the cmov's it
// is still possible to execute the inner loop in 2 cycles with respect to
// data dependencies. So in effect this cuts running time by 2x, even though
// it's not cutting number of comparisons.
// TODO: auto, moves
template <typename It, typename Compare>
void BubbleSort2(It first, It last, Compare comp) {
  auto n = last - first;
  for (auto i = n; i > 1; i -= 2) {
    auto x = first[0];
    auto y = first[1];
    if (comp(y, x)) swap(y, x);
    for (decltype(n) j = 2; j < i; j++) {
      auto z = first[j];
      bool is_smaller = comp(z, y);
      auto w = is_smaller ? z : y;
      y = is_smaller ? y : z;
      is_smaller = comp(z, x);
      first[j - 2] = is_smaller ? z : x;
      x = is_smaller ? x : w;
    }
    first[i - 2] = x;
    first[i - 1] = y;
  }
}

template <typename T, typename It, typename Compare>
NANOSORT_NOINLINE void sort(It first, It last, Compare comp) {
  const size_t kSmallSortThreshold = 16;

  // TODO: fall back to heap sort after a certain limit
  while (last - first > kSmallSortThreshold) {
    T pivot = median5<T>(first, last, comp);
    It mid = partition(pivot, first, last, comp);

    // For skewed partitions compute new midpoint by separating equal elements
    bool skew = mid - first <= (last - first) >> 3;
    It midr = skew ? partition_rev(pivot, mid, last, comp) : mid;

    // Recurse into smaller partition resulting in log2(N) recursion limit
    if (mid - first <= last - midr) {
      sort<T>(first, mid, comp);
      first = midr;
    } else {
      sort<T>(midr, last, comp);
      last = mid;
    }
  }

  // TODO: evaluate alternatives
  BubbleSort2(first, last, comp);
}

}  // namespace nanosort_detail

template <typename It, typename Compare>
NANOSORT_NOINLINE void nanosort(It first, It last, Compare comp) {
  typedef typename nanosort_detail::IteratorTraits<It>::value_type T;
  nanosort_detail::sort<T>(first, last, comp);
}

template <typename It>
NANOSORT_NOINLINE void nanosort(It first, It last) {
  typedef typename nanosort_detail::IteratorTraits<It>::value_type T;
  nanosort_detail::sort<T>(first, last, nanosort_detail::Less());
}
