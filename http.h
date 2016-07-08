#pragma once

#ifdef _MSC_VER
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#else
#include <curl/curl.h>
#endif

#include <memory>
#include <string>
#include "file.h"

class HttpRequest {
public:
  enum RequestType {GET, POST};

  HttpRequest(std::string const& url, RequestType type = GET);

  void addHeader(std::string const& name, std::string const& value);
  void addHeader(std::string const& header);
  void addData(std::string const& key, std::string const& value);

  bool send();
  uint32 status();
  std::map<std::string, std::string> headers();
  File response();

  static File get(std::string const& url);

private:
#ifdef _MSC_VER
  struct SessionHolder {
    HINTERNET session = nullptr;
    HINTERNET connect = nullptr;
    HINTERNET request = nullptr;
    ~SessionHolder();
  };
  friend class HttpBuffer;
  std::shared_ptr<SessionHolder> handles_;
  std::wstring headers_;
#else
  struct Response {
    std::string headers;
    MemoryFile data;
    uint32 code;

    struct curl_slist* request_headers = nullptr;

    ~Response();
  };
  std::string url_;
  std::unique_ptr<Response> response_;
#endif
  RequestType type_;
  std::string post_;
};
