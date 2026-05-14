#pragma once

#include "Controller.h"
#include "BehaviorTree.h"
#include <memory>

class BTMsPacmanController: public Controller {
private:
    std::shared_ptr<Composite> root;
public:
    BTMsPacmanController(std::shared_ptr<Character> character);
    virtual ~BTMsPacmanController();
    virtual Move getMove(const GameState& game) override;
};
