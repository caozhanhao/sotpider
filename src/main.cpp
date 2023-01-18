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
#include "sotpider.hpp"
#include "libczh/czh.hpp"
#include <iostream>
#include <string>

int main()
{
  czh::Czh e("config.czh", czh::InputMode::nonstream);
  auto node = e.parse();
  size_t id_pos = 0;
  size_t post_pos =  0;
  std::vector<sp::Post> posts;
  int timeout = node["config"]["timeout"].is<czh::value::Null>() ? -1 : node["config"]["timeout"].get<int>();
  sp::Uploader uploader{node["config"]["to_server"].get<std::string>(),
                        node["config"]["username"].get<std::string>(),
                        node["config"]["password"].get<std::string>(),
                        timeout};
  sp::PostGetter vg{
      node["config"]["from_server"].get<std::string>(),
      node["config"]["download_server"].get<std::string>(),
      timeout
  };
  auto ids = node["config"]["search_id"].get <std::vector<std::string>>();
  auto fp = node["config"]["from_page"].get<int>();
  auto ep = node["config"]["end_page"].is<czh::value::Null>() ? -1 : node["config"]["end_page"].get<int>();
  bool done = false;
  while(!done)
  {
    try
    {
      for (;id_pos < ids.size(); ++id_pos)
      {
        auto tmp = vg.fetch_video(ids[id_pos], {fp, ep});
        posts.insert(posts.end(), std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()));
      }
      for (; post_pos < posts.size(); ++post_pos)
      {
        posts[post_pos].print();
        uploader.upload(posts[post_pos]);
        std::cout << "-------------------------------------------\n";
      }
      done = true;
    }
    catch (sp::Error &e)
    {
      std::cerr << e.get_content() << std::endl;
    }
    catch (...)
    {
      std::cerr << "Unexpected Error." << std::endl;
    }
  }
  return 0;
}

