/*  -*-c++-*- 
 *  Copyright (C) 2010 Cedric Pinson <cedric.pinson@plopbyte.net>
 */

#include "JSON_Objects"
#include <osgDB/WriteFile>
#include <osg/Material>
#include <osg/Texture>
#include <osg/Texture2D>
#include <osg/Texture1D>
#include <osg/Image>
#include <sstream>

int JSONObjectBase::level = 0;
std::string JSONObjectBase::indent()
{
    std::string str;
    for (int i = 0; i < JSONObjectBase::level; ++i) {
        str += "  ";
    }
    return str;
}

void JSONObject::addChild(JSONObject* child) 
{
    if (!getMaps()["children"])
        getMaps()["children"] = new JSONArray;
    getMaps()["children"]->asArray()->getArray().push_back(child);
}

void JSONObject::write(std::ostream& str)
{
    str <<  "{\n";
    JSONObjectBase::level++;
    for (JSONMap::iterator it = _maps.begin(); it != _maps.end(); ++it) {
        std::string key = it->first;
        if (key.empty())
            continue;
        JSONObject* obj = it->second.get();
        if (!obj)
            continue;

        str << JSONObjectBase::indent() << '"' << key << '"' << ": ";
        obj->write(str);
        JSONMap::iterator itend = it;
        itend++;
        if (itend != _maps.end()) {
            str << ", ";
            str << "\n";
        }
    }
    JSONObjectBase::level--;
    str << std::endl << JSONObjectBase::indent() << "}";
}

JSONVec4Array::JSONVec4Array(const osg::Vec4& v) : JSONVec3Array()
{
    for (int i = 0; i < 4; ++i) {
        _array.push_back(new JSONValue<float>(v[i]));
    }
}

JSONVec3Array::JSONVec3Array(const osg::Vec3& v)
{
    for (int i = 0; i < 3; ++i) {
        _array.push_back(new JSONValue<float>(v[i]));
    }
}

void JSONVec3Array::write(std::ostream& str)
{
    str << "[ ";
    for (unsigned int i = 0; i < _array.size(); i++) {
        if (_array[i].valid()) { 
            _array[i]->write(str);
        } else {
            str << "undefined";
        }
        if (i != _array.size()-1)
            str << ", ";
    }
    str << "]";
}


void JSONArray::write(std::ostream& str)
{
    str << "[ ";
    JSONObjectBase::level++;
    for (unsigned int i = 0; i < _array.size(); i++) {
        if (_array[i].valid()) {
            _array[i]->write(str);
        } else {
            str << "undefined";
        }
        if (i != _array.size() -1) {
            str << ", ";
            str << "\n";
        }
    }
    JSONObjectBase::level--;
    str << std::endl << JSONObjectBase::indent() << "]";
}

void JSONBufferArray::write(std::ostream& str)
{
    str << "{\n";
    JSONObjectBase::level++;
    for (JSONMap::iterator it = _maps.begin(); it != _maps.end(); ++it) {
        std::string key = it->first;
        if (key.empty())
            continue;
        JSONObject* obj = it->second.get();
        if (!obj)
            continue;

        str << JSONObjectBase::indent() << '"' << key << '"' << ": ";
        obj->write(str);
        JSONMap::iterator itend = it;
        itend++;
        if (itend != _maps.end()) {
            str << ", ";
            str << "\n";
        }

    }
    JSONObjectBase::level--;
    str << std::endl << JSONObjectBase::indent() << "}";
}

// use to convert draw array quads to draw elements triangles
JSONDrawElements<osg::DrawElementsUShort>* createJSONDrawElements(const osg::DrawArrays& drawArray)
{

    if (drawArray.getMode() != GL_QUADS) {
        osg::notify(osg::WARN) << "" << std::endl;
        return 0;
    }

    osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
    for (int i = 0; i < drawArray.getCount()/4; ++i) {
        int base = drawArray.getFirst() + i*4;
        de->push_back(base + 0);
        de->push_back(base + 1);
        de->push_back(base + 3);

        de->push_back(base + 1);
        de->push_back(base + 2);
        de->push_back(base + 3);
    }
    return new JSONDrawElements<osg::DrawElementsUShort>(*de);
}


