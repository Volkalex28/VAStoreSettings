#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <any>

#include "linker.hpp"

class Serializer
{
protected:
  class PropertyManager;

private:
  class PropertyBase
  {
    const std::string nameid;

    virtual void copy_from(linker::object_t & map) const = 0;
    virtual void copy_to(linker::object_t & map)         const = 0;

    virtual bool isSerializer(void) const = 0;

    virtual std::optional<Serializer *> getSerializer(void) const = 0;

  protected:
    PropertyBase(const std::string & name) : nameid(name)
    {
      // Empty
    }

  public:
    virtual ~PropertyBase(void)
    {
      // Empty
    }

    inline std::string name(void) const
    {
      return this->nameid;
    }

    virtual void   toDefValue(void) const = 0;
    template<typename Type>
    PropertyBase & setDefValue(const Type & defValue)
    {
      return this->m_setDefValue(defValue);
    }

  private:
    virtual PropertyBase & m_setDefValue(const std::any & defValue) = 0;

    friend class StoreSettings;
  };

  template<typename Type>
  class Property : public PropertyBase
  {
  public:
    using fRead  = std::function<Type(void)>;
    using fWrite = std::function<void(Type)>;

  private:
    Type * pPtr;
    fRead  fGet;
    fWrite fSet;
    Type   defValue = Type();

    Property(Property && prop) = default;
    Property(const std::string & name, Type * pPtr, fRead fGet = nullptr, fWrite fSet = nullptr)
      : PropertyBase(name), pPtr(pPtr), fGet(fGet), fSet(fSet)
    {
      // Empty
    }

    void copy_from(linker::object_t & map) const override
    {
      bool contains = false;
      if(map.empty() == false)
      {
        for(auto & pair : map)
        {
          if(pair.first == this->name())
            contains = true;
        }
      }

      if(contains)
      {
        this->write(linker::value<Type>(map[this->name()]));
        return;
      }
      this->toDefValue();
    }
    void copy_to(linker::object_t & map) const override
    {
      Type value;

      if(this->fGet)      value = this->fGet();
      else if(this->pPtr) value = *this->pPtr;
      else                value = Type();

      map[this->name()] = linker::from(value);
    }
    void write(const Type & value) const
    {
      if(this->fSet)      this->fSet(value);
      else if(this->pPtr) *this->pPtr = value;
    }

    inline bool isSerializer(void) const override
    {
      return std::is_base_of_v<Serializer, Type>;
    }
    inline std::optional<Serializer *> getSerializer() const override
    {
      if constexpr (std::is_base_of_v<Serializer, Type>)
      {
        return (this->pPtr && !this->fSet) ? std::optional<Serializer *>(this->pPtr) : std::nullopt;
      }
      else
      {
        return std::nullopt;
      }
    }

    void toDefValue(void) const override
    {
      this->write(this->defValue);
    }
    PropertyBase & m_setDefValue(const std::any & defValue) override
    {
      try
      {
        this->defValue = std::any_cast<Type>(defValue);
      }
      catch(...)
      {
        // Empty
      }

      return *this;
    }

    template<typename _Tp>
    friend class __gnu_cxx::new_allocator;
    friend class PropertyManager;
  };

protected:
  class PropertyManager
  {
    std::vector<std::shared_ptr<PropertyBase>> arrpProps;

    template<typename Type>
    inline std::shared_ptr<Property<Type>>
      make_and_move_shared_prop(const char * name,
                                Type * pPtr,
                                typename Property<Type>::fRead fGet = nullptr,
                                typename Property<Type>::fWrite fSet = nullptr)
    {
      return std::make_shared<Property<Type>>(name, pPtr, fGet, fSet);
    }

    PropertyBase * get(const std::string & name) const
    {
      for(auto & prop : this->arrpProps)
      {
        if(prop->name() == name)
        {
          return prop.get();
        }
      }
      return nullptr;
    }

  public:
    PropertyManager(void)
    {
      // Empty
    }

    template<typename Type>
    PropertyBase * add(const char * name, Type * pPtr)
    {
      PropertyBase * pRet = this->get(name);

      if(pRet == nullptr)
      {
        this->arrpProps.push_back(make_and_move_shared_prop<Type>(name, pPtr));
        pRet = this->arrpProps.back().get();
      }

      return pRet;
    }
    template<typename Type>
    PropertyBase * add(const char * name,
                       typename Property<Type>::fRead fGet,
                       typename Property<Type>::fWrite fSet = nullptr)
    {
      PropertyBase * pRet = this->get(name);

      if(pRet == nullptr)
      {
        this->arrpProps.push_back(make_and_move_shared_prop<Type>(name, nullptr, fGet, fSet));
        pRet = this->arrpProps.back().get();
      }

      return pRet;
    }

    friend class Serializer;
  };

public:
           Serializer(void) { /* Empty */ }
  virtual ~Serializer(void) { /* Empty */ }

  virtual void configPropertys(PropertyManager & mng) = 0;

  PropertyBase * getProperty(const char * pName)
  {
    PropertyManager mng;
    this->configPropertys(mng);

    return mng.get(pName);
  }
  const decltype(PropertyManager::arrpProps) getPropertysArray(void)
  {
    PropertyManager mng;
    this->configPropertys(mng);

    return std::move(mng.arrpProps);
  }
};
