#ifndef ANIM_ENTITY_ENTITY_H
#define ANIM_ENTITY_ENTITY_H

#include <vector>
#include <memory>
#include <string>
#include <memory>
#include <functional>
#include <map>

#include <json/json.h>

#include "components/component.h"
#include "components/transform_component.h"

#include "../util/log.h"
#include "shared_resources.h"

#include <ImSequencer.h>
#include <ImCurveEdit.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "icons/icons.h"

enum class CompositionStage {
	Sequence,
	SubSequence
};

namespace anim
{

static const char* SequencerItemTypeNames[] = { "Animation", "Transform" };

class SharedResources;

class TracksSequencer : public ImSequencer::SequenceInterface
{
public:
	enum class SequencerType {
		Single,
		Composition
	};
	
	Json::Value serialize() const;
	
	void deserialize(const Json::Value& root);
	
	virtual int GetFrameMin() const override {
		return 0;
		//		return mFrameMin;
	}
	virtual int GetFrameMax() const override {
		return mFrameMax;
	}
	virtual int GetItemCount() const override {
		if(mCompositionStage == CompositionStage::Sequence){
			return (int)mItems.size();
		} else {
			return (int)mItems[mCompositionIndex]->mTracks.size();
		}
	}
	
	virtual int GetItemTypeCount() const override{ return sizeof(SequencerItemTypeNames) / sizeof(char*); }
	virtual const char* GetItemTypeName(int typeIndex) const override { return SequencerItemTypeNames[typeIndex]; }
	virtual const char* GetItemLabel(int index) const override
	{
		static char tmps[512];
		
		ImGuiIO &io = ImGui::GetIO();
		
		if(mCompositionStage == CompositionStage::SubSequence){
			auto& item = mItems[mCompositionIndex]->mTracks[index];
			
			if(item->mType == ItemType::Transform){
				ImGui::SetCurrentFont(io.Fonts->Fonts[ICON_MD]);
				snprintf(tmps, 512, "%s%s", ICON_MD_3D_ROTATION, item->mName.c_str());
			} else if(item->mType == ItemType::Animation) {
				ImGui::SetCurrentFont(io.Fonts->Fonts[ICON_FA]);
				snprintf(tmps, 512, "%s %s", ICON_FA_PERSON_WALKING, item->mName.c_str());
			}
		} else {
			auto& item = mItems[index];
			
			if(item->mType == TrackType::Actor){
				ImGui::SetCurrentFont(io.Fonts->Fonts[ICON_FA]);
				snprintf(tmps, 512, "%s %s", ICON_FA_PERSON, mItems[index]->mName.c_str());
			} else if(item->mType == TrackType::Camera){
				ImGui::SetCurrentFont(io.Fonts->Fonts[ICON_FA]);
				snprintf(tmps, 512, "%s %s", ICON_FA_CAMERA_RETRO, mItems[index]->mName.c_str());
				
			} else if(item->mType == TrackType::Light){
				ImGui::SetCurrentFont(io.Fonts->Fonts[ICON_FA]);
				snprintf(tmps, 512, "%s %s", ICON_FA_LIGHTBULB, mItems[index]->mName.c_str());
			}
		}
		
		
		return tmps;
	}
	
	int mBeginEditIndex = 0;
	int mBeginEditSegmentIndex = 0;
	int mEditFrameStart = 0;
	int mEditSize = 0;
	int mSelectedEntry = -1;
	int mFirstFrame = 0;
	int mCurrentTime = 0;
	bool mEditing = false;
	CompositionStage mCompositionStage = CompositionStage::Sequence;
	int mCompositionIndex;
	virtual void BeginEdit(int index, int segment) override {
		if(mCompositionStage == CompositionStage::SubSequence){
			
			mSelectedEntry = mCompositionIndex;
			
			mBeginEditIndex = index;
			mBeginEditSegmentIndex = segment;
			
			auto& item = mItems[mCompositionIndex];
			
			if(item->mTracks[index]->mType == ItemType::Transform){
				return;
			}
			
			mEditFrameStart = item->mTracks[index]->mSegments[segment].mFrameStart;
			
			mEditSize = item->mTracks[index]->mSegments[segment].mFrameEnd - item->mTracks[index]->mSegments[segment].mFrameStart;
			
			mEditing = true;
		} else {
			mSelectedEntry = index;
		}
		
	}
	
	virtual void EndEdit() override {
		if(mCompositionStage == CompositionStage::SubSequence){
			
			auto& item = mItems[mCompositionIndex];
			
			if(item->mTracks[mBeginEditIndex]->mType == ItemType::Transform){
				return;
			}
			mEditing = false;
		}
	}
	
