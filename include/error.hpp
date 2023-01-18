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
#ifndef SOTPIDER_ERROR_HPP
#define SOTPIDER_ERROR_HPP
#include <string>
#include <stdexcept>
#include <experimental/source_location>
namespace sp
{
  std::string location_to_str(const std::experimental::source_location &l)
  {
    return std::string(l.file_name()) + ":" + std::to_string(l.line()) +
           ":" + l.function_name() + "()";
  }
  
  class Error : public std::logic_error
  {
  private:
    std::string location;
    std::string detail;
  
  public:
    Error(const std::string &detail_, const std::experimental::source_location &l =
    std::experimental::source_location::current())
        : logic_error(detail_),
          location(location_to_str(l)),
          detail(detail_) {}
    
    [[nodiscard]] std::string get_content() const
    {
      return "\033[0;32;31mError: \033[m" + location + ":\033[m " + detail;
    }
  };
  
  auto sp_unreachable(const std::string &detail_ = "", const std::experimental::source_location &l =
  std::experimental::source_location::current())
  {
    throw Error("Unreachable code: " + detail_, l);
  }
  
  void sp_assert(bool b,
                 const std::string &detail_ = "Assertion failed.",
                 const std::experimental::source_location &l =
                 std::experimental::source_location::current())
  {
    if (!b)
    {
      throw Error(detail_, l);
    }
  }
}
#endif