#pragma once

#include <string_view>
#include <vector>
#include <string>
#include <string.h>
#include <filesystem>
#include <thread>
#include <map>
#include <chrono>
#include <curl/curl.h>

namespace oo
{
  using namespace std;

  namespace utils
  {
    string_view trim(string_view src, char ignoreChar);
  }

  enum class METHOD
  {
    HEAD,
    GET,
    POST,
  };

  oo::METHOD methodHit(string_view method);

  struct commandPart
  {
    string_view name;
    vector<string_view> values;
    bool isFilePath;
  };

  struct runResult
  {
    chrono::milliseconds time;
    uint32_t threadCount;
    uint32_t requestedCount;
    uint32_t successCount;
    uint32_t errorCount;
    size_t respDataCount;
  };

  class RequestOption
  {
  public:
    oo::METHOD method{METHOD::GET};
    string_view url;
    map<string_view, string_view> headers;
    string_view postData;
    vector<commandPart> postMultipart;
    uint32_t requestCount{1};

    bool needRespHeader{false};
    bool needBody{false};

    RequestOption(int argc, char *argv[]);
  };

  class Response
  {
  private:
    RequestOption &opt;

  public:
    long statusCode;

    string respHeaderString;

    uint8_t *bodyBytes;
    size_t bodySize;

    // 记录返回的字节数，通常是header+body
    size_t responseByteSize{0};

    Response(RequestOption &opt) : statusCode(0), bodyBytes(0), bodySize(0), opt{opt}
    {
    }
    ~Response();

    // 将一个chunk data写入 responseBytes，并更新 length
    size_t writeBody(uint8_t *data, size_t size);
    size_t writeHeader(char *buffer, size_t size);
    map<string_view, string_view> headerMap();
    inline void clear() noexcept;
  };

  void httpSend(RequestOption &opt);
  size_t responseBodyCallback(void *data, size_t size, size_t nmemb, void *userp);
  int run(RequestOption &opt, runResult &result);

  inline void setCurlUrl(CURL *hCurl, string_view url);
  void setCurlMethod(CURL *hCurl, oo::METHOD method);
  struct curl_slist *setCurlHeader(CURL *hCurl, map<string_view, string_view> &headers);
  curl_mime *setCurlBody(CURL *hCurl, oo::RequestOption &opt);
}