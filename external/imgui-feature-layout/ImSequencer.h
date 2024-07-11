// https://github.com/CedricGuillemet/ImGuizmo
// v 1.84 WIP
//
// The MIT License(MIT)
//
// Copyright(c) 2021 Cedric Guillemet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include "json/json.h"

#include <cstddef>
#include <vector>
#include <functional>
#include <map>


#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"


struct ImDrawList;
struct ImRect;
struct ImGuiPayload;

enum class EntityType : int {
	Mesh,
	Bone
};

enum class ItemType : int {
	Transform,
	Animation
};

enum class TrackType : int {
	Actor,
	Camera,
	Light,
	Other,
};

struct Keyframe {
	bool active = false;
	glm::mat4 transform = glm::identity<glm::mat4>();
};

struct KeyframeRange {
	std::tuple<int, int> range;
};


struct SequenceItem;
struct SequenceSegment;

namespace ImSequencer
{


enum SEQUENCER_OPTIONS
{
	SEQUENCER_EDIT_NONE = 0,
	SEQUENCER_EDIT_STARTEND = 1 << 1,
	SEQUENCER_CHANGE_FRAME = 1 << 3,
	SEQUENCER_ADD = 1 << 4,
	SEQUENCER_DEL = 1 << 5,
	SEQUENCER_COPYPASTE = 1 << 6,
	SEQUENCER_SELECT = 1 << 7,
	SEQUENCER_EDIT_ALL = SEQUENCER_EDIT_STARTEND | SEQUENCER_CHANGE_FRAME
};


struct SequenceInterface
{
	bool focused = false;
	bool OnDragDropFilter = false;
	virtual int GetFrameMin() const = 0;
	virtual int GetFrameMax() const = 0;
	virtual int GetItemCount() const = 0;
	
	virtual void BeginEdit(int index, int segment)  {}
	virtual void EndEdit() {}
	virtual int GetItemTypeCount() const { return 0; }
	virtual const char* GetItemTypeName(int /*typeIndex*/) const { return ""; }
	virtual const char* GetItemLabel(int /*index*/) const { return ""; }
	virtual const char* GetCollapseFmt() const { return "%d Frames / %d entries"; }
	
	virtual void Get(int index, int* type, bool* existing, unsigned int* color, std::vector<SequenceSegment>** segments, std::map<int, Keyframe>** keyframes, SEQUENCER_OPTIONS* options) = 0;
	virtual void AddKeyFrame() {}
	virtual void AddKeyFrameRange(std::map<int, KeyframeRange>& selectedKeyframes) {}
	virtual void Add(int /*type*/) {}
	virtual void Del(int /*index*/, int /*segment*/) {}
	virtual void Duplicate(int /*index*/) {}
	
	virtual void Copy() {}
	virtual void Paste() {}
	
	virtual size_t GetCustomHeight(int /*index*/) { return 0; }
	virtual void Click(int /*index*/) {}
	virtual void DoubleClick(int /*index*/) {}
	virtual void CustomDraw(int /*index*/, ImDrawList* /*draw_list*/, const ImRect& /*rc*/, const ImRect& /*legendRect*/, const ImRect& /*clippingRect*/, const ImRect& /*legendClippingRect*/) {}
	virtual void CustomDrawCompact(int /*index*/, ImDrawList* /*draw_list*/, const ImRect& /*rc*/, const ImRect& /*clippingRect*/) {}
	
	std::function<void(int, const ImGuiPayload*, int)> OnMissingActorPayload;

};


// return true if selection is made
bool Sequencer(SequenceInterface* sequence, int* currentFrame, bool* expanded, int* selectedEntry, int* firstFrame, int sequenceOptions);

}

struct SequenceSegment
{
	int mInitialIndex, mFrameStart, mFrameEnd, mOffset;
};

struct SequenceItem
{
	SequenceItem(){}
	
	SequenceItem(int id, const std::string& name, ItemType type, int frameStart, int frameTotal, ImSequencer::SEQUENCER_OPTIONS options = (ImSequencer::SEQUENCER_OPTIONS)(ImSequencer::SEQUENCER_ADD | ImSequencer::SEQUENCER_DEL | ImSequencer::SEQUENCER_EDIT_STARTEND)){
		mId = id;
		mName = name;
		mType = type;
		
		mSegments.push_back({frameStart, frameStart, frameTotal, 0});
		
		if(type == ItemType::Transform){
			mOptions = ImSequencer::SEQUENCER_EDIT_NONE;
		} else {
			mOptions = options;
		}
	}
	
	void AddSegment(){
		mSegments.push_back({mSegments.back().mInitialIndex, mSegments.back().mFrameEnd + 1, mSegments.back().mFrameEnd + 1 + mSegments.back().mFrameEnd - mSegments.back().mFrameStart, 0});
	}

int mId;
bool mMissing = false;
std::string mName;
ItemType mType;
ImSequencer::SEQUENCER_OPTIONS mOptions;

std::map<int, Keyframe> mKeyFrames;

std::vector<SequenceSegment> mSegments;
};


struct AnimationItem
{
	AnimationItem(){}
	
	AnimationItem(int id, const std::string& name, EntityType type, int frameStart, int frameTotal){
		mId = id;
		mName = name;
		mType = type;
		
		mSegments.push_back({frameStart, frameStart, frameTotal, 0});
		
		mOptions = ImSequencer::SEQUENCER_EDIT_NONE;
	}
	
	void AddSegment(){
		mSegments.push_back({mSegments.back().mInitialIndex, mSegments.back().mFrameEnd + 1, mSegments.back().mFrameEnd + 1 + mSegments.back().mFrameEnd - mSegments.back().mFrameStart, 0});
	}
	
