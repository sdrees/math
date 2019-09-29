#ifndef STAN_MATH_REV_MAT_FUNCTOR_FP_SOLVER_HPP
#define STAN_MATH_REV_MAT_FUNCTOR_FP_SOLVER_HPP

#include <stan/math/rev/meta.hpp>
#include <stan/math/rev/core.hpp>
#include <stan/math/prim/mat/fun/to_array_1d.hpp>
#include <stan/math/prim/mat/fun/to_vector.hpp>
#include <stan/math/prim/mat/fun/value_of.hpp>
#include <stan/math/rev/scal/fun/value_of.hpp>
#include <stan/math/prim/mat/fun/mdivide_left.hpp>
#include <stan/math/prim/scal/err/check_nonnegative.hpp>
#include <stan/math/rev/mat/functor/algebra_system.hpp>
#include <stan/math/rev/mat/functor/kinsol_data.hpp>
#include <stan/math/rev/mat/functor/algebra_system.hpp>
#include <stan/math/prim/mat/err/check_flag_sundials.hpp>

#include <kinsol/kinsol.h>
#include <sunmatrix/sunmatrix_dense.h>
#include <sunlinsol/sunlinsol_dense.h>
#include <nvector/nvector_serial.h>

#include <iostream>
#include <string>
#include <vector>

namespace stan {
namespace math {

  /**
   * KINSOL algebraic system data holder.
   *
   * @tparam F1 functor type for system function.
   * @tparam F2 functor type for jacobian function. Default is 0.
   *         If 0, use rev mode autodiff to compute the Jacobian.
   */
  template <typename F>
  struct KinsolFixedPointEnv {
    const F& f_;
    const Eigen::VectorXd y_dummy;
    const Eigen::VectorXd& y_;
    const size_t N_;
    const size_t M_;
    const std::vector<double>& x_r_;
    const std::vector<int>& x_i_;
    std::ostream* msgs_;

    void* mem_;
    N_Vector nv_x_;
    N_Vector nv_u_scal_;
    N_Vector nv_f_scal_;

    /* Constructor when @y is data */
    template<typename T>
    KinsolFixedPointEnv(const F& f, const Eigen::Matrix<T, -1, 1>& x,
                       const Eigen::VectorXd& y, const std::vector<double>& x_r,
                       const std::vector<int>& x_i, std::ostream* msgs,
                       const std::vector<double>& u_scale,
                       const std::vector<double>& f_scale)
      : f_(f),
        y_dummy(),
        y_(y),
        x_r_(x_r),
        x_i_(x_i),
        msgs_(msgs),
        N_(x.size()),
        M_(y.size()),
        mem_(KINCreate()),
        nv_x_(N_VNew_Serial(N_)),
        nv_u_scal_(N_VNew_Serial(N_)),
        nv_f_scal_(N_VNew_Serial(N_))
    {
      for (int i = 0; i < N_; ++i) {
        NV_Ith_S(nv_x_, i) = stan::math::value_of(x(i));
        NV_Ith_S(nv_u_scal_, i) = u_scale[i];
        NV_Ith_S(nv_f_scal_, i) = f_scale[i];
      }
    }

    /* Constructor when @y is param */
    template<typename T>
    KinsolFixedPointEnv(const F& f, const Eigen::Matrix<T, -1, 1>& x,
                        const Eigen::Matrix<stan::math::var, -1, 1>& y,
                        const std::vector<double>& x_r,
                        const std::vector<int>& x_i, std::ostream* msgs,
                        const std::vector<double>& u_scale,
                        const std::vector<double>& f_scale)
      : f_(f),
        y_dummy(stan::math::value_of(y)),
        y_(y_dummy),
        x_r_(x_r),
        x_i_(x_i),
        msgs_(msgs),
        N_(x.size()),
        M_(y.size()),
        mem_(KINCreate()),
        nv_x_(N_VNew_Serial(N_)),
        nv_u_scal_(N_VNew_Serial(N_)),
        nv_f_scal_(N_VNew_Serial(N_))
    {
      for (int i = 0; i < N_; ++i) {
        NV_Ith_S(nv_x_, i) = stan::math::value_of(x(i));
        NV_Ith_S(nv_u_scal_, i) = u_scale[i];
        NV_Ith_S(nv_f_scal_, i) = f_scale[i];
      }
    }

    ~KinsolFixedPointEnv() {
      N_VDestroy_Serial(nv_x_);
      N_VDestroy_Serial(nv_u_scal_);
      N_VDestroy_Serial(nv_f_scal_);
      KINFree(&mem_);
    }

    /* Implements the user-defined function passed to KINSOL. */
    static int kinsol_f_system(N_Vector x, N_Vector f, void* user_data) {
      auto g = static_cast<const KinsolFixedPointEnv<F>*>(user_data);
      Eigen::VectorXd x_eigen(Eigen::Map<Eigen::VectorXd>(NV_DATA_S(x), g->N_));
      Eigen::Map<Eigen::VectorXd>(N_VGetArrayPointer(f), g->N_)
        = g->f_(x_eigen, g->y_, g->x_r_, g->x_i_, g->msgs_);
      return 0;
    }
  };

