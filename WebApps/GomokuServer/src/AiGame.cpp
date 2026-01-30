#include "AiGame.h"

#include <chrono>
#include <thread>
#include <algorithm>


AiGame::AiGame(int userId, Difficulty difficulty)
    : gameOver_(false)
    , userId_(userId)
    , moveCount_(0)
    , difficulty_(difficulty)
    , lastMove_(-1, -1)
    , board_(BOARD_SIZE, std::vector<std::string>(BOARD_SIZE, EMPTY))
{
	srand(time(0)); // 初始化随机数种子
}

// 处理人类玩家移动
bool AiGame::humanMove(int x, int y) 
{
    if (!isValidMove(x, y)) 
        return false;
    
    board_[x][y] = HUMAN_PLAYER;
    moveCount_++;
    lastMove_ = {x, y};
    
    if (checkWin(x, y, HUMAN_PLAYER)) 
    {
        gameOver_ = true;
        winner_ = "human";
    }
    return true;
}

 // AI移动
void AiGame::aiMove() 
{
    if (gameOver_ || isDraw()) return;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 添加500毫秒延时
    int x, y;
    // 获取AI的移动位置，按难度选择策略
    switch (difficulty_)
    {
        case Difficulty::Easy:
            std::tie(x, y) = getMoveEasy();
            break;
        case Difficulty::Hard:
            std::tie(x, y) = getMoveHard();
            break;
        case Difficulty::Medium:
        default:
            std::tie(x, y) = getMoveMedium();
            break;
    }
    board_[x][y] = AI_PLAYER;
    moveCount_++;
    lastMove_ = {x, y};
    
    if (checkWin(x, y, AI_PLAYER)) 
    {
        gameOver_ = true;
        winner_ = "ai";
    }
}


// 检查胜利条件
bool AiGame::checkWin(int x, int y, const std::string& player) 
{
    // 检查方向数组：水平、垂直、对角线、反对角线
    const int dx[] = {1, 0, 1, 1};
    const int dy[] = {0, 1, 1, -1};
    
    for (int dir = 0; dir < 4; dir++) 
    {
        int count = 1;  // 当前位置已经有一个棋子
        
        // 正向检查
        for (int i = 1; i < 5; i++) 
        {
            int newX = x + dx[dir] * i;
            int newY = y + dy[dir] * i;
            if (!isInBoard(newX, newY) || board_[newX][newY] != player) break;
            count++;
        }
        
        // 反向检查
        for (int i = 1; i < 5; i++) 
        {
            int newX = x - dx[dir] * i;
            int newY = y - dy[dir] * i;
            if (!isInBoard(newX, newY) || board_[newX][newY] != player) break;
            count++;
        }
        
        if (count >= 5) return true;
    }
    return false;
}

// 辅助函数：评估某个位置的威胁程度（用于中等难度）
int AiGame::evaluateThreat(int r, int c) 
{
    int threat = 0;

    // 检查四个方向上的玩家连子数
    const int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    for (auto& dir : directions) 
    {
        int count = 1;
        for (int i = 1; i <= 2; i++) 
        { // 探查2步
            int nr = r + i * dir[0], nc = c + i * dir[1];
            if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board_[nr][nc] == HUMAN_PLAYER) 
            {
                count++;
            }
        }
        threat += count; // 威胁分数累加
    }
    return threat;
}

// 辅助函数：判断某个空位是否靠近已有棋子
bool AiGame::isNearOccupied(int r, int c) 
{
    const int directions[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}
    };
    for (auto& dir : directions) 
    {
        int nr = r + dir[0], nc = c + dir[1];
        if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board_[nr][nc] != EMPTY) 
        {
            return true; // 该空位靠近已有棋子
        }
    }
    return false;
}

// 收集候选落子点（限制数量，靠近已有棋子）
std::vector<std::pair<int, int>> AiGame::collectCandidateMoves(int limit)
{
    std::vector<std::pair<int, int>> candidates;
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            if (board_[r][c] == EMPTY && isNearOccupied(r, c))
            {
                candidates.push_back({r, c});
            }
        }
    }
    if (candidates.empty())
    {
        for (int r = 0; r < BOARD_SIZE && static_cast<int>(candidates.size()) < limit; r++)
        {
            for (int c = 0; c < BOARD_SIZE && static_cast<int>(candidates.size()) < limit; c++)
            {
                if (board_[r][c] == EMPTY)
                {
                    candidates.push_back({r, c});
                }
            }
        }
    }
    if (static_cast<int>(candidates.size()) > limit)
    {
        candidates.resize(limit);
    }
    return candidates;
}