	int mId;
	bool mMissing = false;
	std::string mName;
	EntityType mType;
	ImSequencer::SEQUENCER_OPTIONS mOptions;
	
	std::map<int, Keyframe> mKeyFrames;
	
	std::vector<SequenceSegment> mSegments;
};

struct TracksContainer {
	int mId;
	bool mMissing = false;
	std::string mName;
	TrackType mType;
	ImSequencer::SEQUENCER_OPTIONS mOptions;
	
	TracksContainer(){
		mOptions = (ImSequencer::SEQUENCER_OPTIONS)(ImSequencer::SEQUENCER_EDIT_ALL | ImSequencer::SEQUENCER_DEL);
	}
	
	TracksContainer(int id, const std::string& name, TrackType type, ImSequencer::SEQUENCER_OPTIONS options = (ImSequencer::SEQUENCER_OPTIONS)(ImSequencer::SEQUENCER_EDIT_ALL | ImSequencer::SEQUENCER_DEL)){
		mId = id;
		mName = name;
		mType = type;
		
		mOptions = options;
	}

	Json::Value serialize() {
		
		Json::Value jsonContainer;
		
		jsonContainer["mId"] = mId;
		jsonContainer["mMissing"] = mMissing;
		jsonContainer["mName"] = mName;
		jsonContainer["mType"] = static_cast<int>(mType);
		jsonContainer["mOptions"] = static_cast<int>(mOptions);

		Json::Value jsonTracks(Json::arrayValue);
		
		for (const auto& track : mTracks) {
			Json::Value jsonTrack;
			jsonTrack["mId"] = track->mId;
			jsonTrack["mMissing"] = track->mMissing;
			jsonTrack["mName"] = track->mName;
			jsonTrack["mType"] = static_cast<int>(track->mType);
			jsonTrack["mOptions"] = static_cast<int>(track->mOptions);
			
			Json::Value jsonSegments(Json::arrayValue);
			for (const auto& segment : track->mSegments) {
				Json::Value jsonSegment;
				jsonSegment["mInitialIndex"] = segment.mInitialIndex;
				jsonSegment["mFrameStart"] = segment.mFrameStart;
				jsonSegment["mFrameEnd"] = segment.mFrameEnd;
				jsonSegment["mOffset"] = segment.mOffset;
				jsonSegments.append(jsonSegment);
			}
			jsonTrack["mSegments"] = jsonSegments;
			
			Json::Value jsonKeyFrames(Json::arrayValue);
			for (const auto& [frame, keyframe] : track->mKeyFrames) {
				Json::Value jsonKeyFrame;
				jsonKeyFrame["frame"] = frame;
				jsonKeyFrame["value"] = keyframe.active;
				
				Json::Value transformArray(Json::arrayValue);
				const glm::mat4& transform = keyframe.transform;
				for (int i = 0; i < 4; ++i) {
					for (int j = 0; j < 4; ++j) {
						transformArray.append(transform[i][j]);
					}
				}
				jsonKeyFrame["transform"] = transformArray;
				jsonKeyFrames.append(jsonKeyFrame);
			}
			jsonTrack["mKeyFrames"] = jsonKeyFrames;
			
			jsonTracks.append(jsonTrack);
		}
		
		jsonContainer["mTracks"] = jsonTracks;

		return jsonContainer;
	}

	void deserialize(const Json::Value& jsonContainer) {
		
		mId = jsonContainer["mId"].asInt();
		mMissing = jsonContainer["mMissing"].asBool();
		mName = jsonContainer["mName"].asString();
		mType = static_cast<TrackType>(jsonContainer["mType"].asInt());
		mOptions = static_cast<ImSequencer::SEQUENCER_OPTIONS>(jsonContainer["mOptions"].asInt());
		
		mTracks.clear();
		
		const Json::Value& jsonTracks = jsonContainer["mTracks"];

		for (const auto& jsonTrack : jsonTracks) {
			auto track = std::make_shared<SequenceItem>();
			track->mId = jsonTrack["mId"].asInt();
			track->mMissing = jsonTrack["mMissing"].asBool();
			track->mName = jsonTrack["mName"].asString();
			track->mType = static_cast<ItemType>(jsonTrack["mType"].asInt());
			track->mOptions = static_cast<ImSequencer::SEQUENCER_OPTIONS>(jsonTrack["mOptions"].asInt());
			
			const Json::Value& jsonSegments = jsonTrack["mSegments"];
			for (const auto& jsonSegment : jsonSegments) {
				SequenceSegment segment;
				segment.mInitialIndex = jsonSegment["mInitialIndex"].asInt();
				segment.mFrameStart = jsonSegment["mFrameStart"].asInt();
				segment.mFrameEnd = jsonSegment["mFrameEnd"].asInt();
				segment.mOffset = jsonSegment["mOffset"].asInt();
				track->mSegments.push_back(segment);
			}
			
			const Json::Value& jsonKeyFrames = jsonTrack["mKeyFrames"];
			for (const auto& jsonKeyFrame : jsonKeyFrames) {
				int frame = jsonKeyFrame["frame"].asInt();
				bool value = jsonKeyFrame["value"].asBool();
				
				glm::mat4 transform;
				const Json::Value& transformArray = jsonKeyFrame["transform"];
				int index = 0;
				for (int i = 0; i < 4; ++i) {
					for (int j = 0; j < 4; ++j) {
						transform[i][j] = transformArray[index++].asFloat();
					}
				}
				
				track->mKeyFrames[frame] = Keyframe{ value, transform };
			}
			
			mTracks.push_back(track);
		}
	}

	std::vector<std::shared_ptr<SequenceItem>> mTracks;
};