  /*
   * Given the solution, we want to calculate jacobian wrt
   * the params. This functor does that using AD.
   */
  struct FixedPointADJac {

    /*
     * Calculate Jacobian Jxy.
     *
     * @tparam F RHS functor type
     * @param x fixed point solution
     * @param y RHS parameters
     * @param env KINSOL working envionment
     */
    template<typename F>
    inline Eigen::Matrix<stan::math::var, -1, 1>
    operator()(const Eigen::VectorXd& x,
               const Eigen::Matrix<stan::math::var, -1, 1>& y,
               KinsolFixedPointEnv<F>& env) {
      using stan::math::var;
      using stan::math::to_array_1d;
      using stan::math::precomputed_gradients;

      auto g = [&env](const Eigen::Matrix<var, -1, 1>& par_) {
        Eigen::Matrix<var, -1, 1> x_(par_.head(env.N_));
        Eigen::Matrix<var, -1, 1> y_(par_.tail(env.M_));
        return env.f_(x_, y_, env.x_r_, env.x_i_, env.msgs_);
      };

      Eigen::VectorXd theta(x.size() + y.size());
      for (int i = 0; i < env.N_; ++i) {
        theta(i) = x(i);
      }
      for (int i = 0; i < env.M_; ++i) {
        theta(i + env.N_) = env.y_(i);
      }
      Eigen::Matrix<double, -1, 1> fx;
      Eigen::Matrix<double, -1, -1> J;
      stan::math::jacobian(g, theta, fx, J);
      Eigen::MatrixXd A(J.block(0, 0, env.N_, env.N_));
      Eigen::MatrixXd b(J.block(0, env.N_, env.N_, env.M_));
      A = Eigen::MatrixXd::Identity(env.N_, env.N_) - A;
      Eigen::MatrixXd Jxy = A.colPivHouseholderQr().solve(b);
      std::vector<double> gradients(env.M_);
      Eigen::Matrix<var, -1, 1> x_sol(env.N_);
      std::vector<stan::math::var> yv(to_array_1d(y));
      for (int i = 0; i < env.N_; ++i) {
        gradients = to_array_1d(Eigen::VectorXd(Jxy.row(i)));
        x_sol[i] = precomputed_gradients(x(i), yv, gradients);
      }
      return x_sol;
    }
  };

  /*
   * Fixed point solver for problem of form
   * 
   * x = F(x; theta)
   *
   * with x as unkowns and theta parameters.
   *
   * The solution for FP iteration
   * doesn't involve derivatives but only data types.
   *
   * @tparam fp_env_type solver envionment setup
   * @tparam fp_jac_type functor type for calculating the jacobian
   */
  template<typename fp_env_type, typename fp_jac_type>
  struct FixedPointSolver;

  /*
   * Specialization for fixed point solver when using KINSOL.
   *
   * @tparam F RHS functor for fixed point iteration.
   * @tparam fp_jac_type functor type for calculating the jacobian
   */
  template<typename F, typename fp_jac_type>
  struct FixedPointSolver<KinsolFixedPointEnv<F>, fp_jac_type> {

    /*
     * Solve FP using KINSOL
     *
     * @param x initial point and final solution.
     * @param env KINSOL solution envionment
     * @param f_tol Function tolenrance
     * @param max_num_steps max nb. of iterations.
     */
    void kinsol_solve_fp(Eigen::VectorXd& x,
                         KinsolFixedPointEnv<F>& env, 
                         double f_tol,
                         long int max_num_steps) {
      int N = env.N_;
      void* mem = env.mem_;

      const int default_anderson_depth = 4;
      int anderson_depth = std::min(N, default_anderson_depth);

      check_flag_sundials(KINSetNumMaxIters(mem, max_num_steps),
                          "KINSetNumMaxIters");
      check_flag_sundials(KINSetMAA(mem, anderson_depth),
                          "KINSetMAA");
      check_flag_sundials(KINInit(mem, &env.kinsol_f_system, env.nv_x_), "KINInit");

      check_flag_sundials(KINSetFuncNormTol(mem, f_tol),
                          "KINSetFuncNormTol");

      check_flag_sundials(KINSetUserData(mem, static_cast<void*>(&env)),
                          "KINSetUserData");

      KINSol(mem, env.nv_x_, KIN_FP, env.nv_u_scal_, env.nv_f_scal_);

      long int nniter;
      double norm;
      KINGetNumNonlinSolvIters(mem, &nniter);
      KINGetFuncNorm(mem, &norm);

      for (int i = 0; i < N; ++i) {
        x(i) = NV_Ith_S(env.nv_x_, i);
      }
    }

