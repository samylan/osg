/*  -*-c++-*-
 *  Copyright (C) 2011 Cedric Pinson <cedric.pinson@plopbyte.com>
 */

#include <osgAnimation/Animation>
#include <osgAnimation/Channel>
#include <osgAnimation/CubicBezier>
#include <osgAnimation/Sampler>
#include <osgAnimation/UpdateMatrixTransform>
#include <osgAnimation/StackedTranslateElement>
#include <osgAnimation/StackedQuaternionElement>
#include <osgAnimation/StackedRotateAxisElement>
#include <osgAnimation/StackedMatrixElement>
#include <osg/Array>
#include "JSON_Objects"

static bool addJSONChannelVec3(osgAnimation::Vec3LinearChannel* channel, JSONObject& anim)
{
    if (channel && channel->getSampler()) {
        osg::ref_ptr<JSONObject> json = new JSONObject;
        json->getMaps()["Name"] = new JSONValue<std::string>(channel->getName());
        json->getMaps()["TargetName"] = new JSONValue<std::string>(channel->getTargetName());
        osgAnimation::Vec3KeyframeContainer* keys = channel->getSamplerTyped()->getKeyframeContainerTyped();
        osg::ref_ptr<JSONObject> jsKeys = new JSONObject;
        osg::ref_ptr<osg::FloatArray> timesArray = new osg::FloatArray;
        osg::ref_ptr<osg::FloatArray> keysArrayX = new osg::FloatArray;
        osg::ref_ptr<osg::FloatArray> keysArrayY = new osg::FloatArray;
        osg::ref_ptr<osg::FloatArray> keysArrayZ = new osg::FloatArray;

        for (unsigned int i = 0; i < keys->size(); i++) {
            timesArray->push_back((*keys)[i].getTime());
            keysArrayX->push_back((*keys)[i].getValue()[0]);
            keysArrayY->push_back((*keys)[i].getValue()[1]);
            keysArrayZ->push_back((*keys)[i].getValue()[2]);
        }

        osg::ref_ptr<JSONArray> keysArray = new JSONArray;
        keysArray->asArray()->getArray().push_back(new JSONBufferArray(keysArrayX.get()));
        keysArray->asArray()->getArray().push_back(new JSONBufferArray(keysArrayY.get()));
        keysArray->asArray()->getArray().push_back(new JSONBufferArray(keysArrayZ.get()));

        jsKeys->getMaps()["Time"] = new JSONBufferArray(timesArray.get());
        jsKeys->getMaps()["Key"] = keysArray;
        json->getMaps()["KeyFrames"] = jsKeys;

        osg::ref_ptr<JSONObject> jsonChannel = new JSONObject();
        jsonChannel->getMaps()["osgAnimation.Vec3LerpChannel"] = json;
        anim.getMaps()["Channels"]->asArray()->getArray().push_back(jsonChannel);
        return true;
    }
    return false;
}

static bool addJSONChannelFloat(osgAnimation::FloatLinearChannel* channel, JSONObject& anim)
{
    if (channel->getSampler()) {
        osg::ref_ptr<JSONObject> json = new JSONObject;
        json->getMaps()["Name"] = new JSONValue<std::string>(channel->getName());
        json->getMaps()["TargetName"] = new JSONValue<std::string>(channel->getTargetName());
        osgAnimation::FloatKeyframeContainer* keys = channel->getSamplerTyped()->getKeyframeContainerTyped();
        osg::ref_ptr<JSONObject> jsKeys = new JSONObject;
        osg::ref_ptr<osg::FloatArray> timesArray = new osg::FloatArray;
        osg::ref_ptr<osg::FloatArray> keysArray = new osg::FloatArray;

        for (unsigned int i = 0; i < keys->size(); i++) {
            timesArray->push_back((*keys)[i].getTime());
            keysArray->push_back((*keys)[i].getValue());
        }

        jsKeys->getMaps()["Time"] = new JSONBufferArray(timesArray.get());
        jsKeys->getMaps()["Key"] = new JSONBufferArray(keysArray.get());
        json->getMaps()["KeyFrames"] = jsKeys;

        osg::ref_ptr<JSONObject> jsonChannel = new JSONObject();
        jsonChannel->getMaps()["osgAnimation.FloatLerpChannel"] = json;
        anim.getMaps()["Channels"]->asArray()->getArray().push_back(jsonChannel);
        return true;
    }
    return false;
}

