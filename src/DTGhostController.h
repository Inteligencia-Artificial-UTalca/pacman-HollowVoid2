/*
 * DTGhostController.h
 *
 *  Created on: May 1, 2026
 */

#ifndef DTGHOSTCONTROLLER_H_
#define DTGHOSTCONTROLLER_H_

#include "Controller.h"
#include "Ghost.h"
#include <memory>
#include <vector>

class DecisionNode {
public:
    virtual ~DecisionNode() = default;
    virtual Move decide(const GameState& gs, std::shared_ptr<Character> character) = 0;
};

class ActionNode : public DecisionNode {
public:
    virtual Move decide(const GameState& gs, std::shared_ptr<Character> character) override = 0;
};

class ConditionNode : public DecisionNode {
public:
    ConditionNode(std::shared_ptr<DecisionNode> trueBranch, std::shared_ptr<DecisionNode> falseBranch);
    virtual ~ConditionNode() = default;
    virtual Move decide(const GameState& gs, std::shared_ptr<Character> character) override;
protected:
    virtual bool evaluate(const GameState& gs, std::shared_ptr<Character> character) const = 0;
private:
    std::shared_ptr<DecisionNode> trueBranch;
    std::shared_ptr<DecisionNode> falseBranch;
};

class IsEdibleCondition : public ConditionNode {
public:
    IsEdibleCondition(std::shared_ptr<DecisionNode> trueBranch, std::shared_ptr<DecisionNode> falseBranch);
protected:
    virtual bool evaluate(const GameState& gs, std::shared_ptr<Character> character) const override;
};

class ChaseAction : public ActionNode {
public:
    virtual Move decide(const GameState& gs, std::shared_ptr<Character> character) override;
};

class FleeAction : public ActionNode {
public:
    virtual Move decide(const GameState& gs, std::shared_ptr<Character> character) override;
};

class DTGhostController: public Controller {
private:
    std::shared_ptr<DecisionNode> root;
public:
    DTGhostController(std::shared_ptr<Character> character);
    virtual ~DTGhostController();
    virtual Move getMove(const GameState& gs) override;
};

#endif /* DTGHOSTCONTROLLER_H_ */
