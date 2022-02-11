#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QVariantList>
#include <QVariantMap>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>

#include "store_setings.hpp"

StoreSettings::StoreSettings(QString sPath) : sPath(sPath)
{
  QStringList homePath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
  this->dDir = QDir(homePath.first().split(QDir::separator()).last() + "/NibeHelper/");

  if(!this->dDir.exists())
  {
    this->dDir.mkdir(".");
  }
}

//--------------------------------------------------------------------------------------------------
QVariant StoreSettings::getObject(QString key) const
{
  QJsonObject joSett = this->getObject();

  if(joSett.contains(key))
  {
    return joSett[key].toVariant();
  }
  return QVariant();
}

StoreSettings::State StoreSettings::setObject(QString key, QVariant value) const
{
  QVariantMap vmSett = this->getObject().toVariantMap();

//  const QMetaObject * metaobject = value.value<>().metaObject();

  vmSett[key] = value;
  QJsonObject joSett = QJsonObject::fromVariantMap(vmSett);

  return this->setObject(joSett);
}

//--------------------------------------------------------------------------------------------------
QJsonObject StoreSettings::getObject() const
{
  if(this->checkDir())
  {
    QFile fJSON(this->dDir.path() + "/" + this->sPath);

    if(fJSON.open(QFile::ReadOnly | QFile::Text))
    {
      QByteArray baJSON = fJSON.readAll();
      fJSON.close();

      QJsonDocument joSett = QJsonDocument::fromJson(baJSON);
      if(joSett.isObject())
      {
        return joSett.object();
      }
    }
  }
  return QJsonObject();
}

StoreSettings::State StoreSettings::setObject(QJsonObject joSett) const
{
  if(this->checkDir())
  {
    QFile fJSON(this->dDir.path() + "/" + this->sPath);

    if(fJSON.open(QFile::WriteOnly | QFile::Text))
    {
      fJSON.write(QJsonDocument(joSett).toJson(QJsonDocument::Indented));
      fJSON.close();

      return State::OK;
    }
  }
  return State::ERROR;
}

bool StoreSettings::checkDir() const
{
  if(!this->dDir.exists())
  {
    this->dDir.mkdir(".");
  }
  return this->dDir.exists();
}

//--------------------------------------------------------------------------------------------------
QVariantMap getVariantMap(Serializer & object)
{
  QVariantMap map;
  Serializer::PropertyManager mng;
  int count = 0;

  object.configPropertys(mng);
  count = mng.props.size();

  for (int i = 0; i < count; i++) {
    const std::string name = mng.props[i]->name();

    if(mng.props[i]->isSerializer() == false)
    {
      map[name.c_str()] = mng.props[i]->read();
    }
    else
    {
      auto var = mng.props[i]->read();
      map[name.c_str()] = getVariantMap(*(Serializer *)var.constData());
    }

  }
  return map;
}

void setVariantMap(Serializer & object, const QVariantMap & map)
{
  Serializer::PropertyManager mng;
  int count = 0;

  object.configPropertys(mng);
  count = mng.props.size();

  for (int i = 0; i < count; i++)
  {
    const Serializer::PropertyBase * prop = mng.props[i].get();

    const char * name = prop->name().c_str();

    if(prop->isSerializer())
    {
      bool contains = false;
      Serializer * new_ptr = prop->getSerializer();

      if(map.empty() == false)
      {
        if(map.contains(name))
        {
          setVariantMap(*new_ptr, *(QVariantMap *)map[name].constData());
          contains = true;
        }
      }
      if(contains == false) setVariantMap(*new_ptr, QVariantMap());
      prop->setSerializer(new_ptr);
    }
    else if(map.empty() == false)
    {
      if(map.contains(name)) prop->write(map[name]);
      else                   prop->toDefValue();
    }
    else
    {
      prop->toDefValue();
    }
  }
}
