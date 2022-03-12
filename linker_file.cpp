#include "linker_file.hpp"

void linkerFile::fromJSON(const std::string & input, std::optional<data_t> & data)
{
  linker::object_t * map = nullptr;
  linker::array_t *  arr = nullptr;

  std::size_t braces = 0, subBraces = 0, quotes = 0;
  std::string symSubBraces = "";
  bool colon = false;     /*   :   */
  bool string = false;    /* "..." */
  bool array = false;     /* [,,,] */

  std::string str = "";
  std::string name = "";

  auto getValueFromVector = [](auto & variant)
  {
    if(variant.index()) return (linker() << std::get<1>(variant));
    else                return (linker() << std::get<0>(variant));
  };

  auto init = [&](void)
  {
    if(array) arr = new linker::array_t();
    else      map = new linker::object_t();
  };
  auto save = [&](const linker & lnk)
  {
    if(array) arr->push_back(lnk); else (*map)[name] = lnk;
  };
  auto retValues = [&](void)
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

  for(const char sym : input)
  {
    if((sym == '\t' || sym == '\n' || sym == '\r' || sym == ' ') && !string) continue;
    if(!braces)
    {
      if(sym == '}' || sym == ']') return;
      else if(sym != '{' && sym != '[')
      {
        if(sym == ' ') continue;
        return;
      }
      else if(sym == '{' || sym == '[')
      {
        if(sym == '[')
          array = true;

        init();
        braces++;
      }
    }
    else
    {
      if((sym == '\"' || string) && !subBraces)
      {
        if(!string)
        {
          string = true;
          str = "";
          continue;
        }

        if(quotes == 0)
        {
          if(sym == '\\')
          {
            quotes++;
            continue;
          }
          else if(sym == '\"')
          {
            string = false;

            if(!colon)
            {
              name = str;
            }
            else save(linker() << str);

            str = "";
            continue;
          }
        }
        else
        {
          quotes = 0;
        }

        str += sym;
      }
      else if ((colon && (sym == '[' || sym == '{')) || subBraces)
      {
        if(!subBraces)
        {
          str = "";
          symSubBraces = "";
        }

        if(symSubBraces == "")
        {
          symSubBraces = (sym == '[' ? "[]" : "{}");
        }

        if(sym == symSubBraces[0]) subBraces++;
        else if(sym == symSubBraces[1]) subBraces--;

        str += sym;

        if(subBraces) continue;

        std::optional<data_t> data;

        fromJSON(str, data);
        str = "";

        if(!data) continue;

        save(linker() << getValueFromVector(*data));
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
            save(linker() << value);
          }
          else if (str.substr(0, 4) == "null")
          {
            save(linker() << linker::null_t());
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
  }

  retValues();
}

bool linkerFile::isJSONArray() const
{
  if(this->data) return (std::get_if<1>(&*this->data) != nullptr);
  return false;
}
bool linkerFile::isJSONObject() const
{
  if(this->data) return (std::get_if<0>(&*this->data) != nullptr);
  return false;
}
bool linkerFile::isEmpty() const
{
  return !this->data;
}

std::string linkerFile::toJSON(const bool is_short)
{
  switch(this->data->index())
  {
  case 0: return this->toJSON<linker::object_t>(std::get<0>(*this->data), is_short, 0); break;
  case 1: return this->toJSON<linker::array_t>(std::get<1>(*this->data), is_short, 0); break;
  }
  return "";
}

linkerFile & linkerFile::fromJSON(const std::string & input)
{
  this->data = std::nullopt;
  this->fromJSON(input, this->data);

  return *this;
}

linker::object_t linkerFile::getJSONObject(void) const
{
  if(!this->isEmpty())
  {
    return std::get<0>(*this->data);
  }
  return linker::object_t();
}
linker::array_t linkerFile::getJSONArray(void) const
{
  if(!this->isEmpty())
  {
    return std::get<1>(*this->data);
  }
  return linker::array_t();
}

void linkerFile::setJSONObject(const linker::object_t & map)
{
  this->data = map;
}
void linkerFile::setJSONArray(const linker::array_t & arr)
{
  this->data = arr;
}
