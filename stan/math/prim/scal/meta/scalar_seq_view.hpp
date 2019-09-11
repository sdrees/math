#ifndef STAN_MATH_PRIM_SCAL_META_SCALAR_SEQ_VIEW_HPP
#define STAN_MATH_PRIM_SCAL_META_SCALAR_SEQ_VIEW_HPP

#include <stan/math/prim/scal/meta/require_generics.hpp>
#include <stan/math/prim/scal/meta/scalar_type.hpp>
#include <stan/math/prim/scal/meta/is_vector_like.hpp>
#include <type_traits>
#include <utility>

namespace stan {
/**
 * scalar_seq_view provides a uniform sequence-like wrapper around either a
 * scalar or a sequence of scalars.
 *
 * @tparam C the container type; will be the scalar type if wrapping a scalar
 * @tparam T the scalar type
 */
template <typename C, typename = void>
class scalar_seq_view;

template <typename C>
class scalar_seq_view<C, require_vector_like_t<is_stan_scalar, C>> {
 public:
  template <typename CC, require_same<C, std::decay_t<CC>>...>
  explicit scalar_seq_view(CC&& c) : c_(std::forward<CC>(c)) {}

  /**
   * Segfaults if out of bounds.
   * @param i index
   * @return the element at the specified position in the container
   */
  auto&& operator[](int i) const { return c_[i]; }
  auto&& operator[](int i) { return c_[i]; }

  int size() const { return c_.size(); }

 private:
  C c_;
};

/**
 * This specialization handles wrapping a scalar as if it were a sequence.
 *
 * @tparam T the scalar type
 */
template <typename C>
class scalar_seq_view<C, require_not_vector_like_t<is_stan_scalar, C>> {
 public:
  template <typename CC, require_same<C, std::decay_t<CC>>...>
  explicit scalar_seq_view(CC&& c) : t_(std::forward<CC>(c)) {}

  auto&& operator[](int /* i */) const { return t_; }
  auto&& operator[](int /* i */) { return t_; }

  int size() const { return 1; }

 private:
  C t_;
};
}  // namespace stan
#endif
