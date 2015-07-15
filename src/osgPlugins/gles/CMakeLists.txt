SET(TARGET_SRC
    ReaderWriterGLES.cpp
    OpenGLESGeometryOptimizer.cpp
    TriangleStripVisitor.cpp
    IndexMeshVisitor.cpp
    UnIndexMeshVisitor.cpp)

SET(TARGET_H
    AnimationVisitor
    BindPerVertexVisitor
    DetachPrimitiveVisitor
    DrawArrayVisitor
    EdgeIndexFunctor
    GeometryArray
    GeometrySplitterVisitor
    GeometryUniqueVisitor
    glesUtil
    IndexMeshVisitor
    LineIndexFunctor
    OpenGLESGeometryOptimizer
    PointIndexFunctor
    PreTransformVisitor
    PrimitiveIndexors
    TangentSpaceVisitor
    TriangleLinePointIndexFunctor
    TriangleStripVisitor
    UnIndexMeshVisitor
    WireframeVisitor
)

#### end var setup  ###
SET(TARGET_ADDED_LIBRARIES
    osgUtil)
SETUP_PLUGIN(gles)
