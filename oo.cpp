#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#endif

#include "oo.h"

#include <atomic>
#include <iostream>

std::atomic_size_t requestedCount{0};
std::atomic_size_t successCount{0};
std::atomic_size_t errorCount{0};
std::atomic_size_t respDataCount{0};

namespace oo {
/**
 * 一旦收到需要保存的数据，libcurl就会调用此回调函数
 * data 一块数据
 * size 始终为1
 * nmemb data 的大小
 */
size_t curlRespBodyCallback(void* data, size_t size, size_t nmemb,
                            void* userp) {
  Response* mem = (Response*)userp;
  size_t realsize = size * nmemb;
  return mem->WriteBody((uint8_t*)data, realsize);
}

size_t curlRespHeaderCallback(char* buffer, size_t size, size_t nitems,
                              void* userdata) {
  Response* resp = (Response*)userdata;
  return resp->WriteHeader(buffer, nitems * size);
}

namespace utils {
string_view trim(string_view src, char ignoreChar = ' ') {
  if (src.empty()) return src;

  auto begin = src.find_first_not_of(ignoreChar);
  auto end = src.find_last_not_of(ignoreChar);

  if (begin == string::npos || end == string::npos)
    return string_view(src.data(), 0);

  return string_view(src.data() + begin, end - begin + 1);
}
}  // namespace utils

Response::~Response() {
  if (body.data) free(body.data);
}

size_t Response::WriteBody(uint8_t* data, size_t size) {
  if (needBody) {
    size_t newSize = body.size + size;

    auto ptr = (uint8_t*)realloc(body.data, newSize);
    if (ptr == NULL) return 0; /* 内存不足!，太大可以写入文件 */

    body.data = ptr;
    memcpy(&(body.data[body.size]), data, size);
    body.size = newSize;
  }

  responseSize += size;
  return size;
}

size_t Response::WriteHeader(char* buffer, size_t size) {
  if (needHeader) headerStr.append(buffer, size);
  responseSize += size;
  return size;
}

map<string_view, string_view, utils::mapComp> Response::GetHeaders() {
  map<string_view, string_view, utils::mapComp> headerMap;
  size_t begin{0}, end{0}, count{0};

  auto stepFind = [&](const char c) {
    end = headerStr.find_first_of(c, begin);
    auto res = string_view(headerStr.data() + begin, end - begin);
    begin = end + sizeof(char);
    return res;
  };

  // Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
  auto httpCersion = stepFind(' ');
  if (end == string::npos) return headerMap;

  auto statusCode = stepFind(' ');
  if (end == string::npos) return headerMap;

  auto reasonPhrase = stepFind('\n');
  if (end == string::npos) return headerMap;

  // Response Header Fields
  for (;;) {
    string_view k, v;
    k = stepFind(':');
    if (end == string::npos) break;

    v = stepFind('\n');
    if (end == string::npos) break;

    headerMap.insert(
        {utils::trim(utils::trim(k), '\n'), utils::trim(utils::trim(v), '\n')});
  }

  return headerMap;
}

inline void Response::Clear() noexcept {
  statusCode = 0;

  headerStr.clear();

  body.size = 0;

  responseSize = 0;
}

Request::Request(int argc, char* argv[]) {
  for (size_t i = 1; i < argc; i++) {
    auto flag = argv[i];

    if (flag[0] != '-') continue;

    switch (flag[1]) {
      case 'h': {
        i++;
        auto k = argv[i], v = argv[i + 1];
        headers.insert({k, v});
        i++;
        break;
      }
      case 'd': {
        i++;
        if (flag[2] == 'f' || flag[2] == 'F') {
          bool isFilePath = flag[2] == 'F';
          bool exists = false;
          auto name = argv[i], value = argv[i + 1];

          for (auto&& i : multipart) {
            if (i.name == name) {
              i.values.push_back(value);
              i.isFilePath = isFilePath;
              exists = true;
              break;
            }
          }

          if (!exists) {
            multipart.push_back({name, {value}, isFilePath});
          }

          i++;
        } else
          data = argv[i];

        break;
      }
      case 'c': {
        i++;
        requestCount = atoi(argv[i]);
        break;
      }
      case 'm': {
        i++;
        methodStr = argv[i];
        break;
      }
      case 'u': {
        i++;
        url = argv[i];
        break;
      }
      case 's': {
        i++;
        scirptPath = argv[i];
      }
      default:
        break;
    }
  }
}

METHOD Request::Method() {
  if (methodStr == "head") return METHOD::Head;
  if (methodStr == "get") return METHOD::Get;
  if (methodStr == "post") return METHOD::Post;
  if (methodStr == "delete") return METHOD::Delete;
  if (methodStr == "put") return METHOD::Put;
  if (methodStr == "patch") return METHOD::Patch;
  exit(1);
}

LuaScript::LuaScript(string_view path) : path{path} {
  L = luaL_newstate();
  luaL_openlibs(L);
}

LuaScript::~LuaScript() { lua_close(L); }

bool LuaScript::empty() { return path.empty(); }

bool LuaScript::HasResponseCallback() {
  lua_getglobal(L, "Response");
  return lua_isfunction(L, -1) == 1;
}

bool LuaScript::HasRunDoneCallback() {
  lua_getglobal(L, "RunDone");
  return lua_isfunction(L, -1) == 1;
}

int lua_bodyText(lua_State* L) {
  // 闭包参数1
  // auto p1 = lua_tostring(L, lua_upvalueindex(1));
  // cout << "闭包参数1: " << p1 << endl;

  auto p1 = (uintptr_t*)lua_touserdata(L, lua_upvalueindex(1));
  auto pResponse = (Response*)*p1;

  // 设置 lua 函数返回值
  lua_pushlstring(L, (char*)pResponse->body.data, pResponse->body.size);
  return 1;
}

bool LuaScript::ResponseCallback(Response* pResponse) {
  if (!HasResponseCallback()) {
    cerr << "Error: "
         << "script not Response function" << endl;
    exit(1);
  }

  // 获取 Response 函数
  lua_getglobal(L, "Response");

  // 设置函数参数 response
  lua_newtable(L);

  // response.statusCode
  lua_pushstring(L, "statusCode");
  lua_pushinteger(L, pResponse->statusCode);
  lua_settable(L, -3);

  // response.headers
  auto _headers = pResponse->GetHeaders();
  lua_pushstring(L, "headers");
  lua_newtable(L);
  for (auto&& [k, v] : _headers) {
    lua_pushlstring(L, k.data(), k.size());
    lua_pushlstring(L, v.data(), v.size());
    lua_settable(L, -3);
  }
  lua_settable(L, -3);

  // response.body
  lua_pushstring(L, "body");
  lua_newtable(L);

  // response.body.size
  lua_pushstring(L, "size");
  lua_pushinteger(L, pResponse->body.size);
  lua_settable(L, -3);

  // response.body.text()
  lua_pushstring(L, "text");

  // 设置闭包参数为 pResponse
  // lua_pushstring(L, "闭包参数1");
  auto bp = (uintptr_t*)lua_newuserdatauv(L, sizeof(uintptr_t), 1);
  *bp = (uintptr_t)pResponse;
  lua_pushcclosure(L, lua_bodyText, 1);
  lua_settable(L, -3);

  lua_settable(L, -3);

  // response.responseSize
  lua_pushstring(L, "responseSize");
  lua_pushinteger(L, pResponse->responseSize);
  lua_settable(L, -3);

  // 调用函数，1个参数，1个返回值
  lua_call(L, 1, 1);

  // 获取返回值
  auto ok = lua_toboolean(L, -1);

  return ok;
}

void LuaScript::RunDoneCallback(RunResult* result) {
  if (!HasRunDoneCallback()) {
    cerr << "Error: "
         << "script not RunDone function" << endl;
    exit(1);
  }

  // 获取函数
  lua_getglobal(L, "RunDone");

  // 设置函数参数 result
  lua_newtable(L);

  // 设置 result.threadCount
  lua_pushstring(L, "threadCount");
  lua_pushinteger(L, result->threadCount);
  lua_settable(L, -3);

  // 设置 result.respDataCount
  lua_pushstring(L, "respDataCount");
  lua_pushinteger(L, result->respDataCount);
  lua_settable(L, -3);

  // 设置 result.successCount
  lua_pushstring(L, "successCount");
  lua_pushinteger(L, result->successCount);
  lua_settable(L, -3);

  // 设置 result.errorCount
  lua_pushstring(L, "errorCount");
  lua_pushinteger(L, result->errorCount);
  lua_settable(L, -3);

  // 设置 result.timeMs
  lua_pushstring(L, "timeMs");
  lua_pushinteger(L, result->time.count());
  lua_settable(L, -3);

  // 调用函数，1个参数，0个返回值
  lua_call(L, 1, 0);
}

void LuaScript::PresetRequestVariable(Request* pRequest) {
  // 全局变量 request table
  lua_newtable(L);

  lua_pushstring(L, "method");
  lua_pushlstring(L, pRequest->methodStr.data(), pRequest->methodStr.size());
  lua_settable(L, -3);

  lua_pushstring(L, "url");
  lua_pushlstring(L, pRequest->url.data(), pRequest->url.size());
  lua_settable(L, -3);

  lua_pushstring(L, "requestCount");
  lua_pushinteger(L, pRequest->requestCount);
  lua_settable(L, -3);

  lua_pushstring(L, "script");
  lua_pushlstring(L, path.data(), path.size());
  lua_settable(L, -3);

  lua_pushstring(L, "data");
  if (pRequest->data.empty())
    lua_pushstring(L, "");
  else
    lua_pushlstring(L, pRequest->data.data(), pRequest->data.size());
  lua_settable(L, -3);

  // multipart = { {name:"" , isFilePath: false, values: {""} }, .. }
  // 如果没有数据则设置为空table，避免nil
  lua_pushstring(L, "multipart");
  lua_newtable(L);
  for (size_t i = 0; i < pRequest->multipart.size(); i++) {
    auto& it = pRequest->multipart[i];
    // key
    lua_pushinteger(L, i);

    // value
    lua_newtable(L);
    lua_pushstring(L, "name");
    lua_pushlstring(L, it.name.data(), it.name.size());
    lua_settable(L, -3);

    lua_pushstring(L, "isFilePath");
    lua_pushboolean(L, it.isFilePath);
    lua_settable(L, -3);

    lua_pushstring(L, "values");
    lua_newtable(L);
    for (int j = 0; j < it.values.size(); j++) {
      lua_pushinteger(L, j);
      lua_pushlstring(L, it.values[j].data(), it.values[j].size());
      lua_settable(L, -3);
    }
    lua_settable(L, -3);

    // to multipart
    lua_settable(L, -3);
  }
  lua_settable(L, -3);

  // headers = { key:value, ... }
  // 如果没有数据则设置为空table，避免nil
  lua_pushstring(L, "headers");
  lua_newtable(L);
  for (auto&& [k, v] : pRequest->headers) {
    lua_pushlstring(L, k.data(), k.size());
    lua_pushlstring(L, v.data(), v.size());
    lua_settable(L, -3);
  }
  lua_settable(L, -3);

  lua_setglobal(L, "request");
}

void LuaScript::PresetDofile(Request* request) {
  // 加载脚本文件
  if (luaL_dofile(L, path.data()) != LUA_OK) {
    cerr << "open lua file error" << endl;
    exit(1);
  }
}

void LuaScript::PresetResponse(Request* pRequest) {
  // 如果脚本要处理response
  if (HasResponseCallback()) {
    pRequest->needRespHeader = true;
    pRequest->needRespBody = true;
  }
}

void LuaScript::PresetPreset(Request* pRequest) {
  // 调用lua预设函数
  lua_getglobal(L, "Preset");
  if (!lua_isfunction(L, -1)) return;

  lua_call(L, 0, 0);

  // 读取requst变化
  lua_getglobal(L, "request");

  lua_pushstring(L, "method");
  lua_gettable(L, -2);
  auto _methodStr = lua_tostring(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "url");
  lua_gettable(L, -2);
  auto _url = lua_tostring(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "requestCount");
  lua_gettable(L, -2);
  auto _requestCount = lua_tointeger(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "data");
  lua_gettable(L, -2);
  auto _data = lua_tostring(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "headers");
  lua_gettable(L, -2);
  // 遍历header table
  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    /* uses 'key' (at index -2) and 'value' (at index -1) */
    // printf("%s - %s\n", lua_typename(L, lua_type(L, -2)), lua_typename(L,
    // lua_type(L, -1)));
    auto k = lua_tostring(L, -2);
    auto v = lua_tostring(L, -1);

    // 如果key存在则替换value
    if (pRequest->headers.find(k) != pRequest->headers.end()) {
      pRequest->headers[k] = v;
    } else {
      pRequest->headers.insert({k, v});
    }

    /* removes 'value'; keeps 'key' for next iteration */
    lua_pop(L, 1);
  }
  lua_pop(L, 1);

