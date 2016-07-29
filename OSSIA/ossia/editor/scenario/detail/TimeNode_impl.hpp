/*!
 * \file TimeNode_impl.h
 *
 * \brief
 *
 * \details
 *
 * \author Théo de la Hogue
 *
 * \copyright This code is licensed under the terms of the "CeCILL-C"
 * http://www.cecill.info
 */

#pragma once

#include <ossia/editor/scenario/time_constraint.hpp>
#include <ossia/editor/scenario/time_node.hpp>
#include <ossia/editor/scenario/time_value.hpp>

#include "TimeEvent_impl.hpp" // because the TimeNode::emplace method is a JamomaTimeEvent factory

using namespace OSSIA;


namespace impl
{
class JamomaTimeNode final : public TimeNode, public std::enable_shared_from_this<JamomaTimeNode>
{

private:

# pragma mark -
# pragma mark Implementation specific

  TimeNode::ExecutionCallback   mCallback;

  std::shared_ptr<Expression>        mExpression;
  bool                          mObserveExpression;
  bool                          mCallbackSet = false;
  Expression::iterator          mResultCallbackIndex;

  TimeValue                     mSimultaneityMargin;

  Container<TimeEvent>          mPendingEvents;

public:

# pragma mark -
# pragma mark Life cycle

  JamomaTimeNode(TimeNode::ExecutionCallback);

  ~JamomaTimeNode();

# pragma mark -
# pragma mark Execution

  void setCallback(TimeNode::ExecutionCallback) override;

  bool trigger() override;

# pragma mark -
# pragma mark Accessors

  TimeValue getDate() const override;

  const std::shared_ptr<Expression> & getExpression() const override;

  TimeNode & setExpression(const std::shared_ptr<Expression>) override;

  TimeValue getSimultaneityMargin() const override;

  TimeNode & setSimultaneityMargin(TimeValue) override;

# pragma mark -
# pragma mark TimeEvents

  iterator emplace(const_iterator,
                   TimeEvent::ExecutionCallback,
                   std::shared_ptr<Expression> = ExpressionTrue()) override;

# pragma mark -
# pragma mark Implementation specific

  /* process all TimeEvents to propagate execution */
  void process(Container<TimeEvent>& statusChangedEvents);

  /* is the TimeNode observing its Expression ? */
  bool isObservingExpression();

  /* enable observation of the Expression */
  void observeExpressionResult(bool);

private:

  void resultCallback(bool result);
};
}
