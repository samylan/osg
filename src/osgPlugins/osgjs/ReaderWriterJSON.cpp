//    copyright: 'Cedric Pinson cedric.pinson@plopbyte.net'
#include <osg/Image>
#include <osg/Notify>
#include <osg/Geode>
#include <osg/GL>
#include <osg/Endian>
#include <osg/Projection>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>

#include <osgUtil/UpdateVisitor>
#include <osgDB/WriteFile>

#include <osgDB/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>

#include <sstream>
#include <map>
#include "JSON_Objects"
#include "GeometrySplitter"

using namespace osg;

struct WriteVisitor : public osg::NodeVisitor
{
    typedef std::vector<osg::ref_ptr<osg::StateSet> > StateSetStack;

    std::map<osg::Object*, osg::ref_ptr<JSONObjectBase> > _maps;
    std::vector<osg::ref_ptr<JSONObject> > _parents;
    osg::ref_ptr<JSONObject> _root;
    StateSetStack _stateset;

    WriteVisitor(): osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}

    JSONObject* createStateSet(osg::StateSet* stateset);

    JSONObject* getParent() {
        if (_parents.empty()) {
            _root = new JSONObject;
            _parents.push_back(_root.get());
        }
        return _parents.back().get();
    }

    void apply(osg::Drawable& drw) {
        if (drw.getStateSet())
            _stateset.push_back(drw.getStateSet());

        osg::Geometry* geom = dynamic_cast<osg::Geometry*>(&drw);
        if (geom) {
            osg::ref_ptr<JSONObject> json = new JSONObject;
            
            if (drw.getStateSet()) {
                JSONObject* stateset = createJSONStateSet(drw.getStateSet());
                if (stateset)
                    json->getMaps()["StateSet"] = stateset;
            }

            JSONObject* parent = getParent();
            if (parent)
                parent->addChild(json);

            if (!geom->getName().empty())
                json->getMaps()["Name"] = new JSONValue<std::string>(geom->getName());

            osg::ref_ptr<JSONObject> attributes = new JSONObject;
            
            if (geom->getVertexArray()) {
                attributes->getMaps()["Vertex"] = new JSONBufferArray(geom->getVertexArray());
            }
            if (geom->getNormalArray()) {
                attributes->getMaps()["Normal"] = new JSONBufferArray(geom->getNormalArray());
            }
            if (geom->getColorArray()) {
                attributes->getMaps()["Color"] = new JSONBufferArray(geom->getColorArray());
            }
            
            if (geom->getTexCoordArray(0)) {
                attributes->getMaps()["TexCoord0"] = new JSONBufferArray(geom->getTexCoordArray(0));
            }

            if (geom->getTexCoordArray(1)) {
                attributes->getMaps()["TexCoord1"] = new JSONBufferArray(geom->getTexCoordArray(1));
            }

            if (geom->getTexCoordArray(2)) {
                attributes->getMaps()["TexCoord2"] = new JSONBufferArray(geom->getTexCoordArray(2));
            }

            if (geom->getTexCoordArray(3)) {
                attributes->getMaps()["TexCoord3"] = new JSONBufferArray(geom->getTexCoordArray(3));
            }

            if (geom->getTexCoordArray(4)) {
                attributes->getMaps()["TexCoord4"] = new JSONBufferArray(geom->getTexCoordArray(4));
            }
            json->getMaps()["Attributes"] = attributes;


            if (!geom->getPrimitiveSetList().empty()) {
                osg::ref_ptr<JSONArray> primitives = new JSONArray();
                for (unsigned int i = 0; i < geom->getPrimitiveSetList().size(); ++i) {

                    if (geom->getPrimitiveSetList()[i]->getType() == osg::PrimitiveSet::DrawArraysPrimitiveType) {
                        osg::DrawArrays& da = dynamic_cast<osg::DrawArrays&>(*(geom->getPrimitiveSetList()[i]));
                        if (da.getMode() == GL_QUADS) {
                            osg::ref_ptr<JSONDrawArray> jsonDA = new JSONDrawArray(da);
                            
                            primitives->getArray().push_back(createJSONDrawElements(da));
                        } else
                            primitives->getArray().push_back(new JSONDrawArray(da));
                    } else if (geom->getPrimitiveSetList()[i]->getType() == osg::PrimitiveSet::DrawElementsUIntPrimitiveType) {
                        osg::DrawElementsUInt& da = dynamic_cast<osg::DrawElementsUInt&>(*(geom->getPrimitiveSetList()[i]));
                        primitives->getArray().push_back(new JSONDrawElements<osg::DrawElementsUInt>(da));

                    }  else if (geom->getPrimitiveSetList()[i]->getType() == osg::PrimitiveSet::DrawElementsUShortPrimitiveType) {
                        osg::DrawElementsUShort& da = dynamic_cast<osg::DrawElementsUShort&>(*(geom->getPrimitiveSetList()[i]));
                        primitives->getArray().push_back(new JSONDrawElements<osg::DrawElementsUShort>(da));

                    }  else if (geom->getPrimitiveSetList()[i]->getType() == osg::PrimitiveSet::DrawElementsUBytePrimitiveType) {
                        osg::DrawElementsUByte& da = dynamic_cast<osg::DrawElementsUByte&>(*(geom->getPrimitiveSetList()[i]));
                        primitives->getArray().push_back(new JSONDrawElements<osg::DrawElementsUByte>(da));

                    } else {
                        osg::notify(osg::WARN) << "Primitive Type " << geom->getPrimitiveSetList()[i]->getType() << " not supported, skipping" << std::endl;
                    }
                }
                json->getMaps()["Primitives"] = primitives;
            }
        }
        if (drw.getStateSet())
            _stateset.pop_back();
    }

    void initJsonObjectFromNode(osg::Node& node, JSONObject& json) {
        if (!node.getName().empty())
            json.getMaps()["Name"] = new JSONValue<std::string>(node.getName());
    }

    void apply(osg::Geode& node) {

        if (node.getStateSet())
            _stateset.push_back(node.getStateSet());

        if (_maps.find(&node) == _maps.end()) {
            osg::ref_ptr<JSONObject> json = new JSONObject;

            if (node.getStateSet()) {
                JSONObject* stateset = createJSONStateSet(node.getStateSet());
                if (stateset)
                    json->getMaps()["StateSet"] = stateset;
            }

            JSONObject* parent = getParent();
            if (parent)
                parent->addChild(json);
            initJsonObjectFromNode(node, *json);
            _parents.push_back(json);
            for (unsigned int i = 0; i < node.getNumDrawables(); ++i) {
                if (node.getDrawable(i))
                    apply(*node.getDrawable(i));
            }
            _parents.pop_back();
        }
        if (node.getStateSet())
            _stateset.pop_back();
    }

    void apply(osg::Group& node) {
        if (node.getStateSet())
            _stateset.push_back(node.getStateSet());

        if (_maps.find(&node) == _maps.end()) {
            osg::ref_ptr<JSONObject> json = new JSONObject;

            if (node.getStateSet()) {
                JSONObject* stateset = createJSONStateSet(node.getStateSet());
                if (stateset)
                    json->getMaps()["StateSet"] = stateset;
            }

            JSONObject* parent = getParent();
            if (parent)
                parent->addChild(json);

            initJsonObjectFromNode(node, *json);
            _parents.push_back(json);
            traverse(node);
            _parents.pop_back();
        }
        if (node.getStateSet())
            _stateset.pop_back();
    }

    void apply(osg::Projection& node) 
    {
        if (node.getStateSet())
            _stateset.push_back(node.getStateSet());

        if (_maps.find(&node) == _maps.end()) {
            osg::ref_ptr<JSONObject> json = new JSONObject;

            if (node.getStateSet()) {
                JSONObject* stateset = createJSONStateSet(node.getStateSet());
                if (stateset)
                    json->getMaps()["StateSet"] = stateset;
            }

            JSONObject* parent = getParent();
            if (parent)
                parent->addChild(json);

            initJsonObjectFromNode(node, *json);
            json->getMaps()["Projection"] = new JSONMatrix(node.getMatrix());
            _parents.push_back(json);
            traverse(node);
            _parents.pop_back();
        }
        if (node.getStateSet())
            _stateset.pop_back();
    }

    void apply(osg::MatrixTransform& node) {
        if (node.getStateSet())
            _stateset.push_back(node.getStateSet());

        if (_maps.find(&node) == _maps.end()) {
            osg::ref_ptr<JSONObject> json = new JSONObject;

            if (node.getStateSet()) {
                JSONObject* stateset = createJSONStateSet(node.getStateSet());
                if (stateset)
                    json->getMaps()["StateSet"] = stateset;
            }

            JSONObject* parent = getParent();
            if (parent)
                parent->addChild(json);

            initJsonObjectFromNode(node, *json);
            json->getMaps()["Matrix"] = new JSONMatrix(node.getMatrix());
            _parents.push_back(json);
            traverse(node);
            _parents.pop_back();
        }
        if (node.getStateSet())
            _stateset.pop_back();
    }
    void apply(osg::PositionAttitudeTransform& node) 
    {
        if (node.getStateSet())
            _stateset.push_back(node.getStateSet());

        if (_maps.find(&node) == _maps.end()) {
            osg::ref_ptr<JSONObject> json = new JSONObject;

            if (node.getStateSet()) {
                JSONObject* stateset = createJSONStateSet(node.getStateSet());
                if (stateset)
                    json->getMaps()["StateSet"] = stateset;
            }

            JSONObject* parent = getParent();
            if (parent)
                parent->addChild(json);
            initJsonObjectFromNode(node, *json);
            Matrix matrix = Matrix::identity();
            node.computeLocalToWorldMatrix(matrix,0);
            json->getMaps()["Matrix"] = new JSONMatrix(matrix);
            _parents.push_back(json);
            traverse(node);
            _parents.pop_back();
        }
        if (node.getStateSet())
            _stateset.pop_back();
    }

};

