#include "jsonscenereader.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

Scene JsonSceneReader::load_scene(QFile &file) {
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray rawData = file.readAll();
        // Parse document
        QJsonDocument doc(QJsonDocument::fromJson(rawData));

        // Get JSON object
        QJsonObject json = doc.object();
        QJsonObject sceneObj, camera;
        QJsonArray primitiveList, materialList, lightList;

        QMap<QString, Material> mtl_name_to_material;
        QJsonArray frames = json["frames"].toArray();

        // check scene object for every frame
        foreach(const QJsonValue &frame, frames) {
            glm::vec3 ref, eye, world_up;
            int width = 512, height = 512;
            float fovy = 45, near = 0., far = 10;

            QJsonObject sceneObj = frame.toObject()["scene"].toObject();
            //load camera
            if (sceneObj.contains(QString("camera"))) {
                camera = sceneObj["camera"].toObject();
                if (camera.contains(QString("target"))) ref = to_vec3(camera["target"].toArray());
                if (camera.contains(QString("eye"))) eye = to_vec3(camera["eye"].toArray());
                if (camera.contains(QString("worldUp"))) world_up = to_vec3(camera["worldUp"].toArray());
                if (camera.contains(QString("width"))) width = camera["width"].toDouble();
                if (camera.contains(QString("height"))) height = camera["height"].toDouble();
                if (camera.contains(QString("fov"))) fovy = camera["fov"].toDouble();
                if (camera.contains(QString("nearClip"))) near = camera["nearClip"].toDouble();
                if (camera.contains(QString("farClip"))) far = camera["farClip"].toDouble();
            }

            // load all materials in QMap with mtl name as key and Material itself as value
            if (sceneObj.contains(QString("materials"))) {
                materialList = sceneObj["materials"].toArray();
                foreach (const QJsonValue &materialVal, materialList) {
                    QJsonObject materialObj = materialVal.toObject();
                    load_material(materialObj, &mtl_name_to_material);
                }
            }

            std::vector<shape_t> shapes;
            if (sceneObj.contains(QString("primitives"))) {
                primitiveList = sceneObj["primitives"].toArray();
                foreach(const QJsonValue &primitiveVal, primitiveList){
                    QJsonObject primitiveObj = primitiveVal.toObject();
                    shapes.push_back(load_shape(primitiveObj, mtl_name_to_material));
                }
            }

            //load lights and attach materials from QMap
            if(sceneObj.contains(QString("lights"))) {
                lightList = sceneObj["lights"].toArray();
                foreach(const QJsonValue &lightVal, lightList){
                    QJsonObject lightObj = lightVal.toObject();
                    shapes.push_back(load_shape(lightObj, mtl_name_to_material));
                }
            }
             Scene s(width, height, fovy, near, far, eye, ref, world_up);
             s.add_shapes(shapes);
             return s;
        }

        file.close();

    }

    throw new std::runtime_error("Invalid scene file!");
}

shape_t JsonSceneReader::load_shape(
        QJsonObject &geometry,
        QMap<QString, Material> mtl_map) {
    shapetype_t shape_type;
    QString type;
    if (geometry.contains(QString("shape"))) {
        type = geometry["shape"].toString();
    }

    if (QString::compare(type, QString("Sphere")) == 0) {
        shape_type = SPHERE;
    } else if (QString::compare(type, QString("SquarePlane")) == 0) {
        shape_type = SQUAREPLANE;
    } else if (QString::compare(type, QString("Cube")) == 0) {
        shape_type = BOX;
    } else {
        std::cout << "Could not parse the geometry!" << std::endl;
        throw new std::runtime_error("No geometry!");
    }

    glm::vec3 le(0.f, 0.f, 0.f);
    if (geometry.contains("lightColor")) {
        le = to_vec3(geometry["lightColor"].toArray());
    }

    if (geometry.contains("intensity")) {
        float intensity = static_cast<float>(geometry["intensity"].toDouble());
        le *= intensity;
    }

    glm::vec3 t, r, s;
    s = glm::vec3(1,1,1);

    //load transform to shape
    if(geometry.contains(QString("transform"))) {
        QJsonObject transform = geometry["transform"].toObject();
        if(transform.contains(QString("translate"))) t = to_vec3(transform["translate"].toArray());
        if(transform.contains(QString("rotate"))) r = to_vec3(transform["rotate"].toArray());
        if(transform.contains(QString("scale"))) s = to_vec3(transform["scale"].toArray());
    }

    shape_t shape = create_shape(shape_type, t, r, s);
    QMap<QString, Material>::iterator i;
    if(geometry.contains(QString("material"))) {
        QString material_name = geometry["material"].toString();
        for (i = mtl_map.begin(); i != mtl_map.end(); ++i) {
            if(i.key() == material_name){
                shape.bsdf = i.value().bsdf;
            }
        }
    } else {
        shape.bsdf.bxdfs_count = 0;
    }
    shape.le = mk_clf3(le.x, le.y, le.z);

    return shape;
}

