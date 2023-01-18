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
#ifndef SOTPIDER_POST_HPP
#define SOTPIDER_POST_HPP
#include "http.hpp"
#include "error.hpp"
#include "curl/curl.h"
#include <vector>
#include <string_view>
#include <string>
#include <iostream>
#include <rapidjson/document.h>
namespace sp
{
  class Post
  {
  public:
    enum class Type { Empty, Image, Video, Text };
  private:
    std::vector<std::string> tags;
    std::string user;
    std::string userid;
    std::string text;
    std::vector<std::pair<Type, std::string>> urls;
  public:
    Post(std::string user_, std::string userid_, std::string text_, std::vector<std::string> tags_,
         std::vector<std::pair<Type, std::string>> url_)
        : tags(std::move(tags_)), user(user_), userid(userid_),
          text(std::move(text_)), urls(std::move(url_)) {}
    
    const auto &get_tags() const { return tags; }
    
    const auto &get_text() const { return text; }
    
    const auto &get_url() const { return urls; }
    
    const auto &get_user() const { return user; }
    
    const auto &get_userid() const { return user; }
    
    void download(int timeout, const std::vector<std::string> &paths = {}) const
    {
      for (size_t i = 0; i < urls.size(); ++i)
      {
        std::cout << "Downloading: " << i << "/" << urls.size() << std::endl;
        std::string filename;
        if (i < paths.size())
          filename = paths[i];
        else
          filename = (paths.empty() ? text : paths.back()) + std::to_string(i - paths.size());
        Http h{urls[i].second};
        if(timeout != -1) h.set_timeout(timeout);
        h.set_file(filename);
        //h.set_bar();
        h.get();
      }
      std::cout << "Downloading: " << urls.size() << "/" << urls.size() << std::endl;
    }
    
    void print() const
    {
      std::cout << "User: " << user << "\n";
      if (!text.empty())
        std::cout << "Text: \n" << text << "\n";
      if (!tags.empty())
      {
        std::cout << "Tags: \n";
        std::string tagstr;
        for (auto it = tags.cbegin(); it < tags.cend(); ++it)
          tagstr += *it + " | ";
        if (!tagstr.empty())
          tagstr.erase(tagstr.size() - 3);
        std::cout << tagstr << "\n";
      }
      if (!urls.empty())
      {
        std::cout << "Url: \n";
        for (auto &m: urls)
        {
          switch (m.first)
          {
            case Type::Image:
              std::cout << "Image";
              break;
            case Type::Video:
              std::cout << "Video";
              break;
            case Type::Text:
              std::cout << "Text";
              break;
            case Type::Empty:
              sp_unreachable("Unexpected type: Type::Empty");
              break;
            default:
              sp_unreachable();
              break;
          }
          std::cout << "  |  " << m.second << "\n";
        }
      }
    }
  };
  
  class PostGetter
  {
  private:
    std::string server;
    std::string download_server;
    CURL *curl;
    int timeout;
  public:
    PostGetter(std::string server_, std::string download_server_, int timeout_ = -1)
    : server(std::move(server_)), download_server(std::move(download_server_)), curl(curl_easy_init()),
    timeout(timeout_)
    {
      sp_assert(curl != nullptr, "curl_easy_init() failed.");
    }
    
    ~PostGetter()
    {
      curl_easy_cleanup(curl);
    }
    
    std::vector<Post> fetch_video(const std::string &id, const std::pair<int, int> &page_range = {1, -1})
    //[page beg, page end)
    {
      sp_assert(page_range.first >= 1 && (page_range.second == -1 || page_range.first < page_range.second),
                "Invalid range");
      std::vector<Post> ret;
      size_t page = page_range.first;
      while (true)
      {
        if (page_range.second != -1 && page >= page_range.second) break;
        std::cout << "Fetching " + id + "'s page " << page << std::endl;
        std::string url =
            std::string(server) + "/v2/user/" + id + "/?after=" + std::to_string(page) + "&page=" + std::to_string(page);
        Http h{url};
        if(timeout != -1) h.set_timeout(timeout);
        h.set_header(
            {"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/109.0.0.0 Safari/537.36 Edg/109.0.1518.52"});
        auto resp = h.set_str().get().response.strp();
        sp_assert(resp != nullptr, "No Response.");
        auto &res = *resp;
        if (h.response_code != 200)
        {
          if (page == 1)  // Page 1 must be available
          {
            sp_unreachable("Unexpected response: Response code: " + std::to_string(h.response_code)
                           + ", Response: \n" + res);
          }
          else if (h.response_code != 404)
          {
            sp_unreachable("Unexpected response: Response code: " + std::to_string(h.response_code)
                           + ", Response: \n" + res);
          }
          else
            break;// 404 -> no more page
        }
        rapidjson::Document doc;
        doc.Parse(res.c_str());
        auto &d = doc["data"];
        for (auto it = d.Begin(); it != d.End(); ++it)
        {
          // user
          std::string user;
          if (it->FindMember("user") != it->MemberEnd())
            user = (*it)["user"]["name"].GetString();
          else
            user = id;
          // text
          std::string text = (*it)["text"].GetString();
          // tags
          std::vector<std::string> tags;
          auto tagsit = it->FindMember("tagEntities");
          if (tagsit != it->MemberEnd())
            for (auto it = tagsit->value.Begin(); it != tagsit->value.End(); ++it)
              tags.emplace_back((*it)["text"].GetString());
          // download url
          std::vector<std::pair<Post::Type, std::string>> download_urls;
          if (auto m = it->FindMember("mediaEntities"); m != it->MemberEnd() && !m->value.Empty())
          {
            for (size_t i = 0; i < m->value.Size(); ++i)
            {
              if (auto v = m->value[i].FindMember("videoInfo"); v != m->value[i].MemberEnd())
              {
                download_urls.emplace_back(
                    Post::Type::Video,
                    download_server + "/download?url=" + escape(v->value["variants"][i]["url"].GetString()));
              }
              else if (auto p = m->value[i].FindMember("mediaURL"); p != m->value[i].MemberEnd())
              {
                download_urls.emplace_back(
                    Post::Type::Image,  download_server + "/download?url=" + escape(p->value.GetString()));
              }
              else
                sp_unreachable();
            }
          }
          ret.emplace_back(std::move(user), id, std::move(text), std::move(tags), std::move(download_urls));
        }
        page++;
      }
      return ret;
    }
  
  private:
    std::string escape(const std::string str)
    {
      char *output = curl_easy_escape(curl, str.c_str(), str.size());
      sp_assert(output != nullptr, "curl_easy_escape() failed.");
      std::string ret{output};
      curl_free(output);
      return ret;
    }
  };
}
#endif