// 评估棋盘得分（简易启发式，用于困难难度）
int AiGame::evaluateBoardScore()
{
    // 简单评分：连子越长分越高，区分 AI 和玩家
    const int dx[] = {1, 0, 1, 1};
    const int dy[] = {0, 1, 1, -1};
    auto scoreLine = [](int len, bool open) {
        if (len >= 5) return 100000;
        if (len == 4) return open ? 10000 : 5000;
        if (len == 3) return open ? 2000 : 800;
        if (len == 2) return open ? 500 : 200;
        return 0;
    };

    int score = 0;
    for (int x = 0; x < BOARD_SIZE; ++x)
    {
        for (int y = 0; y < BOARD_SIZE; ++y)
        {
            if (board_[x][y] == EMPTY) continue;
            bool isAi = board_[x][y] == AI_PLAYER;
            for (int dir = 0; dir < 4; ++dir)
            {
                int len = 1;
                int nx = x + dx[dir], ny = y + dy[dir];
                while (isInBoard(nx, ny) && board_[nx][ny] == board_[x][y])
                {
                    ++len; nx += dx[dir]; ny += dy[dir];
                }
                bool open1 = isInBoard(nx, ny) && board_[nx][ny] == EMPTY;
                nx = x - dx[dir]; ny = y - dy[dir];
                while (isInBoard(nx, ny) && board_[nx][ny] == board_[x][y])
                {
                    ++len; nx -= dx[dir]; ny -= dy[dir];
                }
                bool open2 = isInBoard(nx, ny) && board_[nx][ny] == EMPTY;
                int s = scoreLine(len, open1 || open2);
                score += isAi ? s : -s;
            }
        }
    }
    return score;
}

// 简单难度：随机从可落子点（优先邻近已有棋子）
std::pair<int, int> AiGame::getMoveEasy()
{
    auto candidates = collectCandidateMoves(BOARD_SIZE * BOARD_SIZE);
    if (candidates.empty())
    {
        return {0, 0};
    }
    int idx = rand() % candidates.size();
    return candidates[idx];
}

// 中等难度：原有规则（即时胜负 + 威胁评估）
std::pair<int, int> AiGame::getMoveMedium()
{
    std::pair<int, int> bestMove = {-1, -1};
    int maxThreat = -1;

    // 1. 立即获胜或防守对方获胜
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            if (board_[r][c] != EMPTY) continue;

            board_[r][c] = AI_PLAYER;
            if (checkWin(r, c, AI_PLAYER))
            {
                board_[r][c] = EMPTY;
                return {r, c};
            }
            board_[r][c] = EMPTY;

            board_[r][c] = HUMAN_PLAYER;
            if (checkWin(r, c, HUMAN_PLAYER))
            {
                board_[r][c] = EMPTY;
                return {r, c};
            }
            board_[r][c] = EMPTY;
        }
    }

    // 2. 威胁评估
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            if (board_[r][c] != EMPTY) continue;
            int threatLevel = evaluateThreat(r, c);
            if (threatLevel > maxThreat)
            {
                maxThreat = threatLevel;
                bestMove = {r, c};
            }
        }
    }

    if (bestMove.first != -1)
    {
        return bestMove;
    }

    // 3. 靠近已有棋子，否则第一个空位
    auto nearCells = collectCandidateMoves(BOARD_SIZE * BOARD_SIZE);
    if (!nearCells.empty())
    {
        return nearCells[rand() % nearCells.size()];
    }
    return {0, 0};
}

// 困难难度：简易两层极大极小（带启发式评分）
std::pair<int, int> AiGame::getMoveHard()
{
    int bestScore = -1e9;
    std::pair<int, int> bestMove = {0, 0};
    auto candidates = collectCandidateMoves(60);

    for (auto [x, y] : candidates)
    {
        board_[x][y] = AI_PLAYER;
        if (checkWin(x, y, AI_PLAYER))
        {
            board_[x][y] = EMPTY;
            return {x, y};
        }

        int worstOpponent = 1e9;
        auto oppCandidates = collectCandidateMoves(40);
        if (oppCandidates.empty()) oppCandidates.push_back({x, y});
        for (auto [ox, oy] : oppCandidates)
        {
            if (board_[ox][oy] != EMPTY) continue;
            board_[ox][oy] = HUMAN_PLAYER;
            int score = evaluateBoardScore();
            worstOpponent = std::min(worstOpponent, score);
            board_[ox][oy] = EMPTY;
        }

        if (worstOpponent > bestScore)
        {
            bestScore = worstOpponent;
            bestMove = {x, y};
        }

        board_[x][y] = EMPTY;
    }

    return bestMove;
}