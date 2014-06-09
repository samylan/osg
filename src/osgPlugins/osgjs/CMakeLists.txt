SET(TARGET_SRC ReaderWriterJSON.cpp WriteVisitor.cpp JSON_Objects.cpp GeometryOperation.cpp Animation.cpp GeometryProcess.cpp UnIndexMeshVisitor.cpp Base64.cpp)
SET(TARGET_H
    Adaptator
    Animation
    Base64
    GeometryArray
    GeometryArrayUtils
    GeometryOperation
    GeometryProcess
    JSON_Objects
    LineIndexFunctor
    PointIndexFunctor
    StatsVisitor
    TangentSpace
    UnIndexMeshVisitor
    WriteVisitor
)
#### end var setup  ###
SET(TARGET_ADDED_LIBRARIES osgAnimation osgUtil osgSim) 
SETUP_PLUGIN(osgjs)
