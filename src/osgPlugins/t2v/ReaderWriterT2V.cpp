//    copyright: 'Cedric Pinson cedric.pinson@plopbyte.net'
#include <osg/Image>
#include <osg/Notify>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>

#include <sstream>
#include <map>

using namespace osg;

class ReaderWriterT2V : public osgDB::ReaderWriter
{
public:

    ReaderWriterT2V()
    {
        supportsExtension("t2v","Texture to Vertex operator");
    }

    const char* className() const { return "Texture to Vertex Operator"; }


    ReadResult readObject(const std::string& file, const Options* options = 0) const
    {
        return readNode(file, options);
    }

    osg::Node* readTex2Vert(osg::Image* image) const
    {
        if (image->getPixelSizeInBits() != 32) {
            notify(osg::WARN) << "t2v plugin only works with 32 bits image" << std::endl;
            return 0;
        }

        osg::ref_ptr<osg::Vec3Array> vertexes = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
        for( int y=0; y<image->t(); y++ ) {
            for( int x=0; x<image->s(); x++ ) {
                vertexes->push_back(osg::Vec3(x * 1.0/image->s(), y * 1.0/image->t(),0.5));
                colors->push_back(osg::Vec4(image->data(x,y)[0]/255.0,
                                            image->data(x,y)[1]/255.0,
                                            image->data(x,y)[2]/255.0,
                                            image->data(x,y)[3]/255.0) );
            }
        }
        osg::Geometry* geom = new osg::Geometry;
        geom->setVertexArray(vertexes.get());
        geom->setColorArray(colors.get());
        geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        geom->getPrimitiveSetList().push_back(new osg::DrawArrays(GL_POINTS,0,vertexes->size()));
        geom->getOrCreateStateSet()->setMode(GL_BLEND,true);
        osg::Geode* geode = new osg::Geode;
        geode->addDrawable(geom);
        return geode;
    }

    ReadResult readNode(const std::string& file, const Options* options = 0) const
    {
        std::string ext = osgDB::getLowerCaseFileExtension(file);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        // strip the pseudo-loader extension
        std::string subLocation = osgDB::getNameLessExtension( file );
        if ( subLocation.empty() )
            return ReadResult::FILE_NOT_HANDLED;

        // recursively load the subfile.
        osg::ref_ptr<osg::Image> image = osgDB::readImageFile( subLocation, options );
        if( !image.valid() )
        {
            // propagate the read failure upwards
            osg::notify(osg::WARN) << "Subfile \"" << subLocation << "\" could not be loaded" << std::endl;
            return ReadResult::FILE_NOT_HANDLED;
        }

        ReadResult rr = readTex2Vert(image.get());
        return rr;
    }

};

// now register with Registry to instantiate the above
// reader/writer.
REGISTER_OSGPLUGIN(t2v, ReaderWriterT2V)
