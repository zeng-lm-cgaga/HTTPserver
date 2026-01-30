#pragma once

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>
#include <mutex>

const int BOARD_SIZE = 15;

const std::string EMPTY = "empty";
const std::string AI_PLAYER = "white";   // AI玩家白棋
const std::string HUMAN_PLAYER = "black"; // 人类玩家黑棋

enum class Difficulty
{
    Easy,
    Medium,
    Hard
};

class AiGame
{
public:
    AiGame(int userId, Difficulty difficulty = Difficulty::Medium);

    // 判断是否平局
    bool isDraw() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return moveCount_ >= BOARD_SIZE * BOARD_SIZE;
    }

    bool humanMove(int x, int y);

    bool checkWin(int x,int y, const std::string& player);

    void aiMove();

    void setDifficulty(Difficulty diff)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        difficulty_ = diff;
    }

    // 获取最后一步移动的坐标
    std::pair<int, int> getLastMove() const 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return lastMove_;
    }

     // 获取当前棋盘状态
    const std::vector<std::vector<std::string>>& getBoard() const 
    { 
        std::lock_guard<std::mutex> lock(mutex_);
        return board_; 
    }

    bool isGameOver() const 
    { 
        std::lock_guard<std::mutex> lock(mutex_);
        return gameOver_; 
    }

    std::string getWinner() const 
    { 
        std::lock_guard<std::mutex> lock(mutex_);
        return winner_; 
    }

private:
    // 检查移动是否有效
    bool isValidMove(int x, int y) const 
    {
        if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) return false;
        if (board_[x][y] != EMPTY) return false;
        if (gameOver_ || isDraw()) return false;
        return true;
    }

     // 检查坐标是否在棋盘内
    bool isInBoard(int x, int y) const 
    {
        return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
    }

    // 获取AI的移动位置（分难度）
    std::pair<int, int> getMoveEasy();
    std::pair<int, int> getMoveMedium();
    std::pair<int, int> getMoveHard();

    // 评估相关辅助函数
    int evaluateThreat(int r, int c);
    bool isNearOccupied(int r, int c);
    int evaluateBoardScore();
    std::vector<std::pair<int, int>> collectCandidateMoves(int limit = 40);

private:
    bool                                  gameOver_;
    int                                   userId_;
    int                                   moveCount_;
    Difficulty                            difficulty_;
    std::string                           winner_{"none"};
    std::pair<int, int>                   lastMove_{-1, -1};  // 上一次落子位置
    std::vector<std::vector<std::string>> board_;
    mutable std::mutex                    mutex_;  // 添加互斥锁
};