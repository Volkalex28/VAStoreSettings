#pragma once

#include <type_traits>

#include <experimental\filesystem>

#include "linker_file.hpp"
#include "serializer.hpp"

namespace fs = std::experimental::filesystem;

class StoreSettings
{
  const fs::path   path;
  mutable fs::path dir;

public:
  enum class State
  {
    OK,
    ERROR
  };

  StoreSettings(const std::string path);

protected:
  template <typename Type>
  class Setting
  {    
    StoreSettings * const pStore;
    std::string           key;

  public:
    Setting(StoreSettings * const pStore, const std::string key) : pStore(pStore), key(key)
    {
      // Empty
    }

    Type get(void) const
    {
      if constexpr (std::is_base_of_v<Serializer, Type>)
      {
        Type object;
        auto map = linker::value<linker::object_t>(this->pStore->getObject(this->key));

        std::function<void(Serializer *, const linker::object_t &)> extract =
            [&extract](Serializer * object, const linker::object_t & cmap)
        {
          auto map = *const_cast<linker::object_t*>(&cmap);
          auto arrpProps = object->getPropertysArray();

          for (auto & prop : arrpProps)
          {
            if(bool contains = false; prop->isSerializer())
            {
              if(map.empty() == false)
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
        extract(&object, map);

        return object;
      }
      else return linker::value<Type>(this->pStore->getObject(this->key));
    }
    StoreSettings::State set(const Type value) const
    {
      if constexpr (std::is_base_of_v<Serializer, Type>)
      {
        std::function<linker::object_t(Serializer &)> fill =
            [&fill](Serializer & object)
        {
          linker::object_t map;
          auto arrpProps = object.getPropertysArray();

          for (auto & prop : arrpProps)
          {
            if(auto serializer = prop->getSerializer(); prop->isSerializer())
            {
              if(serializer)
                map[prop->name()] << fill(**serializer);
            }
            else prop->copy_to(map);

          }
          return map;
        };

        auto map = fill(*const_cast<Serializer *>(static_cast<const Serializer *>(&value)));
        return this->pStore->setObject(this->key, linker::from(map));
      }
      else return this->pStore->setObject(this->key, linker::from(value));
    }
  };

private:
  linker      getObject(const std::string key) const;
  State       setObject(const std::string key, const linker value) const;
  linker::array_t getArray(void) const;
  State           setObject(const linker::array_t value) const;
  linkerFile  getFile(void) const;
  State       setFile(linkerFile lfSett) const;
  bool        checkDir(void) const;
  bool        mkDir(void) const;

  template <typename Type>
  friend class Setting;
};
