/*  -*-c++-*- 
 *  Copyright (C) 2010 Cedric Pinson <cedric.pinson@plopbyte.net>
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

#include <osg/MatrixTransform>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osg/ShapeDrawable>
#include <osgUtil/InstanceLogger>
#include <osgDB/ReadFile>

osg::Node* setupCube()
{
    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(new osg::ShapeDrawable(new osg::Box(osg::Vec3(0.0f,0.0f,0.0f),2)));
    return geode;
}


class UpdateCallback : public osg::NodeCallback
{
public:
    UpdateCallback(osg::Node* model): osg::NodeCallback(), _nbInstancied(0), _model(model)  {
    }

    /** Callback method called by the NodeVisitor when visiting a node.*/
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (nv->getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR) {
            osg::Group* grp = dynamic_cast<osg::Group*>(node);
            if (grp) {
                if (_nbInstancied <= 0)
                    _removing = false;
                else if (_nbInstancied > 1000)
                    _removing = true;

                if (!_removing) {
                    _nbInstancied++;
                    osg::Node* model = osg::clone(_model.get(),osg::CopyOp::DEEP_COPY_ALL);
                    osg::MatrixTransform* mtr = new osg::MatrixTransform;
                    mtr->setMatrix(osg::Matrix::translate(osg::Vec3(_nbInstancied, 0,0)));
                    mtr->addChild(model);
                    grp->addChild(mtr);
                } else {
                    grp->removeChild(0, 1);
                    _nbInstancied--;
                }
            }
        }
        traverse(node,nv);
    }

protected:
    int _nbInstancied;
    bool _removing;
    osg::ref_ptr<osg::Node> _model;
};





class LogUpdateCallback : public osg::NodeCallback
{
public:
    double _lastLog;
    int _nbLog;

    LogUpdateCallback() : _lastLog(0), _nbLog(0)
    {
    }

    ~LogUpdateCallback() {
    }

    /** Callback method called by the NodeVisitor when visiting a node.*/
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    { 
        if (nv->getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR) {
            if (nv->getFrameStamp()->getSimulationTime()-_lastLog > 1.0) {
                osgUtil::InstanceLogger* logger = dynamic_cast<osgUtil::InstanceLogger*>(osg::Referenced::getConstructorDestructorHandler());
                if (logger) {
                    logger->reportCurrentMemoryObject();
                    _lastLog = nv->getFrameStamp()->getSimulationTime();
                    _nbLog++;
                }
            }
        }
        // note, callback is responsible for scenegraph traversal so
        // they must call traverse(node,nv) to ensure that the
        // scene graph subtree (and associated callbacks) are traversed.
        traverse(node,nv);
    }
};



int main(int argc, char** argv)
{
    osgUtil::InstanceLogger* logger = new osgUtil::InstanceLogger;
    logger->install();

    osg::ArgumentParser arguments(&argc, argv);
    std::string modelFile;
    while (arguments.read("--model", modelFile)) {}

    osgViewer::Viewer viewer(arguments);

    osg::Node* model = osgDB::readNodeFile(modelFile);
    if (!model)
        model = setupCube();
    osg::Group* grp = new osg::Group;
    grp->addUpdateCallback(new UpdateCallback(model));
    grp->addUpdateCallback(new LogUpdateCallback);

    viewer.setSceneData(grp);
    osgGA::TrackballManipulator* manipulator = new osgGA::TrackballManipulator();
    viewer.setCameraManipulator(manipulator);

    // add the state manipulator
    viewer.addEventHandler( new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()) );
    
    // add the thread model handler
    viewer.addEventHandler(new osgViewer::ThreadingHandler);

    // add the window size toggle handler
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
        
    // add the stats handler
    viewer.addEventHandler(new osgViewer::StatsHandler);

    int r = viewer.run();
    logger->dumpStats();

    return r;
}
