#ifndef GAME_H
#define GAME_H

#include "tower.h"
#include "move.h"
#include <queue>
#include <stack>
#include <vector>
#include <string>

class Game {
public:
    Tower towerA, towerB, towerC;
    int numDisks;
    int moveCount;

    std::queue<Move> solutionQueue;
    std::stack<Move> undoStack;
    std::vector<std::string> moveLog;

    Game();
    void init(int n);
    bool moveDisk(std::string from, std::string to);
    void undoMove();
    void generateSolution(int n, std::string src, std::string aux, std::string dst);
    bool isWon();
    void reset(int n);
    Tower* getTower(std::string name);
};

#endif // GAME_H
