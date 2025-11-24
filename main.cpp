#include "Game.h"
#include <iostream>
#include <exception>

int main()
{
    // 设置随机种子
    srand(static_cast<unsigned int>(time(nullptr)));

    std::cout << "程序启动..." << std::endl;

    try {
        Game game;
        std::cout << "游戏对象创建成功，开始运行..." << std::endl;
        game.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "程序异常: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cerr << "未知异常" << std::endl;
        return -1;
    }

    std::cout << "程序正常退出" << std::endl;
    return 0;
}