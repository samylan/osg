#include <osg/MatrixTransform>
#include <osg/Vec4f>
#include <osg/Light>
#include <osg/LightSource>
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
#include <stdlib.h>

std::set<std::string> transprentMaterials;

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
                    apply(stateSet, bothFaces);
                    
                    // clone the geometries to fake back faces. But reverse normals so lighting isn't messed up.
                    if (bothFaces) {
                        osg::Geometry* geometry = drawable->asGeometry();
                        size_t primitiveSetCount = geometry->getNumPrimitiveSets();
                        std::cout << primitiveSetCount << std::endl;
                        for (unsigned int j = 0; j < primitiveSetCount; j++) {
                            apply(geometry->getPrimitiveSet(j), geometry);
                        }

                    }
                }
            }
        }
        
        osg::NodeVisitor::apply(geode);
    }

    void apply(osg::PrimitiveSet* primitiveSet, osg::Geometry* geometry)
    {
        osg::DrawElements* drawElements = primitiveSet->getDrawElements();
        osg::PrimitiveSet::Type type = primitiveSet->getType();

        // there is no case with indexed vertex arrays (sketchfab flags), so the case isn't treated
        if (!drawElements) {
            if (type == osg::PrimitiveSet::DrawArraysPrimitiveType) {
                osg::DrawArrays* drawArrays = dynamic_cast<osg::DrawArrays*>(primitiveSet);

                // mirror the vertex array
                osg::Vec3Array* vertexArray = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
                if(vertexArray && _mirroredVec3Array.count(vertexArray) == 0) {
                    _mirroredVec3Array.insert(vertexArray);

                    unsigned int num = vertexArray->size();
                    for (unsigned int i = num; i > 0; i--) {
                        vertexArray->push_back((*vertexArray)[i - 1]);
                    }
                }

                // mirror the normal array and invert normals
                osg::Vec3Array* normalArray = dynamic_cast<osg::Vec3Array*>(geometry->getNormalArray());
                if(normalArray && _mirroredVec3Array.count(normalArray) == 0) {
                    _mirroredVec3Array.insert(normalArray);

                    unsigned int num = normalArray->size();
                    for (unsigned int i = num; i > 0; i--) {
                        normalArray->push_back(-(*normalArray)[i - 1]);
                    }
                }

                // mirror the texcoords
                osg::Vec2Array* texCoordArray = dynamic_cast<osg::Vec2Array*>(geometry->getTexCoordArray(0));
                if(texCoordArray && _mirroredVec2Array.count(texCoordArray) == 0) {
                    _mirroredVec2Array.insert(texCoordArray);

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
        }
    }

    void apply(osg::StateSet* stateSet, bool& bothFaces)
    {
        if (stateSet) {
            // apply the texture filter
            for (unsigned int i = 0; i < stateSet->getTextureAttributeList().size(); i++) {
                osg::Texture* texture = dynamic_cast<osg::Texture*>(stateSet->getTextureAttribute(i, osg::StateAttribute::TEXTURE));
                
                texture->setFilter(osg::Texture2D::MIN_FILTER, _min); 
                texture->setFilter(osg::Texture2D::MAG_FILTER, _mag);
            }

            // enable culling for materials mean't to be seen from both sides
            osg::Material* material = dynamic_cast<osg::Material*>(stateSet->getAttribute(osg::StateAttribute::MATERIAL));
            if (transprentMaterials.count(material->getName()) > 0) {
                osg::CullFace* cull = new osg::CullFace(); 
                cull->setMode(osg::CullFace::BACK); 
                stateSet->setAttributeAndModes(cull, osg::StateAttribute::ON); 
                bothFaces = true;

                osg::Texture* texture = dynamic_cast<osg::Texture*>(stateSet->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
            }

            stateSet->setUserValue("map_ka", std::string("0"));
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
        transprentMaterials.clear();
        transprentMaterials.insert("iron_bars");
        transprentMaterials.insert("torch");
        transprentMaterials.insert("torch_flame");
        transprentMaterials.insert("water");
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

        TextureFilterSetter visitor(osg::Texture2D::NEAREST, osg::Texture2D::NEAREST);
        node->accept(visitor);


        osg::LightSource* lightSource = new osg::LightSource();

        osg::Light* light = lightSource->getLight();
        light->setLightNum(0);
        light->setPosition(osg::Vec4(0.0, 0.5, 0.5, 0.0));
        light->setDiffuse(osg::Vec4(1.0, 1.0, 1.0, 1.0));
        light->setSpecular(osg::Vec4(1.0, 0.8, 0.8, 1.0));
        light->setAmbient(osg::Vec4(1.0, 1.0, 1.0, 1.0));


        osg::Group* root = new osg::Group();
        root->addChild(node);
        root->addChild(lightSource);

        return root;
    }
};

// Add ourself to the Registry to instantiate the reader/writer.
REGISTER_OSGPLUGIN(minecraft, ReaderWriterMinecraft)

/*EOF*/

