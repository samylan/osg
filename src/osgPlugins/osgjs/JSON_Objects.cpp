/*  -*-c++-*- 
 *  Copyright (C) 2010 Cedric Pinson <cedric.pinson@plopbyte.net>
 */

#include "JSON_Objects"
#include <osgDB/WriteFile>
#include <osg/Material>
#include <osg/BlendFunc>
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


void JSONNode::write(std::ostream& str)
{
    std::vector<std::string> order;
    order.push_back("Name");
    order.push_back("TargetName");
    order.push_back("Matrix");
    order.push_back("UpdateCallbacks");
    order.push_back("StateSet");
    writeOrder(str, order);
}

void JSONObject::addChild(const std::string& type, JSONObject* child)
{
    if (!getMaps()["Children"])
        getMaps()["Children"] = new JSONArray;

    JSONObject* jsonObject = new JSONObject();
    jsonObject->getMaps()[type] = child;
    getMaps()["Children"]->asArray()->getArray().push_back(jsonObject);
}

static void writeEntry(std::ostream& str, const std::string& key, JSONObject::JSONMap& map)
{
    if (key.empty())
        return;

    if ( map.find(key) != map.end() &&
         map[ key ].valid() ) {
        
        str << JSONObjectBase::indent() << '"' << key << '"' << ": ";
        map[ key ]->write(str);
        map.erase(key);

        if (!map.empty()) {
            str << ", ";
            str << "\n";
        }
    }
}
void JSONObject::writeOrder(std::ostream& str, const std::vector<std::string>& order)
{
    str << "{" << std::endl;
    JSONObjectBase::level++;
    for (unsigned int i = 0; i < order.size(); i++) {
        writeEntry(str, order[i], _maps);
    }

    while(!_maps.empty()) {
        std::string key = _maps.begin()->first;
        writeEntry(str, key, _maps);
    }

    JSONObjectBase::level--;
    str << std::endl << JSONObjectBase::indent() << "}";
}

void JSONObject::write(std::ostream& str)
{
    OrderList defaultOrder;
    defaultOrder.push_back("Name");
    defaultOrder.push_back("TargetName");
    writeOrder(str, defaultOrder);
}

JSONVec4Array::JSONVec4Array(const osg::Vec4& v) : JSONVec3Array()
{
    for (int i = 0; i < 4; ++i) {
        _array.push_back(new JSONValue<float>(v[i]));
    }
}

JSONVec5Array::JSONVec5Array(const Vec5& v) : JSONVec3Array()
{
    for (int i = 0; i < 5; ++i) {
        _array.push_back(new JSONValue<float>(v[i]));
    }
}

