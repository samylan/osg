//    copyright: 'Cedric Pinson cedric.pinson@plopbyte.net'
#include <osg/Image>
#include <osg/Notify>
#include <osg/Geode>
#include <osg/GL>
#include <osg/Version>
#include <osg/Endian>
#include <osg/Projection>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>

#include <osgUtil/UpdateVisitor>
#include <osgDB/WriteFile>

#include <osgDB/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>

#include <osgAnimation/UpdateMatrixTransform>
#include <osgAnimation/AnimationManagerBase>
#include <osgAnimation/BasicAnimationManager>

#include <sstream>
#include <cassert>
#include <map>
#include "TangentSpace"
#include "JSON_Objects"

#include "GeometryOperation"
#include "Animation"
#include "WriteVisitor"



using namespace osg;



// the idea is to create true Geometry if skeleton with RigGeometry
struct FakeUpdateVisitor : public osgUtil::UpdateVisitor
{
    FakeUpdateVisitor() {
        setFrameStamp(new osg::FrameStamp());
    }
};

class ReaderWriterJSON : public osgDB::ReaderWriter
{
public:

     struct OptionsStruct {
         bool generateTangentSpace;
         int tangentSpaceTextureUnit;
         bool disableTriStrip;
         bool disableMergeTriStrip;
         int triStripCacheSize;
         bool useDrawArray;
         bool enableWireframe;
         bool useExternalBinaryArray;

         OptionsStruct() {
             generateTangentSpace = false;
             tangentSpaceTextureUnit = 0;
             disableTriStrip = false;
             disableMergeTriStrip = false;
             triStripCacheSize = 16;
             useDrawArray = false;
             enableWireframe = false;
             useExternalBinaryArray = false;
         }
    };


    ReaderWriterJSON()
    {
        supportsExtension("osgjs","OpenSceneGraph Javascript implementation format");
        supportsOption("generateTangentSpace","Build tangent space to each geometry");
        supportsOption("tangentSpaceTextureUnit=<unit>","Specify on wich texture unit normal map is");
        supportsOption("triStripCacheSize=<int>","set the cache size when doing tristrip");
        supportsOption("disableMergeTriStrip","disable the merge of all tristrip into one");
        supportsOption("disableTriStrip","disable generation of tristrip");
        supportsOption("useDrawArray","prefer drawArray instead of drawelement with split of geometry");
        supportsOption("enableWireframe","create a wireframe geometry for each triangles geometry");
    }
        
    virtual const char* className() const { return "OSGJS json Writer"; }


    virtual WriteResult writeNode(const Node& node, const std::string& fileName, const osgDB::ReaderWriter::Options* options) const
    {
        std::string ext = osgDB::getFileExtension(fileName);
        if (!acceptsExtension(ext)) return WriteResult::FILE_NOT_HANDLED;

        {
            std::ofstream fout(fileName.c_str());
            if (fout)
            {
                OptionsStruct _options;
                _options = parseOptions(options);
                WriteResult res = writeNodeModel(node, fout, osgDB::getNameLessExtension(fileName), _options);
                fout.close();
                return res;
            }
        }
        return WriteResult("Unable to open file for output");
    }

    virtual WriteResult writeNode(const Node& node, std::ostream& fout, const osgDB::ReaderWriter::Options* options) const
    {
        if (!fout) {
            return WriteResult("Unable to write to output stream");
        }

        OptionsStruct _options;
        _options = parseOptions(options);
        return writeNodeModel(node, fout, "stream", _options);
    }

    virtual WriteResult writeNodeModel(const Node& node, std::ostream& fout, const std::string& basename, const OptionsStruct& options) const
    {
        // process regular model
        osg::ref_ptr<osg::Node> model = osg::clone(&node);
        //osgDB::writeNodeFile(*model, "cloned.osg");
        FakeUpdateVisitor fakeUpdateVisitor;
        model->accept(fakeUpdateVisitor);

        GeometryWireframeVisitor wireframer;
        if (options.enableWireframe) {
            model->accept(wireframer);
        }
            
        // generate model tangent space
        if (options.generateTangentSpace && options.enableWireframe == false) {
            TangentSpaceVisitor tgen(options.tangentSpaceTextureUnit);
            model->accept(tgen);
        }

        OpenGLESGeometryOptimizerVisitor visitor;
        visitor.setUseDrawArray(options.useDrawArray);
        visitor.setTripStripCacheSize(options.triStripCacheSize);
        visitor.setDisableTriStrip(options.disableTriStrip);
        visitor.setDisableMergeTriStrip(options.disableMergeTriStrip);
        model->accept(visitor);

        WriteVisitor writer;
        try {
            //osgDB::writeNodeFile(*model, "/tmp/debug_osgjs.osg");
            writer.setBaseName(basename);
            writer.useExternalBinaryArray(options.useExternalBinaryArray);
            model->accept(writer);
            if (writer._root.valid()) {
                writer.write(fout);
                return WriteResult::FILE_SAVED;
            }
        } catch (...){
            osg::notify(osg::FATAL) << "can't save osgjs file" << std::endl;
            return WriteResult("Unable to write to output stream");
        }
        return WriteResult("Unable to write to output stream");
    }

    ReaderWriterJSON::OptionsStruct parseOptions(const osgDB::ReaderWriter::Options* options) const
    {
        OptionsStruct localOptions;

        if (options)
        {
            osg::notify(NOTICE) << "options " << options->getOptionString() << std::endl;
            std::istringstream iss(options->getOptionString());
            std::string opt;
            while (iss >> opt)
            {
                // split opt into pre= and post=
                std::string pre_equals;
                std::string post_equals;

                size_t found = opt.find("=");
                if(found!=std::string::npos)
                {
                    pre_equals = opt.substr(0,found);
                    post_equals = opt.substr(found+1);
                } 
                else
                {
                    pre_equals = opt;
                }

                if (pre_equals == "useDrawArray")
                {
                    localOptions.useDrawArray = true;
                }
                if (pre_equals == "enableWireframe")
                {
                    localOptions.enableWireframe = true;
                }
                if (pre_equals == "disableMergeTriStrip")
                {
                    localOptions.disableMergeTriStrip = true;
                }
                if (pre_equals == "disableTriStrip")
                {
                    localOptions.disableTriStrip = true;
                }                
                if (pre_equals == "generateTangentSpace")
                {
                    localOptions.generateTangentSpace = true;
                }
                if (pre_equals == "useExternalBinaryArray") 
                {
                    localOptions.useExternalBinaryArray = true;
                }

                if (post_equals.length()>0)
                {    
                    if (pre_equals == "tangentSpaceTextureUnit") {
                        localOptions.tangentSpaceTextureUnit = atoi(post_equals.c_str());
                    }
                    if (pre_equals == "triStripCacheSize") {
                        localOptions.triStripCacheSize = atoi(post_equals.c_str());
                    }
                }
            }
        }
        return localOptions;
    }
protected:

};

// now register with Registry to instantiate the above
// reader/writer.
REGISTER_OSGPLUGIN(osgjs, ReaderWriterJSON)
