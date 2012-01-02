/* -*-c++-*- OpenSceneGraph - Copyright (C) Cedric Pinson 
 *
 * This application is open source and may be redistributed and/or modified   
 * freely and without restriction, both in commercial and non commercial
 * applications, as long as this copyright notice is maintained.
 * 
 * This application is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
*/

#include "GeometryOperation"
#include <osg/Math>
#include <osg/Notify>
#include <osg/NodeVisitor>
#include <osg/Texture2D>
#include <osg/StateSet>
#include <osg/Geode>
#include <osg/Array>
#include <osg/TriangleFunctor>
#include <osgUtil/MeshOptimizers>
#include <osgUtil/TriStripVisitor>
#include <string>
#include <osgDB/ReadFile>
#include <osgDB/ReaderWriter>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>


bool needToSplit(osg::PrimitiveSet* p, unsigned int maxIndex, unsigned int* index)
{
    osg::DrawElements* de = p->getDrawElements();
    if (!de) {
        return false;
    }
    for (unsigned int j = 0; j < de->getNumIndices(); j++) {
        unsigned int idx = de->getElement(j);
        if (idx > maxIndex){
            if (index) {
                (*index) = j;
            }
            return true;
        }
    }
    return false;
}

bool needToSplit(osg::Geometry& geom, unsigned int maxIndex)
{
    for ( unsigned int i = 0; i < geom.getPrimitiveSetList().size(); i++) {
        osg::PrimitiveSet* ps = geom.getPrimitiveSetList()[i];
        if (needToSplit(ps, maxIndex)) {
            return true;
        }
    }
    return false;
}



