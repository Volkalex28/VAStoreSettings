#pragma once

#include "linker.hpp"

class linkerFile
{
  using data_t = std::variant<linker::object_t, linker::array_t>;

  std::optional<data_t> data = std::nullopt;

  template<typename T>
  auto toJSON(const T & data, const bool is_short, int tabs) -> std::string
  {
    auto print_tabs = [is_short](const int count) -> std::string
    {
      if(!is_short)
      {
        std::string ret;
        for(int i = 0; i < count; i++)
          ret += "\t";
        return ret;
      }
      return "";
    };
    auto print_enter = [is_short]() -> std::string
    {
      return is_short ? "" : "\n";
    };
    [[maybe_unused]] auto print_space = [is_short]() -> std::string
    {
      return is_short ? "" : " ";
    };

    std::function<std::string(const linker &)> convert =
        [&](const linker & lnk)
    {
      std::string output;

      switch (lnk.type())
      {
      case linker::Types::Number: {
        std::string num = std::to_string(lnk.cast<linker::number_t>());
        int i;
        for(i = (int)num.length() - 1; i > 0; i--)
        {
          if(num[i] != '0')
          {
            if(num[i] == '.') i--;

            break;
          }
        }
        output += num.substr(0, i + 1);
      } break;
      case linker::Types::String: {
        auto str = lnk.cast<linker::string_t>();

        output += "\"";
        for(const auto & sym : str)
        {
            if(sym == '\\' || sym == '\"')
                output += "\\";
            output += sym;
        }
        output += "\"";
      } break;
      case linker::Types::Array: {
        output += this->toJSON(lnk.cast<linker::array_t>(), is_short, tabs);
      } break;
      case linker::Types::Object: {
        output += this->toJSON(lnk.cast<linker::object_t>(), is_short, tabs);
      } break;
      case linker::Types::Bool: {
        output += lnk.cast<linker::bool_t>() ? "true" : "false";
      } break;
      default: {
        output += "null";
      } break;
      }
      return output;
    };

    std::string symSubBraces;
    std::size_t counter = 1;
    std::size_t size    = data.size();

    if constexpr (is_linker_arr_v<T>) symSubBraces = "[]";
    else                              symSubBraces = "{}";

    std::string output = symSubBraces[0] + (size ? print_enter() + print_tabs(++tabs) : "");

    for(const auto & obj : data)
    {
      if constexpr (is_linker_obj_v<T>)
      {
        output += "\"" + obj.first + "\"" + print_space() + ":"
               + print_space() + convert(obj.second);
      }
      else if constexpr (is_linker_arr_v<T>)
      {
        output += convert(obj);
      }
      output += (counter++ < size ? "," + print_enter() + print_tabs(tabs)
                                  : print_enter());
    }
    output += (size ? print_tabs(--tabs) : "") + symSubBraces[1];

    if(tabs <= 0) this->data = data;

    return output;
  }

  void fromJSON(const std::string & input, std::optional<data_t> & data);

public:
  [[nodiscard]] auto isJSONArray() const -> bool;
  [[nodiscard]] auto isJSONObject() const -> bool;
  [[nodiscard]] auto isEmpty() const -> bool;

  auto toJSON(bool is_short = false) -> std::string;

  auto fromJSON(const std::string & input) -> linkerFile &;

  [[nodiscard]] auto getJSONObject() const -> linker::object_t;
  [[nodiscard]] auto getJSONArray() const -> linker::array_t;

  void setJSONObject(const linker::object_t & map);
  void setJSONArray(const linker::array_t & arr);
};
