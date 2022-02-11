#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <typeinfo>

#include <QDebug>
#include <QVariant>

class Serializer
{
protected:
  class PropertyManager;

private:
  class PropertyBase
  {
    const std::string _name;
  protected:
    PropertyBase(const std::string & name) : _name(name) {};
  public:
    virtual ~PropertyBase() {}
    std::string name() const { return this->_name; }
    virtual QVariant read(void) const = 0;
    virtual void write(QVariant) const = 0;
    virtual bool isSerializer(void) const = 0;
    virtual Serializer * getSerializer(void) const = 0;
    virtual void setSerializer(Serializer *) const = 0;
    virtual void toDefValue(void) const = 0;
    virtual PropertyBase & setDefValue(const QVariant & defValue) = 0;
    template<typename Type>
    PropertyBase & setDefValue(const Type & defValue)
    {
      return this->setDefValue(QVariant::fromValue<Type>(defValue));
    }
  };

  template<typename Type>
  class Property : public PropertyBase
  {
  public:
    using fRead  = std::function<Type(void)>;
    using fWrite = std::function<void(Type)>;

  private:
    Type * ptr;
    fRead pget;
    fWrite pset;
    Type defValue = Type();

    QVariant read(void) const override
    {
      if(this->pget)      return QVariant::fromValue<Type>(this->pget());
      else if(this->ptr)  return QVariant::fromValue<Type>(*this->ptr);
      else                return QVariant::fromValue<Type>(Type());
    }
    void write(QVariant value) const override
    {
      if(this->pset)      this->pset(value.value<Type>());
      else if(this->ptr)  *this->ptr = value.value<Type>();
    }
    bool isSerializer(void) const override
    {
      return std::is_base_of_v<Serializer, Type>;
    }
    Serializer * getSerializer() const override
    {
      if(this->isSerializer())
      {
        return (Serializer *)((this->ptr && !this->pset) ? this->ptr : new Type());
      }
      return nullptr;
    }
    void setSerializer(Serializer * ptr) const override
    {
      if(this->isSerializer() && ptr)
      {
        if(this->pset)
        {
          this->pset(*(Type *)ptr);
          delete ptr;
        }
        else if(!this->ptr) delete ptr;
      }
    }
    void toDefValue(void) const override
    {
      this->write(QVariant::fromValue<Type>(this->defValue));
    };
    PropertyBase & setDefValue(const QVariant & defValue) override
    {
      this->defValue = defValue.value<Type>();
      return *this;
    }

    Property(Property && prop) = default;
    Property(const std::string & name, Type * ptr, fRead get = nullptr, fWrite set = nullptr)
      : PropertyBase(name), ptr(ptr), pget(get), pset(set) { }

    template<typename _Tp>
    friend class __gnu_cxx::new_allocator;
    friend class PropertyManager;
  };

protected:
  class PropertyManager
  {
    std::vector<std::shared_ptr<PropertyBase>> props;

    PropertyBase * get(const std::string & name)
    {
      for(auto & prop : this->props)
        if(prop->name() == name) return prop.get();
      return nullptr;
    }
  public:
    PropertyManager(void) { };

    template<typename Type>
    PropertyBase * add(const std::string & name, Type * ptr)
    {
      PropertyBase * ret_ptr = this->get(name);

      if(ret_ptr == nullptr)
      {
        this->props.push_back(std::make_shared<Property<Type>>(
                                std::move(*new Property<Type>(name, ptr))));
        ret_ptr = this->props.back().get();
      }

      return ret_ptr;
    }
    template<class Type>
    PropertyBase * add(const std::string & name, typename Property<Type>::fRead get,
                       typename Property<Type>::fWrite set = nullptr)
    {
      PropertyBase * ret_ptr = this->get(name);

      if(ret_ptr == nullptr)
      {
        this->props.push_back(std::make_shared<Property<Type>>(
                                std::move(*new Property<Type>(name, nullptr, get, set))));
        ret_ptr = this->props.back().get();
      }

      return ret_ptr;
    }

    friend class Serializer;
    friend QVariantMap getVariantMap(Serializer & value);
    friend void setVariantMap(Serializer & object, const QVariantMap & map);
  };

public:
  Serializer() { }
  virtual ~Serializer() { }

  virtual void configPropertys(PropertyManager &) = 0;

  PropertyBase * getProperty(const char * name)
  {
    PropertyManager mng;
    this->configPropertys(mng);

    return mng.get(name);
  }

  friend QVariantMap getVariantMap(Serializer & value);
  friend void setVariantMap(Serializer & object, const QVariantMap & map);
};
