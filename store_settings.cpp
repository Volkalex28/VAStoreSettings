#include <unistd.h>
#include <fstream>

#include "store_settings.hpp"
#include "linker_file.hpp"

static auto setup_path(const std::string & path) -> std::string
{
  std::size_t index = path.find_last_of("\\/");

  if(index != std::string::npos)
  {
    return path.substr(index+1);
  }

  return path;
}

static auto setup_dir(const std::string & path, const fs::path & dir)
    -> std::string
{
  std::string ret;
  std::string subdir;
  std::size_t indexp0 = path.find_first_not_of("\\/");
  std::size_t indexp1 = path.find_last_of("\\/");
  std::size_t indexd  = dir.string().find_last_not_of("\\/");

  ret = dir.string().substr(0, indexd+1) + "/";

  if(indexp1 != std::string::npos)
  {
    if(indexp0 == std::string::npos)
      indexp0 = 0;

    subdir = path.substr(indexp0, indexp1);
  }

  if(subdir.back() != '\\' && subdir.back() != '/')
  {
      subdir += "/";
  }

  return ret + subdir;
}

StoreSettings::StoreSettings(const std::string & path, DirectoryPath dir)
  : mPath(setup_path(path)), mDirType(dir), mDir(setup_dir(path, this->dir()))
{
  // Empty
}

StoreSettings::StoreSettings(const std::string & path, const fs::path & dir)
    : mPath(setup_path(path)), mDirType(std::nullopt), mDir(setup_dir(path, dir))
{
  // Empty
}

//--------------------------------------------------------------------------------------------------
auto StoreSettings::getObject(const std::string & key) const -> linker
{
  linkerFile file = this->getFile();

  if(file.isJSONObject())
  {
    auto sett = file.getJSONObject();
    return sett[key];
  }
  return {};
}

auto StoreSettings::setObject(const std::string & key, linker value) const -> StoreSettings::State
{
  linkerFile file       = this->getFile();
  linker::object_t sett = file.getJSONObject();

  sett[key] = std::move(value);

  file.setJSONObject(sett);
  return this->setFile(file);
}

//--------------------------------------------------------------------------------------------------
auto StoreSettings::getArray() const -> linker::array_t
{
  linkerFile file = this->getFile();

  if(file.isJSONObject())
  {
    return file.getJSONArray();
  }
  return {};
}

auto StoreSettings::setObject(const linker::array_t & value) const -> StoreSettings::State
{
  linkerFile file = linkerFile();

  file.setJSONArray(value);
  return this->setFile(file);
}

//--------------------------------------------------------------------------------------------------
auto StoreSettings::getFile() const -> linkerFile
{
  if(this->mkDir() == State::OK)
  {
    if(std::ifstream json { this->mainDir().path().string() }; json)
    {
      std::string content;
      while(!json.eof())
      {
        std::array<char, 2048> buf = {};
        json.getline(buf.data(), buf.size());
        content += buf.data();
//        content += '\n';
//        std::string local_content;
//        json >> local_content;
//        content += local_content;
      }
      json.close();

      return linkerFile().fromJSON(content);
    }
  }

  return {};
}

auto StoreSettings::setFile(linkerFile lfSett) const -> StoreSettings::State
{
  if(this->mkDir() == State::OK)
  {
    if(std::ofstream json { this->mainDir().path() }; json)
    {
      std::string content = lfSett.toJSON(false);
      json.write(content.c_str(), (std::streamsize)content.length());
      json.close();

      return State::OK;
    }
  }
  return State::ERROR;
}

auto StoreSettings::mkDir() const -> StoreSettings::State
{
  std::size_t index = 0;
  std::string sDir = this->mDir.path().string();

  if(!this->mDir.exists())
  {
    do
    {
      index = sDir.find_first_of("\\/", index);

      if(index == std::string::npos)
        break;

      fs::directory_entry sSubDir { fs::path { sDir.substr(0, ++index) } };
      if(!sSubDir.exists())
      {
        fs::create_directory(sSubDir);
    //    this->mDir.assign(dir);
      }

      index++;

    } while(true);

    this->mDir.assign(sDir);
  }

//  if(!dir.exists())
//  {
//    fs::create_directory(dir);
////    this->mDir.assign(dir);
//  }
  return this->mDir.exists() ? State::OK : State::ERROR;
}

auto StoreSettings::dir() const -> fs::directory_entry
{
  auto getenv = [&](const std::string & name) -> std::string
  {
    if(environ)
    {
      for(char ** penv = environ; *penv; penv++)
      {
        std::string env(*penv);
        if(env.find(name) != std::string::npos)
        {
          return env.substr(env.find('=')+1);
        }
      }
    }
    return "";
  };

  fs::path dirPath;

  if(dirPath.empty())
  {
    if(this->mDirType)
    {
      if(*this->mDirType == DirectoryPath::User)
      {
        dirPath = getenv("USERPROFILE");
        if(dirPath.empty())
        {
          dirPath = getenv("HOME");
        }
      }
      else if(*this->mDirType == DirectoryPath::Temp)
      {
        dirPath = fs::temp_directory_path();
      }
      else dirPath = ".";
    }
    if(dirPath.string().back() != '/' && dirPath.string().back() != '\\')
    {
      dirPath += "/";
    }
    dirPath += "NibeHelper/";

    std::size_t index = this->mPath.string().find_last_of("\\/");
    if(index != std::string::npos)
    {
      dirPath += this->mPath.string().substr(0, index+1);
    }
  }

  return fs::directory_entry(dirPath);
}
