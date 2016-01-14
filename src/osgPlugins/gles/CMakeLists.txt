SET(TARGET_SRC
    ReaderWriterGLES.cpp
    OpenGLESGeometryOptimizer.cpp
    TriangleStripVisitor.cpp
    IndexMeshVisitor.cpp
    UnIndexMeshVisitor.cpp
    forsythtriangleorderoptimizer.cpp)

SET(TARGET_H
    AABBonBoneVisitor
    AnimationCleanerVisitor
    BindPerVertexVisitor
    debug
    DetachPrimitiveVisitor
    DisableAnimationVisitor
    DrawArrayVisitor
    EdgeIndexFunctor
    forsythtriangleorderoptimizer.h
    GeometryArray
    GeometryInspector
    GeometrySplitterVisitor
    GeometryUniqueVisitor
    glesUtil
    IndexMeshVisitor
    LimitMorphTargetCount
    Line
    LineIndexFunctor
    MostInfluencedGeometryByBone
    OpenGLESGeometryOptimizer
    PointIndexFunctor
    PreTransformVisitor
    PrimitiveIndexors
    RigAnimationVisitor
    RigAttributesVisitor
    SmoothNormalVisitor
    StatLogger
    TriangleMeshGraph
    TangentSpaceVisitor
    TriangleStripVisitor
    UnIndexMeshVisitor
    WireframeVisitor
)

#### end var setup  ###
SET(TARGET_ADDED_LIBRARIES
    osgUtil osgAnimation)
SETUP_PLUGIN(gles)
