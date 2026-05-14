#include "BTMsPacmanController.h"
#include "Ghost.h"
#include <SDL2/SDL.h>
#include <limits>
#include <vector>

class BTMsPacmanInfo {
    static BTMsPacmanInfo *info;
    BTMsPacmanInfo() {}
public:
    static BTMsPacmanInfo* getInfo() {
        if(info == nullptr) info = new BTMsPacmanInfo();
        return info;
    }
    const GameState* in_gamestate;
    std::shared_ptr<Character> in_character;
    Move out_move;
};

BTMsPacmanInfo* BTMsPacmanInfo::info = nullptr;

static std::vector<Move> getPacmanMoves(const GameState* gs, std::shared_ptr<Character> character) {
    return gs->getMaze().getPossibleMoves(character->getPos());
}

static Move getClosestMove(const GameState* gs, std::shared_ptr<Character> character, const std::pair<int,int>& target) {
    auto moves = getPacmanMoves(gs, character);
    float bestDist = std::numeric_limits<float>::infinity();
    Move bestMove = PASS;
    for(auto move : moves) {
        if(move == PASS) continue;
        int neighbour = gs->getMaze().getNeighbour(character->getPos(), move);
        if(neighbour < 0) continue;
        auto neighbourPos = gs->getMaze().getNodePos(neighbour);
        float d = euclid2(neighbourPos, target);
        if(d < bestDist) {
            bestDist = d;
            bestMove = move;
        }
    }
    return bestMove;
}

static Move getFarthestMove(const GameState* gs, std::shared_ptr<Character> character, const std::pair<int,int>& target) {
    auto moves = getPacmanMoves(gs, character);
    float bestDist = -1.0f;
    Move bestMove = PASS;
    for(auto move : moves) {
        if(move == PASS) continue;
        int neighbour = gs->getMaze().getNeighbour(character->getPos(), move);
        if(neighbour < 0) continue;
        auto neighbourPos = gs->getMaze().getNodePos(neighbour);
        float d = euclid2(neighbourPos, target);
        if(d > bestDist) {
            bestDist = d;
            bestMove = move;
        }
    }
    return bestMove;
}

class ExistEdibleGhost : public Behavior {
public:
    virtual Status update() override {
        auto gs = BTMsPacmanInfo::getInfo()->in_gamestate;
        for(int i = 0; i < 4; i++) {
            if(gs->isGhostEdible(i)) {
                return BH_SUCCESS;
            }
        }
        return BH_FAILURE;
    }
};

class ClosestEdibleGhost : public Behavior {
public:
    virtual Status update() override {
        auto gs = BTMsPacmanInfo::getInfo()->in_gamestate;
        auto character = BTMsPacmanInfo::getInfo()->in_character;
        float bestDist = std::numeric_limits<float>::infinity();
        std::pair<int,int> target = {-1, -1};
        for(int i = 0; i < 4; i++) {
            if(gs->isGhostEdible(i)) {
                auto ghostPos = gs->getMaze().getNodePos(gs->getGhostsPos(i));
                float d = euclid2(gs->getMaze().getNodePos(character->getPos()), ghostPos);
                if(d < bestDist) {
                    bestDist = d;
                    target = ghostPos;
                }
            }
        }
        if(target.first == -1) {
            return BH_FAILURE;
        }
        BTMsPacmanInfo::getInfo()->out_move = getClosestMove(gs, character, target);
        return BH_SUCCESS;
    }
};

class DangerNearby : public Behavior {
public:
    virtual Status update() override {
        auto gs = BTMsPacmanInfo::getInfo()->in_gamestate;
        auto character = BTMsPacmanInfo::getInfo()->in_character;
        float nearest = std::numeric_limits<float>::infinity();
        for(int i = 0; i < 4; i++) {
            if(!gs->isGhostEdible(i)) {
                auto ghostPos = gs->getMaze().getNodePos(gs->getGhostsPos(i));
                float d = euclid2(gs->getMaze().getNodePos(character->getPos()), ghostPos);
                if(d < nearest) {
                    nearest = d;
                }
            }
        }
        if(nearest < 400.0f) {
            return BH_SUCCESS;
        }
        return BH_FAILURE;
    }
};

class FleeGhost : public Behavior {
public:
    virtual Status update() override {
        auto gs = BTMsPacmanInfo::getInfo()->in_gamestate;
        auto character = BTMsPacmanInfo::getInfo()->in_character;
        float nearest = std::numeric_limits<float>::infinity();
        std::pair<int,int> dangerPos = {-1, -1};
        for(int i = 0; i < 4; i++) {
            if(!gs->isGhostEdible(i)) {
                auto ghostPos = gs->getMaze().getNodePos(gs->getGhostsPos(i));
                float d = euclid2(gs->getMaze().getNodePos(character->getPos()), ghostPos);
                if(d < nearest) {
                    nearest = d;
                    dangerPos = ghostPos;
                }
            }
        }
        if(dangerPos.first == -1) {
            return BH_FAILURE;
        }
        BTMsPacmanInfo::getInfo()->out_move = getFarthestMove(gs, character, dangerPos);
        return BH_SUCCESS;
    }
};

