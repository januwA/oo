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

  enum class METHOD
  {
    HEAD,
    GET,
    POST,
  };

  namespace utils
  {
    string_view trim(string_view src, char ignoreChar);
  }

  METHOD methodHit(string_view method);

  struct commandPart
  {
    string_view name;
    vector<string_view> values;
    bool isFilePath;
  };

  class Request
  {
  public:
    METHOD method{METHOD::GET};
    string_view url;
    map<string_view, string_view> headers;
    string_view postData;
    vector<commandPart> postMultipart;
    uint32_t requestCount{1};

    bool needRespHeader{false};
    bool needBody{false};

    Request();
    Request(string_view url);
    Request(int argc, char *argv[]);
  };

  class Response
  {
  private:
    Request &opt;

  public:
    long statusCode;

    string respHeaderString;

    uint8_t *bodyBytes;
    size_t bodySize;

    // 记录返回的字节数，通常是header+body
    size_t responseByteSize{0};

    Response(Request &opt) : statusCode(0), bodyBytes(0), bodySize(0), opt{opt}
    {
    }
    ~Response();

    // 将一个chunk data写入 responseBytes，并更新 length
    size_t writeBody(uint8_t *data, size_t size);
    size_t writeHeader(char *buffer, size_t size);
    map<string_view, string_view> headerMap();
    inline void clear() noexcept;
  };

  size_t responseBodyCallback(void *data, size_t size, size_t nmemb, void *userp);
  size_t responseHeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata);

  class HttpClint
  {
  private:
    CURL *hCurl{nullptr};
    struct curl_slist *pHeaders{nullptr};
    curl_mime *multipart{nullptr};
    Response *response{nullptr};

  public:
    HttpClint(Request &request);
    ~HttpClint();

    inline void setUrl(string_view url);
    inline void setMethod(METHOD method);
    void setHeader(map<string_view, string_view> &headers);
    void setBody(Request &opt);
    inline CURLcode send();
    inline void clear();
    inline Response *getResposne();
  };

  void blockSend(Request &request);

  struct runResult
  {
    chrono::milliseconds time;
    uint32_t threadCount;
    uint32_t requestedCount;
    uint32_t successCount;
    uint32_t errorCount;
    size_t respDataCount;
  };
  int run(Request &request, runResult &result);
}