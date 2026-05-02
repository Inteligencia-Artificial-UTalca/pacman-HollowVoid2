/*
 * DTGhostController.cpp
 *
 *  Created on: May 1, 2026
 */

#include "DTGhostController.h"
#include <limits>

static std::vector<Move> getAvailableGhostMoves(const GameState& gs, std::shared_ptr<Character> character) {
    if (character->getDirection() == PASS) {
        return gs.getMaze().getPossibleMoves(character->getPos());
    }
    return gs.getMaze().getGhostLegalMoves(character->getPos(), character->getDirection());
}

static Move chooseMoveByDistance(const GameState& gs, std::shared_ptr<Character> character, bool flee) {
    const auto pacmanPos = gs.getMaze().getNodePos(gs.getPacmanPos());
    auto moves = getAvailableGhostMoves(gs, character);
    if (moves.empty()) {
        return PASS;
    }

    Move bestMove = PASS;
    float bestDist = flee ? -1.0f : std::numeric_limits<float>::infinity();
    for (auto move : moves) {
        if (move == PASS) {
            continue;
        }
        int neighbour = gs.getMaze().getNeighbour(character->getPos(), move);
        if (neighbour < 0) {
            continue;
        }
        const auto neighbourPos = gs.getMaze().getNodePos(neighbour);
        float dist = euclid2(pacmanPos, neighbourPos);
        if ((flee && dist > bestDist) || (!flee && dist < bestDist)) {
            bestDist = dist;
            bestMove = move;
        }
    }
    return bestMove;
}

DTGhostController::DTGhostController(std::shared_ptr<Character> character):Controller(character) {
    root = std::make_shared<IsEdibleCondition>(
        std::make_shared<FleeAction>(),
        std::make_shared<ChaseAction>()
    );
}

DTGhostController::~DTGhostController() {
}

Move DTGhostController::getMove(const GameState& gs) {
    return root->decide(gs, character);
}

ConditionNode::ConditionNode(std::shared_ptr<DecisionNode> trueBranch, std::shared_ptr<DecisionNode> falseBranch):
    trueBranch(trueBranch), falseBranch(falseBranch) {
}

Move ConditionNode::decide(const GameState& gs, std::shared_ptr<Character> character) {
    if (evaluate(gs, character)) {
        return trueBranch->decide(gs, character);
    }
    return falseBranch->decide(gs, character);
}

IsEdibleCondition::IsEdibleCondition(std::shared_ptr<DecisionNode> trueBranch, std::shared_ptr<DecisionNode> falseBranch):
    ConditionNode(trueBranch, falseBranch) {
}

bool IsEdibleCondition::evaluate(const GameState& gs, std::shared_ptr<Character> character) const {
    auto ghost = dynamic_cast<Ghost*>(character.get());
    return ghost != nullptr && ghost->isEdible();
}

Move ChaseAction::decide(const GameState& gs, std::shared_ptr<Character> character) {
    return chooseMoveByDistance(gs, character, false);
}

Move FleeAction::decide(const GameState& gs, std::shared_ptr<Character> character) {
    return chooseMoveByDistance(gs, character, true);
}
