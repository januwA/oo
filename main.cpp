#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include "oo.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
        return 0;

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // oo::Request opt{argc, argv};
    oo::Request opt{"http://localhost:7777/ping"};

    oo::runResult res;
    if (oo::run(opt, res))
        return 1;

    fprintf(stdout, "总耗时: %.2Fs\n", res.time.count() / (double)1000.0);
    std::cout << "线程数: " << res.threadCount << "\n";

    fprintf(stdout, "返回字节总数: %zd\n", res.respDataCount);

    if (res.successCount)
    {
        fprintf(stdout, "成功: %d | %.1F%%\n", res.successCount, ((double)res.successCount / (double)res.requestedCount) * 100);
    }

    if (res.errorCount)
    {
        fprintf(stdout, "失败: %d | %.1F%%\n", res.errorCount, ((double)res.errorCount / (double)res.requestedCount) * 100);
    }
    return 0;
}