static bool addJSONChannelQuaternion(osgAnimation::QuatSphericalLinearChannel* channel, JSONObject& anim)
{
    if (channel->getSampler()) {
        osg::ref_ptr<JSONObject> json = new JSONObject;
        json->getMaps()["Name"] = new JSONValue<std::string>(channel->getName());
        json->getMaps()["TargetName"] = new JSONValue<std::string>(channel->getTargetName());
        osgAnimation::QuatKeyframeContainer* keys = channel->getSamplerTyped()->getKeyframeContainerTyped();
        osg::ref_ptr<JSONObject> jsKeys = new JSONObject;
        osg::ref_ptr<osg::FloatArray> timesArray = new osg::FloatArray;

        osg::ref_ptr<osg::FloatArray> keysArrayX = new osg::FloatArray;
        osg::ref_ptr<osg::FloatArray> keysArrayY = new osg::FloatArray;
        osg::ref_ptr<osg::FloatArray> keysArrayZ = new osg::FloatArray;
        osg::ref_ptr<osg::FloatArray> keysArrayW = new osg::FloatArray;

        for (unsigned int i = 0; i < keys->size(); i++) {
            timesArray->push_back((*keys)[i].getTime());
            keysArrayX->push_back((*keys)[i].getValue()[0]);
            keysArrayY->push_back((*keys)[i].getValue()[1]);
            keysArrayZ->push_back((*keys)[i].getValue()[2]);
            keysArrayW->push_back((*keys)[i].getValue()[3]);
        }

        osg::ref_ptr<JSONArray> keysArray = new JSONArray;
        keysArray->asArray()->getArray().push_back(new JSONBufferArray(keysArrayX.get()));
        keysArray->asArray()->getArray().push_back(new JSONBufferArray(keysArrayY.get()));
        keysArray->asArray()->getArray().push_back(new JSONBufferArray(keysArrayZ.get()));
        keysArray->asArray()->getArray().push_back(new JSONBufferArray(keysArrayW.get()));

        jsKeys->getMaps()["Time"] = new JSONBufferArray(timesArray.get());
        jsKeys->getMaps()["Key"] = keysArray;
        json->getMaps()["KeyFrames"] = jsKeys;

        osg::ref_ptr<JSONObject> jsonChannel = new JSONObject();
        jsonChannel->getMaps()["osgAnimation.QuatSlerpChannel"] = json;
        anim.getMaps()["Channels"]->asArray()->getArray().push_back(jsonChannel);
        return true;
    }
    return false;
}

static bool addJSONChannelFloatCubicBezier(osgAnimation::FloatCubicBezierChannel* channel, JSONObject& anim)
{
    if(channel->getSampler()) {
        osg::ref_ptr<JSONObject> json = new JSONObject;
        json->getMaps()["Name"] = new JSONValue<std::string>(channel->getName());
        json->getMaps()["TargetName"] = new JSONValue<std::string>(channel->getTargetName());

        osgAnimation::FloatCubicBezierKeyframeContainer * keys = channel->getSamplerTyped()->getKeyframeContainerTyped();
        osg::ref_ptr<osg::FloatArray> timeArray = new osg::FloatArray,
                positionArray = new osg::FloatArray,
                controlPointInArray = new osg::FloatArray,
                controlPointOutArray = new osg::FloatArray;

        for (unsigned int i = 0; i < keys->size(); i++) {
            timeArray->push_back((*keys)[i].getTime());
            positionArray->push_back((*keys)[i].getValue().getPosition());
            controlPointInArray->push_back((*keys)[i].getValue().getControlPointIn());
            controlPointOutArray->push_back((*keys)[i].getValue().getControlPointOut());
        }
        osg::ref_ptr<JSONObject> jsKeys = new JSONObject;

        osg::ref_ptr<JSONBufferArray> controlOutVertexArray = new JSONBufferArray(controlPointOutArray.get());
        jsKeys->getMaps()["ControlPointOut"] = controlOutVertexArray;

        osg::ref_ptr<JSONBufferArray> controlInVertexArray = new JSONBufferArray(controlPointInArray.get());
        jsKeys->getMaps()["ControlPointIn"] = controlInVertexArray;

        osg::ref_ptr<JSONBufferArray> positionVertexArray = new JSONBufferArray(positionArray.get());
        jsKeys->getMaps()["Position"] = positionVertexArray;

        osg::ref_ptr<JSONBufferArray> timeVertexArray = new JSONBufferArray(timeArray.get());
        jsKeys->getMaps()["Time"] = timeVertexArray;

        json->getMaps()["KeyFrames"] = jsKeys;

        osg::ref_ptr<JSONObject> jsonChannel = new JSONObject();
        jsonChannel->getMaps()["osgAnimation.FloatCubicBezierChannel"] = json;
        anim.getMaps()["Channels"]->asArray()->getArray().push_back(jsonChannel);
        return true;
    }
    return false;
}