  // TODO: multipart

  pRequest->methodStr = _methodStr;
  pRequest->url = _url;
  pRequest->requestCount = (uint32_t)_requestCount;
  pRequest->data = _data;
}

void LuaScript::Preset(Request* request) {
  PresetRequestVariable(request);
  PresetDofile(request);
  PresetResponse(request);
  PresetPreset(request);
}

LuaScript* LuaScript::Copy() {
  auto c = new LuaScript(path);

  return c;
}

HttpClint::HttpClint(Request* pRequest) : pRequest{pRequest} {
  hCurl = curl_easy_init();
  pResponse = new Response(pRequest->needRespHeader, pRequest->needRespBody);

  SetUrl();
  SetMethod();
  SetHeader();
  SetBody();

  // 返回的body
  curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, curlRespBodyCallback);
  curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, pResponse);

  // 返回的headers
  curl_easy_setopt(hCurl, CURLOPT_HEADERFUNCTION, curlRespHeaderCallback);
  curl_easy_setopt(hCurl, CURLOPT_HEADERDATA, pResponse);
}

HttpClint::~HttpClint() {
  if (pResponse != nullptr) delete pResponse;

  if (pHeaerSlist != nullptr) curl_slist_free_all(pHeaerSlist);

  if (pMultipart != nullptr) curl_mime_free(pMultipart);

  if (hCurl != nullptr) curl_easy_cleanup(hCurl);
}

