#include <OpenSoT/utils/VelocityAllocation.h>
#include <OpenSoT/constraints/velocity/VelocityLimits.h>

using namespace OpenSoT;

VelocityAllocation::VelocityAllocation(OpenSoT::AutoStack::Ptr autoStack,
                                       const double dT,
                                       const double min_velocity,
                                       const double max_velocity) :
    _dT(dT),
    _min_velocity(min_velocity),
    _max_velocity(max_velocity)
{
    this->processStack(autoStack->getStack());

    typedef std::list<OpenSoT::constraints::Aggregated::ConstraintPtr>::const_iterator it_c;
    for(it_c constraint = autoStack->getBoundsList().begin();
        constraint != autoStack->getBoundsList().end();
        ++constraint)
    {
        if(boost::dynamic_pointer_cast<
            OpenSoT::constraints::velocity::VelocityLimits>(
                *constraint))
        {
            OpenSoT::constraints::velocity::VelocityLimits::Ptr velocityLimits =
                boost::dynamic_pointer_cast<
                    OpenSoT::constraints::velocity::VelocityLimits>(
                        *constraint);
            assert(_dT == velocityLimits->getDT());
            velocityLimits->setVelocityLimits(_max_velocity);
        }
    }
}

VelocityAllocation::VelocityAllocation(OpenSoT::Solver<yarp::sig::Matrix, yarp::sig::Vector>::Stack& stack,
                                       const double dT,
                                       const double min_velocity,
                                       const double max_velocity) :
    _dT(dT),
    _min_velocity(min_velocity),
    _max_velocity(max_velocity)
{
    this->processStack(stack);
}

void VelocityAllocation::processStack(OpenSoT::Solver<yarp::sig::Matrix, yarp::sig::Vector>::Stack& stack)
{
    for(unsigned int i = 0; i < stack.size(); ++i)
    {
        const double velocityLimit = this->computeVelocityLimit(i, stack.size());
        OpenSoT::Task<yarp::sig::Matrix, yarp::sig::Vector>::TaskPtr task = stack[i];
        OpenSoT::constraints::velocity::VelocityLimits::Ptr velocityLimits;

        // TODO notice this does not work if every stack has already a VelocityLimits
        // but they are instances of the same object - we should check duplication
        typedef std::list<OpenSoT::constraints::Aggregated::ConstraintPtr>::const_iterator it_c;
        for(it_c constraint = task->getConstraints().begin();
            constraint != task->getConstraints().end();
            ++constraint)
            if(boost::dynamic_pointer_cast<
                    OpenSoT::constraints::velocity::VelocityLimits>(
                        *constraint))
            {
                velocityLimits = boost::dynamic_pointer_cast<
                        OpenSoT::constraints::velocity::VelocityLimits>(
                            *constraint);
                assert(_dT == velocityLimits->getDT());
                velocityLimits->setVelocityLimits(velocityLimit);
            }

        if(!velocityLimits)
        {
            velocityLimits = OpenSoT::constraints::velocity::VelocityLimits::Ptr(
                        new OpenSoT::constraints::velocity::VelocityLimits(velocityLimit,
                                                                           _dT,
                                                                           task->getXSize()));
            task->getConstraints().push_back(velocityLimits);
        }
    }
}

double VelocityAllocation::computeVelocityLimit(const unsigned int taskIndex,
                                                const unsigned int stackSize)
{
    return _min_velocity+taskIndex*(_max_velocity-_min_velocity)/(stackSize-1);
}
