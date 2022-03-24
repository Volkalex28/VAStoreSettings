#pragma once

#include <filesystem>
#include <type_traits>

#include "linker.hpp"
#include "serializer.hpp"

namespace fs = std::filesystem;

class linkerFile;

class StoreSettings
{
public:
  enum class State : uint8_t
  {
    OK,
    ERROR
  };

  enum class DirectoryPath : uint8_t
  {
    User,
    Temp
  };

  StoreSettings(const std::string & path, DirectoryPath = DirectoryPath::User);
  StoreSettings(const std::string & path, const fs::path & dir);

protected:
  template <typename Type>
  class Setting
  {    
    StoreSettings * const pStore;
    std::string           key;

  public:
    Setting(StoreSettings * const pStore, std::string key) : pStore(pStore), key(std::move(key))
    {
      // Empty
    }

    auto get() const -> Type
    {
      if constexpr (std::is_base_of_v<Serializer, Type>)
      {
        Type object;
//        auto map = linker::value<linker::object_t>(this->pStore->getObject(this->key));

/*        std::function<void(Serializer *, const linker::object_t &)> extract =
            [&extract](Serializer * object, linker::object_t & map)
        {
          auto arrpProps = object->getPropertysArray();

          for (auto & prop : arrpProps)
          {
            if(bool contains = false; prop->isSerializer())
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
                extract(*prop->getSerializer(), map[prop->name()].value<linker::object_t>());
              }
              else if(serializer)
              {
                extract(*prop->getSerializer(), linker::object_t());
              }
              else prop->toDefValue();
            }
            else prop->copy_from(map);
          }
        };
        extract(&object, map);*/

        this->pStore->getObject(this->key) >> *static_cast<Serializer *>(&object);
        return object;
      }
      else return linker::value<Type>(this->pStore->getObject(this->key));
    }
    auto set(const Type value) const -> StoreSettings::State
    {
/*      if constexpr (std::is_base_of_v<Serializer, Type>)
      {
//        std::function<linker::object_t(Serializer &)> fill =
//            [&fill](Serializer & object)
//        {
//          linker::object_t map;
//          auto arrpProps = object.getPropertysArray();

//          for (auto & prop : arrpProps)
//          {
//            if(auto serializer = prop->getSerializer(); prop->isSerializer())
//            {
//              if(serializer)
//                map[prop->name()] << fill(**serializer);
//            }
//            else prop->copy_to(map);

//          }
//          return map;
//        };

//        linker::object_t map;
//        map << *static_cast<Serializer *>(&value);
        return this->pStore->setObject(this->key, linker::from(*(Serializer *)&value));
      }
      else */return this->pStore->setObject(this->key, linker::from(value));
    }
  };

private:
  fs::path                     mPath;
  std::optional<DirectoryPath> mDirType;
  fs::path                     mSubDir;
  mutable fs::directory_entry  mDir;

  [[nodiscard]] auto getObject(const std::string & key)               const -> linker;
  [[nodiscard]] auto setObject(const std::string & key, linker value) const -> State;
  [[nodiscard]] auto getArray()                                       const -> linker::array_t;
  [[nodiscard]] auto setObject(const linker::array_t & value)         const -> State;
  [[nodiscard]] auto getFile()                                        const -> linkerFile;
  [[nodiscard]] auto setFile(linkerFile lfSett)                       const -> State;
  [[nodiscard]] auto mkDir()                                          const -> State;
  [[nodiscard]] auto dir()                                            const -> fs::directory_entry;

  [[nodiscard]] inline auto mainDir() const -> fs::directory_entry
  {
    return fs::directory_entry(this->mDir.path().string() + this->mPath.string());
  }

  template <typename>
  friend class Setting;
};
