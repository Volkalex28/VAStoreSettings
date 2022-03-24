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
    std::string nameid;

    virtual void copy_from(const linker::object_t & map) const = 0;
    virtual void copy_to(linker::object_t & map)   const = 0;

    [[nodiscard]] virtual auto isSerializer() const -> bool = 0;
    [[nodiscard]] virtual auto getSerializer() const -> std::optional<Serializer *> = 0;

  protected:
    PropertyBase(std::string name) : nameid(std::move(name))
    {
      // Empty
    }

  public:
    PropertyBase(PropertyBase &&) noexcept = default;
    PropertyBase(const PropertyBase &) = default;
    auto operator=(PropertyBase && other) noexcept -> PropertyBase & = default;
    auto operator=(const PropertyBase &) -> PropertyBase & = default;
    virtual ~PropertyBase() = default;

    [[nodiscard]] inline auto name() const -> std::string
    {
      return this->nameid;
    }

    virtual void toDefValue() const = 0;
    template<typename Type>
    auto setDefValue(const Type & defValue) -> PropertyBase &
    {
      return this->m_setDefValue(defValue);
    }

  private:
    virtual auto m_setDefValue(const std::any & defValue) -> PropertyBase & = 0;

    friend class StoreSettings;
    friend auto linker::operator>>(Serializer & object) const -> Serializer &;
    friend auto linker::operator<<(const Serializer & object) -> linker &;
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

    Property(Property &&) noexcept = default;
    Property(const Property &) = default;
    auto operator=(Property &&) noexcept -> Property & = default;
    auto operator=(const Property &) -> Property & = default;
    Property(std::string && name, Type * pPtr, fRead fGet = nullptr, fWrite fSet = nullptr)
      : PropertyBase(name), pPtr(pPtr), fGet(fGet), fSet(fSet)
    {
      // Empty
    }
    ~Property() override = default;

    void copy_from(const linker::object_t & map) const override
    {
      bool contains = false;
      if(!map.empty())
      {
        for(auto & pair : map)
        {
          if(pair.first == this->name())
            contains = true;
        }
      }

      if(contains)
      {
        this->write(linker::value<Type>(map.at(this->name())));
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

    [[nodiscard]] inline auto isSerializer() const -> bool override
    {
      return std::is_base_of_v<Serializer, Type>;
    }
    [[nodiscard]] inline auto getSerializer() const -> std::optional<Serializer *> override
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

    void toDefValue() const override
    {
      this->write(this->defValue);
    }
    auto m_setDefValue(const std::any & defValue) -> PropertyBase & override
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

    friend class PropertyManager;
  };

protected:
  class PropertyManager
  {
    std::vector<std::shared_ptr<PropertyBase>> arrpProps;

    template<typename Type>
    inline auto make_and_move_shared_prop(const char * name, Type * pPtr,
                                          typename Property<Type>::fRead fGet = nullptr,
                                          typename Property<Type>::fWrite fSet = nullptr)
      ->std::shared_ptr<PropertyBase>
    {
      return std::shared_ptr<PropertyBase>(static_cast<PropertyBase *>(new Property<Type>(name, pPtr, fGet, fSet)));
    }

    [[nodiscard]] auto get(const std::string & name) const -> PropertyBase *
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
    PropertyManager() = default;

    template<typename Type>
    auto add(const char * name, Type * pPtr) -> PropertyBase *
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
    auto add(const char * name,
             typename Property<Type>::fRead fGet,
             typename Property<Type>::fWrite fSet = nullptr) -> PropertyBase *
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
  Serializer() = default;
  Serializer(Serializer &&) noexcept = default;
  Serializer(const Serializer &) = default;
  auto operator=(Serializer &&) noexcept -> Serializer & = default;
  auto operator=(const Serializer &) -> Serializer & = default;
  virtual ~Serializer() = default;

  virtual void configPropertys(PropertyManager & mng) = 0;

  auto getProperty(const char * pName) -> PropertyBase *
  {
    PropertyManager mng;
    this->configPropertys(mng);

    return mng.get(pName);
  }
  auto getPropertysArray() -> std::vector<std::shared_ptr<PropertyBase>>
  {
    PropertyManager mng;
    this->configPropertys(mng);

    return std::move(mng.arrpProps);
  }
};
