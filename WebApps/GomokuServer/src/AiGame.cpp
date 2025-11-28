#include "AiGame.h"

#include <chrono>
#include <thread>


AiGame::AiGame(int userId)
    : gameOver_(false)
    , userId_(userId)
    , moveCount_(0)
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
    // 获取AI的最佳移动位置
    std::tie(x, y) = getBestMove();
    board_[x][y] = AI_PLAYER;
    moveCount_++;
    lastMove_ = {x, y};
    
    if (checkWin(x, y, AI_PLAYER)) 
    {
        gameOver_ = true;
        winner_ = "ai";
    }
}


// 辅助函数：评估某个位置的威胁程度
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


std::pair<int, int> AiGame::getBestMove()
{
    std::pair<int, int> bestMove = {-1, -1}; // 最佳落子位置
    int maxThreat = -1;                      // 记录最大的威胁分数

    // 1. 优先尝试进攻获胜或阻止玩家获胜
    for (int r = 0; r < BOARD_SIZE; r++) 
    {
        for (int c = 0; c < BOARD_SIZE; c++) 
        {
            if (board_[r][c] != EMPTY) continue; // 确保当前位置为空闲

            // 模拟 AI 落子，判断是否可以获胜
            board_[r][c] = AI_PLAYER;
            if (checkWin(r, c, AI_PLAYER)) 
            {
                // board_[r][c] = AI_PLAYER; // 恢复棋盘
                return {r, c};      // 立即获胜
            }
            board_[r][c] = EMPTY;

            // 模拟玩家落子，判断是否需要防守
            board_[r][c] = HUMAN_PLAYER;
            if (checkWin(r, c, HUMAN_PLAYER)) 
            {
                board_[r][c] = AI_PLAYER; // 恢复棋盘
                return {r, c};      // 立即防守
            }
            board_[r][c] = EMPTY;
        }
    }

    // 2. 评估每个空位的威胁程度，选择最佳防守位置
    for (int r = 0; r < BOARD_SIZE; r++) 
    {
        for (int c = 0; c < BOARD_SIZE; c++) 
        {
            if (board_[r][c] != EMPTY) continue; // 确保当前位置为空闲

            int threatLevel = evaluateThreat(r, c); // 评估威胁程度
            if (threatLevel > maxThreat) 
            {
                maxThreat = threatLevel;
                bestMove = {r, c};
            }
        }
    }

    // 3. 如果找不到威胁点，选择靠近玩家或已有棋子的空位
    if (bestMove.first == -1) 
    {
        std::vector<std::pair<int, int>> nearCells;

        for (int r = 0; r < BOARD_SIZE; r++) 
        {
            for (int c = 0; c < BOARD_SIZE; c++) 
            {
                if (board_[r][c] == EMPTY && isNearOccupied(r, c)) 
                { // 确保当前位置为空闲且靠近已有棋子
                    nearCells.push_back({r, c});
                }
            }
        }

        // 如果找到靠近已有棋子的空位，随机选择一个
        if (!nearCells.empty()) 
		{
            int num = rand();
			board_[nearCells[num % nearCells.size()].first][nearCells[num % nearCells.size()].second] = AI_PLAYER;
            return nearCells[num % nearCells.size()];
        }

        // 4. 如果所有策略都无效，选择第一个空位（保证 AI 落子）
        for (int r = 0; r < BOARD_SIZE; r++) 
        {
            for (int c = 0; c < BOARD_SIZE; c++) 
            {
                if (board_[r][c] == EMPTY) 
				{
					board_[r][c] = AI_PLAYER;
                    return {r, c}; // 返回第一个空位
                }
            }
        }
    }
	
	board_[bestMove.first][bestMove.second] = AI_PLAYER;
    return bestMove; // 返回最佳防守点或其他策略的结果
}