inline void HttpClint::SetUrl() {
  if (pRequest->url.empty()) {
    cerr << "Error: request url empty" << endl;
    exit(1);
  }
  curl_easy_setopt(hCurl, CURLOPT_URL, pRequest->url.data());
}

inline void HttpClint::SetMethod() {
  auto m = pRequest->Method();
  CURLcode code = CURLE_OK;

  switch (m) {
    case METHOD::Head:
      code = curl_easy_setopt(hCurl, CURLOPT_NOBODY, 1L);
      break;

    case METHOD::Get:
      code = curl_easy_setopt(hCurl, CURLOPT_HTTPGET, 1L);
      break;

    case METHOD::Post:
      code = curl_easy_setopt(hCurl, CURLOPT_POST, 1L);
      break;

    case METHOD::Delete:
      code = curl_easy_setopt(hCurl, CURLOPT_CUSTOMREQUEST, "DELETE");
      break;

    case METHOD::Put:
      code = curl_easy_setopt(hCurl, CURLOPT_CUSTOMREQUEST, "PUT");
      break;

    case METHOD::Patch:
      code = curl_easy_setopt(hCurl, CURLOPT_CUSTOMREQUEST, "PATCH");
      break;

    default:
      cerr << "Error: set method not match " << (int)m << endl;
      exit(1);
      break;
  }

  if (code != CURLE_OK) {
    cerr << "Error: set mathod " << (int)m << " code " << code << endl;
    exit(1);
  }
}

