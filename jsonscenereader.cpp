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
                if (camera.contains(QString("target"))) ref = ToVec3(camera["target"].toArray());
                if (camera.contains(QString("eye"))) eye = ToVec3(camera["eye"].toArray());
                if (camera.contains(QString("worldUp"))) world_up = ToVec3(camera["worldUp"].toArray());
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
        le = ToVec3(geometry["lightColor"].toArray());
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
        if(transform.contains(QString("translate"))) t = ToVec3(transform["translate"].toArray());
        if(transform.contains(QString("rotate"))) r = ToVec3(transform["rotate"].toArray());
        if(transform.contains(QString("scale"))) s = ToVec3(transform["scale"].toArray());
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
        glm::vec3 Kd = ToVec3(material["Kd"].toArray());
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
        glm::vec3 Kr = ToVec3(material["Kr"].toArray());
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
        glm::vec3 Kt = ToVec3(material["Kt"].toArray());
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

        bxdf.props = (bxdfprops_t)(BSDF_TRANSMISSION | BSDF_SPECULAR);
        bxdf.color.x = Kt.x;
        bxdf.color.y = Kt.y;
        bxdf.color.z = Kt.z;
        bxdf.type = BTDF_SPECULAR;
        bxdf.eta = eta;
        result.bsdf.bxdfs[1] = bxdf;


        result.bsdf.bxdfs_count = 2;
        mtl_map->insert(material["name"].toString(), result);
    }
//    else if(QString::compare(type, QString("TranslucentMaterial")) == 0) {
//        Color3f lb_r = ToVec3(material["lb_r"].toArray());
//        Color3f lb_t = ToVec3(material["lb_t"].toArray());
//        Color3f mf_r = ToVec3(material["mr_r"].toArray());
//        Color3f mf_t = ToVec3(material["mf_t"].toArray());
//        float roughness = 0.f;
//        if(material.contains(QString("roughness"))) {
//            roughness = material["roughness"].toDouble();
//        }
//        auto result = std::make_shared<TranslucentMaterial>(lb_r, lb_t, mf_r, mf_t, roughness);
//        mtl_map->insert(material["name"].toString(), result);
//    }
//    else if(QString::compare(type, QString("GlassMaterial")) == 0)
//    {
//        std::shared_ptr<QImage> textureMapRefl;
//        std::shared_ptr<QImage> textureMapTransmit;
//        std::shared_ptr<QImage> normalMap;
//        Color3f Kr = ToVec3(material["Kr"].toArray());
//        Color3f Kt = ToVec3(material["Kt"].toArray());
//        float eta = material["eta"].toDouble();
//        if(material.contains(QString("textureMapRefl"))) {
//            QString img_filepath = local_path.append(material["textureMapRefl"].toString());
//            textureMapRefl = std::make_shared<QImage>(img_filepath);
//        }
//        if(material.contains(QString("textureMapTransmit"))) {
//            QString img_filepath = local_path.append(material["textureMapTransmit"].toString());
//            textureMapTransmit = std::make_shared<QImage>(img_filepath);
//        }
//        if(material.contains(QString("normalMap"))) {
//            QString img_filepath = local_path.append(material["normalMap"].toString());
//            normalMap = std::make_shared<QImage>(img_filepath);
//        }
//        auto result = std::make_shared<GlassMaterial>(Kr, Kt, eta, textureMapRefl, textureMapTransmit, normalMap);
//        mtl_map->insert(material["name"].toString(), result);
//    }
//    else if(QString::compare(type, QString("PlasticMaterial")) == 0)
//    {
//        std::shared_ptr<QImage> roughnessMap;
//        std::shared_ptr<QImage> textureMapDiffuse;
//        std::shared_ptr<QImage> textureMapSpecular;
//        std::shared_ptr<QImage> normalMap;
//        Color3f Kd = ToVec3(material["Kd"].toArray());
//        Color3f Ks = ToVec3(material["Ks"].toArray());
//        float roughness = material["roughness"].toDouble();
//        if(material.contains(QString("roughnessMap"))) {
//            QString img_filepath = local_path.append(material["roughnessMap"].toString());
//            roughnessMap = std::make_shared<QImage>(img_filepath);
//        }
//        if(material.contains(QString("textureMapDiffuse"))) {
//            QString img_filepath = local_path.append(material["textureMapDiffuse"].toString());
//            textureMapDiffuse = std::make_shared<QImage>(img_filepath);
//        }
//        if(material.contains(QString("textureMapSpecular"))) {
//            QString img_filepath = local_path.append(material["textureMapSpecular"].toString());
//            textureMapSpecular = std::make_shared<QImage>(img_filepath);
//        }
//        if(material.contains(QString("normalMap"))) {
//            QString img_filepath = local_path.append(material["normalMap"].toString());
//            normalMap = std::make_shared<QImage>(img_filepath);
//        }
//        auto result = std::make_shared<PlasticMaterial>(Kd, Ks, roughness, roughnessMap, textureMapDiffuse, textureMapSpecular, normalMap);
//        mtl_map->insert(material["name"].toString(), result);
//    }
    else
    {
        std::cerr << "Could not parse the material!" << std::endl;
        return false;
    }

    return true;
}

glm::vec3 JsonSceneReader::ToVec3(const QJsonArray &s)
{
    glm::vec3 result(s.at(0).toDouble(), s.at(1).toDouble(), s.at(2).toDouble());
    return result;
}

glm::vec3 JsonSceneReader::ToVec3(const QString &s)
{
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

