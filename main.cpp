#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>

#include "oo.h"

int main(int argc, char* argv[]) {
  if (argc < 2) return 0;

#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
#endif

  oo::Request request{argc, argv};
  oo::RunResult result;

  if (oo::run(&request, &result)) {
    std::cerr << "Error: run" << std::endl;
    return 1;
  }

  if (!result.hasRunDone) {
    fprintf(stdout, "总耗时: %.2Fs\n", result.time.count() / (double)1000.0);
    std::cout << "线程数: " << result.threadCount << "\n";
    fprintf(stdout, "返回字节总数: %zd\n", result.respDataCount);

    if (result.successCount)
      fprintf(
          stdout, "成功: %d | %.1F%%\n", result.successCount,
          ((double)result.successCount / (double)result.requestedCount) * 100);

    if (result.errorCount)
      fprintf(
          stdout, "失败: %d | %.1F%%\n", result.errorCount,
          ((double)result.errorCount / (double)result.requestedCount) * 100);
  }

  return 0;
}
