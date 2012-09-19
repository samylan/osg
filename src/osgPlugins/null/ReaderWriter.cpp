// simple pseudo loader to get scene informations

#include <osgDB/WriteFile>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>

class ReaderWriter : public osgDB::ReaderWriter
{
public:

    ReaderWriter()
    {
        supportsExtension("null","Null output");
    }
        
    virtual const char* className() const { return "null driver"; }

    virtual WriteResult writeNode(const osg::Node& node, const std::string& fileName, const osgDB::ReaderWriter::Options* options) const
    {
        return WriteResult::FILE_SAVED;
    }

    virtual WriteResult writeNode(const osg::Node& node, std::ostream& fout, const osgDB::ReaderWriter::Options* options) const
    {
        return WriteResult::FILE_SAVED;
    }

protected:

};

// now register with Registry to instantiate the above
// reader/writer.
REGISTER_OSGPLUGIN(null, ReaderWriter)
