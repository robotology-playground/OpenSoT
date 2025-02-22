/*
 * Copyright (C) 2014 Walkman
 * Author: Alessio Rocchi
 * email:  alessio.rocchi@iit.it
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

#include <OpenSoT/tasks/Aggregated.h>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <assert.h>

using namespace OpenSoT::tasks;

const std::string Aggregated::_TASK_PLUS_ = "+";
std::string Aggregated::concatenatedId = "";

Aggregated::Aggregated(const std::list<TaskPtr> tasks,
                       const unsigned int x_size) :
    Task(concatenateTaskIds(tasks),x_size), _tasks(tasks)
{
    assert(tasks.size()>0);

    this->checkSizes();
    /* calling update to generate bounds */
    this->generateAll();

    _W.resize(_A.rows(),_A.rows());
    _W.setIdentity(_A.rows(),_A.rows());

    generateWeight();

    _hessianType = this->computeHessianType();
}

Aggregated::Aggregated(TaskPtr task1,
                       TaskPtr task2,
                       const unsigned int x_size) :
Task(task1->getTaskID()+_TASK_PLUS_+task2->getTaskID(),x_size)
{
    _tasks.push_back(task1);
    _tasks.push_back(task2);

    this->checkSizes();

    /* calling update to generate bounds */
    this->generateAll();

    _W.resize(_A.rows(),_A.rows());
    _W.setIdentity(_A.rows(),_A.rows());

    generateWeight();

    _hessianType = this->computeHessianType();
}

Aggregated::Aggregated(TaskPtr task,
                       const unsigned int x_size) :
Task(task->getTaskID(),x_size)
{
    _tasks.push_back(task);

    this->checkSizes();

    /* calling update to generate bounds */
    this->generateAll();

    _W.resize(_A.rows(),_A.rows());
    _W.setIdentity(_A.rows(),_A.rows());

    generateWeight();

    _hessianType = this->computeHessianType();
}


Aggregated::~Aggregated()
{
}

void Aggregated::_update() {
    for(std::list< TaskPtr >::iterator i = _tasks.begin();
        i != _tasks.end(); ++i) {
        TaskPtr t = *i;
        t->update();
    }
    this->generateAll();

    generateWeight();
}


void Aggregated::checkSizes() {
    for(std::list< TaskPtr >::iterator i = _tasks.begin();
        i != _tasks.end(); ++i) {
        TaskPtr t = *i;
        assert(this->getXSize() == t->getXSize());
    }
}


void Aggregated::generateAll() {
    _tmpA.reset(_x_size);
    _tmpb.reset(1);
    _c.setZero(_x_size);

    for(std::list< TaskPtr >::iterator i = _tasks.begin();
        i != _tasks.end(); ++i) {
        TaskPtr t = *i;
//        _tmpA.pile(t->getWA());
//        _tmpb.pile(t->getWb());
        _tmpA.pile(t->getA());
        _tmpb.pile(t->getb());
        _c += t->getc();
    }

    _A = _tmpA.generate_and_get();
    _b = _tmpb.generate_and_get();

    generateConstraints();
}

void OpenSoT::tasks::Aggregated::generateConstraints()
{
    int constraintsSize = this->_constraints.size();
    int expectedConstraintsSize = this->_aggregatedConstraints.size() + this->_ownConstraints.size();
    if(constraintsSize >= expectedConstraintsSize)
    {
        if(constraintsSize > expectedConstraintsSize) // checking whether the user really added only constraints
        {
            std::list < ConstraintPtr > orderedConstraints = this->_constraints;
            orderedConstraints.sort();
            this->_aggregatedConstraints.sort();

            std::vector< ConstraintPtr > diffs;
            diffs.resize( constraintsSize + expectedConstraintsSize );

            std::vector< ConstraintPtr >::iterator diffs_end = std::set_symmetric_difference( orderedConstraints.begin(),
                                                                                              orderedConstraints.end(),
                                                                                              _aggregatedConstraints.begin(),
                                                                                              _aggregatedConstraints.end(),
                                                                                              diffs.begin() );
            diffs.resize(diffs_end - diffs.begin());

            // checking the differences between the two lists are only additions
            if(diffs.size() != (constraintsSize - expectedConstraintsSize))
                throw std::runtime_error("ERROR. The constraints list has grown since last update, "
                                         "but the changes are not coherent.\n"
                                         "You probably tried deleting some constraints, or you added "
                                         "to the constraints something which was already in the tasks "
                                         "constraints.\n");
            else // saving the added constraints to the ownConstraints list, in the original order
            {
                for (std::list<ConstraintPtr>::iterator j = _constraints.begin(); j!=_constraints.end(); ++j)
                    for( std::vector< ConstraintPtr >::iterator i = diffs.begin() ; i != diffs.end() ; ++i)
                        if(*i==*j)
                            _ownConstraints.push_back(*i);
            }
        }
        else
        {
            // notice how there might be a case where constraintsSize == expectedConstraintsSize,
            // but the list has been modified. These changes are discarded without notifying the user
            // for efficiency reasons. This is unforgiving and a bad design decision.
            // TODO We should therefore change this in the future.
        }

    } else throw std::runtime_error("ERROR. You can only add constraints through getConstraints, "
                                    "not remove them. Look at the documentation for more details.\n");

    this->generateAggregatedConstraints();

    this->_constraints.clear();
    _constraints.insert(_constraints.end(), _aggregatedConstraints.begin(), _aggregatedConstraints.end());
    _constraints.insert(_constraints.end(), _ownConstraints.begin(), _ownConstraints.end());

}