void HttpClint::SetHeader() {
  if (pRequest->headers.empty()) return;

  for (auto&& [k, v] : pRequest->headers)
    pHeaerSlist =
        curl_slist_append(pHeaerSlist, (string(k) + ":" + string(v)).c_str());

  curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, pHeaerSlist);
}

void HttpClint::SetBody() {
  if (!pRequest->multipart.empty()) {
    pMultipart = curl_mime_init(hCurl);

    curl_mimepart* part = nullptr;

    for (auto&& i : pRequest->multipart) {
      auto name = i.name;
      for (auto&& value : i.values) {
        if (i.isFilePath) {
          filesystem::path p{value};

          part = curl_mime_addpart(pMultipart);
          curl_mime_name(part, name.data());
          curl_mime_filedata(part, p.filename().string().c_str());

          FILE* file = fopen(p.string().c_str(), "rb");
          if (!file) {
            cerr << "Error: open file " << p << endl;
            exit(1);
          }
          curl_mime_data_cb(part, filesystem::file_size(p),
                            (curl_read_callback)fread,
                            (curl_seek_callback)fseek, NULL, file);
        } else {
          part = curl_mime_addpart(pMultipart);
          curl_mime_name(part, name.data());
          // CURL_ZERO_TERMINATED 特殊size_t值，表示以空结尾的字符串
          curl_mime_data(part, value.data(), CURL_ZERO_TERMINATED);
        }
      }
    }

    curl_easy_setopt(hCurl, CURLOPT_MIMEPOST, pMultipart);
  }

  if (!pRequest->data.empty()) {
    curl_easy_setopt(hCurl, CURLOPT_POSTFIELDSIZE, pRequest->data.size());
    curl_easy_setopt(hCurl, CURLOPT_POSTFIELDS, pRequest->data.data());
  } else {
    curl_easy_setopt(hCurl, CURLOPT_POSTFIELDSIZE, 0);
  }
}