	virtual void Get(int index, int* type, bool* existing, unsigned int* color, std::vector<SequenceSegment>** segments, std::map<int, Keyframe>** keyframes, ImSequencer::SEQUENCER_OPTIONS* options) override
	{
		
		if(mCompositionStage == CompositionStage::Sequence){
			auto item = mItems[index];
			
			if (color){
				if(item->mType == TrackType::Actor){
					if(item->mMissing){
						*color = 0xFF000060;
					} else {
						*color = 0xFF600000;
					}
				} else if(item->mType == TrackType::Camera || item->mType == TrackType::Light){
					
					if(item->mMissing){
						*color = 0xFF000060;
					} else {
						*color = 0xFF777777;
					}
				}
			}
			
			if(existing){
				*existing = !item->mMissing;
			}
			
			if(segments){
				static std::vector<SequenceSegment> dummySegments = {{}};
				dummySegments[0].mFrameEnd = mFrameMax; //@TODO
				*segments = &dummySegments;
			}
			
			if(keyframes){
				*keyframes = NULL;
			}
			
			if (type)
				*type = (int)item->mType;
			
			if(options){
				*options = item->mOptions;
			}
			
		} else {
			auto item = mItems[mCompositionIndex]->mTracks[index];
			
			if(color){
				if(item->mType == ItemType::Animation){
					*color = 0xFFAA8080;
				} else if(item->mType == ItemType::Transform){
					*color = 0xFF0080FF;
				}
			}
			
			if(existing){
				*existing = !item->mMissing;
			}
			
			
			if(item->mType == ItemType::Animation){
				
				if(mEditing){
					int currentEditSize = item->mSegments[mBeginEditSegmentIndex].mFrameEnd - item->mSegments[mBeginEditSegmentIndex].mFrameStart;
					
					if(mEditSize != currentEditSize){
						int editOpResult = item->mSegments[mBeginEditSegmentIndex].mFrameStart - mEditFrameStart;
						
						item->mSegments[mBeginEditSegmentIndex].mOffset += editOpResult;
						
						mEditSize = currentEditSize;
						
					} else {
						item->mSegments[mBeginEditSegmentIndex].mInitialIndex +=
						item->mSegments[mBeginEditSegmentIndex].mFrameStart -
						mEditFrameStart;
						mEditFrameStart = item->mSegments[mBeginEditSegmentIndex].mFrameStart;
					}
				}
				
			}
			
			if (type)
				*type = (int)item->mType;
			
			if(segments){
				*segments = &item->mSegments;
			}
			
			if(keyframes){
				*keyframes = &item->mKeyFrames;
			}
			
			if(options){
				*options = item->mOptions;
			}
			
		}
		
	}
	
	virtual void AddKeyFrame() override {
		if(mCompositionStage == CompositionStage::SubSequence){
			auto item = mItems[mCompositionIndex];
			if(!item->mTracks.empty()){
				auto track = (*item->mTracks.begin());
				
				if(track->mType == ItemType::Transform){
					if(track->mKeyFrames[mCurrentTime].active){
						track->mKeyFrames[mCurrentTime].active = false;
					} else {
						track->mKeyFrames[mCurrentTime].active = true;
					}
				}
			}
			
			if(mOnKeyFrameSet){
				mOnKeyFrameSet(mCompositionIndex, mCurrentTime);
			}
		}
		
	};
	
	virtual void AddKeyFrameRange(std::map<int, KeyframeRange>& selectedKeyframes) override {
		if(mCompositionStage == CompositionStage::SubSequence){
			auto item = mItems[mCompositionIndex];
			
			bool fullFrames = true;
			
			for(auto& range : selectedKeyframes){
				auto track = item->mTracks[range.first];
				
				auto [start, end] = range.second.range;
				
				for(int i = start; i <= end; ++i){
					if(!track->mKeyFrames[i].active){
						fullFrames = false;
						break;
					}
				}
			}
			
			for(auto& range : selectedKeyframes){
				auto track = item->mTracks[range.first];

				auto [start, end] = range.second.range;
				
				if(fullFrames){
					for(int i = end; i >= start; --i){
						track->mKeyFrames[i].active = false;
						if(mOnKeyFrameSet){
							mOnKeyFrameSet(range.first, i);
						}
					}
				} else {
					for(int i = start; i <= end; ++i){
						track->mKeyFrames[i].active = true;
						
						if(mOnKeyFrameSet){
							mOnKeyFrameSet(range.first, i);
						}
					}
				}
			}
		}
	}
	virtual void Duplicate(int index) override {
		if(mItems.size() == 1){
			return;
		}
		
		if(index > 0){
			if(mCompositionStage == CompositionStage::SubSequence){
				mItems[mCompositionIndex]->mTracks[index]->AddSegment();
			}
		}
	}
	