bool JsonSceneReader::load_material(QJsonObject &material, QMap<QString, Material> *mtl_map)
{
    QString type;

    //First check what type of material we're supposed to load
    if (material.contains(QString("type"))) type = material["type"].toString();

    if (QString::compare(type, QString("MatteMaterial")) == 0)
    {
        glm::vec3 Kd = to_vec3(material["Kd"].toArray());
        //float sigma = static_cast< float >(material["sigma"].toDouble());
        Material result;
        bxdf_t bxdf;
        bxdf.color.x = Kd.x;
        bxdf.color.y = Kd.y;
        bxdf.color.z = Kd.z;
        bxdf.type = BRDF_LAMBERT;
        bxdf.props = (bxdfprops_t)(BSDF_REFLECTION | BSDF_DIFFUSE);
        result.bsdf.bxdfs[0] = bxdf;
        result.bsdf.bxdfs_count = 1;
        mtl_map->insert(material["name"].toString(), result);
    }
    else if(QString::compare(type, QString("MirrorMaterial")) == 0) {
        glm::vec3 Kr = to_vec3(material["Kr"].toArray());
        //float sigma = static_cast< float >(material["sigma"].toDouble());
        Material result;
        bxdf_t bxdf;
        bxdf.props = (bxdfprops_t)(BSDF_REFLECTION | BSDF_SPECULAR);;
        bxdf.color.x = Kr.x;
        bxdf.color.y = Kr.y;
        bxdf.color.z = Kr.z;
        bxdf.type = BRDF_SPECULAR;
        result.bsdf.bxdfs[0] = bxdf;
        result.bsdf.bxdfs_count = 1;
        mtl_map->insert(material["name"].toString(), result);
    }
    else if(QString::compare(type, QString("GlassMaterial")) == 0)
    {
        glm::vec3 Kt = to_vec3(material["Kt"].toArray());
        //float sigma = static_cast< float >(material["sigma"].toDouble());
        Material result;

        float eta = material["eta"].toDouble();
        bxdf_t bxdf;
        bxdf.props =  (bxdfprops_t)(BSDF_REFLECTION | BSDF_SPECULAR);
        bxdf.color.x = Kt.x;
        bxdf.color.y = Kt.y;
        bxdf.color.z = Kt.z;
        bxdf.type = BRDF_SPECULAR;
        bxdf.eta = eta;
        result.bsdf.bxdfs[0] = bxdf;
    }
    else
    {
        std::cerr << "Could not parse the material!" << std::endl;
        return false;
    }

    return true;
}

glm::vec3 JsonSceneReader::to_vec3(const QJsonArray &s) {
    return glm::vec3(s.at(0).toDouble(), s.at(1).toDouble(), s.at(2).toDouble());
}

glm::vec3 JsonSceneReader::to_vec3(const QString &s) {
    glm::vec3 result;
    int start_idx;
    int end_idx = -1;
    for(int i = 0; i < 3; i++){
        start_idx = ++end_idx;
        while(end_idx < s.length() && s.at(end_idx) != QChar(' '))
        {
            end_idx++;
        }
        result[i] = s.mid(start_idx, end_idx - start_idx).toFloat();
    }
    return result;
}