static bool addJSONChannelVec3CubicBezier(osgAnimation::Vec3CubicBezierChannel* channel, JSONObject& anim)
{
    if(channel->getSampler()) {
        osg::ref_ptr<JSONObject> json = new JSONObject;
        json->getMaps()["Name"] = new JSONValue<std::string>(channel->getName());
        json->getMaps()["TargetName"] = new JSONValue<std::string>(channel->getTargetName());

        osgAnimation::Vec3CubicBezierKeyframeContainer * keys = channel->getSamplerTyped()->getKeyframeContainerTyped();
        osg::ref_ptr<osg::FloatArray> timeArray = new osg::FloatArray,
                positionArrayX = new osg::FloatArray,
                positionArrayY = new osg::FloatArray,
                positionArrayZ = new osg::FloatArray,

                controlPointInArrayX = new osg::FloatArray,
                controlPointInArrayY = new osg::FloatArray,
                controlPointInArrayZ = new osg::FloatArray,

                controlPointOutArrayX = new osg::FloatArray,
                controlPointOutArrayY = new osg::FloatArray,
                controlPointOutArrayZ = new osg::FloatArray;

        for (unsigned int i = 0; i < keys->size(); i++) {
            timeArray->push_back((*keys)[i].getTime());

            positionArrayX->push_back((*keys)[i].getValue().getPosition().x());
            positionArrayY->push_back((*keys)[i].getValue().getPosition().y());
            positionArrayZ->push_back((*keys)[i].getValue().getPosition().z());

            controlPointInArrayX->push_back((*keys)[i].getValue().getControlPointIn().x());
            controlPointInArrayY->push_back((*keys)[i].getValue().getControlPointIn().y());
            controlPointInArrayZ->push_back((*keys)[i].getValue().getControlPointIn().z());

            controlPointOutArrayX->push_back((*keys)[i].getValue().getControlPointOut().x());
            controlPointOutArrayY->push_back((*keys)[i].getValue().getControlPointOut().y());
            controlPointOutArrayZ->push_back((*keys)[i].getValue().getControlPointOut().z());
        }
        osg::ref_ptr<JSONObject> jsKeys = new JSONObject;

        osg::ref_ptr<JSONArray> jsControlPointOutArray = new JSONArray;
        jsControlPointOutArray->asArray()->getArray().push_back(new JSONBufferArray(controlPointOutArrayX.get()));
        jsControlPointOutArray->asArray()->getArray().push_back(new JSONBufferArray(controlPointOutArrayY.get()));
        jsControlPointOutArray->asArray()->getArray().push_back(new JSONBufferArray(controlPointOutArrayZ.get()));
        jsKeys->getMaps()["ControlPointOut"] = jsControlPointOutArray;

        osg::ref_ptr<JSONArray> jsControlPointInArray = new JSONArray;
        jsControlPointInArray->asArray()->getArray().push_back(new JSONBufferArray(controlPointInArrayX.get()));
        jsControlPointInArray->asArray()->getArray().push_back(new JSONBufferArray(controlPointInArrayY.get()));
        jsControlPointInArray->asArray()->getArray().push_back(new JSONBufferArray(controlPointInArrayZ.get()));
        jsKeys->getMaps()["ControlPointIn"] = jsControlPointInArray;

        osg::ref_ptr<JSONArray> jsPositionVertexArray = new JSONArray;
        jsPositionVertexArray->asArray()->getArray().push_back(new JSONBufferArray(positionArrayX.get()));
        jsPositionVertexArray->asArray()->getArray().push_back(new JSONBufferArray(positionArrayY.get()));
        jsPositionVertexArray->asArray()->getArray().push_back(new JSONBufferArray(positionArrayZ.get()));
        jsKeys->getMaps()["Position"] = jsPositionVertexArray;

        osg::ref_ptr<JSONBufferArray> timeVertexArray = new JSONBufferArray(timeArray.get());
        jsKeys->getMaps()["Time"] = timeVertexArray;

        json->getMaps()["KeyFrames"] = jsKeys;

        osg::ref_ptr<JSONObject> jsonChannel = new JSONObject();
        jsonChannel->getMaps()["osgAnimation.Vec3CubicBezierChannel"] = json;
        anim.getMaps()["Channels"]->asArray()->getArray().push_back(jsonChannel);
        return true;
    }
    return false;
}

static void addJSONChannel(osgAnimation::Channel* channel, JSONObject& anim)
{
    {
        osgAnimation::Vec3LinearChannel* c = dynamic_cast<osgAnimation::Vec3LinearChannel*>(channel);
        if (c) {
            if (addJSONChannelVec3(c, anim))
                return;
        }
    }

    {
        osgAnimation::FloatLinearChannel* c = dynamic_cast<osgAnimation::FloatLinearChannel*>(channel);
        if (c) {
            if (addJSONChannelFloat(c, anim))
                return;
        }
    }

    {
        osgAnimation::QuatSphericalLinearChannel* c = dynamic_cast<osgAnimation::QuatSphericalLinearChannel*>(channel);
        if (c) {
            if (addJSONChannelQuaternion(c, anim))
                return;
        }
    }

    {
        osgAnimation::FloatCubicBezierChannel* c = dynamic_cast<osgAnimation::FloatCubicBezierChannel*>(channel);
        if (c) {
            if (addJSONChannelFloatCubicBezier(c, anim))
                return;
        }
    }

    {
        osgAnimation::Vec3CubicBezierChannel *c = dynamic_cast<osgAnimation::Vec3CubicBezierChannel*>(channel);
        if (c) {
            if (addJSONChannelVec3CubicBezier(c, anim))
                return;
        }
    }
}

