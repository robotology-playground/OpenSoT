/*
 * Copyright (C) 2014 Walkman
 * Author: Arturo Laurenzi
 * email: arturo.laurenzi@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU Lesser General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

#ifndef __SOT_SOLVERS_NULLSPACE_HQP_H__
#define __SOT_SOLVERS_NULLSPACE_HQP_H__

#include <vector>
#include <iostream>

#include <memory>


#include <OpenSoT/Task.h>
#include <OpenSoT/Solver.h>
#include <OpenSoT/constraints/Aggregated.h>
#include <OpenSoT/solvers/BackEndFactory.h>
#include <OpenSoT/utils/Piler.h>

#include <eigen3/Eigen/SVD>


namespace OpenSoT { namespace solvers {
/**
     * @brief Hierarchical QP solver based on nullspace basis optimization.
     *
     * Layer #0: min |A0*x-b0| -> let N0 = Null(A0) be a basis of the nullspace of A0, and x0_opt = sol0 the solution
     *
     * Layer #1: min |A1*x-b1|. Optimize on the nullspace coordinates (x1) by setting: x = sol0 + N0*x1.
     *   This results in A1 -> A1*N0,  b1 -> b1 - A1*sol0.
     *   Then, let N1 be N0*Null(A1*N0), sol1 = sol0 + N0*x1_opt
     *
     * Layer #K: min |AK*x-bK| -> x = solK-1 + NK-1*xK
     *   AK -> AK*NK-1, bK -> bK - AK*solK-1
     *   solK = solK-1 + NK-1*xK_opt
     *   NK = NK-1 * Null(AK*NK-1)
     *
     * Notice how each layer optimizes only over the remaining dofs after higher priority tasks
     * have been optimized. Hence, the size of QP probles decreases along the hierarchy.
     *
     * Limitations:
     *  - no support for equality constraints
     *  - no support for local constraints
     *  - [!!!] ranks of tasks should not change during runtime (e.g. disabling a task)
     *
     * @todo: implement equality constraints by considering the nullspace Neq = Null(Aeq).
     * All optimizations will then take place in the null space of equality constraints.
     * @todo: add reference
     */
    class nHQP: public Solver<Eigen::MatrixXd, Eigen::VectorXd>
    {

    public:

        static constexpr double DEFAULT_MIN_SV_RATIO = 0.05;

        // Shared pointer typedef
        typedef std::shared_ptr<nHQP> Ptr;

        /**
         * @brief nHQP constructor
         * @param stack_of_tasks vector of tasks
         * @param bounds and constraints of the problem
         * @param eps_regularisation resularization for the back-end
         * @param be_solver to solve the QP
         */
        nHQP(Stack& stack_of_tasks,
             ConstraintPtr bounds,
             const double eps_regularisation,
             const solver_back_ends be_solver = solver_back_ends::qpOASES);

        /**
         * @brief ~nHQP destructor
         */
        virtual ~nHQP() override;

        /**
         * @brief solve implementation
         * @param solution
         * @return false if something went wrong
         */
        virtual bool solve(Eigen::VectorXd& solution) override;

        /**
         * @brief setMinSingularValueRatio for the A and b regularization for all priority levels
         * @param sv_min between 0. and 1.
         */
        void setMinSingularValueRatio(double sv_min);

        /**
         * @brief setMinSingularValueRatio for the A and b regularization for a single priority levels
         * @param sv_min between 0. and 1.
         */
        void setMinSingularValueRatio(std::vector<double> sv_min);

        /**
         * @brief setPerformAbRegularization at a certain hierarchy level
         * @param hierarchy_level
         * @param perform_A_b_regularization true or false (default is true for all levels)
         * @note throw if asked hierarchy that does not exists
         */
        void setPerformAbRegularization(int hierarchy_level, bool perform_A_b_regularization);

        /**
         * @brief setPerformAbRegularization to all leveles
         * @param perform_A_b_regularization true or false (default is true for all levels)
         */
        void setPerformAbRegularization(bool perform_A_b_regularization);

        /**
         * @brief setPerformSelectiveNullSpaceRegularization at a certain hierarchy level
         * @param hierarchy_level
         * @param perform_selective_null_space_regularization true or false (default is true for all levels)
         * @note throw if asked hierarchy that does not exists
         */
        void setPerformSelectiveNullSpaceRegularization(int hierarchy_level, bool perform_selective_null_space_regularization);

        /**
         * @brief setPerformSelectiveNullSpaceRegularization to all leveles
         * @param perform_selective_null_space_regularization true or false (default is true for all levels)
         */
        void setPerformSelectiveNullSpaceRegularization(bool perform_selective_null_space_regularization);

    private:

        /**
         * @brief The TaskData class is a helper class containing all that is needed to setup,
         * regularize, and solve the i-th QP problem.
         */
        class TaskData
        {

        public:

            TaskData(int num_free_vars,
                     TaskPtr task,
                     ConstraintPtr constraint,
                     BackEnd::Ptr back_end);

            void set_min_sv_ratio(double sv);

            int compute_nullspace_dimension(double threshold);

            void set_nullspace_dimension(int ns_dim);

            void compute_cost(const Eigen::MatrixXd * AN_nullspace,
                              const Eigen::VectorXd& q0);

            void compute_contraints(const Eigen::MatrixXd * AN_nullspace,
                                    const Eigen::VectorXd& q0);

            bool update_and_solve();

            bool compute_nullspace();

            const Eigen::MatrixXd& get_nullspace() const;

            const Eigen::VectorXd& get_solution() const;

            bool enable_logger(XBot::MatLogger2::Ptr logger, std::string log_prefix);

            /**
             * @brief skip_A_b_regularization skip the regularization of the A matrix and b vector based on the
             * method void regularize_A_b(double threshold).
             * This is particularly suggested when a single layer is used together with a joint-space task. Regularization in this
             * case is not needed and usually perform badly.

             * @param perform_A_b_regularization true or false, default true
             */
            void set_perform_A_b_regularization(bool perform_A_b_regularization_);

            /**
             * @brief set_perform_selective_null_space_regularization regularization of the Hessian matrix using max singular value and null_space
             * @param perform_selective_null_space_regularization_ true or false, default true
             */
            void set_perform_selective_null_space_regularization(bool perform_selective_null_space_regularization_);

        private:


            // this task
            TaskPtr task;

            // global constraints
            ConstraintPtr constraints;

            // nullspace of AN (used by next task)
            Eigen::MatrixXd AN_nullspace;

            // A matrix of this task (A projected onto previous tasks nullspace)
            Eigen::MatrixXd AN;

            // b vector for this task
            Eigen::VectorXd b0;

            // min singular value ratio
            double min_sv_ratio;

            // quadratic cost matrices
            Eigen::MatrixXd H;
            Eigen::VectorXd g;

            // inequality constraints (including bounds for i = 1, 2, ...)
            utils::MatrixPiler Aineq;

            // inequality bounds
            utils::MatrixPiler lb, ub;

            // simple bounds (only i = 0)
            Eigen::VectorXd lb_bound, ub_bound;

            // svd computation class
            Eigen::BDCSVD<Eigen::MatrixXd> svd;

            // nullspace dimension (for next task)
            int ns_dim;

            // backend for solving the QP
            BackEnd::Ptr back_end;

            // flag indicating if back end was initialized
            bool back_end_initialized;

            // logger (can be nullptr)
            XBot::MatLogger2::Ptr logger;

            // prefix for logged variables
            std::string log_prefix;

            // if true the regularize_A_b(double threshold) is called in compute_cost()
            bool perform_A_b_regularization;

            // if true the selective regularization is performed in compute_cost()
            bool perform_selective_null_space_regularization;

            /**
             * @brief Perform SVD-based regularization of AN and b0 as follows:
             *  - components of b0 along A's singular vectors with low singular value are deflated
             *  - singular values of A under the threshold are inflated
             *
             * @param threshold
             */
            void regularize_A_b(double threshold);

        };


        virtual void _log(XBot::MatLogger2::Ptr logger, const std::string& prefix) override;

        // vector of previous task nullspaces (first elem is nx-by-nx identity)
        std::vector<Eigen::MatrixXd> _cumulated_nullspace;

        // task data for all layers
        std::vector<TaskData> _data_struct;

        // to store solution
        Eigen::VectorXd _solution;


    };


} }

#endif