CURLcode HttpClint::Send() { return curl_easy_perform(hCurl); }

// 清理上一次请求的返回结果
inline void HttpClint::Clear() { pResponse->Clear(); }

inline Response* HttpClint::GetResponsePtr() {
  curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &pResponse->statusCode);
  return pResponse;
}

void blockHttpSend(Request* pRequest, LuaScript* pLuaScript) {
  LuaScript* _luaScript{nullptr};

  if (pLuaScript != nullptr && pLuaScript->HasResponseCallback()) {
    _luaScript = pLuaScript->Copy();
    _luaScript->PresetDofile(pRequest);
  }

  HttpClint clint{pRequest};
  CURLcode code;
  size_t _successCount{0}, _errorCount{0}, _respDataCount{0};

  for (; requestedCount < pRequest->requestCount;) {
    requestedCount++;

    clint.Clear();

    code = clint.Send();
    if (code) {
      errorCount++;
      continue;
    }

    auto pResp = clint.GetResponsePtr();
    _respDataCount += pResp->responseSize;

    bool isSuccess = _luaScript != nullptr && _luaScript->HasResponseCallback()
                         ? _luaScript->ResponseCallback(pResp)
                         : pResp->statusCode == 200;

    if (isSuccess)
      _successCount++;
    else
      _errorCount++;
  }

  successCount += _successCount;
  errorCount += _errorCount;
  respDataCount += _respDataCount;

  if (_luaScript != nullptr) delete _luaScript;
}

int run(Request* pRequest, RunResult* pResult) {
  LuaScript* pLuaScript{nullptr};

  if (!pRequest->scirptPath.empty()) {
    pLuaScript = new LuaScript(pRequest->scirptPath);
    pLuaScript->Preset(pRequest);
  }

  auto threadCount = min(max(thread::hardware_concurrency(), (uint32_t)2) - 1,
                         pRequest->requestCount);
  vector<thread> threads;

  auto startClock = chrono::steady_clock::now();
  for (size_t i = 0; i < threadCount; i++)
    threads.push_back(thread(blockHttpSend, pRequest, pLuaScript));
  for (auto&& i : threads) i.join();
  auto endClock = chrono::steady_clock::now();

  pResult->time =
      chrono::duration_cast<chrono::milliseconds>(endClock - startClock);
  pResult->threadCount = threadCount;
  pResult->requestedCount = (uint32_t)requestedCount;
  pResult->successCount = (uint32_t)successCount;
  pResult->errorCount = (uint32_t)errorCount;
  pResult->respDataCount = (size_t)respDataCount;

  if (pLuaScript != nullptr && pLuaScript->HasRunDoneCallback()) {
    pResult->hasRunDone = true;
    pLuaScript->RunDoneCallback(pResult);
  }

  if (pLuaScript != nullptr) delete pLuaScript;

  return 0;
}

}  // namespace oo
