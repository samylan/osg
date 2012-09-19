// simple pseudo loader to get scene informations

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include "StatsVisitor"

class ReaderWriter : public osgDB::ReaderWriter
{
public:

    ReaderWriter()
    {
        supportsExtension("scene-stats","OpenSceneGraph report stats of the scene");
    }
        
    virtual const char* className() const { return "scene stats pseudo loader"; }


    virtual ReadResult readNode(const std::string& fileName, const osgDB::ReaderWriter::Options* options) const
    {
        std::string ext = osgDB::getFileExtension(fileName);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        std::string file2 = osgDB::getNameLessExtension(fileName);
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(file2);
        StatsVisitor sv;
        node->accept(sv);
        sv.dump();
        return node.release();
    }

protected:

};

// now register with Registry to instantiate the above
// reader/writer.
REGISTER_OSGPLUGIN(scene_stats, ReaderWriter)
