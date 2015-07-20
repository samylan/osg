#include <osg/MatrixTransform>
#include <osg/Vec4f>
#include <osg/CullFace>
#include <osg/Material>
#include <osg/UserDataContainer>
#include <osg/ValueObject>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/ReaderWriter>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgUtil/Optimizer>
#include <osgUtil/SmoothingVisitor>
#include <stdlib.h>

std::set<std::string> doubleSidedMaterials;
std::set<std::string> backFaceCulledMaterials;

class TextureFilterSetter : public osg::NodeVisitor
{
protected:
    osg::Texture::FilterMode _min, _mag;
    std::set<osg::Vec2Array*> _mirroredVec2Array;
    std::set<osg::Vec3Array*> _mirroredVec3Array;
public:
    TextureFilterSetter(osg::Texture::FilterMode min, osg::Texture::FilterMode mag)
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), _min(min), _mag(mag)
    {
    }

    virtual void apply(osg::Geode& geode)
    {
        for (unsigned int i = 0; i < geode.getNumDrawables(); ++i) {
            osg::ref_ptr<osg::Drawable> drawable = geode.getDrawable(i);
            if (drawable) {
                osg::ref_ptr<osg::StateSet> stateSet = drawable->getStateSet();
                if (stateSet) {
                    bool bothFaces = false;
                    apply(stateSet.get(), bothFaces);

                    // clone the geometries to fake back faces. But reverse normals so lighting isn't messed up.
                    osg::Geometry* geometry = drawable->asGeometry();
                    size_t primitiveSetCount = geometry->getNumPrimitiveSets();
                    std::cout << primitiveSetCount << std::endl;
                    for (unsigned int j = 0; j < primitiveSetCount; j++) {
                        apply(geometry->getPrimitiveSet(j), geometry, bothFaces);
                    }
                }
            }
        }

        osg::NodeVisitor::apply(geode);
    }

    void apply(osg::PrimitiveSet* primitiveSet, osg::Geometry* geometry, bool bothFaces)
    {
        osg::DrawElements* drawElements = primitiveSet->getDrawElements();
        osg::PrimitiveSet::Type type = primitiveSet->getType();

        // there is no case with indexed vertex arrays (sketchfab flags), so the case isn't treated
        if (!drawElements) {
            if (type == osg::PrimitiveSet::DrawArraysPrimitiveType) {
                osg::DrawArrays* drawArrays = dynamic_cast<osg::DrawArrays*>(primitiveSet);

                osg::ref_ptr<osg::Vec3Array> vertexArray = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
                osg::ref_ptr<osg::Vec2Array> texCoordArray = dynamic_cast<osg::Vec2Array*>(geometry->getTexCoordArray(0));

                if (bothFaces) {
                    // mirror the vertex array
                    if (vertexArray && _mirroredVec3Array.count(vertexArray.get()) == 0) {
                        _mirroredVec3Array.insert(vertexArray.get());

                        unsigned int num = vertexArray->size();
                        for (unsigned int i = 0; i < num; i++) {
                            osg::Vec3f a = (*vertexArray)[i];
                            vertexArray->push_back(osg::Vec3f(a.x() / 2.0f, a.y(), a.z()));
                        }
                        for (unsigned int i = num; i > 0; i--) {
                            vertexArray->push_back((*vertexArray)[i - 1]);
                        }
                    }

                    // mirror the texcoords
                    if (texCoordArray && _mirroredVec2Array.count(texCoordArray.get()) == 0) {
                        _mirroredVec2Array.insert(texCoordArray.get());

                        unsigned int num = texCoordArray->size();
                        for (unsigned int i = num; i > 0; i--) {
                            texCoordArray->push_back((*texCoordArray)[i - 1]);
                        }
                    }

                    // add mirrored as new primitiveset
                    GLint first = vertexArray->size() - (drawArrays->getFirst() + drawArrays->getCount());
                    GLsizei count = drawArrays->getCount();
                    osg::DrawArrays* drawArraysMirror = new osg::DrawArrays(drawArrays->getMode(), first, count);
                    geometry->addPrimitiveSet(drawArraysMirror);
                }

                // regenerate normals
                osgUtil::SmoothingVisitor::smooth(*geometry, osg::PI / 4.0f);
            }
        }
    }

    void apply(osg::StateSet* stateSet, bool& bothFaces)
    {
        if (stateSet) {
            stateSet->setUserValue("source", std::string("minecraft"));

            // apply the texture filter
            for (unsigned int i = 0; i < stateSet->getTextureAttributeList().size(); i++) {
                osg::Texture* texture = dynamic_cast<osg::Texture*>(stateSet->getTextureAttribute(i, osg::StateAttribute::TEXTURE));

                texture->setFilter(osg::Texture2D::MIN_FILTER, _min);
                texture->setFilter(osg::Texture2D::MAG_FILTER, _mag);
                //texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
                //texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
                //texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
                //texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
            }

            // enable culling for materials mean't to be seen from both sides
            osg::Material* material = dynamic_cast<osg::Material*>(stateSet->getAttribute(osg::StateAttribute::MATERIAL));
            bool isDoubleSided = doubleSidedMaterials.count(material->getName()) > 0;
            bool isBackFaceCulled = backFaceCulledMaterials.count(material->getName()) > 0;

            if (isDoubleSided || isBackFaceCulled) {
                osg::CullFace* cull = new osg::CullFace();
                cull->setMode(osg::CullFace::BACK);
                stateSet->setAttributeAndModes(cull, osg::StateAttribute::ON);
            }
            if (isDoubleSided) {
                bothFaces = true;
            }

            stateSet->setUserValue("map_ka", std::string("0")); // EmitColor

            if (stateSet->getTextureAttributeList().size() > 1) {
                stateSet->setUserValue("map_d", std::string("1")); // Opacity
            }
        }
    }
};

