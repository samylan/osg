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

float convertByteToFloat(unsigned char v) { return (float)(v)/255.0f; }
float toFloat(const osg::Vec4ub& rgba)
{
    osg::Vec4f depth(convertByteToFloat(rgba[0]),
                     convertByteToFloat(rgba[1]),
                     convertByteToFloat(rgba[2]),
                     convertByteToFloat(rgba[3]));
    float z;
    z = depth[0] * 255.0f / 256.0f + depth[1] * 255.0f / (256.0f * 256.0f) + depth[2] * 255.0f / (256.0f * 256.0f * 256.0f);
    return z;
}

class ReaderWriterT2V : public osgDB::ReaderWriter
{
public:

    ReaderWriterT2V()
    {
        supportsExtension("text2vert32","Texture to Vertex operator");
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
            for( int x=0; x<image->s()/4; x++ ) {
                float xx = toFloat(osg::Vec4ub(image->data(x*4+ 0,y)[0],
                                               image->data(x*4+ 0,y)[1],
                                               image->data(x*4+ 0,y)[2],
                                               image->data(x*4+ 0,y)[3]));
                float yy = toFloat(osg::Vec4ub(image->data(x*4+ 1,y)[0],
                                               image->data(x*4+ 1,y)[1],
                                               image->data(x*4+ 1,y)[2],
                                               image->data(x*4+ 1,y)[3]));
                float zz = toFloat(osg::Vec4ub(image->data(x*4+ 2,y)[0],
                                               image->data(x*4+ 2,y)[1],
                                               image->data(x*4+ 2,y)[2],
                                               image->data(x*4+ 2,y)[3]));
                osg::Vec4 cc = osg::Vec4(image->data(x*4+ 3,y)[0]/255.0,
                                         image->data(x*4+ 3,y)[1]/255.0,
                                         image->data(x*4+ 3,y)[2]/255.0,
                                         image->data(x*4+ 3,y)[3]/255.0);

                vertexes->push_back(osg::Vec3(xx, yy, zz));
                colors->push_back(cc);
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
REGISTER_OSGPLUGIN(text2vert32, ReaderWriterT2V)
