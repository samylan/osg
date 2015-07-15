SET(TARGET_SRC
  ReaderWriterJSON.cpp
  WriteVisitor.cpp
  JSON_Objects.cpp
  Base64.cpp
  Animation.cpp)

SET(TARGET_H
    Adaptator
    Animation
    Base64
    CompactBufferVisitor
    JSON_Objects
    WriteVisitor
)
#### end var setup  ###
SET(TARGET_ADDED_LIBRARIES
    osgAnimation
    osgSim
    osgUtil)

SETUP_PLUGIN(osgjs)