void OpenSoT::tasks::Aggregated::generateAggregatedConstraints()
{
    _aggregatedConstraints.clear();
    for(std::list< TaskPtr >::iterator i = _tasks.begin();
        i != _tasks.end(); ++i) {
        TaskPtr t = *i;
        _aggregatedConstraints.insert(_aggregatedConstraints.end(),
                                      t->getConstraints().begin(),
                                      t->getConstraints().end());
    }
}

OpenSoT::HessianType OpenSoT::tasks::Aggregated::computeHessianType()
{
    if(_tasks.size() == 1)
        return (*_tasks.begin())->getHessianAtype();

    // we make a guess: all tasks are HST_POSDEF
    bool allZero = true;
    for(std::list< TaskPtr >::iterator i = _tasks.begin();
        i != _tasks.end(); ++i) {
        TaskPtr t = *i;
        // if at least a task has HST_UNKNOWN, propagate that
        if(t->getHessianAtype() == HST_UNKNOWN) return HST_UNKNOWN;
        if(t->getHessianAtype() != HST_ZERO) allZero = false;
        if(t->getHessianAtype() == HST_POSDEF) return HST_POSDEF;
    }

    if(allZero)
        return HST_ZERO;

    // we assume an hessian type HST_SEMIDEF
    return HST_SEMIDEF;
}

const std::string Aggregated::concatenateTaskIds(const std::list<TaskPtr> tasks) {
    concatenatedId = "";
    int taskSize = tasks.size();
    for(std::list<TaskPtr>::const_iterator i = tasks.begin(); i != tasks.end(); ++i) {
        concatenatedId += (*i)->getTaskID();
        if(--taskSize > 0)
            concatenatedId += _TASK_PLUS_;
    }

    return concatenatedId;
}

void Aggregated::setLambda(double lambda)
{
    if(lambda >= 0.0)
    {
        _lambda = lambda;
        for(std::list<TaskPtr>::const_iterator i = _tasks.begin(); i != _tasks.end(); ++i)
        {
            TaskPtr t = *i;
            t->setLambda(_lambda);
        }
    }
}

bool OpenSoT::tasks::Aggregated::isAggregated(OpenSoT::Task<Eigen::MatrixXd, Eigen::VectorXd>::TaskPtr task)
{
    return (bool)std::dynamic_pointer_cast<OpenSoT::tasks::Aggregated>(task);
}

void OpenSoT::tasks::Aggregated::log(XBot::MatLogger2::Ptr logger)
{
#ifdef OPENSOT_VERBOSE_MATLOG
    Task::log(logger);
#else
    for(auto task : _tasks)
        task->log(logger);
#endif
}

void OpenSoT::tasks::Aggregated::generateWeight()
{
        //this->_W.setZero(_W.rows(), _W.cols());

        int block = 0;
        std::list< TaskPtr >::iterator t;
        for(t = _tasks.begin(); t != _tasks.end(); t++)
        {
            for(unsigned int r = 0; r < (*t)->getWeight().rows(); ++r)
                for(unsigned int c = 0; c < (*t)->getWeight().cols(); ++c)
                    this->_W.block(block, block, (*t)->getWeight().rows(), (*t)->getWeight().cols())(r,c) =
                        (*t)->getWeight()(r,c);
            block += (*t)->getWeight().rows();
        }
}

void OpenSoT::tasks::Aggregated::setWeight(const Eigen::MatrixXd &W)
{
    assert(W.rows() == this->getTaskSize());
    assert(W.cols() == W.rows());

    this->_W = W;

    std::list< TaskPtr >::iterator t;
    int block = 0;
    for(t = _tasks.begin(); t != _tasks.end(); t++)
    {
        (*t)->setWeight(W.block(block, block, (*t)->getWeight().rows(),(*t)->getWeight().cols()));
        block += (*t)->getWeight().rows();
    }
}
