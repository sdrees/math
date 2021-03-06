#include <stan/math/fwd/scal.hpp>
#include <stan/math/prim/scal.hpp>
#include <test/unit/math/require_util.hpp>
#include <gtest/gtest.h>
#include <type_traits>
#include <string>

TEST(requires, fvar_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_fvar_t, fvar<double>>::unary();
}
TEST(requires, fvar_not_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_not_fvar_t, fvar<double>>::not_unary();
}
TEST(requires, fvar_all_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_all_fvar_t, fvar<double>, fvar<double>,
                       fvar<double>>::all();
}
TEST(requires, fvar_all_not_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_all_not_fvar_t, fvar<double>,
                       fvar<double>>::all_not();
}
TEST(requires, fvar_any_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_any_fvar_t, fvar<double>,
                       fvar<double>>::any();
}
TEST(requires, fvar_any_not_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_any_not_fvar_t, fvar<double>,
                       fvar<double>>::any_not();
}

TEST(requires, var_or_fvar_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_var_or_fvar_t, fvar<double>,
                       fvar<double>>::unary();
}
TEST(requires, var_or_fvar_not_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_not_var_or_fvar_t, fvar<double>,
                       fvar<double>>::not_unary();
}
TEST(requires, var_or_fvar_all_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_all_var_or_fvar_t, fvar<double>,
                       fvar<double>>::all();
}
TEST(requires, var_or_fvar_all_not_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_all_not_var_or_fvar_t, fvar<double>,
                       fvar<double>>::all_not();
}
TEST(requires, var_or_fvar_any_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_any_var_or_fvar_t, fvar<double>,
                       fvar<double>>::any();
}
TEST(requires, var_or_fvar_any_not_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_any_not_var_or_fvar_t, fvar<double>,
                       fvar<double>>::any_not();
}

TEST(requires, stan_scalar_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_stan_scalar_t, fvar<double>,
                       fvar<double>>::unary();
}
TEST(requires, stan_scalar_not_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_not_stan_scalar_t, fvar<double>,
                       fvar<double>>::not_unary();
}
TEST(requires, stan_scalar_all_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_all_stan_scalar_t, fvar<double>,
                       fvar<double>>::all();
}
TEST(requires, stan_scalar_all_not_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_all_not_stan_scalar_t, fvar<double>,
                       fvar<double>>::all_not();
}
TEST(requires, stan_scalar_any_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_any_stan_scalar_t, fvar<double>,
                       fvar<double>>::any();
}
TEST(requires, stan_scalar_any_not_test) {
  using stan::math::fvar;
  using stan::test::require_scal_checker;
  require_scal_checker<stan::require_any_not_stan_scalar_t, fvar<double>,
                       fvar<double>>::any_not();
}
