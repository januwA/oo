#pragma once

#include <string.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
using json = nlohmann::json;

extern "C" {
#include <curl/curl.h>

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

namespace oo {
using namespace std;

enum class METHOD {
  Head,
  Get,
  Post,
  Delete,
  Put,
  Patch,
};

enum class NEED_FLAGS {
  Header = 1 << 0,
  Body = 1 << 1,
};

namespace utils {
string_view trim(string_view src, char ignoreChar);
void lua_pushjson(lua_State* L, const json& data);

struct mapComp {
  bool operator()(string_view lhs, string_view rhs) const {
    if (lhs.size() != rhs.size()) return true;
    for (size_t i = 0; i < lhs.size(); i++) {
      if (::tolower((int)lhs[i]) != ::tolower((int)rhs[i])) return true;
    }
    return false;
  }
};
}  // namespace utils

struct FilePart {
  string_view name;
  vector<string_view> values;
  bool isFilePath;
};

class Request {
 public:
  string_view scirptPath;
  string_view scirptCode;
  string methodStr{"get"};
  string_view url;
  map<string_view, string_view, utils::mapComp> headers;
  string_view data;
  vector<FilePart> multipart;
  uint32_t requestCount{1};

  uint8_t needflag{0};

  Request() = default;
  Request(int argc, char* argv[]);

  METHOD Method();

  bool hasScript();
};

struct Body {
  uint8_t* data{nullptr};
  size_t size{0};
};

class Response {
 public:
  uint8_t needflag;

  long statusCode{0};
  string headerStr;
  Body body;

  size_t size{0};  // response size (Status-Line size + header size + body size)

  Response() = default;
  Response(uint8_t needflag) : needflag{needflag} {}
  ~Response();

  size_t WriteBody(uint8_t* data, size_t size);
  size_t WriteHeader(char* buffer, size_t size);
  map<string_view, string_view, utils::mapComp> GetHeaders();
  inline void Clear();
};

size_t curlRespBodyCallback(void* data, size_t size, size_t nmemb, void* userp);
size_t curlRespHeaderCallback(char* buffer, size_t size, size_t nitems,
                              void* userdata);

struct RunResult {
  chrono::milliseconds time;
  uint32_t threadCount;
  uint32_t requestedCount;
  uint32_t successCount;
  uint32_t errorCount;
  size_t respDataCount;
  bool hasRunDone{false};
};

class LuaScript {
 private:
  string_view path;
  string_view code;
  lua_State* L;

 public:
  LuaScript(string_view path, string_view code);
  ~LuaScript();
  void PresetRequestVariable(Request* pRequest);
  void PresetDoScript(Request* pRequest);
  void PresetResponse(Request* pRequest);
  void PresetPreset(Request* pRequest);
  void Preset(Request* pRequest);
  bool HasResponseFunc();
  bool HasRunDoneFunc();
  void CallRunDone(RunResult* pResult);
  bool CallResponse(Response* pResponse);
  LuaScript* Copy();
};

class HttpClint {
 private:
  CURL* hCurl{nullptr};
  struct curl_slist* pHeaerSlist{nullptr};
  curl_mime* pMultipart{nullptr};

  Request* pRequest{nullptr};
  Response* pResponse{nullptr};

 public:
  HttpClint(Request* pRequest);
  ~HttpClint();

  inline void SetMethod();
  inline void SetUrl();
  void SetHeader();
  void SetBody();
  CURLcode Send();
  inline void Clear();
  inline Response* GetResponsePtr();
};

void blockHttpSend(Request* pRequest, LuaScript* pLuaScript);
int run(Request* pRequest, RunResult* pResult);
}  // namespace oo
