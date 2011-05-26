/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield 
 *
 * This library is open source and may be redistributed and/or modified under  
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or 
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * OpenSceneGraph Public License for more details.
*/

#include <osgUtil/InstanceLogger>
#include <osg/Object>
#include <fstream>
#include <iostream>
#include <typeinfo>

using namespace osgUtil;

void InstanceLogger::clear()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(getLogObjectMutex());
    _allObjects.clear();
}

void InstanceLogger::log(const osg::Referenced* obj)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(getLogObjectMutex());
    _allObjects.insert(obj);
}

void InstanceLogger::unlog(const osg::Referenced* obj)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(getLogObjectMutex());
    _allObjects.erase(obj);
}
void InstanceLogger::dumpStats()
{
    std::ofstream data("data.dat");
    std::ofstream script("script.plot");
    gnuPlot(_toPlot, data, script);
}

void InstanceLogger::gnuPlot(std::map<int, TypeInstance >& data, std::ostream& outputData, std::ostream& outputScript)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(getLogObjectMutex());
    // collect all type
    TypeInstance alltype;
    for (std::map<int, TypeInstance >::iterator it = data.begin(); it != data.end(); ++it) {
        TypeInstance& types = it->second;
        for (TypeInstance::iterator it2 = types.begin(); it2 != types.end(); ++it2)
            alltype[it2->first] = 0;
    }

    // need to finish here
    outputData << "# sample ";
    for (TypeInstance::iterator it = alltype.begin(); it != alltype.end(); ++it)
        outputData << "\t" << it->first;
    outputData << std::endl;
        
    for (std::map<int, TypeInstance >::iterator it = data.begin(); it != data.end(); ++it) {
        TypeInstance& t = it->second;
        outputData << it->first;
        for (TypeInstance::iterator it2 = alltype.begin(); it2 != alltype.end(); ++it2) {
            const std::string& key = it2->first;
            outputData << "\t" << t[key];
        }
        outputData << std::endl;
    }

    outputScript << "set term png size 1280,1024" << std::endl;
    outputScript << "set output \"combined.png\"" << std::endl;
    outputScript << "plot ";
    int i = 1;
    for (TypeInstance::iterator it = alltype.begin(); it != alltype.end(); ++it) {
        if (i == 1)
            outputScript << "\\" << std::endl;
        else
            outputScript << ",\\" << std::endl;
        outputScript << "\"data.dat\" u 1:" << ++i << " t \'" << it->first << "\' with lines";
    }
    outputScript << std::endl << std::endl;

    // draw each graph
    outputScript << "set term png size 1280,8192" << std::endl;
    outputScript << "set output \"individual.png\"" << std::endl;
    outputScript << "set tmargin 2" << std::endl << "set bmargin 2" << std::endl << "set lmargin 9" << std::endl << "set rmargin 2" << std::endl;
    outputScript << "set multiplot" << std::endl;
    int nbColumn = 2;
    double size = 1.0/(alltype.size() * 1.0 /nbColumn);
    outputScript << "set size " << 1.0/nbColumn << "," << size << std::endl;
    i = 1;
    int currentIndex = 0;
    for (TypeInstance::iterator it = alltype.begin(); it != alltype.end(); ++it) {
        int col = currentIndex/(alltype.size()/nbColumn);
        outputScript << "set origin " << col * 1.0 / nbColumn << "," << (currentIndex%(alltype.size()/nbColumn)) * 1.0 * size << std::endl;
        outputScript << "plot \"data.dat\" u 1:" << ++i << " t \'" << it->first << "\' with lines" << std::endl;
        currentIndex++;
    }
}

void InstanceLogger::reportCurrentMemoryObject(bool dump)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(getLogObjectMutex());
    int nbCall = _nbSamples;
    if (dump)
        std::cout << "Total Num Objects allocated " << _allObjects.size() << std::endl;

    TypeInstance& types = _toPlot[nbCall];
    for (ObjectObserver::iterator it = _allObjects.begin(); it != _allObjects.end(); ++it) {
        const osg::Referenced* obj = (*it);
        if (obj && obj->referenceCount() >= 1) {
            //std::cout << " obj " << obj << std::endl;
            const osg::Object* o = dynamic_cast<const osg::Object*>(obj);
            if (o) {
                std::string type = std::string(o->libraryName()) + ":" + std::string(o->className());
                types[type]++;
            } else {
                types[typeid(obj).name()]++;
            }
        }
    }
    if (dump) {
        for (TypeInstance::iterator it = types.begin(); it != types.end(); ++it)
            std::cout << it->first << " nb instances " << it->second << std::endl;
    }

    _nbSamples++;
}


void InstanceLogger::createInstance(const osg::Referenced* object)
{
    log(object);
}
void InstanceLogger::deleteInstance(const osg::Referenced* object)
{
    unlog(object);
}


InstanceLogger::InstanceLogger(): _installed(false)
{
}

void InstanceLogger::install()
{
    if (_installed)
        return;

    osg::Referenced::setConstructorDestructorHandler(this);
    _installed = true;
}