	virtual void Del(int index, int segment) override {
		if(mItems.size() == 1){
			return;
		}
		
		if(mCompositionStage == CompositionStage::SubSequence){
			
			auto item = mItems[mCompositionIndex];
			
			if(index > 0){
				if(mBeginEditSegmentIndex != -1){
					if(item->mTracks[index]->mSegments.empty()){
						item->mTracks.erase(item->mTracks.begin() + index);
					} else {
						
						if(mBeginEditSegmentIndex >= item->mTracks[index]->mSegments.size()){
							mBeginEditSegmentIndex = static_cast<int>(item->mTracks[index]->mSegments.size() - 1);
						}
						
						item->mTracks[index]->mSegments.erase(item->mTracks[index]->mSegments.begin() + mBeginEditSegmentIndex);
					}
				} else {
					if(!item->mTracks[index]->mSegments.empty()){
						item->mTracks[index]->mSegments.pop_back();
					}
				}
			}
		}
		
	}
	
	virtual size_t GetCustomHeight(int index) override { return 0; }
	
	virtual void DoubleClick(int index) override {
		if(mCompositionStage == CompositionStage::SubSequence){
			mCompositionStage = CompositionStage::Sequence;
			mCompositionIndex = -1;
			mSelectedEntry = 0;
		} else{
			mCompositionStage = CompositionStage::SubSequence;
			mCompositionIndex = index;
			mSelectedEntry = 0;
		}
	}
	
	virtual void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect) override
	{
		//		if(mItems[index].mType == ItemType::Animation){
		//			draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
		//
		//			for(auto& segment : mItems[index].mItems){
		//
		//				int standardSize = segment.mSequenceSize;
		//
		//				int currentPosition = segment.mInitialIndex;
		//
		//				currentPosition = segment.mInitialIndex - segment.mOffset;
		//
		//				float x = rc.Min.x;
		//				float xClipped = rc.Min.x;
		//
		//				float normalizedFactor = currentPosition / float(mFrameMax);
		//				float realFactor = segment.mFrameStart / float(mFrameMax);
		//
		//				float r = standardSize / float(mFrameMax);
		//
		//				float barWidth = rc.Max.x - rc.Min.x;
		//
		//				x += normalizedFactor * barWidth;
		//				xClipped += realFactor * barWidth;
		//
		//				do {
		//					x += (barWidth * r);
		//
		//					currentPosition += standardSize;
		//
		//					// Move to the next position
		//					if(x < xClipped){
		//						continue;
		//					}
		//
		//					if(currentPosition >= segment.mFrameEnd){
		//						break;
		//					}
		//
		//					draw_list->AddLine(ImVec2(x, rc.Min.y + 6), ImVec2(x, rc.Max.y - 4), 0xAA000000, 4.f);
		//				} while(currentPosition <= segment.mFrameEnd - standardSize);
		//
		//
		//				if(segment.mOffset < 0){
		//					currentPosition = segment.mInitialIndex;
		//
		//					x = rc.Min.x;
		//					x += normalizedFactor * barWidth;
		//
		//					draw_list->AddLine(ImVec2(x, rc.Min.y + 6), ImVec2(x, rc.Max.y - 4), 0xAA000000, 4.f);
		//
		//					do {
		//						x -= (barWidth * r);
		//
		//						currentPosition -= standardSize;
		//
		//						// Move to the next position
		//						if(x >= xClipped){
		//							draw_list->AddLine(ImVec2(x, rc.Min.y + 6), ImVec2(x, rc.Max.y - 4), 0xAA000000, 4.f);
		//						}
		//					} while(currentPosition >= segment.mFrameStart - standardSize);
		//				}
		//			}
		//			draw_list->PopClipRect();
		//		} else if(mItems[index].mType == ItemType::Transform){
		//			draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
		//
		//			for(auto& keyframe : mKeyFrames){
		//				if(keyframe.second.active){
		//					float x = rc.Min.x;
		//					float normalizedFactor = keyframe.first / float(mFrameMax);
		//
		//					float barWidth = rc.Max.x - rc.Min.x;
		//
		//					x +=  normalizedFactor * barWidth;
		//
		//					draw_list->AddLine(ImVec2(x - 0.5f, rc.Min.y + 6), ImVec2(x - 0.5f, rc.Max.y - 4), 0xAA000000, 4.f);
		//				}
		//			}
		//
		//			draw_list->PopClipRect();
		//		}
	}
	
