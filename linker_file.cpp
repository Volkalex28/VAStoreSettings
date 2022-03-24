#include <string_view>

#include <QDebug>

#include "serializer.hpp"
#include "linker_file.hpp"

void linkerFile::fromJSON(const std::string & input, std::optional<data_t> & data)
{
  linker::object_t * map = nullptr;
  linker::array_t  * arr = nullptr;

  std::size_t braces    = 0;
  std::size_t subBraces = 0;
  std::string symSubBraces;
  bool colon  = false;     /*   :   */
  bool string = false;     /* "..." */
  bool array  = false;     /* [,,,] */
  bool quotes = false;     /*   /    */

  std::string str;
  std::string name;

  auto getValueFromVector = [](auto & variant)
  {
    if(variant.index())
      return (linker() << std::get<1>(variant));

    return (linker() << std::get<0>(variant));
  };

  auto init = [&]()
  {
    if(array) arr = new linker::array_t();
    else      map = new linker::object_t();
  };
  auto save = [&](const linker & lnk)
  {
    if(array) arr->push_back(lnk);
    else      (*map)[name] = lnk;
  };
  auto retValues = [&]()
  {
    if(array && arr)
    {
      data = *arr;
      delete arr;
    }
    else if(map)
    {
      data = *map;
      delete map;
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
              if(!colon && !array)
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
      else if (((colon || array) && (sym == '[' || sym == '{')) || subBraces)
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

        save(linker::from(getValueFromVector(*data)));
      }
      else if(colon || array)
      {
        if(!subBraces && ((!array && sym == '}') || (array && sym == ']'))) braces--;

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
        if(sym == '[')
          array = true;

        init();
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
  auto map = this->value<object_t>();
  auto arrpProps = object.getPropertysArray();

  if (Types::Object != this->m_type) return object;

  for (auto & prop : arrpProps)
  {
    /*if(bool contains = false; prop->isSerializer())
    {
      if(!map.empty())
      {
        for(auto & pair : map)
        {
          if(pair.first == prop->name())
            contains = true;
        }
      }

      if(auto serializer = prop->getSerializer(); serializer && contains)
      {
        map.at(prop->name()) >> **prop->getSerializer();
      }
      else if(serializer)
      {
        linker() >> **prop->getSerializer();
      }
      else prop->toDefValue();
    }
    else*/ prop->copy_from(map);
  }

  return object;
}

auto linker::operator<<(const Serializer & object) -> linker &
{
  object_t map;
  auto arrpProps = const_cast<Serializer *>(&object)->getPropertysArray();

  for (auto & prop : arrpProps)
  {
    if(prop->isSerializer())
    {
      if(auto serializer = prop->getSerializer(); serializer)
        map[prop->name()] << **serializer;
    }
    else prop->copy_to(map);
  }

  this->m_type = Types::Object;
  this->m_value = map;

  return *this;
};
