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
#include "ImSequencer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "icons/icons.h"

#include <cstdlib>
#include <string>
#include <vector>

namespace ImSequencer
{
#ifndef IMGUI_DEFINE_MATH_OPERATORS
static ImVec2 operator+(const ImVec2& a, const ImVec2& b) {
	return ImVec2(a.x + b.x, a.y + b.y);
}
#endif
static ImVec2 selectionStart = ImVec2(-1, -1); // Start position of the selection box
static ImVec2 selectionEnd = ImVec2(-1, -1);   // End position of the selection box
static bool selecting = false;                 // Flag indicating if the user is currently selecting

// Function to draw the selection box
void DrawSelectionBox(ImDrawList* drawList) {
	if (selecting) {
		// Adjust the alpha value here for transparency, e.g., 0.3f for the alpha component
		auto hoverColor = IM_COL32(0, 0, 64, 64);
		
		// Calculate the minimum and maximum points to make the coordinates absolute
		ImVec2 topLeft, bottomRight;
		
		topLeft.x = std::min(selectionStart.x, selectionEnd.x);
		topLeft.y = std::min(selectionStart.y, selectionEnd.y);
		
		bottomRight.x = std::max(selectionStart.x, selectionEnd.x);
		bottomRight.y = std::max(selectionStart.y, selectionEnd.y);
		
		drawList->AddRectFilled(topLeft, bottomRight, hoverColor, 1); // Border
	}
}

// Function to handle selection box logic
void HandleSelectionBox() {
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 mousePos = io.MousePos;
	bool mouseDown = io.MouseDown[0];
	
	if (mouseDown) {
		if (!selecting) {
			// Start the selection box
			selectionStart = mousePos;
			selectionEnd = mousePos;
			selecting = true;
		} else {
			// Continue the selection box
			selectionEnd = mousePos;
		}
	} else {
		// End the selection box
		selecting = false;
	}
}
static bool SequencerAddDelButton(ImDrawList* draw_list, ImVec2 pos, bool add = true)
{
	ImGuiIO& io = ImGui::GetIO();
	ImRect delRect(pos, ImVec2(pos.x + 16, pos.y + 16));
	bool overDel = delRect.Contains(io.MousePos);
	int delColor = overDel ? 0xFFAAAAAA : 0x77A3B2AA;
	float midy = pos.y + 16 / 2 - 0.5f;
	float midx = pos.x + 16 / 2 - 0.5f;
	draw_list->AddRect(delRect.Min, delRect.Max, delColor, 4);
	draw_list->AddLine(ImVec2(delRect.Min.x + 3, midy), ImVec2(delRect.Max.x - 3, midy), delColor, 2);
	if (add)
		draw_list->AddLine(ImVec2(midx, delRect.Min.y + 3), ImVec2(midx, delRect.Max.y - 3), delColor, 2);
	return overDel;
}

bool Sequencer(SequenceInterface* sequence, int* currentFrame, bool* expanded, int* selectedEntry, int* firstFrame, int sequenceOptions)
{
	bool ret = false;
	ImGuiIO& io = ImGui::GetIO();
	int cx = (int)(io.MousePos.x);
	int cy = (int)(io.MousePos.y);
	static float framePixelWidth = 10.f;
	static float framePixelWidthTarget = 10.0f;
	int legendWidth = 200;
	
	static int movingEntry = -1;
	static int movingSegment = -1;
	static int movingPos = -1;
	static int movingPart = -1;
	int delEntry = -1;
	int dupEntry = -1;
	int ItemHeight = 20;
	
	bool popupOpened = false;
	int sequenceCount = sequence->GetItemCount();
	if (!sequenceCount)
		return false;
	
	bool isWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	
	bool isWindowHovered = ImGui::IsWindowHovered(ImGuiFocusedFlags_RootAndChildWindows);
	
	ImGui::BeginGroup();
	
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
	ImVec2 canvas_size = ImGui::GetContentRegionAvail();        // Resize canvas to what's available
	
	int firstFrameUsed = firstFrame ? *firstFrame : 0;
	
	int controlHeight = sequenceCount * ItemHeight;
	for (int i = 0; i < sequenceCount; i++)
		controlHeight += int(sequence->GetCustomHeight(i));
	int frameCount = ImMax(sequence->GetFrameMax() - sequence->GetFrameMin(), 1);
	
	static bool MovingScrollBar = false;
	static bool MovingCurrentFrame = false;
	static std::map<int, KeyframeRange> selectedKeyframes;
	
	
	struct CustomDraw
	{
		int index;
		ImRect customRect;
		ImRect legendRect;
		ImRect clippingRect;
		ImRect legendClippingRect;
	};
	ImVector<CustomDraw> customDraws;
	ImVector<CustomDraw> compactCustomDraws;
	// zoom in/out
	const int visibleFrameCount = (int)floorf((canvas_size.x - legendWidth) / framePixelWidth);
	const float barWidthRatio = ImMin(visibleFrameCount / (float)frameCount, 1.f);
	const float barWidthInPixels = barWidthRatio * (canvas_size.x - legendWidth);
	
	ImRect regionRect(canvas_pos, canvas_pos + canvas_size);
	
	static bool panningView = false;
	static ImVec2 panningViewSource;
	static int panningViewFrame;
	if (ImGui::IsWindowFocused() && io.KeyAlt && io.MouseDown[2])
	{
		if (!panningView)
		{
			panningViewSource = io.MousePos;
			panningView = true;
			panningViewFrame = *firstFrame;
		}
		*firstFrame = panningViewFrame - int((io.MousePos.x - panningViewSource.x) / framePixelWidth);
		*firstFrame = ImClamp(*firstFrame, sequence->GetFrameMin(), sequence->GetFrameMax() - visibleFrameCount);
	}
	if (panningView && !io.MouseDown[2] && isWindowFocused)
	{
		panningView = false;
	}
	framePixelWidthTarget = ImClamp(framePixelWidthTarget, 0.1f, 50.f);
	
	framePixelWidth = ImLerp(framePixelWidth, framePixelWidthTarget, 0.33f);
	
	frameCount = sequence->GetFrameMax() - sequence->GetFrameMin();
	if (visibleFrameCount >= frameCount && firstFrame)
		*firstFrame = sequence->GetFrameMin();
	
	
	// --
	if (expanded && !*expanded)
	{
		ImGui::InvisibleButton("canvas", ImVec2(canvas_size.x - canvas_pos.x, (float)ItemHeight));
		draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_size.x + canvas_pos.x, canvas_pos.y + ItemHeight), 0xFF3D3837, 0);
		char tmps[512];
		ImFormatString(tmps, IM_ARRAYSIZE(tmps), sequence->GetCollapseFmt(), frameCount, sequenceCount);
		draw_list->AddText(ImVec2(canvas_pos.x + 26, canvas_pos.y + 2), 0xFFFFFFFF, tmps);
	}
	else
	{
		bool hasScrollBar(true);
		/*
		 int framesPixelWidth = int(frameCount * framePixelWidth);
		 if ((framesPixelWidth + legendWidth) >= canvas_size.x)
		 {
		 hasScrollBar = true;
		 }
		 */
		// test scroll area
		ImVec2 headerSize(canvas_size.x, (float)ItemHeight);
		ImVec2 scrollBarSize(canvas_size.x, 14.f);
		ImGui::InvisibleButton("topBar", headerSize);
		draw_list->AddRectFilled(canvas_pos, canvas_pos + headerSize, 0xFFFF0000, 0);
		ImVec2 childFramePos = ImGui::GetCursorScreenPos();
		ImVec2 childFrameSize(canvas_size.x, canvas_size.y - 8.f - headerSize.y - (hasScrollBar ? scrollBarSize.y : 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
		ImGui::BeginChildFrame(889, childFrameSize);
		sequence->focused = ImGui::IsWindowFocused();
		ImGui::InvisibleButton("contentBar", ImVec2(canvas_size.x, float(controlHeight)));
		const ImVec2 contentMin = ImGui::GetItemRectMin();
		const ImVec2 contentMax = ImGui::GetItemRectMax();
		const ImRect contentRect(contentMin, contentMax);
		const float contentHeight = contentMax.y - contentMin.y;
		
		// full background
		draw_list->AddRectFilled(canvas_pos, canvas_pos + canvas_size, 0xFF242424, 0);
		
		// current frame top
		ImRect topRect(ImVec2(canvas_pos.x + legendWidth, canvas_pos.y), ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + ItemHeight));
		
		if (!MovingCurrentFrame && !MovingScrollBar && movingEntry == -1 && sequenceOptions & SEQUENCER_CHANGE_FRAME && currentFrame && *currentFrame >= 0 && topRect.Contains(io.MousePos) && io.MouseDown[0] && isWindowFocused)
		{
			MovingCurrentFrame = isWindowFocused;
		}
		
		if (MovingCurrentFrame)
		{
			if (frameCount)
			{
				*currentFrame = (int)((io.MousePos.x - topRect.Min.x) / framePixelWidth) + firstFrameUsed;
				if (*currentFrame < sequence->GetFrameMin())
					*currentFrame = sequence->GetFrameMin();
				if (*currentFrame >= sequence->GetFrameMax())
					*currentFrame = sequence->GetFrameMax();
			}
			if (!io.MouseDown[0] && isWindowFocused)
				MovingCurrentFrame = false;
		}
		//header
		draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_size.x + canvas_pos.x, canvas_pos.y + ItemHeight), 0xFF3D3837, 0);
		if (sequenceOptions & SEQUENCER_ADD)
		{
			if (SequencerAddDelButton(draw_list, ImVec2(canvas_pos.x + legendWidth - ItemHeight, canvas_pos.y + 2), true) && io.MouseClicked[0] && isWindowFocused){
				if(selectedKeyframes.empty()){
					sequence->AddKeyFrame();
				} else {
					sequence->AddKeyFrameRange(selectedKeyframes);
				}
			}
		}
		
		//header frame number and lines
		int modFrameCount = 10;
		int frameStep = 1;
		while ((modFrameCount * framePixelWidth) < 150)
		{
			modFrameCount *= 2;
			frameStep *= 2;
		};
		int halfModFrameCount = modFrameCount / 2;
		
		auto drawLine = [&](int i, int regionHeight) {
			bool baseIndex = ((i % modFrameCount) == 0) || (i == sequence->GetFrameMax() || i == sequence->GetFrameMin());
			bool halfIndex = (i % halfModFrameCount) == 0;
			int px = (int)canvas_pos.x + int(i * framePixelWidth) + legendWidth - int(firstFrameUsed * framePixelWidth);
			int tiretStart = baseIndex ? 4 : (halfIndex ? 10 : 14);
			int tiretEnd = baseIndex ? regionHeight : ItemHeight;
			
			if (px <= (canvas_size.x + canvas_pos.x) && px >= (canvas_pos.x + legendWidth))
			{
				draw_list->AddLine(ImVec2((float)px, canvas_pos.y + (float)tiretStart), ImVec2((float)px, canvas_pos.y + (float)tiretEnd - 1), 0xFF606060, 1);
				
				draw_list->AddLine(ImVec2((float)px, canvas_pos.y + (float)ItemHeight), ImVec2((float)px, canvas_pos.y + (float)regionHeight - 1), 0x30606060, 1);
			}
			
			if (baseIndex && px > (canvas_pos.x + legendWidth))
			{
				char tmps[512];
				ImFormatString(tmps, IM_ARRAYSIZE(tmps), "%d", i);
				draw_list->AddText(ImVec2((float)px + 3.f, canvas_pos.y), 0xFFBBBBBB, tmps);
			}
			
		};
		
		auto drawLineContent = [&](int i, int /*regionHeight*/) {
			int px = (int)canvas_pos.x + int(i * framePixelWidth) + legendWidth - int(firstFrameUsed * framePixelWidth);
			int tiretStart = int(contentMin.y);
			int tiretEnd = int(contentMax.y);
			
			if (px <= (canvas_size.x + canvas_pos.x) && px >= (canvas_pos.x + legendWidth))
			{
				//draw_list->AddLine(ImVec2((float)px, canvas_pos.y + (float)tiretStart), ImVec2((float)px, canvas_pos.y + (float)tiretEnd - 1), 0xFF606060, 1);
				
				draw_list->AddLine(ImVec2(float(px), float(tiretStart)), ImVec2(float(px), float(tiretEnd)), 0x30606060, 1);
			}
		};
		for (int i = sequence->GetFrameMin(); i <= sequence->GetFrameMax(); i += frameStep)
		{
			drawLine(i, ItemHeight);
		}
		drawLine(sequence->GetFrameMin(), ItemHeight);
		drawLine(sequence->GetFrameMax(), ItemHeight);
		/*
		 draw_list->AddLine(canvas_pos, ImVec2(canvas_pos.x, canvas_pos.y + controlHeight), 0xFF000000, 1);
		 draw_list->AddLine(ImVec2(canvas_pos.x, canvas_pos.y + ItemHeight), ImVec2(canvas_size.x, canvas_pos.y + ItemHeight), 0xFF000000, 1);
		 */
		// clip content
		
		draw_list->PushClipRect(childFramePos, childFramePos + childFrameSize);
		
		// draw item names in the legend rect on the left
		
		size_t customHeight = 0;
		for (int i = 0; i < sequenceCount; i++)
		{
			int type;
			std::vector<SequenceSegment>* segments;
			SEQUENCER_OPTIONS options;
			bool existing = true;
			
			sequence->Get(i, &type, &existing, NULL, &segments, NULL, &options);
			
			ImVec2 tpos(contentMin.x + 3, contentMin.y + i * ItemHeight + 2 + customHeight);
			
			ImGuiIO &io = ImGui::GetIO();
			ImGui::PushFont(io.Fonts->Fonts[ICON_FA]);
			ImGui::PushFont(io.Fonts->Fonts[ICON_MD]);
			
			// Position your text as before
			draw_list->AddText(tpos, existing ? 0xFFFFFFFF : 0xFF3333FF, sequence->GetItemLabel(i));
			
			ImGui::PopFont();
			ImGui::PopFont();
			
			if (options & SEQUENCER_DEL)
			{
				bool overDel = SequencerAddDelButton(draw_list, ImVec2(contentMin.x + legendWidth - ItemHeight + 2 - 10, tpos.y + 2), false);
				if (overDel && io.MouseReleased[0] && isWindowFocused)
					delEntry = i;
				
				bool overDup = SequencerAddDelButton(draw_list, ImVec2(contentMin.x + legendWidth - ItemHeight - ItemHeight + 2 - 10, tpos.y + 2), true);
				if (overDup && io.MouseReleased[0] && isWindowFocused)
					dupEntry = i;
			}
			customHeight += sequence->GetCustomHeight(i);
		}
		
		// clipping rect so items bars are not visible in the legend on the left when scrolled
		//
		
		// slots background
		customHeight = 0;
		for (int i = 0; i < sequenceCount; i++)
		{
			unsigned int col = (i & 1) ? 0xFF3A3636 : 0xFF413D3D;
			
			size_t localCustomHeight = sequence->GetCustomHeight(i);
			ImVec2 pos = ImVec2(contentMin.x + legendWidth, contentMin.y + ItemHeight * i + 1 + customHeight);
			ImVec2 sz = ImVec2(canvas_size.x + canvas_pos.x, pos.y + ItemHeight - 1 + localCustomHeight);
			if (!popupOpened && cy >= pos.y && cy < pos.y + (ItemHeight + localCustomHeight) && movingEntry == -1 && cx>contentMin.x && cx < contentMin.x + canvas_size.x)
			{
				col += 0x80201008;
				pos.x -= legendWidth;
			}
			
			if(isWindowHovered){
				draw_list->AddRectFilled(pos, sz, col, 0);
			}
			
			ImRect labelRect = {pos, sz};
			
			if (labelRect.Contains(ImGui::GetIO().MousePos) && io.MouseClicked[0] && isWindowHovered)
			{
				sequence->Click(i);
				
				sequenceCount = sequence->GetItemCount();
			}
			
			bool existing = true;
			sequence->Get(i, NULL, &existing, NULL, NULL, NULL, NULL);
			
			if(!existing){
				ImVec2 tmin = {contentMin.x,  contentMin.y + ItemHeight * i + 1 + customHeight};
				ImVec2 tmax = {tmin.x + legendWidth, tmin.y + ItemHeight - 1 + localCustomHeight};
				
				ImRect itemRect = {tmin, tmax};
				
				sequence->OnDragDropFilter = false;
				
				if (itemRect.Contains(ImGui::GetIO().MousePos))
				{
					// Check for drag and drop
					if (ImGui::BeginDragDropTarget())
					{
						sequence->OnDragDropFilter = true;
						
						ImVec2 tmin = {contentMin.x,  contentMin.y + ItemHeight * i + 1 + customHeight};
						ImVec2 tmax = {tmin.x + legendWidth, tmin.y + ItemHeight - 1 + localCustomHeight};
						
						draw_list->AddRect(tmin, tmax, 0xFF0000FF, 0);
						
						
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ACTOR"); payload)
						{
							if(sequence->OnMissingActorPayload){
								sequence->OnMissingActorPayload(i, payload, (int)TrackType::Actor);
							}
						} else if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CAMERA"); payload){
							if(sequence->OnMissingActorPayload){
								sequence->OnMissingActorPayload(i, payload, (int)TrackType::Camera);
							}
							
						} else if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LIGHT"); payload){
							if(sequence->OnMissingActorPayload){
								sequence->OnMissingActorPayload(i, payload, (int)TrackType::Light);
							}
						}
						ImGui::EndDragDropTarget();
						
						sequence->OnDragDropFilter = false;
					}
				}
			}
			
			
			customHeight += localCustomHeight;
		}
		
		draw_list->PushClipRect(childFramePos + ImVec2(float(legendWidth), 0.f), childFramePos + childFrameSize);
		
		// vertical frame lines in content area
		for (int i = sequence->GetFrameMin(); i <= sequence->GetFrameMax(); i += frameStep)
		{
			drawLineContent(i, int(contentHeight));
		}
		drawLineContent(sequence->GetFrameMin(), int(contentHeight));
		drawLineContent(sequence->GetFrameMax(), int(contentHeight));
		
		// selection
		bool selected = selectedEntry && (*selectedEntry >= 0);
		if (selected)
		{
			customHeight = 0;
			for (int i = 0; i < *selectedEntry; i++)
				customHeight += sequence->GetCustomHeight(i);;
			//            draw_list->AddRectFilled(ImVec2(contentMin.x, contentMin.y + ItemHeight * *selectedEntry + customHeight), ImVec2(contentMin.x + canvas_size.x, contentMin.y + ItemHeight * (*selectedEntry + 1) + customHeight), 0x801080FF, 1.f);
		}
		
		// slots
		
		customHeight = 0;
		
		for (int i = 0; i < sequenceCount; i++)
		{
			int* start, * end;
			int type;
			unsigned int color;
			std::vector<SequenceSegment>* segments;
			
			SEQUENCER_OPTIONS options;
			
			sequence->Get(i, &type, NULL, &color, &segments, NULL, &options);
			
			size_t localCustomHeight = sequence->GetCustomHeight(i);
			
			int segmentIndex = 0;
			
			std::vector<std::pair<float, float>> intersections; // Store intersections as pairs of start and end
			
			if(!segments->empty()){
				for (size_t i = 0; i < segments->size() - 1; ++i) {
					const auto& currentSegment = (*segments)[i];
					const auto& nextSegment = (*segments)[i + 1];
					
					// Check if the current segment overlaps with the next segment
					if (currentSegment.mFrameEnd > nextSegment.mFrameStart) {
						// There's an overlap
						float overlapStart = nextSegment.mFrameStart;
						float overlapEnd = std::min(currentSegment.mFrameEnd, nextSegment.mFrameEnd);
						intersections.push_back({overlapStart, overlapEnd});
					}
				}
			}
			
			for (auto& segment : *segments) {
				start = &segment.mFrameStart;
				end = &segment.mFrameEnd;
				
				ImVec2 pos = ImVec2(contentMin.x + legendWidth - firstFrameUsed * framePixelWidth, contentMin.y + ItemHeight * i + 1 + customHeight);
				ImVec2 slotP1(pos.x + *start * framePixelWidth, pos.y + 2);
				ImVec2 slotP2(pos.x + *end * framePixelWidth + framePixelWidth, pos.y + ItemHeight - 2);
				ImVec2 slotP3(pos.x + *end * framePixelWidth + framePixelWidth, pos.y + ItemHeight - 2 + localCustomHeight);
				unsigned int slotColor = color | 0xFF000000;
				unsigned int slotColorHalf = (color & 0xFFFFFF) | 0x40000000;
				
				if (slotP1.x <= (canvas_size.x + contentMin.x) && slotP2.x >= (contentMin.x + legendWidth))
				{
					draw_list->AddRectFilled(slotP1, slotP3, slotColorHalf, 2);
					draw_list->AddRectFilled(slotP1, slotP2, slotColor, 2);
				}
				
				// Draw the intersections
				for (const auto& intersection : intersections) {
					ImVec2 intersectionPos1(contentMin.x + legendWidth - firstFrameUsed * framePixelWidth + intersection.first * framePixelWidth, contentMin.y + ItemHeight * i + 2 + customHeight);
					ImVec2 intersectionPos2(contentMin.x + legendWidth - firstFrameUsed * framePixelWidth + intersection.second * framePixelWidth, contentMin.y + ItemHeight * i + ItemHeight - 2 + customHeight);
					
					unsigned int slotColor = color & 0xFF000000; // Keep the alpha channel unchanged
					
					// Reduce the intensity of each color channel by multiplying with a factor (e.g., 0.5)
					slotColor |= ((color & 0x00FF0000) >> 1) & 0x00FF0000; // Red channel
					slotColor |= ((color & 0x0000FF00) >> 1) & 0x0000FF00; // Green channel
					slotColor |= ((color & 0x000000FF) >> 1) & 0x000000FF; // Blue channel
					
					draw_list->AddRectFilled(intersectionPos1, intersectionPos2, slotColor, 2);
				}
				
				if (ImRect(slotP1, slotP2).Contains(io.MousePos) && io.MouseClicked[0] && isWindowFocused)
				{
					sequence->Click(i);
					
					sequenceCount = sequence->GetItemCount();
				}
				
				if (ImRect(slotP1, slotP2).Contains(io.MousePos) && io.MouseDoubleClicked[0] && isWindowFocused)
				{
					sequence->DoubleClick(i);
					sequenceCount = sequence->GetItemCount();
				}
				
				
				segmentIndex++;
			}
			
			
			// custom draw
			if (localCustomHeight > 0)
			{
				ImVec2 rp(canvas_pos.x, contentMin.y + ItemHeight * i + 1 + customHeight);
				ImRect customRect(rp + ImVec2(legendWidth - (firstFrameUsed - sequence->GetFrameMin() - 0.5f) * framePixelWidth, float(ItemHeight)),
								  rp + ImVec2(legendWidth + (sequence->GetFrameMax() - firstFrameUsed - 0.5f + 2.f) * framePixelWidth, float(localCustomHeight + ItemHeight)));
				ImRect clippingRect(rp + ImVec2(float(legendWidth), float(ItemHeight)), rp + ImVec2(canvas_size.x, float(localCustomHeight + ItemHeight)));
				
				ImRect legendRect(rp + ImVec2(0.f, float(ItemHeight)), rp + ImVec2(float(legendWidth), float(localCustomHeight)));
				ImRect legendClippingRect(canvas_pos + ImVec2(0.f, float(ItemHeight)), canvas_pos + ImVec2(float(legendWidth), float(localCustomHeight + ItemHeight)));
				customDraws.push_back({ i, customRect, legendRect, clippingRect, legendClippingRect });
			}
			else
			{
				ImVec2 rp(canvas_pos.x, contentMin.y + ItemHeight * i + customHeight);
				ImRect customRect(rp + ImVec2(legendWidth - (firstFrameUsed - sequence->GetFrameMin() - 0.5f) * framePixelWidth, float(0.f)),
								  rp + ImVec2(legendWidth + (sequence->GetFrameMax() - firstFrameUsed - 0.5f + 2.f) * framePixelWidth, float(ItemHeight)));
				ImRect clippingRect(rp + ImVec2(float(legendWidth), float(0.f)), rp + ImVec2(canvas_size.x, float(ItemHeight)));
				
				compactCustomDraws.push_back({ i, customRect, ImRect(), clippingRect, ImRect() });
			}
			customHeight += localCustomHeight;
			
			segmentIndex = 0;
			
			// Draw handles again to ensure they are topmost
			for (auto& segment : *segments) {
				start = &segment.mFrameStart;
				end = &segment.mFrameEnd;
				unsigned int slotColor = color | 0xFF000000;
				unsigned int slotColorHalf = (color & 0xFFFFFF) | 0x40000000;
				
				ImVec2 pos = ImVec2(contentMin.x + legendWidth - firstFrameUsed * framePixelWidth, contentMin.y + ItemHeight * i + 1 + customHeight);
				ImVec2 slotP1(pos.x + *start * framePixelWidth, pos.y + 2);
				ImVec2 slotP2(pos.x + *end * framePixelWidth + framePixelWidth, pos.y + ItemHeight - 2);
				
				// Ensure grabbable handles
				const float max_handle_width = slotP2.x - slotP1.x / 3.0f;
				const float min_handle_width = ImMin(10.0f, max_handle_width);
				const float handle_width = ImClamp(framePixelWidth / 2.0f, min_handle_width, max_handle_width);
				ImRect rects[3] = { ImRect(slotP1, ImVec2(slotP1.x + handle_width, slotP2.y))
					, ImRect(ImVec2(slotP2.x - handle_width, slotP1.y), slotP2)
					, ImRect(ImVec2(slotP1.x + handle_width, slotP1.y), ImVec2(slotP2.x - handle_width, slotP2.y)) };
				
				const unsigned int quadColor[]
				= { 0xFFFFFFFF, 0xFFFFFFFF, slotColor };
				if (movingEntry == -1 && options & SEQUENCER_EDIT_STARTEND) {
					
					for (int j = 2; j >= 0; j--) {
						ImRect& rc = rects[j];
						if (!rc.Contains(io.MousePos))
							continue;
						
						// Draw handles except for the main segment rectangle
						if(j != 2){
							draw_list->AddRectFilled(rc.Min, rc.Max, quadColor[j], 2);
						}
						
						if (!ImRect(childFramePos, childFramePos + childFrameSize).Contains(io.MousePos))
							continue;
						
						
						bool intersectionCollide = false;
						
						// Draw the intersections
						for (const auto& intersection : intersections) {
							ImVec2 intersectionPos1(contentMin.x + legendWidth - firstFrameUsed * framePixelWidth + intersection.first * framePixelWidth, rc.Min.y);
							ImVec2 intersectionPos2(contentMin.x + legendWidth - firstFrameUsed * framePixelWidth + intersection.second * framePixelWidth, rc.Max.y);
							
							ImRect intersectionRect(intersectionPos1, intersectionPos2);
							if(j == 2){
								if(rc.Contains(intersectionRect) && intersectionRect.Contains(io.MousePos)){
									intersectionCollide = true;
									break;
								}
							}
						}
						
						// Check if the click is inside intersections and j == 2
						if (intersectionCollide) {
							// Do nothing
						} else if (!ImGui::IsMouseDoubleClicked(0) && ImGui::IsMouseClicked(0) && !MovingScrollBar && !MovingCurrentFrame && isWindowFocused) {
							// Draw the intersections
							for (const auto& intersection : intersections) {
								ImVec2 intersectionPos1(contentMin.x + legendWidth - firstFrameUsed * framePixelWidth + intersection.first * framePixelWidth, contentMin.y + ItemHeight * i + 2 + customHeight);
								ImVec2 intersectionPos2(contentMin.x + legendWidth - firstFrameUsed * framePixelWidth + intersection.second * framePixelWidth, contentMin.y + ItemHeight * i + ItemHeight - 2 + customHeight);
								draw_list->AddRectFilled(intersectionPos1, intersectionPos2, slotColor, 2);
							}
							movingEntry = i;
							movingPos = cx;
							movingPart = j + 1;
							sequence->BeginEdit(movingEntry, segmentIndex);
							movingSegment = segmentIndex;
							break;
						}
						
						
					}
				}
				segmentIndex++;
			}
		}
		
		// moving
		if (/*backgroundRect.Contains(io.MousePos) && */movingEntry >= 0)
		{
			ImGui::CaptureMouseFromApp();
			int diffFrame = int((cx - movingPos) / framePixelWidth);
			if (std::abs(diffFrame) > 0)
			{
				int* start, * end;
				std::vector<SequenceSegment>* segments;
				SEQUENCER_OPTIONS options;
				
				sequence->Get(movingEntry, NULL, NULL, NULL, &segments, NULL, &options);
				
				int segmentIndex = 0;
				
				for (auto& segment : *segments) {
					
					if(segmentIndex != movingSegment){
						segmentIndex++;
						continue;
					}
					
					start = &segment.mFrameStart;
					end = &segment.mFrameEnd;
					if (selectedEntry)
						*selectedEntry = movingEntry;
					int& l = *start;
					int& r = *end;
					if (movingPart & 1)
						l += diffFrame;
					if (movingPart & 2)
						r += diffFrame;
					if (l < 0)
					{
						if (movingPart & 2)
							r -= l;
						l = 0;
					}
					if (movingPart & 1 && l > r)
						l = r;
					if (movingPart & 2 && r < l)
						r = l;
					movingPos += int(diffFrame * framePixelWidth);
					
					segmentIndex++;
				}
			}
			
			if (!io.MouseDown[0] && isWindowFocused)
			{
				// single select
				if (!diffFrame && movingPart && selectedEntry)
				{
					*selectedEntry = movingEntry;
					ret = true;
				}
				
				movingEntry = -1;
				movingSegment = -1;
				sequence->EndEdit();
			}
			
		}
		
		// cursor
		if (currentFrame && firstFrame && *currentFrame >= *firstFrame && *currentFrame <= sequence->GetFrameMax())
		{
			static const float cursorWidth = 6.f;
			float cursorOffset = contentMin.x + legendWidth + (*currentFrame - firstFrameUsed) * framePixelWidth + framePixelWidth / 2 - cursorWidth * 0.5f;
			draw_list->AddLine(ImVec2(cursorOffset, canvas_pos.y), ImVec2(cursorOffset, contentMax.y), 0xA02A2AFF, cursorWidth);
		}
		
		for (int i = 0; i < sequenceCount; i++)
		{
			size_t localCustomHeight = sequence->GetCustomHeight(i);
			ImVec2 pos = ImVec2(contentMin.x + legendWidth, contentMin.y + ItemHeight * i + 1 + customHeight);
			
			std::map<int, Keyframe>* keyframes = NULL;
			
			sequence->Get(i, NULL, NULL, NULL, NULL, &keyframes, NULL);
			
			if(keyframes != NULL){
				for(auto& keyframe : *keyframes){
					int frame = keyframe.first;
					
					if (frame >= *firstFrame && frame <= sequence->GetFrameMax() && keyframe.second.active)
					{
						static const float cursorWidth = 6.f;
						float cursorOffset = contentMin.x + legendWidth + (frame - firstFrameUsed) * framePixelWidth;
						
						float dotRadius = 3.0f;
						
						draw_list->AddCircleFilled(ImVec2(cursorOffset + framePixelWidth / 2.0f - dotRadius + 0.5f, pos.y + ItemHeight / 2.0f + localCustomHeight / 2.0f), dotRadius, 0xFF000000);
					}
				}
			}
		}
		
		
		if(sequenceOptions & SEQUENCER_SELECT){
			
			static const float cursorWidth = 6.f;
			int canvasStart = static_cast<int>((0 - contentMin.x - legendWidth) / framePixelWidth) + firstFrameUsed;
			
			float canvasOffsetStart = contentMin.x + legendWidth + (0 - firstFrameUsed) * framePixelWidth + framePixelWidth / 2 - cursorWidth * 2;
			// Call to handle selection box logic
			bool selectingThisFrame = selecting;
			
			
			const ImRect canvasRect(ImVec2(canvasOffsetStart, canvas_pos.y + headerSize.y), ImVec2(contentMax.x, contentMax.y));
			
			if (canvasRect.Contains(ImGui::GetIO().MousePos) && isWindowFocused)
			{
				if(io.MouseDown[0]){
					
					HandleSelectionBox();
					
					float cursorOffsetStart = selectionEnd.x;
					
					int frameStart = static_cast<int>((cursorOffsetStart - contentMin.x - legendWidth) / framePixelWidth) + firstFrameUsed;
					
					if(currentFrame){
						*currentFrame = frameStart;
					}
				} else {
					HandleSelectionBox();
				}
			}
			
			
			
			if (sequenceOptions & SEQUENCER_SELECT)
			{
				// Calculate the minimum and maximum points to make the coordinates absolute
				ImVec2 topLeft, bottomRight;
				
				topLeft.x = std::min(selectionStart.x, selectionEnd.x);
				topLeft.y = std::min(selectionStart.y, selectionEnd.y);
				
				bottomRight.x = std::max(selectionStart.x, selectionEnd.x);
				bottomRight.y = std::max(selectionStart.y, selectionEnd.y);
				
				ImRect selectionRect = {topLeft, bottomRight};

				if (canvasRect.Contains(ImGui::GetIO().MousePos) && isWindowFocused){
					if(!selectingThisFrame && selecting){
						selectedKeyframes.clear();
					}
					
					// Call to draw the selection box
					DrawSelectionBox(draw_list);
				}
				
				if (canvasRect.Contains(ImGui::GetIO().MousePos) && isWindowFocused && selecting)
				{
					for(int i = 0; i<sequenceCount; ++i){
						
						float cursorOffsetStart = selectionStart.x;
						float cursorOffsetEnd = selectionEnd.x;
						
						int frameStart = static_cast<int>((cursorOffsetStart - contentMin.x - legendWidth) / framePixelWidth) + firstFrameUsed;
						
						cursorOffsetStart = contentMin.x + legendWidth + (frameStart - firstFrameUsed) * framePixelWidth + framePixelWidth / 2 - cursorWidth * 2;
						
						int frameEnd = static_cast<int>((cursorOffsetEnd - contentMin.x - legendWidth) / framePixelWidth) + firstFrameUsed;
						
						const ImVec2 start(cursorOffsetStart, contentMin.y + ItemHeight * i + 1 + customHeight);
						
						cursorOffsetEnd = contentMin.x + legendWidth + (frameEnd - firstFrameUsed) * framePixelWidth + framePixelWidth / 2 + cursorWidth;
						
						const ImVec2 end(cursorOffsetEnd, start.y + ItemHeight - 2);
						
						// Make start always the top-left point and end the bottom-right point
						ImVec2 correctedStart, correctedEnd;
						
						correctedStart.x = std::min(start.x, end.x);
						correctedStart.y = std::min(start.y, end.y);
						
						correctedEnd.x = std::max(start.x, end.x);
						correctedEnd.y = std::max(start.y, end.y);
						
						const ImRect selectionSegment(correctedStart, correctedEnd);

						if (selectionSegment.Overlaps(selectionRect)) {
							selectedKeyframes[i].range = std::make_tuple(frameStart, frameEnd);
							
							draw_list->AddRectFilled(correctedStart, correctedEnd, IM_COL32(64, 64, 64, 64), cursorWidth);
						} else {
							selectedKeyframes.erase(i);
						}
						
					}
				} else {
					const float cursorWidth = 6.f;
					
					for(auto& range : selectedKeyframes){
						
						int i = range.first;
						
						auto [frameStart, frameEnd] = range.second.range;
						
						float cursorOffsetStart = contentMin.x + legendWidth + (frameStart - firstFrameUsed) * framePixelWidth + framePixelWidth / 2 - cursorWidth * 2;
						
						const ImVec2 start(cursorOffsetStart, contentMin.y + ItemHeight * i + 1 + customHeight);
						
						float cursorOffsetEnd = contentMin.x + legendWidth + (frameEnd - firstFrameUsed) * framePixelWidth + framePixelWidth / 2 + cursorWidth;
						
						const ImVec2 end(cursorOffsetEnd, start.y + ItemHeight - 2);
						
						ImVec2 correctedStart, correctedEnd;
						
						correctedStart.x = std::min(start.x, end.x);
						correctedStart.y = std::min(start.y, end.y);
						
						correctedEnd.x = std::max(start.x, end.x);
						correctedEnd.y = std::max(start.y, end.y);

						draw_list->AddRectFilled(correctedStart, correctedEnd, IM_COL32(64, 64, 64, 64), cursorWidth);
					}
				}
				
			}
		}

		draw_list->PopClipRect();
		draw_list->PopClipRect();
		
		for (auto& customDraw : customDraws)
			sequence->CustomDraw(customDraw.index, draw_list, customDraw.customRect, customDraw.legendRect, customDraw.clippingRect, customDraw.legendClippingRect);
		for (auto& customDraw : compactCustomDraws)
			sequence->CustomDrawCompact(customDraw.index, draw_list, customDraw.customRect, customDraw.clippingRect);
		
		ImRect rectFrame(ImVec2(contentMin.x, canvas_pos.y + 2)
						 , ImVec2(contentMin.x + 30, canvas_pos.y + ItemHeight - 2));
		unsigned int frameColor = 0xFFFFFFFF;
		
		// copy paste
		if (sequenceOptions & SEQUENCER_COPYPASTE)
		{
			ImRect rectCopy(ImVec2(contentMin.x + 100, canvas_pos.y + 2)
							, ImVec2(contentMin.x + 100 + 30, canvas_pos.y + ItemHeight - 2));
			bool inRectCopy = rectCopy.Contains(io.MousePos);
			unsigned int copyColor = inRectCopy ? 0xFF1080FF : 0xFF000000;
			draw_list->AddText(rectCopy.Min, copyColor, "Copy");
			
			ImRect rectPaste(ImVec2(contentMin.x + 140, canvas_pos.y + 2)
							 , ImVec2(contentMin.x + 140 + 30, canvas_pos.y + ItemHeight - 2));
			bool inRectPaste = rectPaste.Contains(io.MousePos);
			unsigned int pasteColor = inRectPaste ? 0xFF1080FF : 0xFF000000;
			draw_list->AddText(rectPaste.Min, pasteColor, "Paste");
			
			if (inRectCopy && io.MouseReleased[0] && isWindowFocused)
			{
				sequence->Copy();
			}
			if (inRectPaste && io.MouseReleased[0] && isWindowFocused)
			{
				sequence->Paste();
			}
		}
		//
		
		ImGui::EndChildFrame();
		ImGui::PopStyleColor();
		if (hasScrollBar)
		{
			ImGui::InvisibleButton("scrollBar", scrollBarSize);
			ImVec2 scrollBarMin = ImGui::GetItemRectMin();
			ImVec2 scrollBarMax = ImGui::GetItemRectMax();
			
			// ratio = number of frames visible in control / number to total frames
			
			float startFrameOffset = ((float)(firstFrameUsed - sequence->GetFrameMin()) / (float)frameCount) * (canvas_size.x - legendWidth);
			ImVec2 scrollBarA(scrollBarMin.x + legendWidth, scrollBarMin.y - 2);
			ImVec2 scrollBarB(scrollBarMin.x + canvas_size.x, scrollBarMax.y - 1);
			draw_list->AddRectFilled(scrollBarA, scrollBarB, 0xFF222222, 0);
			
			ImRect scrollBarRect(scrollBarA, scrollBarB);
			bool inScrollBar = scrollBarRect.Contains(io.MousePos);
			
			draw_list->AddRectFilled(scrollBarA, scrollBarB, 0xFF101010, 8);
			
			
			ImVec2 scrollBarC(scrollBarMin.x + legendWidth + startFrameOffset, scrollBarMin.y);
			ImVec2 scrollBarD(scrollBarMin.x + legendWidth + barWidthInPixels + startFrameOffset, scrollBarMax.y - 2);
			draw_list->AddRectFilled(scrollBarC, scrollBarD, (inScrollBar || MovingScrollBar) ? 0xFF606060 : 0xFF505050, 6);
			
			ImRect barHandleLeft(scrollBarC, ImVec2(scrollBarC.x + 14, scrollBarD.y));
			ImRect barHandleRight(ImVec2(scrollBarD.x - 14, scrollBarC.y), scrollBarD);
			
			bool onLeft = barHandleLeft.Contains(io.MousePos);
			bool onRight = barHandleRight.Contains(io.MousePos);
			
			static bool sizingRBar = false;
			static bool sizingLBar = false;
			
			draw_list->AddRectFilled(barHandleLeft.Min, barHandleLeft.Max, (onLeft || sizingLBar) ? 0xFFAAAAAA : 0xFF666666, 6);
			draw_list->AddRectFilled(barHandleRight.Min, barHandleRight.Max, (onRight || sizingRBar) ? 0xFFAAAAAA : 0xFF666666, 6);
			
			ImRect scrollBarThumb(scrollBarC, scrollBarD);
			static const float MinBarWidth = 44.f;
			if (sizingRBar)
			{
				if (!io.MouseDown[0] && isWindowFocused)
				{
					sizingRBar = false;
				}
				else
				{
					float barNewWidth = ImMax(barWidthInPixels + io.MouseDelta.x, MinBarWidth);
					float barRatio = barNewWidth / barWidthInPixels;
					framePixelWidthTarget = framePixelWidth = framePixelWidth / barRatio;
					int newVisibleFrameCount = int((canvas_size.x - legendWidth) / framePixelWidthTarget);
					int lastFrame = *firstFrame + newVisibleFrameCount;
				}
			}
			else if (sizingLBar)
			{
				if (!io.MouseDown[0] && isWindowFocused)
				{
					sizingLBar = false;
				}
				else
				{
					if (fabsf(io.MouseDelta.x) > FLT_EPSILON)
					{
						float barNewWidth = ImMax(barWidthInPixels - io.MouseDelta.x, MinBarWidth);
						float barRatio = barNewWidth / barWidthInPixels;
						float previousFramePixelWidthTarget = framePixelWidthTarget;
						framePixelWidthTarget = framePixelWidth = framePixelWidth / barRatio;
						int newVisibleFrameCount = int(visibleFrameCount / barRatio);
						int newFirstFrame = *firstFrame + newVisibleFrameCount - visibleFrameCount;
						newFirstFrame = ImClamp(newFirstFrame, sequence->GetFrameMin(), ImMax(sequence->GetFrameMax() - visibleFrameCount, sequence->GetFrameMin()));
						if (newFirstFrame == *firstFrame)
						{
							framePixelWidth = framePixelWidthTarget = previousFramePixelWidthTarget;
						}
						else
						{
							*firstFrame = newFirstFrame;
						}
					}
				}
			}
			else
			{
				if (MovingScrollBar)
				{
					if (!io.MouseDown[0] && isWindowFocused)
					{
						MovingScrollBar = false;
					}
					else
					{
						float framesPerPixelInBar = barWidthInPixels / (float)visibleFrameCount;
						*firstFrame = int((io.MousePos.x - panningViewSource.x) / framesPerPixelInBar) - panningViewFrame;
						*firstFrame = ImClamp(*firstFrame, sequence->GetFrameMin(), ImMax(sequence->GetFrameMax() - visibleFrameCount, sequence->GetFrameMin()));
					}
				}
				else
				{
					float barNewWidth = ImMax(barWidthInPixels, MinBarWidth);
					float barRatio = barNewWidth / barWidthInPixels;
					framePixelWidthTarget = framePixelWidth = framePixelWidth / barRatio;
					int newVisibleFrameCount = int((canvas_size.x - legendWidth) / framePixelWidthTarget);
					int lastFrame = *firstFrame + newVisibleFrameCount;
					if (lastFrame > sequence->GetFrameMax() + 1)
					{
						framePixelWidthTarget = framePixelWidth = (canvas_size.x - legendWidth + 4.5f) / float(sequence->GetFrameMax() + 1 - *firstFrame);
					}
					
					if (scrollBarThumb.Contains(io.MousePos) && ImGui::IsMouseClicked(0) && firstFrame && !MovingCurrentFrame && movingEntry == -1 && isWindowFocused)
					{
						MovingScrollBar = true;
						panningViewSource = io.MousePos;
						panningViewFrame = -*firstFrame;
					}
					if (!sizingRBar && onRight && ImGui::IsMouseClicked(0) && isWindowFocused)
						sizingRBar = true;
					if (!sizingLBar && onLeft && ImGui::IsMouseClicked(0) && isWindowFocused)
						sizingLBar = true;
					
				}
			}
		}
	}
	
	ImGui::EndGroup();
	
	if (regionRect.Contains(io.MousePos))
	{
		bool overCustomDraw = false;
		for (auto& custom : customDraws)
		{
			if (custom.customRect.Contains(io.MousePos))
			{
				overCustomDraw = true;
			}
		}
		if (overCustomDraw)
		{
		}
		else
		{
#if 0
			frameOverCursor = *firstFrame + (int)(visibleFrameCount * ((io.MousePos.x - (float)legendWidth - canvas_pos.x) / (canvas_size.x - legendWidth)));
			//frameOverCursor = max(min(*firstFrame - visibleFrameCount / 2, frameCount - visibleFrameCount), 0);
			
			/**firstFrame -= frameOverCursor;
			 *firstFrame *= framePixelWidthTarget / framePixelWidth;
			 *firstFrame += frameOverCursor;*/
			if (io.MouseWheel < -FLT_EPSILON)
			{
				*firstFrame -= frameOverCursor;
				*firstFrame = int(*firstFrame * 1.1f);
				framePixelWidthTarget *= 0.9f;
				*firstFrame += frameOverCursor;
			}
			
			if (io.MouseWheel > FLT_EPSILON)
			{
				*firstFrame -= frameOverCursor;
				*firstFrame = int(*firstFrame * 0.9f);
				framePixelWidthTarget *= 1.1f;
				*firstFrame += frameOverCursor;
			}
#endif
		}
	}
	
	if (expanded)
	{
		//		bool overExpanded = SequencerAddDelButton(draw_list, ImVec2(canvas_pos.x + 2, canvas_pos.y + 2), !*expanded);
		//		if (overExpanded && io.MouseReleased[0])
		//			*expanded = !*expanded;
	}
	
	if (delEntry != -1)
	{
		sequence->Del(delEntry, movingSegment);
		if (selectedEntry && (*selectedEntry == delEntry || *selectedEntry >= sequence->GetItemCount()))
			*selectedEntry = -1;
	}
	
	if (dupEntry != -1)
	{
		sequence->Duplicate(dupEntry);
	}
	return ret;
}
}