    /*
     * Solve data-only FP problem so no need to calculate jacobian.
     * @tparam T1 type of init point of iterations
     *
     * @param x initial point and final solution.
     * @param y RHS functor parameters
     * @param env KINSOL solution envionment
     * @param f_tol Function tolenrance
     * @param max_num_steps max nb. of iterations.
     */
    template <typename T1>
    Eigen::Matrix<double, -1, 1>
    solve(const Eigen::Matrix<T1, -1, 1>& x,
          const Eigen::Matrix<double, -1, 1>& y,
          KinsolFixedPointEnv<F>& env,
          double f_tol,
          long int max_num_steps) {
      Eigen::VectorXd xd(stan::math::value_of(x));
      kinsol_solve_fp(xd, env, f_tol, max_num_steps);
      return xd;
    }

    /*
     * Solve FP problem and calculate jacobian.
     * @tparam T1 type of init point of iterations
     *
     * @param x initial point and final solution.
     * @param y RHS functor parameters
     * @param env KINSOL solution envionment
     * @param f_tol Function tolenrance
     * @param max_num_steps max nb. of iterations.
     */
    template <typename T1>
    Eigen::Matrix<stan::math::var, -1, 1>
    solve(const Eigen::Matrix<T1, -1, 1>& x,
          const Eigen::Matrix<stan::math::var, -1, 1>& y,
          KinsolFixedPointEnv<F>& env,
          double f_tol,
          long int max_num_steps) {
      using stan::math::value_of;
      using stan::math::var;

      // FP solution
      Eigen::VectorXd xd(value_of(x));
      kinsol_solve_fp(xd, env, f_tol, max_num_steps);

      fp_jac_type jac_sol;
      return jac_sol(xd, y, env);
    }
  };

  /**
   * Return the solution to the specified system of algebraic
   * equations given an initial guess, and parameters and data,
   * which get passed into the algebraic system. Use the
   * KINSOL solver from the SUNDIALS suite.
   *
   * The user can also specify the scaled step size, the function
   * tolerance, and the maximum number of steps.
   *
   * @tparam F type of equation system function.
   * @tparam T type of initial guess vector.
   * @param[in] f Functor that evaluated the system of equations.
   * @param[in] x Vector of starting values.
   * @param[in] y Parameter vector for the equation system. The function
   *            is overloaded to treat y as a vector of doubles or of a
   *            a template type T.
   * @param[in] dat Continuous data vector for the equation system.
   * @param[in] dat_int Integer data vector for the equation system.
   * @param[in, out] msgs The print stream for warning messages.
   * @param[in] scaling_step_size Scaled-step stopping tolerance. If
   *            a Newton step is smaller than the scaling step
   *            tolerance, the code breaks, assuming the solver is no
   *            longer making significant progress (i.e. is stuck)
   * @param[in] function_tolerance determines whether roots are acceptable.
   * @param[in] max_num_steps  maximum number of function evaluations.
   *  * @throw <code>std::invalid_argument</code> if x has size zero.
   * @throw <code>std::invalid_argument</code> if x has non-finite elements.
   * @throw <code>std::invalid_argument</code> if y has non-finite elements.
   * @throw <code>std::invalid_argument</code> if dat has non-finite elements.
   * @throw <code>std::invalid_argument</code> if dat_int has non-finite elements.
   * @throw <code>std::invalid_argument</code> if scaled_step_size is strictly
   * negative.
   * @throw <code>std::invalid_argument</code> if function_tolerance is strictly
   * negative.
   * @throw <code>std::invalid_argument</code> if max_num_steps is not positive.
   * @throw <code>boost::math::evaluation_error</code> (which is a subclass of
   * <code>std::runtime_error</code>) if solver exceeds max_num_steps.
   */
  template <typename F, typename T1, typename T2>
  Eigen::Matrix<T2, -1, 1>
  algebra_solver_fp(const F& f,
                    const Eigen::Matrix<T1, -1, 1>& x,
                    const Eigen::Matrix<T2, -1, 1>& y,
                    const std::vector<double>& x_r,
                    const std::vector<int>& x_i,
                    const std::vector<double>& u_scale,
                    const std::vector<double>& f_scale,
                    std::ostream* msgs = nullptr,
                    double f_tol = 1e-6,
                    long int max_num_steps = 200) {  // NOLINT(runtime/int)
    algebra_solver_check(x, y, x_r, x_i, f_tol, max_num_steps);
    check_nonnegative("algebra_solver", "u_scale", u_scale);
    check_nonnegative("algebra_solver", "f_scale", f_scale);
    check_matching_sizes("algebra_solver", "the algebraic system's output",
                         value_of(f(x, y, x_r, x_i, msgs)),
                         "the vector of unknowns, x,", x);

    KinsolFixedPointEnv<F> env(f, x, y, x_r, x_i, msgs, u_scale, f_scale); // NOLINT
    FixedPointSolver<KinsolFixedPointEnv<F>, FixedPointADJac> fp;
    return fp.solve(x, y, env, f_tol, max_num_steps);
  }

}
}

#endif
