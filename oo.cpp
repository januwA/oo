#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#endif

#include <iostream>
#include <algorithm>
#include <atomic>
#include "oo.h"

std::atomic_size_t requestedCount{0};
std::atomic_size_t successCount{0};
std::atomic_size_t errorCount{0};
std::atomic_size_t respDataCount{0};

namespace oo
{
    /**
     * 一旦收到需要保存的数据，libcurl就会调用此回调函数
     * data 一块数据
     * size 始终为1
     * nmemb data 的大小
     */
    size_t responseBodyCallback(void *data, size_t size, size_t nmemb, void *userp)
    {
        Response *mem = (Response *)userp;
        size_t realsize = size * nmemb;
        return mem->writeBody((uint8_t *)data, realsize);
    }

    size_t responseHeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata)
    {
        Response *resp = (Response *)userdata;
        return resp->writeHeader(buffer, nitems * size);
    }

    namespace utils
    {
        string_view trim(string_view src, char ignoreChar = ' ')
        {
            if (src.empty())
                return src;

            auto begin = src.find_first_not_of(ignoreChar);
            auto end = src.find_last_not_of(ignoreChar);

            if (begin == string::npos || end == string::npos)
                return string_view(src.data(), 0);

            return string_view(src.data() + begin, end - begin + 1);
        }
    }

    Response::~Response()
    {
        if (bodyBytes)
            free(bodyBytes);
    }

    size_t Response::writeBody(uint8_t *data, size_t size)
    {
        if (opt.needBody)
        {
            size_t newSize = bodySize + size;

            auto ptr = (uint8_t *)realloc(bodyBytes, newSize);
            if (ptr == NULL)
                return 0; /* 内存不足!，文件太大可以写入文件 */

            bodyBytes = ptr;
            memcpy(&(bodyBytes[bodySize]), data, size);
            bodySize = newSize;
        }

        responseByteSize += size;
        return size;
    }

    size_t Response::writeHeader(char *buffer, size_t size)
    {
        if (opt.needRespHeader)
            respHeaderString.append(buffer, size);

        responseByteSize += size;
        return size;
    }

    map<string_view, string_view> Response::headerMap()
    {
        map<string_view, string_view> respHeaderMap;
        size_t begin{0}, end{0}, count{0};

        // skip `HTTP/1.1 200 OK`
        begin = respHeaderString.find_first_of("\n", 0);

        if (begin == string::npos)
            return respHeaderMap;

        begin += sizeof(char);

        for (;;)
        {
            string_view k, v;

            // Content-Type:
            end = respHeaderString.find_first_of(":", begin);

            if (end == string::npos)
                break;

            count = end - begin;
            k = string_view(respHeaderString.data() + begin, count);

            /* skip `:` */
            end += sizeof(char);

            // text/plain; charset=utf-8
            begin = respHeaderString.find_first_of("\n", end);

            if (begin == string::npos)
                break;

            count = begin - end - sizeof(char);

            v = string_view(respHeaderString.data() + end, count);

            respHeaderMap.insert({utils::trim(utils::trim(k), '\n'),
                                  utils::trim(utils::trim(v), '\n')});
        }

        return respHeaderMap;
    }

    inline void Response::clear() noexcept
    {
        statusCode = 0;

        respHeaderString.clear();

        bodySize = 0;

        responseByteSize = 0;
    }

    METHOD methodHit(string_view method)
    {
        if (method == "head")
            return METHOD::HEAD;
        if (method == "get")
            return METHOD::GET;
        if (method == "post")
            return METHOD::POST;
        exit(1);
    }

    Request::Request() {}
    Request::Request(string_view url) : url{url} {}
    Request::Request(int argc, char *argv[])
    {
        for (size_t i = 1; i < argc; i++)
        {
            auto flag = argv[i];

            if (flag[0] == '-')
            {
                switch (flag[1])
                {
                case 'h':
                {
                    i++;
                    auto k = argv[i], v = argv[i + 1];
                    headers.insert({k, v});
                    i++;
                    break;
                }
                case 'd':
                {
                    i++;
                    if (flag[2] == 'f' || flag[2] == 'F')
                    {
                        bool isFilePath = flag[2] == 'F';
                        bool exists = false;
                        auto name = argv[i], value = argv[i + 1];

                        for (auto &&i : postMultipart)
                        {
                            if (i.name == name)
                            {
                                i.values.push_back(value);
                                i.isFilePath = isFilePath;
                                exists = true;
                                break;
                            }
                        }

                        if (!exists)
                        {
                            postMultipart.push_back({name, {value}, isFilePath});
                        }

                        i++;
                    }
                    else
                        postData = argv[i];

                    break;
                }
                case 'c':
                {
                    i++;
                    requestCount = atoi(argv[i]);
                    break;
                }
                case 'm':
                {
                    i++;
                    method = methodHit(argv[i]);
                    break;
                }

                default:
                    break;
                }
            }
            else
            {
                url = flag;
            }
        }
    }

    HttpClint::HttpClint(Request &request)
    {
        hCurl = curl_easy_init();
        response = new Response(request);

        setUrl(request.url);
        setMethod(request.method);
        setHeader(request.headers);
        setBody(request);

        // 返回的body
        curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, responseBodyCallback);
        curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, response);

        // 返回的headers
        curl_easy_setopt(hCurl, CURLOPT_HEADERFUNCTION, responseHeaderCallback);
        curl_easy_setopt(hCurl, CURLOPT_HEADERDATA, response);
    }

    HttpClint::~HttpClint()
    {
        if (response != nullptr)
            delete response;

        if (pHeaders != nullptr)
            curl_slist_free_all(pHeaders);

        if (multipart != nullptr)
            curl_mime_free(multipart);

        if (hCurl != nullptr)
            curl_easy_cleanup(hCurl);
    }

    inline void HttpClint::setUrl(string_view url)
    {
        curl_easy_setopt(hCurl, CURLOPT_URL, url.data());
    }

    inline void HttpClint::setMethod(METHOD method)
    {
        switch (method)
        {
        case METHOD::HEAD:
            curl_easy_setopt(hCurl, CURLOPT_NOBODY, 1L);
            break;

        case METHOD::GET:
            curl_easy_setopt(hCurl, CURLOPT_HTTPGET, 1L);
            break;

        case METHOD::POST:
            curl_easy_setopt(hCurl, CURLOPT_POST, 1L);
            break;

        default:
            cerr << "Error: setCurlMethod not match method" << endl;
            exit(1);
            break;
        }
    }

    void HttpClint::setHeader(map<string_view, string_view> &headers)
    {
        if (headers.empty())
            return;

        for (auto &&[k, v] : headers)
            pHeaders = curl_slist_append(pHeaders, (string(k) + ": " + string(v)).c_str());

        curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, pHeaders);
    }

    void HttpClint::setBody(Request &opt)
    {
        if (!opt.postMultipart.empty())
        {
            multipart = curl_mime_init(hCurl);

            curl_mimepart *part = nullptr;

            for (auto &&i : opt.postMultipart)
            {
                auto name = i.name;
                for (auto &&value : i.values)
                {
                    if (i.isFilePath)
                    {
                        filesystem::path p{value};

                        part = curl_mime_addpart(multipart);
                        curl_mime_name(part, name.data());
                        curl_mime_filedata(part, p.filename().string().c_str());

                        FILE *file = fopen(p.string().c_str(), "rb");
                        if (!file)
                        {
                            cerr << "Error: open file " << p << endl;
                            exit(1);
                        }
                        curl_mime_data_cb(part, filesystem::file_size(p), (curl_read_callback)fread,
                                          (curl_seek_callback)fseek, NULL, file);
                    }
                    else
                    {
                        part = curl_mime_addpart(multipart);
                        curl_mime_name(part, name.data());
                        // CURL_ZERO_TERMINATED 特殊size_t值，表示以空结尾的字符串
                        curl_mime_data(part, value.data(), CURL_ZERO_TERMINATED);
                    }
                }
            }

            curl_easy_setopt(hCurl, CURLOPT_MIMEPOST, multipart);
        }

        if (!opt.postData.empty())
        {
            curl_easy_setopt(hCurl, CURLOPT_POSTFIELDSIZE, opt.postData.size());
            curl_easy_setopt(hCurl, CURLOPT_POSTFIELDS, opt.postData.data());
        }
    }

    inline CURLcode HttpClint::send()
    {
        return curl_easy_perform(hCurl);
    }

    // 清理上一次请求的返回结果
    inline void HttpClint::clear()
    {
        response->clear();
    }

    inline Response *HttpClint::getResposne()
    {
        curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response->statusCode);
        return response;
    }

    void blockSend(Request &request)
    {
        HttpClint clint{request};
        CURLcode code;
        size_t _successCount{0}, _errorCount{0}, _respDataCount{0};

        for (;;)
        {
            if (requestedCount >= request.requestCount)
                break;

            requestedCount++;

            clint.clear();

            code = clint.send();

            if (code)
            {
                errorCount++;
                continue;
            }

            auto resp = clint.getResposne();

            _respDataCount += resp->responseByteSize;

            if (resp->statusCode == 200)
                _successCount++;
            else
                _errorCount++;
        }

        successCount += _successCount;
        errorCount += _errorCount;
        respDataCount += _respDataCount;
    }

    int run(Request &request, runResult &result)
    {
        auto threadCount = min(max(thread::hardware_concurrency(), (uint32_t)2) - 1, request.requestCount);
        vector<thread> threads;

        auto startDate = chrono::steady_clock ::now();
        for (size_t i = 0; i < threadCount; i++)
            threads.push_back(thread(blockSend, ref(request)));
        for (auto &&i : threads)
            i.join();
        auto endDate = chrono::steady_clock::now();

        result.time = chrono::duration_cast<chrono::milliseconds>(endDate - startDate);
        result.threadCount = threadCount;
        result.requestedCount = (uint32_t)requestedCount;
        result.successCount = (uint32_t)successCount;
        result.errorCount = (uint32_t)errorCount;
        result.respDataCount = (size_t)respDataCount;

        return 0;
    }

}