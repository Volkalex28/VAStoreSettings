#include <string_view>

#include "serializer.hpp"
#include "linker_file.hpp"

void linkerFile::fromJSON(const std::string & input, std::optional<data_t> & data)
{
  data_t rdata;

  std::size_t braces    = 0;
  std::size_t subBraces = 0;
  std::string symSubBraces;
  bool colon  = false;     /*   :   */
  bool string = false;     /* "..." */
//  bool array  = false;     /* [,,,] */
  bool quotes = false;     /*   /    */

  std::string str;
  std::string name;

  auto isArray = [&rdata]
  {
    return rdata.index() == 1;
  };

  auto getValueFromVariant = [](auto & variant)
  {
    if(variant.index())
      return (linker() << std::get<1>(variant));

    return (linker() << std::get<0>(variant));
  };

  auto init = [&](bool isArray)
  {
    if(isArray) rdata = linker::array_t();
    else        rdata = linker::object_t();
  };
  auto save = [&](const linker & lnk)
  {
    if(isArray()) std::get<1>(rdata).push_back(lnk);
    else          std::get<0>(rdata)[name] = lnk;
  };
  auto retValues = [&]()
  {
    if(isArray())
    {
      data = std::get<1>(rdata);
    }
    else
    {
      data = std::get<0>(rdata);
    }
  };

  for(auto sym : input)
  {
    if((sym == '\t' || sym == '\n' || sym == '\r' || sym == ' ') && !string) continue;

    if(braces)
    {
      if(sym == '\"' || string)
      {
        if(!string)
        {
          string = true;

          if(!subBraces) str = "";
          else           str += sym;

          continue;
        }

        if(!quotes)
        {
          if(sym == '\\' && !subBraces)
          {
            quotes = true;
            continue;
          }
          if(sym == '\"')
          {
            string = false;

            if(!subBraces)
            {
              if(!colon && !isArray())
              {
                name = str;
              }
              else save(linker::from(str));

              str = "";
              continue;
            }
          }
        }
        else
        {
          quotes = false;
        }

        str += sym;
      }
      else if (((colon || isArray()) && (sym == '[' || sym == '{')) || subBraces)
      {
        if(!subBraces)
        {
          str = "";
          symSubBraces = "";
        }

        if(symSubBraces.empty())
        {
          symSubBraces = (sym == '[' ? std::string_view("[]") : std::string_view("{}"));
        }

        if(sym == symSubBraces[0]) subBraces++;
        else if(sym == symSubBraces[1]) subBraces--;

        str += sym;

        if(subBraces) continue;

        std::optional<data_t> data;

        fromJSON(str, data);
        str = "";

        if(!data) continue;

        save(linker::from(getValueFromVariant(*data)));
      }
      else if(colon || isArray())
      {
        if(!subBraces && ((!isArray() && sym == '}') || (isArray() && sym == ']'))) braces--;

        if(sym == ',' || !braces)
        {
          colon = false;

          if(str.empty()) continue;

          if(str.substr(0, 4) == "true" || str.substr(0, 5) == "false")
          {
            bool value = (str.substr(0, 4) == "true");
            save(linker::from(value));
          }
          else if (str.substr(0, 4) == "null")
          {
            save(linker::from(linker::null_t()));
          }
          else
          {
          try
          {
            linker::number_t value = std::stold(str);
            save(linker() << value);
            // std::cout << "{" << value << "|" << name << "}" << std::endl;
          }
          catch(...)
          {
//           std::cout << name << "|" << str << "|" << std::endl;
          }
          }
          str = "";
        }
        else
        {
          str += sym;
        }
      }
      else if(sym == ':')
      {
        str = "";
        colon = true;
      }
    }
    else
    {
      if(sym == '{' || sym == '[')
      {
        init(sym == '[');
        braces++;

        continue;
      }
      return;
    }
  }

  retValues();
}

auto linkerFile::isJSONArray() const -> bool
{
  if(this->data) return (std::get_if<1>(&*this->data) != nullptr);
  return false;
}
auto linkerFile::isJSONObject() const -> bool
{
  if(this->data) return (std::get_if<0>(&*this->data) != nullptr);
  return false;
}
auto linkerFile::isEmpty() const -> bool
{
  return !this->data;
}

auto linkerFile::toJSON(bool is_short) -> std::string
{
  switch(this->data->index())
  {
  case 0: return this->toJSON<linker::object_t>(std::get<0>(*this->data), is_short, 0); break;
  case 1: return this->toJSON<linker::array_t>(std::get<1>(*this->data), is_short, 0); break;
  }
  return "";
}

auto linkerFile::fromJSON(const std::string & input) -> linkerFile &
{
  this->data = std::nullopt;
  this->fromJSON(input, this->data);

  return *this;
}

auto linkerFile::getJSONObject() const -> linker::object_t
{
  if(!this->isEmpty())
  {
    return std::get<0>(*this->data);
  }
  return {};
}
auto linkerFile::getJSONArray() const -> linker::array_t
{
  if(!this->isEmpty())
  {
    return std::get<1>(*this->data);
  }
  return {};
}

void linkerFile::setJSONObject(const linker::object_t & map)
{
  this->data = map;
}
void linkerFile::setJSONArray(const linker::array_t & arr)
{
  this->data = arr;
}


auto linker::operator>>(Serializer & object) const -> Serializer &
{
  if (Types::Object != this->m_type)
    return object;

  for (auto arrpProps = object.getPropertysArray(); auto & prop : arrpProps)
    prop->copy_from(this->value<object_t>());

  return object;
}

auto linker::operator<<(const Serializer & object) -> linker &
{
  object_t map;

  for(auto arrpProps = const_cast<Serializer *>(&object)->getPropertysArray(); auto & prop : arrpProps)
    prop->copy_to(map);

  this->m_type = Types::Object;
  this->m_value = map;

  return *this;
};
