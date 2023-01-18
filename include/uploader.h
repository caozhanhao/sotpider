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
#ifndef SOTPIDER_UPLOADER_HPP
#define SOTPIDER_UPLOADER_HPP
#include "post.hpp"
#include "base64.hpp"
#include <rapidjson/document.h>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>
#include <map>
#include <filesystem>
namespace sp
{
  std::string get_timestamp()
  {
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp
        = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>
                              (tp.time_since_epoch()).count());
  }
  
  class Uploader
  {
  private:
    std::string server;
    std::string user;
    std::string passwd;
    std::map<std::string, int> tags_cache;
    int timeout;
  public:
    Uploader(std::string server_, const std::string &username, const std::string &password, int timeout_ = -1)
        :server(std::move(server_)), user(std::move(username)), passwd(std::move(password)), timeout(timeout_)
    {
    }
    
    int get_tag_id(const std::string &tag)
    {
      auto it = tags_cache.find(tag);
      if (it != tags_cache.end()) return it->second;
      int tag_id = 0;
      std::string url = server + "/?rest_route=/wp/v2/tags";
      Http h{url};
      if(timeout != -1) h.set_timeout(timeout);
      h.set_header({"Authorization: Basic " + base64_encode(user + ":" + passwd)});
      h.set_str().post(
          {
              {Http::FormType::String, "name", tag},
          }
      );
      auto &res = *h.response.strp();
      rapidjson::Document tag_make;
      tag_make.Parse(res.c_str());
      if (tag_make.FindMember("code") != tag_make.MemberEnd())
        tag_id = tag_make["data"]["term_id"].GetInt();//already exist, but not in cache
      else
        tag_id = tag_make["id"].GetInt();
      tags_cache[tag] = tag_id;
      return tag_id;
    }
    void upload(const Post &t)
    {
      // download
      std::string resources;
      int feature = 0;
      if(!t.get_url().empty())
      {
        auto medias = get_media_content(t);
        for(auto& r : medias)
          resources += r.second + "\n";
        feature = medias[0].first;
      }
      std::cout << "Uploading " << std::endl;
      // upload
      std::string url = server + "/?rest_route=/wp/v2/posts";
      std::string tagstr;
      auto plain_tags = t.get_tags();
      plain_tags.emplace_back(t.get_userid());
      for (size_t i = 0; i < plain_tags.size(); ++i)
        tagstr += std::to_string(get_tag_id(plain_tags[i])) + ",";
      if (!tagstr.empty())
        tagstr.pop_back();
      Http h{url};
      if(timeout != -1) h.set_timeout(timeout);
      h.set_header({"Authorization: Basic " + base64_encode(user + ":" + passwd)});
      std::string title = t.get_user() + "-" + get_timestamp();
      Http::Form form {
          {Http::FormType::String, "title",   title},
          {Http::FormType::String, "content", t.get_text() + "\n" + resources},
          {Http::FormType::String, "status",  "publish"},
          {Http::FormType::String, "tags",    tagstr}
      };
      if(!resources.empty())
        form.emplace_back(Http::FormType::String, "featured_media", std::to_string(feature));
      h.set_str().post(form);
      auto &res = *h.response.strp();
      sp_assert(res.substr(0, 8) != "{\"code\":", "Unexpected response: Response code: " + std::to_string(h.response_code)
                                               + ", Response: \n" + res);
    }
  
  private:
    std::string get_home()
    {
      char const *home = getenv("HOME");
      if (home == nullptr) home = getenv("USERPROFILE");
      if (home == nullptr) home = getenv("HOMEDRIVE");
      if (home == nullptr) home = getenv("HOMEPATH");
      sp_assert(home != nullptr);
      std::string h{home};
      if (h.back() == '/') h.pop_back();
      return h;
    }
  public:
    std::vector<std::pair<int, std::string>> get_media_content(const Post& post)
    {
      auto paths = cache(post);
      std::vector<std::pair<int, std::string>> ret;
      for (auto &p: paths)
      {
        auto path = std::filesystem::path(p);
        auto fn = path.filename().string();
        auto remove_filename = path.remove_filename().string();
        auto timestamp = remove_filename.substr(remove_filename.rfind("/", remove_filename.size() - 2) + 1);
        timestamp.pop_back();
        std::string url = server + "/?rest_route=/wp/v2/media";
        Http h{url};
        if(timeout != -1) h.set_timeout(timeout);
        h.set_header({"Authorization: Basic " + base64_encode(user + ":" + passwd),
                      "Content-Disposition: attachment;filename=" + fn});
        std::string title = timestamp + "-" + fn;
        h.set_str().post(
            {
                {Http::FormType::String, "title", title},
                {Http::FormType::File,   "file",  p}
            }
        );
        auto &res = *h.response.strp();
        sp_assert(res.substr(0, 8) != "{\"code\":",
                  "Unexpected response: Response code: " + std::to_string(h.response_code)
                  + ", Response: \n" + res);
        rapidjson::Document json;
        json.Parse(res.c_str());
        ret.emplace_back(json["id"].GetInt(), json["description"]["rendered"].GetString());
      }
      std::filesystem::remove_all(std::filesystem::path(paths[0]).remove_filename());
      return ret;
    }
    std::vector<std::string> cache(const Post& t)
    {
      sp_assert(!t.get_url().empty());
      auto ts = get_timestamp();
      auto dpath = get_home() + "/.sotpider/";
      if (!std::filesystem::exists(dpath))
        std::filesystem::create_directory(dpath);
      std::filesystem::path p{dpath + ts + "/"};
      std::filesystem::create_directory(p);
      std::vector<std::string> paths;
      for (size_t i = 0; i < t.get_url().size(); ++i)
      {
        std::string ext = ".";
        switch (t.get_url()[i].first)
        {
          case Post::Type::Image:
            ext += "jpg";
            break;
          case Post::Type::Video:
            ext += "mp4";
            break;
          default:
            break;
        }
        paths.emplace_back(p.string() + std::to_string(i) + ext);
      }
      t.download(timeout, paths);
      return paths;
    }
  };
}
#endif