	TracksSequencer(std::shared_ptr<SharedResources> resources);
	virtual ~TracksSequencer() = default;
	
	int mFrameMax;
	
	std::shared_ptr<SharedResources> _resources;
	std::vector<std::shared_ptr<TracksContainer>> mItems;
	
	std::function<void(int, int)> mOnKeyFrameSet;
	
	SequencerType mSequencerType = SequencerType::Single;
};

class AnimationSequencer : public ImSequencer::SequenceInterface
{
public:
	//	Json::Value serialize() const;
	
	//	void deserialize(const Json::Value& root);
	
	virtual int GetFrameMin() const override {
		return 0;
	}
	virtual int GetFrameMax() const override {
		return mFrameMax;
	}
	virtual int GetItemCount() const override {
		return static_cast<int>(mItems.size());
	}
	
	virtual int GetItemTypeCount() const override{ return sizeof(SequencerItemTypeNames) / sizeof(char*); }
	virtual const char* GetItemTypeName(int typeIndex) const override { return SequencerItemTypeNames[typeIndex]; }
	virtual const char* GetItemLabel(int index) const override
	{
		static char tmps[512];
		
		ImGuiIO &io = ImGui::GetIO();
		
		auto& item = mItems[index];
		
		if(item->mType == EntityType::Mesh){
			ImGui::SetCurrentFont(io.Fonts->Fonts[ICON_MD]);
			snprintf(tmps, 512, "%s%s", ICON_MD_VIEW_IN_AR, item->mName.c_str());
		} else {
			ImGui::SetCurrentFont(io.Fonts->Fonts[ICON_MD]);
			snprintf(tmps, 512, "%s%s", ICON_MD_SHARE, item->mName.c_str());
		}
		
		
		return tmps;
	}
	
	int mSelectedEntry = -1;
	int mCurrentTime = 0;
	
	virtual void Click(int index) override {
		mSelectedEntry = index;
	}
	
	virtual void Get(int index, int* type, bool* existing, unsigned int* color, std::vector<SequenceSegment>** segments, std::map<int, Keyframe>** keyframes, ImSequencer::SEQUENCER_OPTIONS* options) override
	{
		auto item = mItems[index];
		
		if(color){
			if(index == mSelectedEntry){
				*color = 0xFFAAAAAA;
			} else {
				*color = 0xFF777777;
			}
		}
		
		if(existing){
			*existing = !item->mMissing;
		}
		
		if (type)
			*type = (int)item->mType;
		
		if(segments){
			*segments = &item->mSegments;
		}
		
		if(keyframes){
			*keyframes = &item->mKeyFrames;
		}
		
		if(options){
			*options = item->mOptions;
		}
		
	}
	
	virtual void AddKeyFrame() override {
		if(mSelectedEntry != -1){
			auto item = mItems[mSelectedEntry];
			if(item->mKeyFrames[mCurrentTime].active){
				item->mKeyFrames[mCurrentTime].active = false;
			} else {
				item->mKeyFrames[mCurrentTime].active = true;
			}
			if(mOnKeyFrameSet){
				mOnKeyFrameSet(mSelectedEntry, mCurrentTime);
			}
		}
	}
	
	virtual void AddKeyFrameRange(std::map<int, KeyframeRange>& selectedKeyframes) override {
		bool fullFrames = true;

		for(auto& range : selectedKeyframes){
			auto track = mItems[range.first];
			
			auto [start, end] = range.second.range;
			
			for(int i = start; i <= end; ++i){
				if(!track->mKeyFrames[i].active){
					fullFrames = false;
					break;
				}
			}
		}
		
		for(auto& range : selectedKeyframes){
			auto track = mItems[range.first];
			
			auto [start, end] = range.second.range;
			
			if(fullFrames){
				for(int i = end; i >= start; --i){
					track->mKeyFrames[i].active = false;
					if(mOnKeyFrameSet){
						mOnKeyFrameSet(range.first, i);
					}
				}
			} else {
				for(int i = start; i <= end; ++i){
					track->mKeyFrames[i].active = true;
					
					if(mOnKeyFrameSet){
						mOnKeyFrameSet(range.first, i);
					}
				}
			}
		}
	}
	
	
	AnimationSequencer(std::shared_ptr<SharedResources> resources);
	
	virtual ~AnimationSequencer() = default;
	
