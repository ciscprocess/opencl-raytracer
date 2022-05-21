#pragma once
#include "scene.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QList>
class Material {
public:
    bsdf_t bsdf;
};

class Transform {
public:
    cl_float4 trans [4];
    cl_float4 invtr_trans[4];
};

class Primitive {

};

class Light {

};

class JsonSceneReader
{
public:
    Scene load_scene (QFile &file);
    shape_t load_shape(
            QJsonObject &geometry,
            QMap<QString, Material> mtl_map);
    bool load_material(QJsonObject &material, QMap<QString, Material> *mtl_map);
    glm::vec3 ToVec3(const QJsonArray &s);
    glm::vec3 ToVec3(const QString &s);
};