struct ConvertToBindPerVertex {
    template <class T> void convert(T& array, osg::Geometry::AttributeBinding fromBinding, osg::Geometry::PrimitiveSetList& primitives, unsigned int size)
    {
        osg::ref_ptr<T> result = new T();
        for (unsigned int p = 0; p < primitives.size(); p++) {
            switch ( primitives[p]->getMode() ) {
            case osg::PrimitiveSet::POINTS:
                osg::notify(osg::WARN) << "ConvertToBindPerVertex not supported for POINTS" << std::endl;
                break;

            case osg::PrimitiveSet::LINE_STRIP:
                switch(fromBinding) {
                case osg::Geometry::BIND_OFF:
                case osg::Geometry::BIND_PER_VERTEX:
                    break;
                case osg::Geometry::BIND_OVERALL:
                {
                    for (unsigned int i = 0; i < primitives[p]->getNumIndices(); i++)
                        result->push_back(array[0]);
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE_SET:
                {
                    unsigned int nb = primitives[p]->getNumIndices();
                    for (unsigned int i = 0; i < nb; i++)
                        result->push_back(array[p]);
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE:
                {
                    unsigned int nb = primitives[p]->getNumPrimitives();
                    for (unsigned int i = 0; i < nb; i++) {
                        result->push_back(array[i]);
                    }
                }
                break;
                }
                break;
                
            case osg::PrimitiveSet::LINES:
                switch(fromBinding) {
                case osg::Geometry::BIND_OFF:
                case osg::Geometry::BIND_PER_VERTEX:
                    break;
                case osg::Geometry::BIND_OVERALL:
                {
                    for (unsigned int i = 0; i < primitives[p]->getNumIndices(); i++)
                        result->push_back(array[0]);
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE_SET:
                {
                    unsigned int nb = primitives[p]->getNumIndices();
                    for (unsigned int i = 0; i < nb; i++)
                        result->push_back(array[p]);
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE:
                {
                    unsigned int nb = primitives[p]->getNumPrimitives();
                    for (unsigned int i = 0; i < nb; i++) {
                        result->push_back(array[i]);
                        result->push_back(array[i]);
                    }
                }
                break;
                }
                break;

            case osg::PrimitiveSet::TRIANGLES:
                switch(fromBinding) {
                case osg::Geometry::BIND_OFF:
                case osg::Geometry::BIND_PER_VERTEX:
                    break;
                case osg::Geometry::BIND_OVERALL:
                {
                    for (unsigned int i = 0; i < primitives[p]->getNumIndices(); i++)
                        result->push_back(array[0]);
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE_SET:
                {
                    unsigned int nb = primitives[p]->getNumIndices();
                    for (unsigned int i = 0; i < nb; i++)
                        result->push_back(array[p]);
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE:
                {
                    unsigned int nb = primitives[p]->getNumPrimitives();
                    for (unsigned int i = 0; i < nb; i++) {
                        result->push_back(array[i]);
                        result->push_back(array[i]);
                        result->push_back(array[i]);
                    }
                }
                break;
                }
                break;

            case osg::PrimitiveSet::TRIANGLE_STRIP:
                switch(fromBinding) {
                case osg::Geometry::BIND_OFF:
                case osg::Geometry::BIND_PER_VERTEX:
                    break;
                case osg::Geometry::BIND_OVERALL:
                {
                    for (unsigned int i = 0; i < primitives[p]->getNumIndices(); i++)
                        result->push_back(array[0]);
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE_SET:
                {
                    osg::notify(osg::FATAL) << "Can't convert Array from BIND_PER_PRIMITIVE_SET to BIND_PER_VERTEX, for TRIANGLE_STRIP" << std::endl;
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE:
                {
                    osg::notify(osg::FATAL) << "Can't convert Array from BIND_PER_PRIMITIVE to BIND_PER_VERTEX, for TRIANGLE_STRIP" << std::endl;
                }
                break;
                }
                break;

            case osg::PrimitiveSet::TRIANGLE_FAN:
                switch(fromBinding) {
                case osg::Geometry::BIND_OFF:
                case osg::Geometry::BIND_PER_VERTEX:
                    break;
                case osg::Geometry::BIND_OVERALL:
                {
                    for (unsigned int i = 0; i < primitives[p]->getNumIndices(); i++)
                        result->push_back(array[0]);
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE_SET:
                {
                    osg::notify(osg::FATAL) << "Can't convert Array from BIND_PER_PRIMITIVE_SET to BIND_PER_VERTEX, for TRIANGLE_FAN" << std::endl;
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE:
                {
                    osg::notify(osg::FATAL) << "Can't convert Array from BIND_PER_PRIMITIVE to BIND_PER_VERTEX, for TRIANGLE_FAN" << std::endl;
                }
                break;
                }
                break;

            case osg::PrimitiveSet::QUADS:
                switch(fromBinding) {
                case osg::Geometry::BIND_OFF:
                case osg::Geometry::BIND_PER_VERTEX:
                    break;
                case osg::Geometry::BIND_OVERALL:
                {
                    for (unsigned int i = 0; i < primitives[p]->getNumIndices(); i++)
                        result->push_back(array[0]);
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE_SET:
                {
                    osg::notify(osg::FATAL) << "Can't convert Array from BIND_PER_PRIMITIVE_SET to BIND_PER_VERTEX, for QUADS" << std::endl;
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE:
                {
                    osg::notify(osg::FATAL) << "Can't convert Array from BIND_PER_PRIMITIVE to BIND_PER_VERTEX, for QUADS" << std::endl;
                }
                break;
                }
                break;

            case osg::PrimitiveSet::QUAD_STRIP:
                switch(fromBinding) {
                case osg::Geometry::BIND_OFF:
                case osg::Geometry::BIND_PER_VERTEX:
                    break;
                case osg::Geometry::BIND_OVERALL:
                {
                    for (unsigned int i = 0; i < primitives[p]->getNumIndices(); i++)
                        result->push_back(array[0]);
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE_SET:
                {
                    osg::notify(osg::FATAL) << "Can't convert Array from BIND_PER_PRIMITIVE_SET to BIND_PER_VERTEX, for QUAD_STRIP" << std::endl;
                }
                break;
                case osg::Geometry::BIND_PER_PRIMITIVE:
                {
                    osg::notify(osg::FATAL) << "Can't convert Array from BIND_PER_PRIMITIVE to BIND_PER_VERTEX, for QUAD_STRIP" << std::endl;
                }
                break;
                }
                break;
            }
        }
        array = *result;
    }

    template <class T> bool doConvert(osg::Array* src, osg::Geometry::AttributeBinding fromBinding, osg::Geometry::PrimitiveSetList& primitives, unsigned int size) {
        T* array= dynamic_cast<T*>(src);
        if (array) {
            convert(*array, fromBinding, primitives, size);
            return true;
        }
        return false;
    }

    void operator()(osg::Array* src, osg::Geometry::AttributeBinding fromBinding, osg::Geometry::PrimitiveSetList& primitives, unsigned int size) {

        if (doConvert<osg::Vec3Array>(src, fromBinding, primitives, size))
            return;

        if (doConvert<osg::Vec2Array>(src, fromBinding, primitives, size))
            return;

        if (doConvert<osg::Vec4Array>(src, fromBinding, primitives, size))
            return;

        if (doConvert<osg::Vec4ubArray>(src, fromBinding, primitives, size))
            return;
    }
};



template <class T> bool arraySetNumElements(osg::Array* src, unsigned int numElements)
{
    T* array= dynamic_cast<T*>(src);
    if (array) {
        array->resize(numElements);
        return true;
    }
    return false;
}

template <class T> bool arrayAppendElement(osg::Array* src, unsigned int index, osg::Array* dst)
{
    T* array= dynamic_cast<T*>(src);
    if (array) {
        T* arrayDst = dynamic_cast<T*>(dst);
        arrayDst->push_back((*array)[index]);
        return true;
    }
    return false;
}

struct ArrayList {

    struct ArrayAppendElement {
        void operator()(osg::Array* src, unsigned int index, osg::Array* dst) {

            if (arrayAppendElement<osg::FloatArray>(src, index, dst))
                return;

            if (arrayAppendElement<osg::Vec2Array>(src, index, dst))
                return;

            if (arrayAppendElement<osg::Vec3Array>(src, index, dst))
                return;

            if (arrayAppendElement<osg::Vec4Array>(src, index, dst))
                return;

            if (arrayAppendElement<osg::Vec4ubArray>(src, index, dst))
                return;
        }
    };

    struct ArraySetNumElements {
        void operator()(osg::Array* array, unsigned int numElements) {

            if (arraySetNumElements<osg::FloatArray>(array, numElements))
                return;

            if (arraySetNumElements<osg::Vec2Array>(array, numElements))
                return;

            if (arraySetNumElements<osg::Vec3Array>(array, numElements))
                return;

            if (arraySetNumElements<osg::Vec4Array>(array, numElements))
                return;

            if (arraySetNumElements<osg::Vec4ubArray>(array, numElements))
                return;
        }
    };

    osg::ref_ptr<osg::Array> _vertexes;
    osg::ref_ptr<osg::Array> _normals;
    osg::ref_ptr<osg::Array> _colors;
    osg::ref_ptr<osg::Array> _secondaryColors;
    osg::ref_ptr<osg::Array> _fogCoords;
    std::vector<osg::ref_ptr<osg::Array> > _texCoordArrays;
    std::vector<osg::ref_ptr<osg::Array> > _attributesArrays;

    ArrayList() {}
    ArrayList(osg::Geometry& geometry) {
        _vertexes = geometry.getVertexArray();
        _normals = geometry.getNormalArray();
        _colors = geometry.getColorArray();
        _secondaryColors = geometry.getSecondaryColorArray();
        _fogCoords = geometry.getFogCoordArray();

        _texCoordArrays.resize(geometry.getNumTexCoordArrays());
        for(unsigned int i=0;i<geometry.getNumTexCoordArrays();++i)
            _texCoordArrays[i] = geometry.getTexCoordArray(i);

        _attributesArrays.resize(geometry.getNumVertexAttribArrays());
        for(unsigned int i=0;i<geometry.getNumVertexAttribArrays();++i)
            _attributesArrays[i] = geometry.getVertexAttribArrayList()[i].array;
    }

    void setNumElements(unsigned int nbElements) {
        if (_vertexes.valid())
            ArraySetNumElements()(_vertexes.get(), nbElements);

        if (_normals.valid())
            ArraySetNumElements()(_normals.get(), nbElements);

        if (_colors.valid())
            ArraySetNumElements()(_colors.get(), nbElements);

        if (_secondaryColors.valid())
            ArraySetNumElements()(_secondaryColors.get(), nbElements);

        if (_fogCoords.valid())
            ArraySetNumElements()(_fogCoords.get(), nbElements);

        for (unsigned int i = 0; i < _texCoordArrays.size(); i++)
            if (_texCoordArrays[i].valid())
                ArraySetNumElements()(_texCoordArrays[i].get(), nbElements);

        for (unsigned int i = 0; i < _attributesArrays.size(); i++)
            if (_attributesArrays[i].valid())
                ArraySetNumElements()(_attributesArrays[i].get(), nbElements);
    }

    unsigned int append(unsigned int index, ArrayList& dst) {
        if (_vertexes.valid())
            ArrayAppendElement()(_vertexes.get(), index, dst._vertexes.get());

        if (_normals.valid())
            ArrayAppendElement()(_normals.get(), index, dst._normals.get());

        if (_colors.valid())
            ArrayAppendElement()(_colors.get(), index, dst._colors.get());

        if (_secondaryColors.valid())
            ArrayAppendElement()(_secondaryColors.get(), index, dst._secondaryColors.get());

        if (_fogCoords.valid())
            ArrayAppendElement()(_fogCoords.get(), index, dst._fogCoords.get());

        for (unsigned int i = 0; i < _texCoordArrays.size(); i++)
            if (_texCoordArrays[i].valid())
                ArrayAppendElement()(_texCoordArrays[i].get(), index, dst._texCoordArrays[i].get());

        for (unsigned int i = 0; i < _attributesArrays.size(); i++)
            if (_attributesArrays[i].valid())
                ArrayAppendElement()(_attributesArrays[i].get(), index, dst._attributesArrays[i].get());

        return dst._vertexes->getNumElements()-1;
    }

    ArrayList cloneType() const {
        ArrayList array;
        if (_vertexes.valid())
            array._vertexes = dynamic_cast<osg::Array*>(_vertexes->cloneType());

        if (_normals.valid())
            array._normals = dynamic_cast<osg::Array*>(_normals->cloneType());

        if (_colors.valid())
            array._colors = dynamic_cast<osg::Array*>(_colors->cloneType());

        if (_secondaryColors.valid())
            array._secondaryColors = dynamic_cast<osg::Array*>(_secondaryColors->cloneType());

        if (_fogCoords.valid())
            array._fogCoords = dynamic_cast<osg::Array*>(_fogCoords->cloneType());

        array._texCoordArrays.resize(_texCoordArrays.size());
        for (unsigned int i = 0; i < _texCoordArrays.size(); i++) {
            if (_texCoordArrays[i].valid())
                array._texCoordArrays[i] = dynamic_cast<osg::Array*>(_texCoordArrays[i]->cloneType());
        }

        array._attributesArrays.resize(_attributesArrays.size());
        for (unsigned int i = 0; i < _attributesArrays.size(); i++) {
            if (_attributesArrays[i].valid())
                array._attributesArrays[i] = dynamic_cast<osg::Array*>(_attributesArrays[i]->cloneType());
        }

        return array;
    }

    unsigned int size() const {
        return _vertexes->getNumElements();
    }

    void setToGeometry(osg::Geometry& geom) {
        if (_vertexes.valid())
            geom.setVertexArray(_vertexes.get());

        if (_normals.valid()) {
            geom.setNormalArray(_normals.get());
            geom.setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        }

        if (_colors.valid()) {
            geom.setColorArray(_colors.get());
            geom.setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        }

        if (_secondaryColors.valid()) {
            geom.setSecondaryColorArray(_secondaryColors.get());
            geom.setSecondaryColorBinding(osg::Geometry::BIND_PER_VERTEX);
        }

        if (_fogCoords.valid()) {
            geom.setFogCoordArray(_fogCoords.get());
            geom.setFogCoordBinding(osg::Geometry::BIND_PER_VERTEX);
        }

        for (unsigned int i = 0; i < _texCoordArrays.size(); ++i) {
            if (_texCoordArrays[i].valid())
                geom.setTexCoordArray(i, _texCoordArrays[i].get());
        }

        for (unsigned int i = 0; i < _attributesArrays.size(); ++i) {
            if (_attributesArrays[i].valid())
                geom.setVertexAttribArray(i, _attributesArrays[i].get());
        }
    }
};



class GeometryIndexSplitVisitor : public osg::NodeVisitor
{
public:
    GeometryIndexSplitVisitor(unsigned int maxIndex = 65535) : _maxIndexToSplit(maxIndex) {}
    bool split(osg::Geometry& geometry) {

        // split only index geometry triangles. 
        // Use another visitor like tristrip once the split is done
        for (unsigned int i = 0; i < geometry.getPrimitiveSetList().size(); i++) {
            if (geometry.getPrimitiveSetList()[i]) {

                if (!geometry.getPrimitiveSetList()[i]->getDrawElements()) {
                    osg::notify(osg::INFO) << "can't split Geometry " << geometry.getName() << " (" << &geometry << ") contains non indexed primitives" << std::endl;
                    return false;
                }

                switch (geometry.getPrimitiveSetList()[i]->getMode()) {
                case osg::PrimitiveSet::TRIANGLES:
                    break;
                default:
                    osg::notify(osg::FATAL) << "can't split Geometry " << geometry.getName() << " (" << &geometry << ") contains non triangles primitives" << std::endl;
                    return false;
                    break;
                }
            }
        }

        // check indexes
        if (!needToSplit(geometry, _maxIndexToSplit)) {
            return false;
        }

        // copy it because we will modify vertex arrays
        osg::ref_ptr<osg::Geometry> processing = osg::clone(&geometry, osg::CopyOp::DEEP_COPY_ALL);
        osg::ref_ptr<osg::Geometry> reported;
        bool doit = true;
        while (doit) {
            reported = doSplit(*processing, _maxIndexToSplit);

            // reduce vertex array if needed
            ArrayList arrayList(*processing);
            arrayList.setNumElements(osg::minimum(arrayList.size(), _maxIndexToSplit+1));

            _geometryList.push_back(processing);

            if (!reported.valid()) {
                doit = false;
            } else {
                processing = reported;
                reported = 0;
                {
                    // re order index elements
                    osgUtil::VertexAccessOrderVisitor cache;
                    cache.optimizeOrder(*processing);
                }
            }
        }

        osg::notify(osg::NOTICE) << "geometry " << &geometry << " " << geometry.getName() << " vertexes (" << geometry.getVertexArray()->getNumElements() << ") has DrawElements index > " << _maxIndexToSplit << ", splitted to " << _geometryList.size() << " geometry" << std::endl;

        return true;
    }

    typedef std::vector<osg::ref_ptr<osg::Geometry> > GeometryList;

protected:

    osg::Geometry* doSplit(osg::Geometry& geometry, unsigned int maxIndex)
    {
        osg::Geometry::PrimitiveSetList geomPrimitives;
        osg::Geometry::PrimitiveSetList reportedPrimitives;

        typedef std::pair<unsigned int, osg::ref_ptr<osg::DrawElementsUInt> > IndexPrimitive;
        std::vector<IndexPrimitive> primitivesToFix;
        osg::Geometry::PrimitiveSetList primitives = geometry.getPrimitiveSetList();
        for (unsigned int i = 0; i < primitives.size(); i++) {
            if (needToSplit(primitives[i], maxIndex)) {
                osg::DrawElementsUInt* deUInt = dynamic_cast<osg::DrawElementsUInt*>(primitives[i].get());
                primitivesToFix.push_back(IndexPrimitive(0,deUInt));
            } else {
                geomPrimitives.push_back(primitives[i]);
            }
        }

        if (!primitivesToFix.empty()) {
            osg::ref_ptr<osg::Geometry> reportGeometry = osg::clone(&geometry, osg::CopyOp::DEEP_COPY_ARRAYS);

            // need to fix draw elements with index > 65535
            for ( unsigned int i = 0; i < primitivesToFix.size(); i++) {
                osg::DrawElementsUInt* de = primitivesToFix[i].second;
            
                switch (de->getMode()) {
                case osg::PrimitiveSet::TRIANGLES:
                {
                    osg::DrawElementsUInt* fixed = de;
                    osg::DrawElementsUInt* reported = new osg::DrawElementsUInt(de->getMode());
                    for (int j = 0; j < de->getNumIndices()/3; j++) {
                        unsigned int base = j*3;
                        unsigned int i0 = de->getElement(base + 0);
                        unsigned int i1 = de->getElement(base + 1);
                        unsigned int i2 = de->getElement(base + 2);
                        if (i0 > maxIndex || i1 > maxIndex || i2 > maxIndex) {
                            reported->addElement(de->getElement(base + 0));
                            reported->addElement(de->getElement(base + 1));
                            reported->addElement(de->getElement(base + 2));
                        }
                    }
                    for (int j = de->getNumIndices()/3-1; j >= 0; j--) {
                        unsigned int base = j*3;
                        unsigned int i0 = de->getElement(base + 0);
                        unsigned int i1 = de->getElement(base + 1);
                        unsigned int i2 = de->getElement(base + 2);
                        if (i0 > maxIndex || i1 > maxIndex || i2 > maxIndex) {
                            fixed->erase(fixed->begin() + base + 2);
                            fixed->erase(fixed->begin() + base + 1);
                            fixed->erase(fixed->begin() + base + 0);
                        }
                    }
                    reportedPrimitives.push_back(reported);
                    geomPrimitives.push_back(fixed);
                }
                break;
                }
            }
            geometry.setPrimitiveSetList(geomPrimitives);
            if (!reportedPrimitives.empty()) {
                reportGeometry->setPrimitiveSetList(reportedPrimitives);
                return reportGeometry.release();
            }
        }
        return 0;
    }

public:
    unsigned int _maxIndexToSplit;
    GeometryList _geometryList;
};

class GeometrySeparateSurfaceAndNonSurfaceVisitor : public osg::NodeVisitor
{
public:
    GeometrySeparateSurfaceAndNonSurfaceVisitor() {}
    void separate(osg::Geometry& geometry) {

        _surfaces = osg::clone(&geometry);
        _nonSurfaces = osg::clone(&geometry);
        _surfaces->getPrimitiveSetList().clear();
        _nonSurfaces->getPrimitiveSetList().clear();

        for (unsigned int i = 0; i < geometry.getPrimitiveSetList().size(); i++) {
            if (geometry.getPrimitiveSetList()[i]) {
                switch (geometry.getPrimitiveSetList()[i]->getMode()) {
                case osg::PrimitiveSet::TRIANGLES:
                case osg::PrimitiveSet::TRIANGLE_STRIP:
                case osg::PrimitiveSet::TRIANGLE_FAN:
                case osg::PrimitiveSet::QUADS:
                case osg::PrimitiveSet::QUAD_STRIP:
                case osg::PrimitiveSet::POLYGON:
                    _surfaces->getPrimitiveSetList().push_back(geometry.getPrimitiveSetList()[i]);
                    break;
                
                default:
                    _nonSurfaces->getPrimitiveSetList().push_back(geometry.getPrimitiveSetList()[i]);
                    break;
                }
            }
        }
        if (_nonSurfaces->getPrimitiveSetList().empty()) {
            _nonSurfaces = 0;
        }
        if (_surfaces->getPrimitiveSetList().empty()) {
            _surfaces = 0;
        }
    }
public:
    osg::ref_ptr<osg::Geometry> _surfaces;
    osg::ref_ptr<osg::Geometry> _nonSurfaces;
};



static void convertToBindPerVertex(osg::Geometry& srcGeom)
{
    unsigned int size = srcGeom.getVertexArray()->getNumElements();
    if (srcGeom.getNormalArray() && srcGeom.getNormalBinding() != osg::Geometry::BIND_PER_VERTEX) {
        ConvertToBindPerVertex()(srcGeom.getNormalArray(), srcGeom.getNormalBinding(), srcGeom.getPrimitiveSetList(), size);
        srcGeom.setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }

    if (srcGeom.getColorArray() && srcGeom.getColorBinding() != osg::Geometry::BIND_PER_VERTEX) {
        ConvertToBindPerVertex()(srcGeom.getColorArray(), srcGeom.getColorBinding(), srcGeom.getPrimitiveSetList(), size);
        srcGeom.setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    }

    if (srcGeom.getSecondaryColorArray() && srcGeom.getSecondaryColorBinding() != osg::Geometry::BIND_PER_VERTEX) {
        ConvertToBindPerVertex()(srcGeom.getSecondaryColorArray(), srcGeom.getSecondaryColorBinding(), srcGeom.getPrimitiveSetList(), size);
        srcGeom.setSecondaryColorBinding(osg::Geometry::BIND_PER_VERTEX);
    }

    if (srcGeom.getFogCoordArray() && srcGeom.getFogCoordBinding() != osg::Geometry::BIND_PER_VERTEX) {
        ConvertToBindPerVertex()(srcGeom.getFogCoordArray(), srcGeom.getFogCoordBinding(), srcGeom.getPrimitiveSetList(), size);
        srcGeom.setFogCoordBinding(osg::Geometry::BIND_PER_VERTEX);
    }
}

static osg::Geometry* convertToDrawArray(osg::Geometry& geom)
{
    
    ArrayList srcArrays(geom);

    // clone but clear content
    osg::Geometry* newGeometry = new osg::Geometry;
    newGeometry->setStateSet(geom.getStateSet());
    ArrayList dst = srcArrays.cloneType();

    for (unsigned int i = 0; i < geom.getPrimitiveSetList().size(); i++) {

        osg::PrimitiveSet* ps = geom.getPrimitiveSetList()[i];
        switch (ps->getType()) {
        case osg::PrimitiveSet::DrawArraysPrimitiveType:
        {
            osg::DrawArrays* dw = dynamic_cast<osg::DrawArrays*>(ps);
            unsigned int start = dst.size();
            osg::DrawArrays* ndw = new osg::DrawArrays(dw->getMode(), start, dw->getNumIndices());
            newGeometry->getPrimitiveSetList().push_back(ndw);
            for ( unsigned int j = 0; j < dw->getNumIndices(); j++) {
                srcArrays.append(dw->getFirst()+j, dst);
            }
        }
        break;
        case osg::PrimitiveSet::DrawElementsUBytePrimitiveType:
        case osg::PrimitiveSet::DrawElementsUShortPrimitiveType:
        case osg::PrimitiveSet::DrawElementsUIntPrimitiveType:
        {
            osg::DrawElements* de = ps->getDrawElements();
            unsigned int start = dst.size();
            osg::DrawArrays* ndw = new osg::DrawArrays(de->getMode(), start, de->getNumIndices());
            newGeometry->getPrimitiveSetList().push_back(ndw);
            for (unsigned int j = 0; j < de->getNumIndices(); j++) {
                unsigned int idx = de->getElement(j);
                srcArrays.append(idx, dst);
            }
        }
        break;
        case osg::PrimitiveSet::DrawArrayLengthsPrimitiveType:
        {
            osg::DrawArrayLengths* dal = dynamic_cast<osg::DrawArrayLengths*>(ps);
            unsigned int start = dst.size();
            osg::DrawArrayLengths* ndal = new osg::DrawArrayLengths(dal->getMode(), start);
            newGeometry->getPrimitiveSetList().push_back(ndal);
            unsigned int offset = dal->getFirst();
            for (unsigned int j = 0; j < dal->size(); j++) {
                unsigned int nbVertexes = (*dal)[j];
                ndal->push_back(nbVertexes);

                for (unsigned int v = 0; v < nbVertexes; v++) {
                    srcArrays.append(offset, dst);
                }
                offset += nbVertexes;
            }
        }
        break;
        default:
            break;
        }
    }
    dst.setToGeometry(*newGeometry);
    return newGeometry;
}

static void mergeTrianglesStrip(osg::Geometry& geom)
{
    int nbtristrip = 0;
    int nbtristripVertexes = 0;

    for ( unsigned int i = 0; i < geom.getPrimitiveSetList().size(); i++) {
        osg::PrimitiveSet* ps = geom.getPrimitiveSetList()[i];
        osg::DrawElements* de = ps->getDrawElements();
        if (de && de->getMode() == osg::PrimitiveSet::TRIANGLE_STRIP) {
            nbtristrip++;
            nbtristripVertexes += de->getNumIndices();
        }
    }
    
    if (nbtristrip > 0) {
        osg::notify(osg::NOTICE) << "found " << nbtristrip << " tristrip, total vertexes " << nbtristripVertexes << " should result to " << nbtristripVertexes + nbtristrip*2 << " after connection" << std::endl;
        osg::DrawElementsUInt* ndw = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLE_STRIP);
        for ( unsigned int i = 0; i < geom.getPrimitiveSetList().size(); i++) {
            osg::PrimitiveSet* ps = geom.getPrimitiveSetList()[i];
            if (ps && ps->getMode() ==osg::PrimitiveSet::TRIANGLE_STRIP) {
                osg::DrawElements* de = ps->getDrawElements();
                if (de) {
                    // if connection needed insert degenerate triangles
                    if (ndw->getNumIndices() != 0) {
                        // duplicate last vertex
                        ndw->addElement(ndw->back());
                        // insert first vertex of next strip
                        ndw->addElement(de->getElement(0));
                    }

                    if (ndw->getNumIndices() % 2 != 0 ) {
                        // add a dummy vertex to reverse the strip
                        ndw->addElement(de->getElement(0));
                    }

                    for (unsigned int j = 0; j < de->getNumIndices(); j++) {
                        ndw->addElement(de->getElement(j));
                    }
                } else if (ps->getType() == osg::PrimitiveSet::DrawArraysPrimitiveType ) {
                    // trip strip can generate drawarray of 5 elements we want to merge them too
                    osg::DrawArrays* da = dynamic_cast<osg::DrawArrays*> (ps);
                    // if connection needed insert degenerate triangles
                    if (ndw->getNumIndices() != 0) {
                        // duplicate last vertex
                        ndw->addElement(ndw->back());
                        // insert first vertex of next strip
                        ndw->addElement(da->getFirst());
                    }

                    if (ndw->getNumIndices() % 2 != 0 ) {
                        // add a dummy vertex to reverse the strip
                        ndw->addElement(da->getFirst());
                    }

                    for (unsigned int j = 0; j < da->getNumIndices(); j++) {
                        ndw->addElement(da->getFirst() + j);
                    }
                }
            }
        }

        for ( int i = geom.getPrimitiveSetList().size()-1; i >= 0; i--) {
            osg::PrimitiveSet* ps = geom.getPrimitiveSetList()[i];
            if (ps && ps->getMode() == osg::PrimitiveSet::TRIANGLE_STRIP) {
                osg::DrawElements* de = ps->getDrawElements();
                if (de || ps->getType() == osg::PrimitiveSet::DrawArraysPrimitiveType) {
                    geom.getPrimitiveSetList().erase(geom.getPrimitiveSetList().begin() + i);
                }
            }
        }
        geom.getPrimitiveSetList().push_back(ndw);
    }
}

static void generateTriStrip(osg::Geometry* geom, int vertexCache, bool merge) 
{
    osgUtil::TriStripVisitor tristrip;
    tristrip.setCacheSize(vertexCache);
    tristrip.stripify(*geom);
                            
    // merge stritrip to one call
    if (merge)
        mergeTrianglesStrip(*geom);
}

static osg::Geometry* createWireframeGeometry(osg::Geometry& geom) {
    osg::ref_ptr<osg::Geometry> wireframe = osg::clone(&geom,"osgjs_wireframe",osg::CopyOp::DEEP_COPY_ALL);
    wireframe->getPrimitiveSetList().clear();
    for (unsigned int i = 0; i < geom.getPrimitiveSetList().size(); i++) {
        osg::PrimitiveSet* p = geom.getPrimitiveSetList()[i];
        if (!p)
            continue;
        switch (p->getMode()) {
        case GL_TRIANGLES:
        {
            osg::DrawElements* de = osg::cloneType(p->getDrawElements());
            de->setMode(GL_LINES);
            for (unsigned int j = 0; j < p->getDrawElements()->getNumIndices()/3; j++) {
                unsigned int v0 = p->getDrawElements()->getElement(j*3);
                unsigned int v1 = p->getDrawElements()->getElement(j*3+1);
                unsigned int v2 = p->getDrawElements()->getElement(j*3+2);
                de->addElement(v0);
                de->addElement(v1);
                de->addElement(v1);
                de->addElement(v2);
                de->addElement(v2);
                de->addElement(v0);
            }
            wireframe->getPrimitiveSetList().push_back(de);
            break;
        }
        default:
            osg::notify(osg::WARN) <<"wireframe model only support input geometry with triangles" << std::endl;
            break;
        }
    }
    return wireframe.release();
}

typedef std::vector<osg::ref_ptr<osg::Geometry> > GeometryList;
void OpenGLESGeometryOptimizerVisitor::apply(osg::Geode& node)
{
    GeometryList listGeometry;
    for (unsigned int i = 0; i < node.getNumDrawables(); ++i) {
        osg::ref_ptr<osg::Geometry> originalGeometry = dynamic_cast<osg::Geometry*>(node.getDrawable(i));

        if (originalGeometry) {
            ::convertToBindPerVertex(*originalGeometry);
            GeometryList localListGeometry;

            // first separate polygons from other primitives (lines/points)
            GeometrySeparateSurfaceAndNonSurfaceVisitor sepVisitor;
            sepVisitor.separate(*originalGeometry);

            osg::ref_ptr<osg::Geometry> triangles = sepVisitor._surfaces;
            osg::ref_ptr<osg::Geometry> nonTriangles = sepVisitor._nonSurfaces;

            // convert index triangles
            if (triangles.valid()) {

                osgUtil::IndexMeshVisitor indexer;
                indexer.setForceReIndex(true);
                indexer.makeMesh(*triangles);

                if (!_useDrawArray) {

                    // now split if needed too much data per geometry
                    GeometryIndexSplitVisitor splitter;
                    if (splitter.split(*triangles)) {
                        for (unsigned int j = 0; j < splitter._geometryList.size(); j++) {
                            osg::Geometry* geom = splitter._geometryList[j];
                            generateTriStrip(geom, _triStripCacheSize, !_disableMergeTriStrip);
                            // done
                            localListGeometry.push_back(geom);
                        }
                    } else {
                        generateTriStrip(triangles, _triStripCacheSize, !_disableMergeTriStrip);
                        // done
                        localListGeometry.push_back(triangles);
                    }
                } else {
                    if (!_disableTriStrip) {
                        generateTriStrip(triangles, _triStripCacheSize, !_disableMergeTriStrip);
                    }
                    triangles = convertToDrawArray(*triangles);
                    localListGeometry.push_back(triangles);
                }
            }
            
            // draw array all non triangles
            if (nonTriangles.valid()) {
                nonTriangles = convertToDrawArray(*nonTriangles);
                localListGeometry.push_back(nonTriangles);
            }

            if (!localListGeometry.empty()) {
                for (unsigned int g = 0; g < localListGeometry.size(); g++) {
                    listGeometry.push_back(localListGeometry[g]);
                }
            }
        }
    }
    node.removeDrawables(0,node.getNumDrawables());
    for (unsigned int d = 0; d < listGeometry.size(); d++)
        node.addDrawable(listGeometry[d].get());
}


bool GeometryWireframeVisitor::hasPolygonSurface(const osg::Geometry& geometry) {
    for (unsigned int i = 0 ; i < geometry.getNumPrimitiveSets(); i++) {
        const osg::PrimitiveSet* prim = geometry.getPrimitiveSet(i);
        if (!prim)
            continue;

        switch (prim->getMode() ) {
        case GL_TRIANGLES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
        case GL_QUADS:
        case GL_QUAD_STRIP:
        case GL_POLYGON:
            return true;
            break;
        default:
            continue;
        }
    }
    return false;
}
osg::Geometry* GeometryWireframeVisitor::applyGeometry(osg::Geometry& geometry) {
    geometry.setNormalArray(0);
    geometry.setColorArray(0);
    geometry.setSecondaryColorArray(0);
    geometry.setFogCoordArray(0);
    // we keep only vertexes
    for (unsigned int i = 0 ; i < geometry.getNumTexCoordArrays(); i++) {
        geometry.setTexCoordArray(i,0);
    }

    // filter primitive set list to keep only polygon surface
    osg::Geometry::PrimitiveSetList newlist;
    for (unsigned int i = 0 ; i < geometry.getNumPrimitiveSets(); i++) {
        osg::PrimitiveSet* prim = geometry.getPrimitiveSet(i);
        if (!prim)
            continue;

        switch (prim->getMode() ) {
        case GL_TRIANGLES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
        case GL_QUADS:
        case GL_QUAD_STRIP:
        case GL_POLYGON:
            newlist.push_back(prim);
            break;
        default:
            continue;
        }
    }

    if (newlist.empty())
        return 0;

    geometry.setPrimitiveSetList(newlist);

    // reindex primitives to creates triangles only
    osgUtil::IndexMeshVisitor indexer;
    indexer.setForceReIndex(true);
    indexer.makeMesh(geometry);

    return createWireframeGeometry(geometry);
}

void GeometryWireframeVisitor::apply(osg::Node& node) {
    node.setStateSet(0);
    traverse(node);
}

void GeometryWireframeVisitor::apply(osg::Geode& geode) {
    geode.setStateSet(0);
    GeometryList geomList;
    for (unsigned int i = 0; i < geode.getNumDrawables(); i++) {
        if (geode.getDrawable(i) && geode.getDrawable(i)->asGeometry()) {
            osg::Geometry* wireframe = applyGeometry(*geode.getDrawable(i)->asGeometry());
            if (wireframe) {
                wireframe->setStateSet(0);
                geomList.push_back(wireframe);
            }
        }
    }
    geode.removeDrawables(0, geode.getNumDrawables());
    for (unsigned int i = 0; i < geomList.size(); i++) {
        geode.addDrawable(geomList[i]);
    }
}