JSONObject* createImage(osg::Image* image)
{
    if (!image) {
        osg::notify(osg::WARN) << "unknown image from texture2d " << std::endl;
        return new JSONValue<std::string>("/unknown.png");
    } else {
        if (!image->getFileName().empty()) {
            int new_s = osg::Image::computeNearestPowerOfTwo(image->s());
            int new_t = osg::Image::computeNearestPowerOfTwo(image->t());
            bool needToResize = false;
            if (new_s != image->s()) needToResize = true;
            if (new_t != image->t()) needToResize = true;
            
            if (needToResize) {
                // resize and rewrite image file
                image->ensureValidSizeForTexturing(2048);
                osgDB::writeImageFile(*image, image->getFileName());
            }
            return new JSONValue<std::string>(image->getFileName());
        }
    }
    return 0;
}


static JSONValue<std::string>* getJSONFilterMode(osg::Texture::FilterMode mode)
{
    switch(mode) {
    case GL_LINEAR:
        return new JSONValue<std::string>("LINEAR");
    case GL_LINEAR_MIPMAP_LINEAR:
        return new JSONValue<std::string>("LINEAR_MIPMAP_LINEAR");
    case GL_LINEAR_MIPMAP_NEAREST:
        return new JSONValue<std::string>("LINEAR_MIPMAP_NEAREST");
    case GL_NEAREST:
        return new JSONValue<std::string>("NEAREST");
    case GL_NEAREST_MIPMAP_LINEAR:
        return new JSONValue<std::string>("NEAREST_MIPMAP_LINEAR");
    case GL_NEAREST_MIPMAP_NEAREST:
        return new JSONValue<std::string>("NEAREST_MIPMAP_NEAREST");
    default:
        return 0;
    }
    return 0;
}

static JSONValue<std::string>* getJSONWrapMode(osg::Texture::WrapMode mode)
{
    switch(mode) {
    case GL_CLAMP:
        return new JSONValue<std::string>("CLAMP");
    case GL_CLAMP_TO_EDGE:
        return new JSONValue<std::string>("CLAMP_TO_EDGE");
    case GL_CLAMP_TO_BORDER_ARB:
        return new JSONValue<std::string>("CLAMP_TO_BORDER");
    case GL_REPEAT:
        return new JSONValue<std::string>("REPEAT");
    case GL_MIRRORED_REPEAT_IBM:
        return new JSONValue<std::string>("MIRROR");
    default:
        return 0;
    }
    return 0;
}

JSONObject* createTexture(osg::Texture* texture)
{
    if (!texture) {
        return 0;
    }

    osg::ref_ptr<JSONObject> jsonTexture = new JSONObject;
    jsonTexture->getMaps()["mag_filter"] = getJSONFilterMode(texture->getFilter(osg::Texture::MAG_FILTER));
    jsonTexture->getMaps()["min_filter"] = getJSONFilterMode(texture->getFilter(osg::Texture::MIN_FILTER));

    jsonTexture->getMaps()["wrap_s"] = getJSONWrapMode(texture->getWrap(osg::Texture::WRAP_S));
    jsonTexture->getMaps()["wrap_t"] = getJSONWrapMode(texture->getWrap(osg::Texture::WRAP_T));

    osg::Texture2D* t2d = dynamic_cast<osg::Texture2D*>(texture);
    if (t2d) {
        JSONObject* image = createImage(t2d->getImage());
        if (image)
            jsonTexture->getMaps()["file"] = image;
        return jsonTexture.release();
    }
    osg::Texture1D* t1d = dynamic_cast<osg::Texture1D*>(texture);
    if (t1d) {
        JSONObject* image = createImage(t1d->getImage());
        if (image)
            jsonTexture->getMaps()["file"] = image;

        return jsonTexture.release();
    }
    return 0;
}

