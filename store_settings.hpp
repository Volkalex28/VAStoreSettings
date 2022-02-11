#pragma once

#include <type_traits>

#include <QDebug>
#include <QDir>
#include <QString>
#include <QJsonObject>
#include <QVariant>

#include "serializer.hpp"


QVariantMap getVariantMap(Serializer & value);
void setVariantMap(Serializer & object, const QVariantMap & map);

class StoreSettings
{
  const QString sPath;
  QDir          dDir;

public:
  enum class State
  {
    OK,
    ERROR
  };

  StoreSettings(QString sPath);

protected:
  template <typename Type>
  class Setting
  {
    StoreSettings * store;
    QString         key;

    template<typename T>
    T get(char, typename std::enable_if_t<std::is_base_of_v<Serializer, T>, T>* = 0) const
    {
      QVariantMap map = qvariant_cast<QVariantMap>(this->store->getObject(this->key));

      Type object;
      setVariantMap(object, map);

      return object;
    }
    template<typename T>
    T get(char, typename std::enable_if_t<!std::is_base_of_v<Serializer, T>, T>* = 0) const
    {
      return qvariant_cast<Type>(this->store->getObject(this->key));
    }

    template<typename T> StoreSettings::State
      set(char, T & object, typename std::enable_if_t<std::is_base_of_v<Serializer, T>, T>* = 0) const
    {
      QVariantMap map = getVariantMap(object);
      return this->store->setObject(this->key, QVariant::fromValue(map));
    }
    template<typename T> StoreSettings::State
      set(char, T & value, typename std::enable_if_t<!std::is_base_of_v<Serializer, T>, T>* = 0) const
    {
      return this->store->setObject(this->key, QVariant::fromValue(value));
    }

  public:
    Setting(StoreSettings * store, QString key) : store(store), key(key) {};

    Type get() const
    {
      return this->get<Type>(' ');
    }
    StoreSettings::State set(Type value) const
    {
      return this->set(' ', value);
    }
  };

private:
  QVariant    getObject(QString key) const;
  State       setObject(QString key, QVariant value) const;
  QJsonObject getObject(void) const;
  State       setObject(QJsonObject joSett) const;
  bool        checkDir(void) const;

  template <typename Type>
  friend class Setting;
};
