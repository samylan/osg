#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgUtil/PrintVisitor>
#include <string>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>

class visitTest : public osg::NodeVisitor
{
public:
    visitTest()
    :osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {

    }

    virtual void apply(osg::Node& parcours)
    {
        osg::NodeVisitor::apply(parcours);
        osg::StateSet *etat = parcours.getStateSet();
        if(etat !=NULL)
        {
            osg::StateAttribute *attrTex = etat->getTextureAttribute(0, osg::StateAttribute::TEXTURE);
            if(attrTex != NULL)
            {
                osg::Texture *stockTex = attrTex->asTexture();
                osg::Image *stockImg = stockTex->getImage(0);
                if(stockImg != NULL)
                {
                std::string imgName = stockImg->getFileName();

                    if(imgName.substr(imgName.find_last_of(".") +1 ) == "dds")
                    {

                    }
                    else
                    {
                    imgName = imgName+".dds";
                    osgDB::writeImageFile(*stockImg, imgName);
                    osg::ref_ptr<osg::Image> loadImage (osgDB::readImageFile (imgName));
                    stockTex->setImage(0, loadImage);
                    }
                }

            }
        }
    }

};

class ReaderWriterValve : public osgDB::ReaderWriter
{
public:
	ReaderWriterValve()
	{
		supportsExtension("valve", "Forces nearest filter, for valve mdls.");
	}

	virtual const char* className()const
	{
		return"ReaderWriterValve";
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

    	visitTest visit;
      	node->accept(visit);

    	//node->setUserValue("source", std::string("valve"));
        return node.release();
    }
};

// Add ourself to the Registry to instantiate the reader/writer.
REGISTER_OSGPLUGIN(valve, ReaderWriterValve)
