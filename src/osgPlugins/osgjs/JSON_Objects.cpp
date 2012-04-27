/*  -*-c++-*- 
 *  Copyright (C) 2010 Cedric Pinson <cedric.pinson@plopbyte.net>
 */

#include "JSON_Objects"
#include <osgDB/WriteFile>
#include <osg/Material>
#include <osg/BlendFunc>
#include <osg/BlendColor>
#include <osg/Texture>
#include <osg/CullFace>
#include <osg/Texture2D>
#include <osg/Texture1D>
#include <osg/Image>
#include <sstream>

int JSONObjectBase::level = 0;
unsigned int JSONObject::uniqueID = 0;

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
    order.push_back("UniqueID");
    order.push_back("Name");
    order.push_back("TargetName");
    order.push_back("Matrix");
    order.push_back("UpdateCallbacks");
    order.push_back("StateSet");
    writeOrder(str, order);
}

JSONObject::JSONObject(const unsigned int id)
{
    _uniqueID = id;
    _maps["UniqueID"] = new JSONValue<unsigned int>(id);
}

JSONObject::JSONObject() 
{
    _uniqueID = -1;
}

void JSONObject::addUniqueID() 
{
    _uniqueID = JSONObject::uniqueID++;
    _maps["UniqueID"] = new JSONValue<unsigned int>(_uniqueID);
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
    defaultOrder.push_back("UniqueID");
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
