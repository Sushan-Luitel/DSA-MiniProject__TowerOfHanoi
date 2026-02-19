#include "game.h"
#include "tower.h"
#include "move.h"
#include <sstream>
#include <cmath>

Game::Game() : numDisks(3), moveCount(0) {
    towerA = Tower("A");
    towerB = Tower("B");
    towerC = Tower("C");
}

void Game::init(int n) {
    numDisks = n;
    moveCount = 0;
    moveLog.clear();

    while (!towerA.disks.empty()) towerA.disks.pop();
    while (!towerB.disks.empty()) towerB.disks.pop();
    while (!towerC.disks.empty()) towerC.disks.pop();
    while (!undoStack.empty()) undoStack.pop();
    while (!solutionQueue.empty()) solutionQueue.pop();

    for (int i = n; i >= 1; i--) {
        towerA.push(i);
    }
}

Tower* Game::getTower(std::string name) {
    if (name == "A") return &towerA;
    if (name == "B") return &towerB;
    if (name == "C") return &towerC;
    return nullptr;
}

bool Game::moveDisk(std::string from, std::string to) {
    Tower* src = getTower(from);
    Tower* dst = getTower(to);

    if (!src || !dst) return false;
    if (src->isEmpty()) return false;

    int diskSize = src->top();
    if (!dst->isEmpty() && dst->top() < diskSize) return false;

    src->pop();
    dst->push(diskSize);

    Move m(from, to, diskSize);
    undoStack.push(m);

    moveCount++;
    std::ostringstream oss;
    oss << moveCount << ". Disk " << diskSize << ": " << from << " -> " << to;
    moveLog.push_back(oss.str());

    return true;
}

void Game::undoMove() {
    if (undoStack.empty()) return;

    Move m = undoStack.top();
    undoStack.pop();

    Tower* src = getTower(m.to);
    Tower* dst = getTower(m.from);

    src->pop();
    dst->push(m.diskSize);

    moveCount--;
    if (!moveLog.empty()) moveLog.pop_back();
}

void Game::generateSolution(int n, std::string src, std::string aux, std::string dst) {
    if (n == 0) return;
    generateSolution(n - 1, src, dst, aux);
    solutionQueue.push(Move(src, dst, n));
    generateSolution(n - 1, aux, src, dst);
}

bool Game::isWon() {
    return (int)towerC.disks.size() == numDisks;
}

void Game::reset(int n) {
    init(n);
}
