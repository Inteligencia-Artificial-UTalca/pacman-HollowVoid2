#include "PinkyController.h"
#include "Ghost.h"
#include <chrono>
#include <limits>
#include <random>

class PinkyInfo {
    static PinkyInfo *info;
    PinkyInfo() {}
public:
    static PinkyInfo* getInfo() {
        if(info == nullptr) info = new PinkyInfo();
        return info;
    }
    const GameState* in_gamestate;
    Move out_move;
    std::shared_ptr<Character> in_character;
};

PinkyInfo* PinkyInfo::info = nullptr;

class PowerpillCondition : public Behavior {
public:
    virtual Status update() override {
        auto character = PinkyInfo::getInfo()->in_character;
        auto ghost = dynamic_cast<Ghost*>(character.get());
        if(ghost != nullptr && ghost->isEdible()) {
            return BH_SUCCESS;
        }
        return BH_FAILURE;
    }
};

class FrightenedPinky : public Behavior {
private:
    std::mt19937 engine;
public:
    FrightenedPinky() : Behavior(), engine(std::random_device{}()) {}
    virtual Status update() override {
        auto character = PinkyInfo::getInfo()->in_character;
        auto gs = PinkyInfo::getInfo()->in_gamestate;
        std::vector<Move> moves;
        if(character->getDirection() == PASS) {
            moves = gs->getMaze().getPossibleMoves(character->getPos());
        } else {
            moves = gs->getMaze().getGhostLegalMoves(character->getPos(), character->getDirection());
        }
        if(moves.empty()) {
            PinkyInfo::getInfo()->out_move = PASS;
            return BH_SUCCESS;
        }
        std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
        PinkyInfo::getInfo()->out_move = moves[dist(engine)];
        return BH_SUCCESS;
    }
};

class TimeOutPinky : public Behavior {
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;
public:
    TimeOutPinky() : Behavior() {
        lastTime = std::chrono::high_resolution_clock::now();
    }
    virtual Status update() override {
        std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now() - lastTime;
        if(((int)elapsed.count() % 27) < 7) {
            return BH_SUCCESS;
        }
        return BH_FAILURE;
    }
};

class ScatterPinky : public Behavior {
private:
    std::pair<int,int> target;
public:
    ScatterPinky() : Behavior(), target(-1, -1) {}
    virtual Status update() override {
        auto character = PinkyInfo::getInfo()->in_character;
        auto gs = PinkyInfo::getInfo()->in_gamestate;
        if(target.first == -1) {
            auto powerPills = gs->getMaze().getPowerPillPositions();
            if(!powerPills.empty()) {
                target = powerPills[0];
            } else {
                target = gs->getMaze().getNodePos(gs->getPacmanPos());
            }
        }
        std::vector<Move> moves;
        if(character->getDirection() == PASS) {
            moves = gs->getMaze().getPossibleMoves(character->getPos());
        } else {
            moves = gs->getMaze().getGhostLegalMoves(character->getPos(), character->getDirection());
        }
        Move minMove = PASS;
        float minDist = std::numeric_limits<float>::infinity();
        for(auto move : moves) {
            if(move == PASS) break;
            auto neighbour = gs->getMaze().getNeighbour(character->getPos(), move);
            auto neighbourPos = gs->getMaze().getNodePos(neighbour);
            float dist = euclid2(target, neighbourPos);
            if(dist < minDist) {
                minDist = dist;
                minMove = move;
            }
        }
        PinkyInfo::getInfo()->out_move = minMove;
        return BH_SUCCESS;
    }
};

class ChasePinky : public Behavior {
public:
    virtual Status update() override {
        auto character = PinkyInfo::getInfo()->in_character;
        auto gs = PinkyInfo::getInfo()->in_gamestate;
        auto target = gs->getMaze().getNodePos(gs->getPacmanPos());
        Move pacmanDir = static_cast<Move>(gs->getPacmanDir());
        int node = gs->getPacmanPos();
        for(int i = 0; i < 4; i++) {
            int next = gs->getMaze().getNeighbour(node, pacmanDir);
            if(next == -1) break;
            node = next;
            target = gs->getMaze().getNodePos(node);
        }
        std::vector<Move> moves;
        if(character->getDirection() == PASS) {
            moves = gs->getMaze().getPossibleMoves(character->getPos());
        } else {
            moves = gs->getMaze().getGhostLegalMoves(character->getPos(), character->getDirection());
        }
        Move minMove = PASS;
        float minDist = std::numeric_limits<float>::infinity();
        for(auto move : moves) {
            if(move == PASS) break;
            auto neighbour = gs->getMaze().getNeighbour(character->getPos(), move);
            auto neighbourPos = gs->getMaze().getNodePos(neighbour);
            float dist = euclid2(target, neighbourPos);
            if(dist < minDist) {
                minDist = dist;
                minMove = move;
            }
        }
        PinkyInfo::getInfo()->out_move = minMove;
        return BH_SUCCESS;
    }
};

PinkyController::PinkyController(std::shared_ptr<Character> character): Controller(character), root(std::make_shared<Selector>()) {
    auto frightenedFilter = std::make_shared<Filter>();
    frightenedFilter->addCondition(std::make_shared<PowerpillCondition>());
    frightenedFilter->addAction(std::make_shared<FrightenedPinky>());
    root->addChild(frightenedFilter);

    auto scatterFilter = std::make_shared<Filter>();
    scatterFilter->addCondition(std::make_shared<TimeOutPinky>());
    scatterFilter->addAction(std::make_shared<ScatterPinky>());
    root->addChild(scatterFilter);

    root->addChild(std::make_shared<ChasePinky>());
}

PinkyController::~PinkyController() {
}

Move PinkyController::getMove(const GameState& game) {
    PinkyInfo::getInfo()->in_character = character;
    PinkyInfo::getInfo()->in_gamestate = &game;
    root->tick();
    return PinkyInfo::getInfo()->out_move;
}
