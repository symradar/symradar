#include "klee/util/ArrayCache.h"
#include "spdlog/spdlog.h"

namespace klee {

ArrayCache::~ArrayCache() {
  // Free Allocated Array objects
  for (ArrayHashMap::iterator ai = cachedSymbolicArrays.begin(),
                              e = cachedSymbolicArrays.end();
       ai != e; ++ai) {
    delete *ai;
  }
  for (ArrayPtrVec::iterator ai = concreteArrays.begin(),
                             e = concreteArrays.end();
       ai != e; ++ai) {
    delete *ai;
  }
}

const Array *
ArrayCache::CreateArray(const std::string &_name, uint64_t _size,
                        const ref<ConstantExpr> *constantValuesBegin,
                        const ref<ConstantExpr> *constantValuesEnd,
                        Expr::Width _domain, Expr::Width _range) {
  SPDLOG_DEBUG("[ArrayCache] Creating array: {} size: {} domain: {} range: {}",
               _name, _size, _domain, _range);
  const Array *array = new Array(_name, _size, constantValuesBegin,
                                 constantValuesEnd, _domain, _range);
  if (array->isSymbolicArray()) {
    std::pair<ArrayHashMap::const_iterator, bool> success =
        cachedSymbolicArrays.insert(array);
    if (success.second) {
      // Cache miss
      return array;
    }
    // Cache hit
    SPDLOG_DEBUG("[ArrayCache] Found cached symbolic array: {}",
                 (*success.first)->getName());
    delete array;
    array = *(success.first);
    assert(array->isSymbolicArray() &&
           "Cached symbolic array is no longer symbolic");
    return array;
  } else {
    // Treat every constant array as distinct so we never cache them
    assert(array->isConstantArray());
    concreteArrays.push_back(array); // For deletion later
    return array;
  }
}
} // namespace klee