	int mFrameMax;
	int mFirstFrame = 0;
	
	std::shared_ptr<SharedResources> _resources;
	std::vector<std::shared_ptr<AnimationItem>> mItems;
	
	std::function<void(int, int)> mOnKeyFrameSet;
};

class Entity : public std::enable_shared_from_this<Entity>
{
public:
	bool is_deactivate_ = false;
	Entity(std::shared_ptr<SharedResources> resources, const std::string &name, int identifier, std::shared_ptr<Entity> parent = nullptr);
	template <class T>
	std::shared_ptr<T>
	get_component();
	
	template <class T>
	std::shared_ptr<T>
	add_component();
	void update();
	std::shared_ptr<Entity> find(const std::string &name);
	std::shared_ptr<Entity> find(std::shared_ptr<Entity> entity, const std::string &name);
	
	void set_name(const std::string &name);
	void add_children(std::shared_ptr<Entity> &children);
	void removeChild(const std::shared_ptr<Entity>& child);
	void set_sub_child_num(int num);
	const std::string &get_name()
	{
		return name_;
	}
	const glm::mat4 get_local() const
	{
		return _transform_component->get_mat4();
	}
	void set_local(const glm::mat4 &local)
	{
		_transform_component->set_transform(local);
	}
	std::vector<std::shared_ptr<Entity>> &get_mutable_children()
	{
		return children_;
	}
	
	std::vector<std::shared_ptr<Entity>> get_children_recursive() {
		// Clear the result vector before populating it
		std::vector<std::shared_ptr<Entity>> result;
		
		// Start the recursive process
		get_children_recursive_internal(result);
		
		return result;
	}
	
	void get_children_recursive_internal(std::vector<std::shared_ptr<Entity>>& result) {
		// Add the immediate children of this entity to the result vector
		std::vector<std::shared_ptr<Entity>> immediateChildren = get_mutable_children();
		result.insert(result.end(), immediateChildren.begin(), immediateChildren.end());
		
		// Recursively call the function on each child
		for (auto& child : immediateChildren) {
			child->get_children_recursive_internal(result);
		}
	}
	
	void set_world_transformation(const glm::mat4 &world)
	{
		world_ = world;
	}
	const glm::mat4 &get_world_transformation() const
	{
		return world_;
	}
	void set_parent(std::shared_ptr<Entity> entity)
	{
		parent_ = entity;
	}
	void set_root(std::shared_ptr<Entity> entity)
	{
		root_ = entity;
	}
	std::shared_ptr<Entity> get_mutable_parent()
	{
		return parent_;
	}
	std::shared_ptr<Entity> get_mutable_root()
	{
		return root_;
	}
	int32_t get_id()
	{
		return id_;
	}
	void set_is_selected(bool is_selected)
	{
		is_selected_ = is_selected;
	}
	bool is_selected_{false};
	
	std::shared_ptr<TracksContainer>& get_animation_tracks() {
		return sequence_items_;
	}
	
	std::shared_ptr<Entity> clone(){
		auto clone = std::shared_ptr<Entity>(new Entity());
		
		std::vector<std::shared_ptr<Component>> cloneComponents;
		std::vector<std::shared_ptr<Entity>> cloneChildren;
		
		for(auto component : components_){
			cloneComponents.push_back(component->clone());
		}
		
		clone->components_  = cloneComponents;
		
		for(auto child : children_){
			cloneChildren.push_back(child->clone());
		}
		
		return clone;
	}
	
private:
	Entity() : sequence_items_(std::make_shared<TracksContainer>()) {
		
	}
	
	std::string name_{};
	std::shared_ptr<Entity> parent_ = nullptr;
	std::shared_ptr<Entity> root_ = nullptr;
	std::vector<std::shared_ptr<Component>> components_{};
	std::vector<std::shared_ptr<Entity>> children_{};
	glm::mat4 world_{1.0f};
	int32_t id_{-1};
	
	std::shared_ptr<TracksContainer> sequence_items_;
	
	std::shared_ptr<TransformComponent> _transform_component;
};

template <class T>
std::shared_ptr<T>  Entity::get_component()
{
	for (auto &component : components_)
	{
		if (component->get_type() == T::type)
		{
			return std::dynamic_pointer_cast<T>(component);
		}
	}
	return nullptr;
}
template <class T>
std::shared_ptr<T> Entity::add_component()
{
	auto cmp = get_component<T>();
	if (cmp)
	{
		return cmp;
	}
	components_.push_back(std::make_shared<T>());
	return std::dynamic_pointer_cast<T>(components_.back());
}
}

#endif
