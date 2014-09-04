//    copyright: 'Cedric Pinson cedric.pinson@plopbyte.com'
#include <osg/Image>
#include <osg/Notify>
#include <osg/Geode>
#include <osg/GL>
#include <osg/Endian>
#include <osg/Projection>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>

#include <osg/io_utils>
#include <osgUtil/ShaderGen>
#include <osgDB/WriteFile>

#include <osgDB/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>

#include <sstream>
#include <map>

using namespace osg;

float frac(float v) { return v-floorf(v);}
unsigned char convertFloatToByte(float v) { return (unsigned char)(v*255.0f); }
float convertByteToFloat(unsigned char v) { return (float)(v)/255.0f; }

osg::Vec4ub toRGBA(float z)
{
    float depth = z;
    depth *= 256.0f;
    float depth0 = floorf(depth);
    depth = frac(depth);

    depth *= 256.0f;
    float depth1 = floorf(depth);
    depth = frac(depth);

    depth *= 256.0f;
    float depth2 = floorf(depth);
    depth = frac(depth);

    float r = depth0;
    float g = depth1;
    float b = depth2;
    float a = 0;

    osg::Vec4ub bytes(convertFloatToByte(r/255.0f),
                      convertFloatToByte(g/255.0f),
                      convertFloatToByte(b/255.0f),
                      convertFloatToByte(a/255.0f));
    return bytes;
}

struct WriteVisitor : public osg::NodeVisitor
{
    int _textureWidth;
    int _textureHeight;
    std::string _filename;

    WriteVisitor(): osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
    void setFilename(const std::string& str) { _filename = str; }
    void setTextureWidth(int w) { _textureWidth = w; }
    void setTextureHeight(int h) { _textureHeight = h; }

    void apply(osg::Drawable& drw) {
        osg::Geometry* geom = dynamic_cast<osg::Geometry*>(&drw);
        if (!geom)
            return;
        std::vector<osg::Vec3> positions;
        std::vector<osg::Vec4> colors;

        osg::Vec3Array* positionSource = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
        osg::Vec4Array* colorSource = dynamic_cast<osg::Vec4Array*>(geom->getColorArray());
        for (int i = 0; i < _textureWidth/4*_textureHeight; ++i) {
            osg::Vec3 pos = (*positionSource)[i%(positionSource->size())];
            positions.push_back(pos);
            if (colorSource) {
                osg::Vec4 color = (*colorSource)[i%(colorSource->size())];
                colors.push_back(color);
            }
        }

        osg::BoundingBox bb = geom->getBoundingBox();
        double normalize = 0;
        osg::Vec3 dim = bb._max-bb._min;
        for (int i = 0; i < 3; ++i)
            if (dim[i] > normalize)
                normalize = dim[i];

        osg::notify(osg::NOTICE) << "min " << bb._min << " max " << bb._max << std::endl;
        osg::notify(osg::NOTICE) << "size " <<dim << std::endl;

        osg::ref_ptr<osg::Image> image = new osg::Image();
        image->allocateImage(_textureWidth, _textureHeight, 1,GL_RGBA, GL_UNSIGNED_BYTE);
        int nbVertexes = 0;
        for (int j = 0; j < _textureHeight; ++j) {
            for (int i = 0; i < _textureWidth/4; i++) {
                osg::Vec3 pos = positions[nbVertexes];
                osg::Vec4 color = osg::Vec4(0.5, 0.5, 0.5, 1.0);
                if (!colors.empty()) {
                    color = colors[nbVertexes];
                }

                // write each component xyz on 24 bits
                for (int z = 0; z < 3; ++z) {
                    double coord = pos[z] - bb._min[z];

                    coord = coord/(normalize) * (1.0 - (2.0/(float)(0xffffff)));
                    osg::Vec4ub value = toRGBA(coord);
                    //osg::notify(osg::NOTICE) << " point " << coord << " vec4 " << value << std::endl;
                    for (int cc = 0; cc < 3; cc++) {
                        image->data()[j*_textureWidth*4 + i*16 + z*4 + cc] = value[cc];
                    }
                    // unsused but easier to preview content
                    image->data()[j*_textureWidth*4 + i*16 + z*4 + 3] = 255;
                }

                // color on last texel
                for (int cc = 0; cc < 4; cc++) {
                    image->data()[j*_textureWidth*4 + i*16 + 3*4 + cc] = (unsigned char)floor(color[cc] * 255.0);
                }
                nbVertexes++;
            }
        }

        osgDB::writeImageFile(*image, _filename);
    }

    void apply(osg::Geode& node) {
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i) {
            if (node.getDrawable(i))
                apply(*node.getDrawable(i));
        }
    }

};


class ReaderWriterV2T : public osgDB::ReaderWriter
{
public:
    ReaderWriterV2T()
    {
        supportsExtension("vert2text32","convert vertexes of model to 4 rgba encoded (x,y,z,w) in a texture of 4*512 x 256 - support 131072 vertexes + color");
    }

    virtual const char* className() const { return "vert2text32 writer"; }


    virtual WriteResult writeNode(const Node& node, const std::string& fileName, const osgDB::ReaderWriter::Options* options) const
    {
        std::string ext = osgDB::getFileExtension(fileName);
        if (!acceptsExtension(ext)) return WriteResult::FILE_NOT_HANDLED;

        WriteVisitor visitor;
        visitor.setFilename(osgDB::getNameLessExtension(fileName));
        visitor.setTextureHeight(256);
        visitor.setTextureWidth(2048);

        osg::ref_ptr<osg::Node> dup = osg::clone(&node);
        dup->accept(visitor);
        return WriteResult::FILE_SAVED;
    }

};

// now register with Registry to instantiate the above
// reader/writer.
REGISTER_OSGPLUGIN(vert2text32, ReaderWriterV2T)