JSONObject* createJSONAnimation(osgAnimation::Animation* anim)
{
    osg::ref_ptr<JSONObject> json = new JSONObject;
    json->getMaps()["Channels"] = new JSONArray();
    json->getMaps()["Name"] = new JSONValue<std::string>(anim->getName());

    for (unsigned int i = 0; i < anim->getChannels().size(); i++) {
        addJSONChannel(anim->getChannels()[i].get(), *json);
    }
    return json.release();
}

JSONObject* createJSONUpdateMatrixTransform(osgAnimation::UpdateMatrixTransform& acb)
{
    std::string name = acb.getName();
    osg::ref_ptr<JSONObject> json = new JSONObject;
    json->getMaps()["Name"] = new JSONValue<std::string>(acb.getName());

    osg::ref_ptr<JSONArray> jsonStackedArray = new JSONArray();
    json->getMaps()["StackedTransforms"] = jsonStackedArray;

    osgAnimation::StackedTransform& st = acb.getStackedTransforms();
    for (unsigned int i = 0; i < st.size(); i++) {
        {
            osgAnimation::StackedTranslateElement* element = dynamic_cast<osgAnimation::StackedTranslateElement*>(st[i].get());
            if (element) {
                osg::ref_ptr<JSONObject> jsonElement = new JSONObject;
                jsonElement->getMaps()["Name"] = new JSONValue<std::string>(element->getName());
                jsonElement->getMaps()["Translate"] = new JSONVec3Array(element->getTranslate());

                osg::ref_ptr<JSONObject> jsonElementObject = new JSONObject;
                jsonElementObject->getMaps()["osgAnimation.StackedTranslate"] = jsonElement;
                jsonStackedArray->getArray().push_back(jsonElementObject);
                continue;
            }
        }


        {
            osgAnimation::StackedQuaternionElement* element = dynamic_cast<osgAnimation::StackedQuaternionElement*>(st[i].get());
            if (element) {
                osg::ref_ptr<JSONObject> jsonElement = new JSONObject;
                jsonElement->getMaps()["Name"] = new JSONValue<std::string>(element->getName());
                jsonElement->getMaps()["Quaternion"] = new JSONVec4Array(element->getQuaternion().asVec4());

                osg::ref_ptr<JSONObject> jsonElementObject = new JSONObject;
                jsonElementObject->getMaps()["osgAnimation.StackedQuaternion"] = jsonElement;
                jsonStackedArray->getArray().push_back(jsonElementObject);
                continue;
            }
        }


        {
            osgAnimation::StackedRotateAxisElement* element = dynamic_cast<osgAnimation::StackedRotateAxisElement*>(st[i].get());
            if (element) {
                osg::ref_ptr<JSONObject> jsonElement = new JSONObject;
                jsonElement->getMaps()["Name"] = new JSONValue<std::string>(element->getName());
                jsonElement->getMaps()["Axis"] = new JSONVec3Array(element->getAxis());
                jsonElement->getMaps()["Angle"] = new JSONValue<double>(element->getAngle());

                osg::ref_ptr<JSONObject> jsonElementObject = new JSONObject;
                jsonElementObject->getMaps()["osgAnimation.StackedRotateAxis"] = jsonElement;
                jsonStackedArray->getArray().push_back(jsonElementObject);
                continue;
            }
        }


        {
            osgAnimation::StackedMatrixElement * element = dynamic_cast<osgAnimation::StackedMatrixElement*>(st[i].get());
            if (element) {
                osg::ref_ptr<JSONObject> jsonElement = new JSONObject;
                jsonElement->getMaps()["Name"] = new JSONValue<std::string>(element->getName());
                jsonElement->getMaps()["Matrix"] = new JSONMatrix(element->getMatrix());

                osg::ref_ptr<JSONObject> jsonElementObject = new JSONObject;
                jsonElementObject->getMaps()["osgAnimation.StackedMatrixElement"] = jsonElement;
                jsonStackedArray->getArray().push_back(jsonElementObject);
                continue;
            }
        }
    }
    if (jsonStackedArray->getArray().empty()) {
        return 0;
    }

    return json.release();
}
