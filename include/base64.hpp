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
#ifndef SOTPIDER_BASE64_HPP
#define SOTPIDER_BASE64_HPP
// adapted from https://github.com/ReneNyffenegger/cpp-base64
#include <iostream>
#include <string>
#include <array>
#include <unordered_map>
namespace sp
{
  std::string base64_encode(const std::string &data)
  {
    static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    // In this program, the data is just user:password, and couldn't change frequently.
    static std::unordered_map<std::string, std::string> cache;
    if (auto it = cache.find(data); it != cache.end()) return it->second;
    std::string ret;
  
    int i = 0;
    int j = 0;
    int k = 0;
    unsigned char chars_3[3];
    unsigned char chars_4[4];
    int size = data.size();
    while (size--)
    {
      chars_3[i++] = data[k++];
      if (i == 3)
      {
        chars_4[0] = (chars_3[0] & 0xfc) >> 2;
        chars_4[1] = ((chars_3[0] & 0x03) << 4) + ((chars_3[1] & 0xf0) >> 4);
        chars_4[2] = ((chars_3[1] & 0x0f) << 2) + ((chars_3[2] & 0xc0) >> 6);
        chars_4[3] = chars_3[2] & 0x3f;
        for (i = 0; (i < 4); i++)
          ret += base64_chars[chars_4[i]];
        i = 0;
      }
    }
  
    if (i)
    {
      for (j = i; j < 3; j++)
        chars_3[j] = '\0';
  
      chars_4[0] = (chars_3[0] & 0xfc) >> 2;
      chars_4[1] = ((chars_3[0] & 0x03) << 4) + ((chars_3[1] & 0xf0) >> 4);
      chars_4[2] = ((chars_3[1] & 0x0f) << 2) + ((chars_3[2] & 0xc0) >> 6);
  
      for (j = 0; (j < i + 1); j++)
        ret += base64_chars[chars_4[j]];
  
      while ((i++ < 3))
        ret += '=';
    }
    
    cache[data] = ret;
    return ret;
  }
}
#endif