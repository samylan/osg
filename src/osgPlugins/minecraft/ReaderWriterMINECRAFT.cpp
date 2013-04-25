#include <osg/MatrixTransform>
#include <osg/Vec4f>
#include <osg/Light>
#include <osg/LightSource>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/ReaderWriter>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgUtil/Optimizer>
#include <stdlib.h>

class TextureFilterSetter : public osg::NodeVisitor
{
protected:
    osg::Texture::FilterMode _min, _mag;
public:
    TextureFilterSetter(osg::Texture::FilterMode min, osg::Texture::FilterMode mag)
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), _min(min), _mag(mag)
	{
	}

    virtual void apply(osg::Geode& geode)
    {
        apply(geode.getStateSet());

        for (unsigned int i = 0; i < geode.getNumDrawables(); ++i) {
            osg::Drawable* drawable = geode.getDrawable(i);
            if (drawable) {
                apply(drawable->getStateSet());
            }
        }
        
        osg::NodeVisitor::apply(geode);
    }

    void apply(osg::StateSet* stateSet)
    {
        if (stateSet) {
            for (unsigned int i = 0; i < stateSet->getTextureAttributeList().size(); i++) {
		osg::Texture* texture = dynamic_cast<osg::Texture*>(stateSet->getTextureAttribute(i, osg::StateAttribute::TEXTURE));
                
                texture->setFilter(osg::Texture2D::MIN_FILTER, _min); 
                texture->setFilter(osg::Texture2D::MAG_FILTER, _mag);
            }
        }
    }
};

class ReaderWriterMinecraft : public osgDB::ReaderWriter
{
public:
    ReaderWriterMinecraft()
    {
        std::cout << ".minecraft" << std::endl;
        supportsExtension("minecraft", "Forces nearest filter, for minecraft maps.");
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
        light->setPosition(osg::Vec4(0.0, 0.0, 1.0, 0.0));
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