class ReaderWriterMinecraft : public osgDB::ReaderWriter
{
public:
    ReaderWriterMinecraft()
    {
        supportsExtension("minecraft", "Forces nearest filter, for minecraft maps.");

        // doubled sided materials
        doubleSidedMaterials.clear();
        doubleSidedMaterials.insert("iron_bars");
        doubleSidedMaterials.insert("torch");
        doubleSidedMaterials.insert("torch_flame");

        backFaceCulledMaterials.clear();
        backFaceCulledMaterials.insert("water");
    }

    virtual const char* className() const
    {
        return "ReaderWriterMinecraft";
    }

    virtual ReadResult readNode(const std::string& file, const osgDB::ReaderWriter::Options* options) const
    {
        // check if the pseudo loader supports the extension
        std::string ext = osgDB::getLowerCaseFileExtension(file);
        if (!acceptsExtension(ext))
            return ReadResult::FILE_NOT_HANDLED;

        // strip the pseudo extension from the filename
        std::string realName = osgDB::getNameLessExtension(file);

        // check if real file exists
        std::string fileName = osgDB::findDataFile(realName, options);
        if (realName.empty())
            return ReadResult::FILE_NOT_FOUND;

        // read file
        OSG_INFO << "realName = \"" << realName << "\"" << std::endl;
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(realName, options);
        if (!node.valid())
        {
            osg::notify(osg::WARN) << "File \"" << fileName << "\" could not be loaded" << std::endl;
            return ReadResult::FILE_NOT_HANDLED;
        }

        TextureFilterSetter visitor(osg::Texture2D::LINEAR_MIPMAP_LINEAR, osg::Texture2D::LINEAR);
        node->accept(visitor);

        node->setUserValue("source", std::string("minecraft"));
        return node.release();
    }
};

// Add ourself to the Registry to instantiate the reader/writer.
REGISTER_OSGPLUGIN(minecraft, ReaderWriterMinecraft)

/*EOF*/
