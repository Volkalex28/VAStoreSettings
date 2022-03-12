#include <cstdlib>
#include <fstream>

#include <bitset>

#include "store_settings.hpp"

StoreSettings::StoreSettings(std::string path) : path(path)
{
  // Empty
}

//--------------------------------------------------------------------------------------------------
linker StoreSettings::getObject(const std::string key) const
{
  linkerFile file  = this->getFile();

  if(file.isJSONObject())
  {
    auto sett = file.getJSONObject();
    return sett[key];
  }
  return linker();
}

StoreSettings::State StoreSettings::setObject(const std::string key, const linker value) const
{
  linkerFile file       = this->getFile();
  linker::object_t sett = file.getJSONObject();

  sett[key] = value;

  file.setJSONObject(sett);
  return this->setFile(file);
}

//--------------------------------------------------------------------------------------------------
linker::array_t StoreSettings::getArray() const
{
  linkerFile file  = this->getFile();

  if(file.isJSONObject())
  {
    return file.getJSONArray();
  }
  return linker::array_t();
}

StoreSettings::State StoreSettings::setObject(const linker::array_t value) const
{
  linkerFile file = linkerFile();

  file.setJSONArray(value);
  return this->setFile(file);
}

//--------------------------------------------------------------------------------------------------
linkerFile StoreSettings::getFile() const
{
  if(this->checkDir())
  {
    if(std::ifstream json { this->dir.string() + "/" + this->path.string() }; json)
    {
      std::string content;
      while(!json.eof())
      {
        std::string local_content;
        json >> local_content;
        content += local_content;
      }
      json.close();

      return linkerFile().fromJSON(content);
    }
  }

  return linkerFile();
}

StoreSettings::State StoreSettings::setFile(linkerFile lfSett) const
{
  if(this->checkDir())
  {
    if(std::ofstream json { this->dir.string() + "/" + this->path.string() }; json)
    {
      std::string content = lfSett.toJSON(false);
      json.write(content.c_str(), content.length());
      json.close();

      return State::OK;
    }
  }
  return State::ERROR;
}

bool StoreSettings::mkDir() const
{
  std::error_code errorCode;
  if(!fs::exists(this->dir, errorCode) || errorCode)
  {
    fs::create_directory(this->dir);
  }
  return !(!fs::exists(this->dir, errorCode) || errorCode);
}

bool StoreSettings::checkDir() const
{
  if(this->dir.empty())
  {
    this->dir = getenv("USERPROFILE");
    if(this->dir.empty())
    {
      this->dir = getenv("HOME");
    }
    this->dir += "/NibeHelper";
  }

  return this->mkDir();
}
