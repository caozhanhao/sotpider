//   Copyright 2023 sotpider - caozhanhao
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
#ifndef SOTPIDER_HTTP_HPP
#define SOTPIDER_HTTP_HPP
#include "error.hpp"
#include "curl/curl.h"
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <utility>
#include <fstream>
#include <iostream>
#include <thread>
namespace sp
{
  class Bar
  {
  private:
    char padding;
    char progress;
    std::size_t size;
    std::size_t finished_size;
    std::size_t last_addition_size;
    bool first;
  public:
    Bar(std::size_t size_ = 36, char progress_ = '>', char padding_ = ' ')
        : padding(padding_), progress(progress_), size(size_), finished_size(0),
          last_addition_size(0), first(true) {}
    Bar& set(std::size_t size_, char progress_, char padding_)
    {
      size = size_;
      progress = progress_;
      padding = padding_;
      return *this;
    }
    Bar& update(double achieved_ratio, const std::string addition = "")
    {
      if (first)
      {
        std::cout << "[" << std::string(size, padding) << "]"
                  << add(addition) << std::flush;
        first = false;
      }
      if (finished_size < size && size * achieved_ratio - finished_size > 0.5)
      {
        finished_size++;
        std::string d(last_addition_size + size + 2, '\b');
        std::string output = "[";
        output += std::string(finished_size, progress);
        output += std::string(size - finished_size, padding);
        output += "]";
        output += add(addition);
        std::cout << d << output << std::flush;
      }
      else
      {
        std::string d(last_addition_size, '\b');
        auto p = add(addition);
        std::cout << d << p << std::flush;
      }
      return *this;
    }
  private:
    std::string add(const std::string& addition)
    {
      std::string opt;
      if (addition.size() < last_addition_size)
        opt += std::string(last_addition_size - addition.size(), ' ');
      last_addition_size = addition.size();
      return addition + opt;
    };
  };
  class Response
  {
  private:
    std::variant<int,
        std::shared_ptr<std::string>,
        std::shared_ptr<std::fstream>> value;//int EMPTY
  public:
    Response() : value(0) {}
    
    Response(const Response &res) : value(res.value) {}
    
    void set_str()
    {
      value.emplace<std::shared_ptr<std::string>>
          (std::make_shared<std::string>
               (std::string()));
    }
    
    void set_file(const std::string &filename)
    {
      auto f = std::make_shared<std::fstream>
          (std::fstream(filename,
                        std::ios_base::out
                        | std::ios_base::trunc
                        | std::ios_base::in
                        | std::ios_base::binary));
      sp::sp_assert(f->is_open(), "Open file failed.");
      value.emplace<std::shared_ptr<std::fstream>>
          (f);
    }
    
    std::string str() { return *std::get<1>(value); }
    
    std::shared_ptr<std::string> strp() { return std::get<1>(value); }
    
    std::shared_ptr<std::fstream> file() { return std::get<2>(value); }
    
    bool empty() { return value.index() == 0; }
  };
  
  std::size_t str_write_callback(void *data, size_t size, size_t nmemb, void *userp)
  {
    auto p = (Response *) userp;
    p->strp()->append((char *) data, size * nmemb);
    return size * nmemb;
  }
  
  std::size_t file_write_callback(void *data, size_t size, size_t nmemb, void *userp)
  {
    auto p = (Response *) userp;
    p->file()->write((char *) data, size * nmemb);
    return size * nmemb;
  }
  
  static size_t progress_callback(void *clientp,
                                  double dltotal,
                                  double dlnow,
                                  double ultotal,
                                  double ulnow)
  {
    auto *d = static_cast<Bar*>(clientp);
    d->update(dlnow / dltotal);
    return 0;
  }
  
  class Http
  {
  public:
    enum class FormType { String, File };
    using Form = std::vector<std::tuple<FormType, std::string, std::string>>;
  public:
    Response response;
    long int response_code;
  private:
    CURL *curl;
    struct curl_slist *headers;
    Bar bar;
  public:
    Http() : response_code(-1), curl(curl_easy_init()), headers(nullptr)
    {
      sp_assert(curl != nullptr, "curl_easy_init() failed");
    }
    
    Http(const std::string &url_) : response_code(-1), curl(curl_easy_init()), headers(nullptr)
    {
      sp_assert(curl != nullptr, "curl_easy_init() failed");
      set_url(url_);
    }
    
    Http(const Http &http) = delete;
    
    ~Http()
    {
      curl_easy_cleanup(curl);
      if(headers != nullptr)
        curl_slist_free_all(headers);
    }
    
    Http &set_url(const std::string &url_)
    {
      auto url = url_;
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      return *this;
    }
    Http &set_timeout(int sotpider_timeout)
    {
      sp_assert(sotpider_timeout > 0);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, sotpider_timeout);
      return *this;
    }
    Http &set_header(const std::vector<std::string> &header)
    {
      sp_assert(!header.empty(), "Empty header.");
      for (auto &r: header)
        headers = curl_slist_append(headers, r.c_str());
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      return *this;
    }
    Http &set_bar()
    {
      curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &bar);
      curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
      return *this;
    }
    Http &set_str()
    {
      response.set_str();
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, str_write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      return *this;
    }
    
    Http &set_file(const std::string &filename)
    {
      response.set_file(filename);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      return *this;
    }
    
    Http &get()
    {
      if (response.empty())
        set_str();
      curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
      auto cret = curl_easy_perform(curl);
      sp::sp_assert(cret == CURLE_OK,
                    "curl_easy_perform() failed: '" + std::string(curl_easy_strerror(cret)) + "'.");
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
      return *this;
    }
    
    Http &post(const Form &form)
    {
      if (response.empty())
        set_str();
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
  
      curl_mime *mime  = curl_mime_init(curl);
      curl_mimepart *part;
      for (auto &r: form)
      {
        auto&[form_type, name, content] = r;
        part = curl_mime_addpart(mime);
        switch (form_type)
          {
            case FormType::String:
              curl_mime_name(part, name.c_str());
              curl_mime_data(part, content.c_str(),CURL_ZERO_TERMINATED);
              break;
            case FormType::File:
              curl_mime_name(part, name.c_str());
              curl_mime_filedata(part, content.c_str());
              break;
            default:
              sp_unreachable();
              break;
          }
      }
      curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
      auto cret = curl_easy_perform(curl);
      sp::sp_assert(cret == CURLE_OK,
                    "curl_easy_perform() failed: '" + std::string(curl_easy_strerror(cret)) + "'.");
      curl_mime_free(mime);
      return *this;
    }
  };
}
#endif