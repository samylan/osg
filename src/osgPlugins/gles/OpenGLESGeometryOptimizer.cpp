#include <limits>

#include "OpenGLESGeometryOptimizer"
#include "glesUtil"

const unsigned glesUtil::Remapper::invalidIndex = std::numeric_limits<unsigned>::max();


osg::Node* OpenGLESGeometryOptimizer::optimize(osg::Node& node) {
    osg::ref_ptr<osg::Node> model = osg::clone(&node);

    // animation: create regular Geometry if RigGeometry
    makeAnimation(model);

    // wireframe
    if (!_wireframe.empty()) {
        makeWireframe(model);
    }

    // bind per vertex
    makeBindPerVertex(model);

    // index (merge exact duplicates + uses simple triangles & lines i.e. no strip/fan/loop)
    makeIndexMesh(model);

    // tangent space
    if (_generateTangentSpace) {
        makeTangentSpace(model);
    }

    if(!_useDrawArray) {
        // split geometries having some primitive index > _maxIndexValue
        makeSplit(model);
    }

    // strip
    if(!_disableTriStrip) {
        makeTriStrip(model);
    }

    if(_useDrawArray) {
        // drawelements to drawarrays
        makeDrawArray(model);
    }
    else if(!_disablePreTransform) {
        // pre-transform
        makePreTransform(model);
    }

    // detach wireframe
    makeDetach(model);

    return model.release();
}