JSONVec2Array::JSONVec2Array(const osg::Vec2& v) : JSONVec3Array()
{
    for (int i = 0; i < 2; ++i) {
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

void JSONKeyframes::write(std::ostream& str)
{
    JSONObjectBase::level++;
    str << "[" << std::endl << JSONObjectBase::indent();
    for (unsigned int i = 0; i < _array.size(); i++) {
        if (_array[i].valid()) {
            _array[i]->write(str);
        } else {
            str << "undefined";
        }
        if (i != _array.size() -1) {
            str << ",";
            str << "\n" << JSONObjectBase::indent();
        }
    }
    str << " ]";
    JSONObjectBase::level--;
}


void JSONArray::write(std::ostream& str)
{
    str << "[ ";
    for (unsigned int i = 0; i < _array.size(); i++) {
        if (_array[i].valid()) {
            _array[i]->write(str);
        } else {
            str << "undefined";
        }
        if (i != _array.size() -1) {
            str << ",";
            str << "\n" << JSONObjectBase::indent();
        }
    }
    //str << std::endl << JSONObjectBase::indent() << "]";
    str << " ]";
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
    jsonTexture->getMaps()["MagFilter"] = getJSONFilterMode(texture->getFilter(osg::Texture::MAG_FILTER));
    jsonTexture->getMaps()["MinFilter"] = getJSONFilterMode(texture->getFilter(osg::Texture::MIN_FILTER));

    jsonTexture->getMaps()["WrapS"] = getJSONWrapMode(texture->getWrap(osg::Texture::WRAP_S));
    jsonTexture->getMaps()["WrapT"] = getJSONWrapMode(texture->getWrap(osg::Texture::WRAP_T));

    osg::Texture2D* t2d = dynamic_cast<osg::Texture2D*>(texture);
    if (t2d) {
        JSONObject* image = createImage(t2d->getImage());
        if (image)
            jsonTexture->getMaps()["File"] = image;
        return jsonTexture.release();
    }
    osg::Texture1D* t1d = dynamic_cast<osg::Texture1D*>(texture);
    if (t1d) {
        JSONObject* image = createImage(t1d->getImage());
        if (image)
            jsonTexture->getMaps()["File"] = image;

        return jsonTexture.release();
    }
    return 0;
}

JSONObject* createMaterial(osg::Material* material)
{
    osg::ref_ptr<JSONObject> jsonMaterial = new JSONMaterial;
    if (!material->getName().empty())
        jsonMaterial->getMaps()["Name"] = new JSONValue<std::string>(material->getName());
    jsonMaterial->getMaps()["Ambient"] = new JSONVec4Array(material->getAmbient(osg::Material::FRONT));
    jsonMaterial->getMaps()["Diffuse"] = new JSONVec4Array(material->getDiffuse(osg::Material::FRONT));
    jsonMaterial->getMaps()["Specular"] = new JSONVec4Array(material->getSpecular(osg::Material::FRONT));
    jsonMaterial->getMaps()["Emission"] = new JSONVec4Array(material->getEmission(osg::Material::FRONT));
    jsonMaterial->getMaps()["Shininess"] = new JSONValue<float>(material->getShininess(osg::Material::FRONT));

    return jsonMaterial.release();
}


JSONObject* createLight(osg::Light* light)
{
    osg::ref_ptr<JSONObject> jsonLight = new JSONLight;
    if (!light->getName().empty())
        jsonLight->getMaps()["Name"] = new JSONValue<std::string>(light->getName());

    jsonLight->getMaps()["LightNum"] = new JSONValue<int>(light->getLightNum());
    jsonLight->getMaps()["Ambient"] = new JSONVec4Array(light->getAmbient());
    jsonLight->getMaps()["Diffuse"] = new JSONVec4Array(light->getDiffuse());
    jsonLight->getMaps()["Specular"] = new JSONVec4Array(light->getSpecular());
    jsonLight->getMaps()["Position"] = new JSONVec4Array(light->getPosition());
    jsonLight->getMaps()["Direction"] = new JSONVec3Array(light->getDirection());

    jsonLight->getMaps()["ConstantAttenuation"] = new JSONValue<float>(light->getConstantAttenuation());
    jsonLight->getMaps()["LinearAttenuation"] = new JSONValue<float>(light->getLinearAttenuation());
    jsonLight->getMaps()["QuadraticAttenuation"] = new JSONValue<float>(light->getQuadraticAttenuation());
    jsonLight->getMaps()["SpotExponent"] = new JSONValue<float>(light->getSpotExponent());
    jsonLight->getMaps()["SpotCutoff"] = new JSONValue<float>(light->getSpotCutoff());
    return jsonLight.release();
}

static JSONValue<std::string>* getBlendFuncMode(GLenum mode) {
    switch (mode) {
    case osg::BlendFunc::DST_ALPHA: return new JSONValue<std::string>("DST_ALPHA");
    case osg::BlendFunc::DST_COLOR: return new JSONValue<std::string>("DST_COLOR");
    case osg::BlendFunc::ONE: return new JSONValue<std::string>("ONE");                      
    case osg::BlendFunc::ONE_MINUS_DST_ALPHA: return new JSONValue<std::string>("ONE_MINUS_DST_ALPHA");      
    case osg::BlendFunc::ONE_MINUS_DST_COLOR: return new JSONValue<std::string>("ONE_MINUS_DST_COLOR");      
    case osg::BlendFunc::ONE_MINUS_SRC_ALPHA: return new JSONValue<std::string>("ONE_MINUS_SRC_ALPHA");      
    case osg::BlendFunc::ONE_MINUS_SRC_COLOR: return new JSONValue<std::string>("ONE_MINUS_SRC_COLOR");      
    case osg::BlendFunc::SRC_ALPHA: return new JSONValue<std::string>("SRC_ALPHA");                
    case osg::BlendFunc::SRC_ALPHA_SATURATE: return new JSONValue<std::string>("SRC_ALPHA_SATURATE");       
    case osg::BlendFunc::SRC_COLOR: return new JSONValue<std::string>("SRC_COLOR");                
    case osg::BlendFunc::CONSTANT_COLOR: return new JSONValue<std::string>("CONSTANT_COLOR");           
    case osg::BlendFunc::ONE_MINUS_CONSTANT_COLOR: return new JSONValue<std::string>("ONE_MINUS_CONSTANT_COLOR"); 
    case osg::BlendFunc::CONSTANT_ALPHA: return new JSONValue<std::string>("CONSTANT_ALPHA");           
    case osg::BlendFunc::ONE_MINUS_CONSTANT_ALPHA: return new JSONValue<std::string>("ONE_MINUS_CONSTANT_ALPHA"); 
    case osg::BlendFunc::ZERO: return new JSONValue<std::string>("ZERO");                     
    default:
        return new JSONValue<std::string>("ONE");
    }
    return new JSONValue<std::string>("ONE");
}

JSONObject* createBlendFunc(osg::BlendFunc* sa)
{
    osg::ref_ptr<JSONObject> json = new JSONObject;
    if (!sa->getName().empty())
        json->getMaps()["Name"] = new JSONValue<std::string>(sa->getName());

    json->getMaps()["SourceRGB"] = getBlendFuncMode(sa->getSource());
    json->getMaps()["DestinationRGB"] = getBlendFuncMode(sa->getDestination());
    json->getMaps()["SourceAlpha"] = getBlendFuncMode(sa->getSourceAlpha());
    json->getMaps()["DestinationAlpha"] = getBlendFuncMode(sa->getDestinationAlpha());
    return json.release();
}


JSONObject* createJSONStateSet(osg::StateSet* stateset)
{
    osg::ref_ptr<JSONObject> jsonStateSet = new JSONStateSet;

    if (!stateset->getName().empty()) {
        jsonStateSet->getMaps()["Name"] = new JSONValue<std::string>(stateset->getName());
    }

    osg::ref_ptr<JSONArray> textureAttributeList = new JSONArray;
    int lastTextureIndex = -1;
    for (int i = 0; i < 32; ++i) {
        osg::Texture* texture = dynamic_cast<osg::Texture*>(stateset->getTextureAttribute(i,osg::StateAttribute::TEXTURE));

        JSONArray* textureUnit = new JSONArray;
        JSONObject* jsonTexture = createTexture(texture);
        textureAttributeList->getArray().push_back(textureUnit);

        if (jsonTexture) {
            JSONObject* textureObject = new JSONObject;
            textureObject->getMaps()["osg.Texture"] = jsonTexture;
            textureUnit->getArray().push_back(textureObject);
            lastTextureIndex = i;
        }
    }
    if (lastTextureIndex > -1) {
        textureAttributeList->getArray().resize(lastTextureIndex+1);
        jsonStateSet->getMaps()["TextureAttributeList"] = textureAttributeList;
    }


    osg::ref_ptr<JSONArray> attributeList = new JSONArray;

    osg::Material* material = dynamic_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
    if (material) {
        JSONObject* obj = new JSONObject;
        obj->getMaps()["osg.Material"] = createMaterial(material);
        attributeList->getArray().push_back(obj);
    }

    osg::BlendFunc* blendFunc = dynamic_cast<osg::BlendFunc*>(stateset->getAttribute(osg::StateAttribute::BLENDFUNC));
    if (blendFunc) {
        JSONObject* obj = new JSONObject;
        obj->getMaps()["osg.BlendFunc"] = createBlendFunc(blendFunc);
        attributeList->getArray().push_back(obj);
    }

    if (!attributeList->getArray().empty()) {
        jsonStateSet->getMaps()["AttributeList"] = attributeList;
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
    getMaps()["First"] = new JSONValue<int>(array.getFirst());
    getMaps()["Count"] = new JSONValue<int>(array.getCount());
    getMaps()["Mode"] = getDrawMode(array.getMode());
}


JSONDrawArrayLengths::JSONDrawArrayLengths(osg::DrawArrayLengths& array)
{
    getMaps()["First"] = new JSONValue<int>(array.getFirst());
    getMaps()["Mode"] = getDrawMode(array.getMode());

    JSONArray* jsonArray = new JSONArray;
    for (unsigned int i = 0; i < array.size(); i++) {
        jsonArray->getArray().push_back(new JSONValue<int>(array[i]));
    }
    getMaps()["ArrayLengths"] = jsonArray;
}