// the idea is to create true Geometry if skeleton with RigGeometry
struct FakeUpdateVisitor : public osgUtil::UpdateVisitor
{
    FakeUpdateVisitor() {
        setFrameStamp(new osg::FrameStamp());
    }
};

class ReaderWriterJSON : public osgDB::ReaderWriter
{
public:
    ReaderWriterJSON()
    {
        supportsExtension("json", "OpenSceneGraph Javascript implementation format");
        supportsExtension("osgjs","OpenSceneGraph Javascript implementation format");
    }
        
    virtual const char* className() const { return "JSON Writer"; }


    virtual WriteResult writeNode(const Node& node, const std::string& fileName, const osgDB::ReaderWriter::Options* options) const
    {
        std::string ext = osgDB::getFileExtension(fileName);
        if (!acceptsExtension(ext)) return WriteResult::FILE_NOT_HANDLED;

        std::ofstream fout(fileName.c_str());
        if (fout)
        {
            WriteResult res = writeNode(node, fout, options);
            fout.close();
            return res;
        }
        return WriteResult("Unable to open file for output");
    }

    virtual WriteResult writeNode(const Node& node, std::ostream& fout, const osgDB::ReaderWriter::Options* options) const
    {
        if (fout)
        {
            osg::ref_ptr<osg::Node> dup = osg::clone(&node);
            {
            SplitGeometryVisitor visitor;
            dup->accept(visitor);
            }

            {
            WriteVisitor visitor;
            FakeUpdateVisitor fakeUpdateVisitor;
            dup->accept(fakeUpdateVisitor);
            dup->accept(visitor);
            if (visitor._root.valid()) {
                visitor._root->write(fout);
                return WriteResult::FILE_SAVED;
            }
            }
        }
        return WriteResult("Unable to write to output stream");
    }
};

// now register with Registry to instantiate the above
// reader/writer.
REGISTER_OSGPLUGIN(osgjs, ReaderWriterJSON)