class HasPowerPill : public Behavior {
public:
    virtual Status update() override {
        auto gs = BTMsPacmanInfo::getInfo()->in_gamestate;
        if(!gs->getMaze().getPowerPillPositions().empty()) {
            return BH_SUCCESS;
        }
        return BH_FAILURE;
    }
};

class GoToPowerPill : public Behavior {
public:
    virtual Status update() override {
        auto gs = BTMsPacmanInfo::getInfo()->in_gamestate;
        auto character = BTMsPacmanInfo::getInfo()->in_character;
        auto powerPills = gs->getMaze().getPowerPillPositions();
        if(powerPills.empty()) {
            return BH_FAILURE;
        }
        float bestDist = std::numeric_limits<float>::infinity();
        std::pair<int,int> target = powerPills[0];
        for(auto pill : powerPills) {
            float d = euclid2(gs->getMaze().getNodePos(character->getPos()), pill);
            if(d < bestDist) {
                bestDist = d;
                target = pill;
            }
        }
        BTMsPacmanInfo::getInfo()->out_move = getClosestMove(gs, character, target);
        return BH_SUCCESS;
    }
};

class HasPill : public Behavior {
public:
    virtual Status update() override {
        auto gs = BTMsPacmanInfo::getInfo()->in_gamestate;
        if(!gs->getMaze().getPillPositions().empty()) {
            return BH_SUCCESS;
        }
        return BH_FAILURE;
    }
};

class GoToPill : public Behavior {
public:
    virtual Status update() override {
        auto gs = BTMsPacmanInfo::getInfo()->in_gamestate;
        auto character = BTMsPacmanInfo::getInfo()->in_character;
        auto pills = gs->getMaze().getPillPositions();
        if(pills.empty()) {
            return BH_FAILURE;
        }
        float bestDist = std::numeric_limits<float>::infinity();
        std::pair<int,int> target = pills[0];
        for(auto pill : pills) {
            float d = euclid2(gs->getMaze().getNodePos(character->getPos()), pill);
            if(d < bestDist) {
                bestDist = d;
                target = pill;
            }
        }
        BTMsPacmanInfo::getInfo()->out_move = getClosestMove(gs, character, target);
        return BH_SUCCESS;
    }
};

class Wander : public Behavior {
public:
    virtual Status update() override {
        auto gs = BTMsPacmanInfo::getInfo()->in_gamestate;
        auto character = BTMsPacmanInfo::getInfo()->in_character;
        float nearest = std::numeric_limits<float>::infinity();
        std::pair<int,int> dangerPos = {-1, -1};
        for(int i = 0; i < 4; i++) {
            if(!gs->isGhostEdible(i)) {
                auto ghostPos = gs->getMaze().getNodePos(gs->getGhostsPos(i));
                float d = euclid2(gs->getMaze().getNodePos(character->getPos()), ghostPos);
                if(d < nearest) {
                    nearest = d;
                    dangerPos = ghostPos;
                }
            }
        }
        if(dangerPos.first == -1) {
            auto moves = getPacmanMoves(gs, character);
            BTMsPacmanInfo::getInfo()->out_move = moves.empty() ? PASS : moves[0];
            return BH_SUCCESS;
        }
        BTMsPacmanInfo::getInfo()->out_move = getFarthestMove(gs, character, dangerPos);
        return BH_SUCCESS;
    }
};

BTMsPacmanController::BTMsPacmanController(std::shared_ptr<Character> character): Controller(character), root(std::make_shared<Selector>()) {
    auto chaseEdible = std::make_shared<Filter>();
    chaseEdible->addCondition(std::make_shared<ExistEdibleGhost>());
    chaseEdible->addAction(std::make_shared<ClosestEdibleGhost>());
    root->addChild(chaseEdible);

    auto flee = std::make_shared<Filter>();
    flee->addCondition(std::make_shared<DangerNearby>());
    flee->addAction(std::make_shared<FleeGhost>());
    root->addChild(flee);

    auto powerpill = std::make_shared<Filter>();
    powerpill->addCondition(std::make_shared<HasPowerPill>());
    powerpill->addAction(std::make_shared<GoToPowerPill>());
    root->addChild(powerpill);

    auto pill = std::make_shared<Filter>();
    pill->addCondition(std::make_shared<HasPill>());
    pill->addAction(std::make_shared<GoToPill>());
    root->addChild(pill);

    root->addChild(std::make_shared<Wander>());
}

BTMsPacmanController::~BTMsPacmanController() {
}

Move BTMsPacmanController::getMove(const GameState& game) {
    SDL_Event e;
    while(SDL_PollEvent(&e) != 0) {
        if(e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_q))) {
            SDL_Quit();
            exit(0);
        }
    }

    BTMsPacmanInfo::getInfo()->in_character = character;
    BTMsPacmanInfo::getInfo()->in_gamestate = &game;
    root->tick();
    return BTMsPacmanInfo::getInfo()->out_move;
}
