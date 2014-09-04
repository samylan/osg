//    copyright: 'Cedric Pinson cedric.pinson@plopbyte.net'
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

struct WriteVisitor : public osg::NodeVisitor
{
    int _textureWidth;
    int _textureHeight;
    std::string _basename;

    WriteVisitor(): osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
    void setBasename(const std::string& str) { _basename = str; }
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
        for (int i = 0; i < _textureWidth*_textureHeight; ++i) {
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

        osg::ref_ptr<osg::Image> low = new osg::Image();
        low->allocateImage(_textureWidth, _textureHeight, 1,GL_RGBA, GL_UNSIGNED_BYTE);

        osg::ref_ptr<osg::Image> high = new osg::Image();
        high->allocateImage(_textureWidth, _textureHeight, 1,GL_RGBA, GL_UNSIGNED_BYTE);

        osg::ref_ptr<osg::Image> color;
        if (!colors.empty()) {
            color = new osg::Image();
            color->allocateImage(_textureWidth, _textureHeight, 1,GL_RGBA, GL_UNSIGNED_BYTE);
        }

        int nbVertexes = 0;
        for (int i = 0; i < _textureWidth; ++i) {
            for (int j = 0; j < _textureHeight; ++j) {
                osg::Vec3 pos = positions[nbVertexes];
                if (color.valid()) {
                    osg::Vec4 c = colors[nbVertexes];
                    for (int z = 0; z < 4; ++z)
                        color->data()[j*_textureWidth*4 + i*4 + z] = c[z] * 255.0;
                }

                for (int z = 0; z < 3; ++z) {
                    double coord = pos[z];
                    if (bb._min[z] < 0.0) {
                        coord += -bb._min[z];
                    }
                    coord = coord/normalize;
#if 0
                    if (z == 2)
                        osg::notify(osg::NOTICE) << coord << std::endl;
#endif
                    coord *= 65535.0;
                    double highvalue = coord /256.0;
                    high->data()[j*_textureWidth*4 + i*4 + z] = (int)floor(highvalue);
                    low->data()[j*_textureWidth*4 + i*4 + z] = (highvalue-floor(highvalue))*255;
                }
                high->data()[j*_textureWidth*4 + i*4 + 3] = 255;
                low->data()[j*_textureWidth*4 + i*4 + 3] = 255;

                nbVertexes++;
            }
        }

        osgDB::writeImageFile(*high, _basename + "_high.png");
        osgDB::writeImageFile(*low, _basename + "_low.png");
        if (color.valid()) {
            osgDB::writeImageFile(*color, _basename + "_color.png");
        }
    }

    void apply(osg::Geode& node) {
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i) {
            if (node.getDrawable(i))
                apply(*node.getDrawable(i));
        }
    }

};


class ReaderWriterVST : public osgDB::ReaderWriter
{
public:
    ReaderWriterVST()
    {
        supportsExtension("vst","VST");
    }

    virtual const char* className() const { return "VST Writer"; }


    virtual WriteResult writeNode(const Node& node, const std::string& fileName, const osgDB::ReaderWriter::Options* options) const
    {
        std::string ext = osgDB::getFileExtension(fileName);
        if (!acceptsExtension(ext)) return WriteResult::FILE_NOT_HANDLED;

        WriteVisitor visitor;
        visitor.setBasename(osgDB::getNameLessExtension(fileName));
        visitor.setTextureHeight(512);
        visitor.setTextureWidth(512);

        osg::ref_ptr<osg::Node> dup = osg::clone(&node);
        dup->accept(visitor);
        return WriteResult::FILE_SAVED;
    }

};

// now register with Registry to instantiate the above
// reader/writer.
REGISTER_OSGPLUGIN(vst, ReaderWriterVST)