JSONObject* createMaterial(osg::Material* material)
{
    osg::ref_ptr<JSONObject> jsonMaterial = new JSONObject;
    jsonMaterial->getMaps()["name"] = new JSONValue<std::string>(material->getName());
    jsonMaterial->getMaps()["ambient"] = new JSONVec4Array(material->getAmbient(osg::Material::FRONT));
    jsonMaterial->getMaps()["diffuse"] = new JSONVec4Array(material->getDiffuse(osg::Material::FRONT));
    jsonMaterial->getMaps()["specular"] = new JSONVec4Array(material->getSpecular(osg::Material::FRONT));
    jsonMaterial->getMaps()["emission"] = new JSONVec4Array(material->getEmission(osg::Material::FRONT));
    jsonMaterial->getMaps()["shininess"] = new JSONValue<float>(material->getShininess(osg::Material::FRONT));

    return jsonMaterial.release();
}

JSONObject* createJSONStateSet(osg::StateSet* stateset)
{
    osg::ref_ptr<JSONObject> jsonStateSet = new JSONObject;
    osg::ref_ptr<JSONArray> textures = new JSONArray;
    int lastTextureIndex = -1;
    for (int i = 0; i < 32; ++i) {
        osg::Texture* texture = dynamic_cast<osg::Texture*>(stateset->getTextureAttribute(i,osg::StateAttribute::TEXTURE));

        JSONObject* jsonTexture = createTexture(texture);
        if (jsonTexture) {
            textures->getArray().push_back(jsonTexture);
            lastTextureIndex = i;
        } else
            textures->getArray().push_back(0);
    }
    if (lastTextureIndex > -1) {
        textures->getArray().resize(lastTextureIndex+1);
        for (unsigned int i = 0; i < textures->getArray().size(); ++i) {
            // replace empty texture by dummy object
            if (textures->getArray()[i] == 0) {
                textures->getArray()[i] = new JSONObject;
            }
        }
        jsonStateSet->getMaps()["textures"] = textures;
    }

    osg::Material* material = dynamic_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
    if (material) {
        jsonStateSet->getMaps()["material"] = createMaterial(material);
    }

    osg::StateSet::ModeList modeList = stateset->getModeList();
    for (unsigned int i = 0; i < modeList.size(); ++i) {
        // add modes
    }

    if (jsonStateSet->getMaps().empty())
        return 0;
    return jsonStateSet.release();
}



JSONObject* getDrawMode(GLenum mode)
{
    JSONObject* result = 0;
    switch (mode) {
    case GL_POINTS:
        result = new JSONValue<std::string>("POINTS");
        break;
    case GL_LINES:
        result = new JSONValue<std::string>("LINES");
        break;
    case GL_LINE_LOOP:
        result = new JSONValue<std::string>("LINE_LOOP");
        break;
    case GL_LINE_STRIP:
        result = new JSONValue<std::string>("LINE_STRIP");
        break;
    case GL_TRIANGLES:
        result = new JSONValue<std::string>("TRIANGLES");
        break;
    case GL_POLYGON:
        result = new JSONValue<std::string>("TRIANGLE_FAN");
        break;
    case GL_QUAD_STRIP:
    case GL_TRIANGLE_STRIP:
        result = new JSONValue<std::string>("TRIANGLE_STRIP");
        break;
    case GL_TRIANGLE_FAN:
        result = new JSONValue<std::string>("TRIANGLE_FAN");
        break;
    case GL_QUADS:
        osg::notify(osg::WARN) << "exporting quads will not be able to work on opengl es" << std::endl;
        break;
    }
    return result;
}

JSONDrawArray::JSONDrawArray(osg::DrawArrays& array) 
{
    getMaps()["first"] = new JSONValue<int>(array.getFirst());
    getMaps()["count"] = new JSONValue<int>(array.getCount());
    getMaps()["mode"] = getDrawMode(array.getMode());
}
