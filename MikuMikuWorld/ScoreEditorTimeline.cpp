#include "ScoreEditorTimeline.h"
#include "Application.h"
#include "ApplicationConfiguration.h"
#include "Colors.h"
#include "Constants.h"
#include "ResourceManager.h"
#include "Tempo.h"
#include "Score.h"
#include "UI.h"
#include "Utilities.h"
#include <algorithm>
#include <string>

namespace MikuMikuWorld
{
	ScoreEditorTimeline* timelineInstance = nullptr;

	void scrollTimeline(ScoreContext& context, const int tick)
	{
		if (timelineInstance != nullptr)
			timelineInstance->scrollTimeline(context, tick);
	}

	bool eventControl(float xPos, Vector2 pos, ImU32 color, const char* txt, bool enabled)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return false;

		pos.x = floorf(pos.x);
		pos.y = floorf(pos.y);

		ImGui::PushID(pos.y);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 1, 0 });
		bool activated = UI::coloredButton(
		    txt, { pos.x, pos.y - ImGui::GetFrameHeightWithSpacing() }, { -1, -1 }, color, enabled);
		ImGui::PopStyleVar();
		ImGui::PopID();
		drawList->AddLine({ xPos, pos.y }, { pos.x + ImGui::GetItemRectSize().x, pos.y }, color,
		                  primaryLineThickness);

		return activated;
	}

	float ScoreEditorTimeline::getNoteYPosFromTick(int tick) const
	{
		return position.y + tickToPosition(tick) - visualOffset + size.y;
	}

	int ScoreEditorTimeline::positionToTick(double pos) const
	{
		return roundf(pos / (unitHeight * zoom));
	}

	float ScoreEditorTimeline::tickToPosition(int tick) const { return tick * unitHeight * zoom; }

	float ScoreEditorTimeline::positionToLane(float pos) const
	{
		return (pos - laneOffset) / laneWidth;
	}

	float ScoreEditorTimeline::laneToPosition(float lane) const
	{
		//lane++;
		return laneOffset + (lane * laneWidth);
	}

	bool ScoreEditorTimeline::isNoteVisible(const Note& note, int offsetTicks) const
	{
		const float y = getNoteYPosFromTick(note.tick + offsetTicks);
		return y >= 0 && y <= size.y + position.y + 100;
	}

	void ScoreEditorTimeline::setZoom(float value)
	{
		int tick = positionToTick(offset - size.y);
		float x1 = position.y - tickToPosition(tick) + offset;

		zoom = std::clamp(value, minZoom, maxZoom);

		// Prevent jittery movement when zooming
		float x2 = position.y - tickToPosition(tick) + offset;
		visualOffset = offset = std::max(offset + x1 - x2, minOffset);
	}

	int ScoreEditorTimeline::snapTickFromPos(double posY) const
	{
		return snapTick(positionToTick(posY), division);
	}

	int ScoreEditorTimeline::laneFromCenterPosition(const Score& score, int lane, int width)
	{
		return std::clamp(lane - (width / 2), MIN_LANE - score.metadata.laneExtension,
		                  MAX_LANE - width + 1 + score.metadata.laneExtension);
	}

	//mod 无轨用
	float ScoreEditorTimeline::laneFromCenterPosition(const Score& score, float lane, int width)
	{
		return std::clamp(lane - (width / 2), (float)MIN_LANE - score.metadata.laneExtension,
			(float)MAX_LANE - width + 1 + score.metadata.laneExtension);
	}

	void ScoreEditorTimeline::focusCursor(ScoreContext& context, Direction direction)
	{
		float cursorY = tickToPosition(context.currentTick);
		if (direction == Direction::Down)
		{
			float timelineOffset = size.y * (1.0f - config.cursorPositionThreshold);
			if (cursorY <= offset - timelineOffset)
				offset = cursorY + timelineOffset;
		}
		else
		{
			float timelineOffset = size.y * config.cursorPositionThreshold;
			if (cursorY >= offset - timelineOffset)
				offset = cursorY + timelineOffset;
		}
	}

	void ScoreEditorTimeline::updateScrollbar()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
		float paddingY = 30.0f;
		ImVec2 windowEndTop = ImGui::GetWindowPos() +
		                      ImVec2{ ImGui::GetWindowSize().x - scrollbarWidth - 4, paddingY };
		ImVec2 windowEndBottom =
		    windowEndTop +
		    ImVec2{ scrollbarWidth + 2,
			        ImGui::GetWindowSize().y - (paddingY * 1.3f) - UI::toolbarBtnSize.y - 5 };

		// calculate handle height
		float heightRatio = size.y / ((maxOffset - minOffset) * zoom);
		float handleHeight =
		    std::max(20.0f, ((windowEndBottom.y - windowEndTop.y) * heightRatio) + 30.0f);
		float scrollHeight = windowEndBottom.y - windowEndTop.y - handleHeight;

		// calculate handle position
		float currentOffset = offset - minOffset;
		float positionRatio = std::min(1.0f, currentOffset / ((maxOffset * zoom) - minOffset));
		float handlePosition = windowEndBottom.y - (scrollHeight * positionRatio) - handleHeight;

		// make handle slightly narrower than the background
		ImVec2 scrollHandleMin = ImVec2{ windowEndTop.x + 2, handlePosition };
		ImVec2 scrollHandleMax =
		    ImVec2{ windowEndTop.x + scrollbarWidth - 2, handlePosition + handleHeight };

		// handle "button"
		ImGuiCol handleColor = ImGuiCol_ScrollbarGrab;
		ImGui::SetCursorScreenPos(scrollHandleMin);
		ImGui::InvisibleButton("##scroll_handle", ImVec2{ scrollbarWidth, handleHeight });
		if (ImGui::IsItemHovered())
			handleColor = ImGuiCol_ScrollbarGrabHovered;

		if (ImGui::IsItemActivated())
			scrollStartY = ImGui::GetMousePos().y;

		if (ImGui::IsItemActive())
		{
			handleColor = ImGuiCol_ScrollbarGrabActive;
			float dragDeltaY = scrollStartY - ImGui::GetMousePos().y;
			if (abs(dragDeltaY) > 0)
			{
				// convert handle position to timeline offset
				handlePosition -= dragDeltaY;
				positionRatio =
				    std::min(1.0f, 1 - ((handlePosition - windowEndTop.y) / scrollHeight));
				float newOffset = ((maxOffset * zoom) - minOffset) * positionRatio;

				offset = newOffset + minOffset;
				scrollStartY = ImGui::GetMousePos().y;
			}
		}

		if (!ImGui::IsItemActive() && positionRatio >= 0.92f)
			maxOffset += 2000;

		ImGui::SetCursorScreenPos(windowEndTop);
		ImGui::InvisibleButton("##scroll_background",
		                       ImVec2{ scrollbarWidth, scrollHeight + handleHeight },
		                       ImGuiButtonFlags_AllowItemOverlap);
		if (ImGui::IsItemActivated())
		{
			float yPos = std::clamp(ImGui::GetMousePos().y, windowEndTop.y,
			                        windowEndBottom.y - handleHeight);

			// convert handle position to timeline offset
			positionRatio = std::clamp(1 - ((yPos - windowEndTop.y)) / scrollHeight, 0.0f, 1.0f);
			float newOffset = ((maxOffset * zoom) - minOffset) * positionRatio;

			offset = newOffset + minOffset;
		}

		ImU32 scrollBgColor =
		    ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarBg));
		ImU32 scrollHandleColor =
		    ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(handleColor));
		drawList->AddRectFilled(windowEndTop, windowEndBottom, scrollBgColor, 0);
		drawList->AddRectFilled(scrollHandleMin, scrollHandleMax, scrollHandleColor,
		                        ImGui::GetStyle().ScrollbarRounding, ImDrawFlags_RoundCornersAll);
	}

	void ScoreEditorTimeline::calculateMaxOffsetFromScore(const Score& score)
	{
		int maxTick = 0;
		for (const auto& [id, note] : score.notes)
			maxTick = std::max(maxTick, note.tick);

		// Current offset maybe greater than calculated offset from score
		maxOffset = std::max(offset / zoom, (maxTick * unitHeight) + 1000);
	}

	void ScoreEditorTimeline::updateScrollingPosition()
	{
		if (config.useSmoothScrolling)
		{
			float scrollAmount = offset - visualOffset;
			float remainingScroll = abs(scrollAmount);
			float delta =
			    scrollAmount / (config.smoothScrollingTime / (ImGui::GetIO().DeltaTime * 1000));

			visualOffset += std::min(remainingScroll, delta);
			remainingScroll = std::max(0.0f, remainingScroll - abs(delta));
		}
		else
		{
			visualOffset = offset;
		}
	}

	void ScoreEditorTimeline::contextMenu(ScoreContext& context)
	{
		if (ImGui::BeginPopupContextWindow(IMGUI_TITLE(ICON_FA_MUSIC, "notes_timeline")))
		{
			if (ImGui::MenuItem(getString("delete"), ToShortcutString(config.input.deleteSelection),
			                    false, !context.selectedNotes.empty()))
				context.deleteSelection();

			ImGui::Separator();
			if (ImGui::MenuItem(getString("cut"), ToShortcutString(config.input.cutSelection),
			                    false, !context.selectedNotes.empty()))
				context.cutSelection();

			if (ImGui::MenuItem(getString("copy"), ToShortcutString(config.input.copySelection),
			                    false, !context.selectedNotes.empty()))
				context.copySelection();

			if (ImGui::MenuItem(getString("paste"), ToShortcutString(config.input.paste)))
				context.paste(false);

			if (ImGui::MenuItem(getString("flip_paste"), ToShortcutString(config.input.flipPaste)))
				context.paste(true);

			if (ImGui::MenuItem(getString("duplicate"), ToShortcutString(config.input.duplicate),
			                    false, !context.selectedNotes.empty()))
				context.duplicateSelection(false);

			if (ImGui::MenuItem(getString("flip_duplicate"),
			                    ToShortcutString(config.input.flipDuplicate), false,
			                    !context.selectedNotes.empty()))
				context.duplicateSelection(true);

			if (ImGui::MenuItem(getString("flip"), ToShortcutString(config.input.flip), false,
			                    !context.selectedNotes.empty()))
				context.flipSelection();

			ImGui::Separator();
			if (ImGui::BeginMenu(getString("ease_type"), context.selectionHasEase()))
			{
				for (int i = 0; i < arrayLength(easeTypes); ++i)
					if (ImGui::MenuItem(getString(easeTypes[i])))
						context.setEase((EaseType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("step_type"), context.selectionHasStep()))
			{
				for (int i = 0; i < arrayLength(stepTypes); ++i)
					if (ImGui::MenuItem(getString(stepTypes[i])))
						context.setStep((HoldStepType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("flick_type"), context.selectionHasFlickable()))
			{
				for (int i = 0; i < arrayLength(flickTypes); ++i)
					if (ImGui::MenuItem(getString(flickTypes[i])))
						context.setFlick((FlickType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("hold_type"), context.selectionCanChangeHoldType()))
			{
				/*
				 * We we won't show the option to change holds to guides and vice versa
				 * at least for now since it will complicate changing hold types
				 */
				for (int i = 0; i < arrayLength(holdTypes) - 1; i++)
					if (ImGui::MenuItem(getString(holdTypes[i])))
						context.setHoldType((HoldNoteType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("fade_type"), context.selectionCanChangeFadeType()))
			{
				for (int i = 0; i < arrayLength(fadeTypes); ++i)
					if (ImGui::MenuItem(getString(fadeTypes[i])))
						context.setFadeType((FadeType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("guide_color"), context.selectionCanChangeFadeType()))
			{
				for (int i = 0; i < arrayLength(guideColors); ++i)
				{
					char str[32];
					sprintf_s(str, "guide_%s", guideColors[i]);
					if (ImGui::MenuItem(getString(str)))
						context.setGuideColor((GuideColor)i);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("layer"), context.selectedNotes.size() > 0))
			{
				for (int i = 0; i < context.score.layers.size(); ++i)
					if (ImGui::MenuItem(context.score.layers[i].name.c_str()))
						context.setLayer(i);
				ImGui::EndMenu();
			}

			ImGui::Separator();
			bool canShrink =
			    (context.selectedNotes.size() + context.selectedHiSpeedChanges.size()) >= 1;
			if (ImGui::MenuItem(getString("shrink_up"), NULL, false, canShrink))
				context.shrinkSelection(Direction::Up);

			if (ImGui::MenuItem(getString("shrink_down"), NULL, false, canShrink))
				context.shrinkSelection(Direction::Down);

			if (ImGui::MenuItem(getString("compress_selection"), NULL, false, canShrink))
				context.compressSelection();

			ImGui::Separator();
			if (ImGui::MenuItem(getString("connect_holds"), NULL, false,
			                    context.selectionCanConnect()))
				context.connectHoldsInSelection();

			if (ImGui::MenuItem(getString("split_hold"), NULL, false,
			                    context.selectionHasStep() && context.selectedNotes.size() == 1))
				context.splitHoldInSelection();

			int selectedMidNum = 0;
			int selectedStartNum = 0;
			for (const auto& noteId : context.selectedNotes)
			{
				auto noteType = context.score.notes.at(noteId).getType();
				if (noteType == NoteType::HoldMid)
					selectedMidNum += 1;
				if (noteType == NoteType::Hold)
					selectedStartNum += 1;
			}
			int selectedTickNum = selectedMidNum + selectedStartNum;

			if (ImGui::MenuItem(getString("repeat_hold_mids"), NULL, false,
			                    selectedTickNum >= 3 and selectedStartNum < 2))
				context.repeatMidsInSelection(context);

			if (ImGui::BeginMenu(getString("hold_to_traces"), context.selectionHasHold()))
			{
				if (ImGui::MenuItem(getString("add_traces_for_hold")))
					context.convertHoldToTraces(division, false);
				if (ImGui::MenuItem(getString("convert_hold_to_traces")))
					context.convertHoldToTraces(division, true);
				ImGui::EndMenu();
			}

			ImGui::Separator();
			if (ImGui::BeginMenu(getString("lerp_hispeeds"), 
				context.selectedHiSpeedChanges.size() >= 2))
			{
				for (int i = 0; i < arrayLength(easeTypes); ++i)
				{
					if (ImGui::MenuItem(getString(easeTypes[i])))
					{
						context.lerpHiSpeeds(division, (EaseType)i);
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}
	}

	void ScoreEditorTimeline::update(ScoreContext& context, EditArgs& edit, Renderer* renderer)
	{
		prevSize = size;
		prevPos = position;

		// Make space for the scrollbar and the status bar
		size = ImGui::GetContentRegionAvail() -
		       ImVec2{ ImGui::GetStyle().ScrollbarSize, UI::toolbarBtnSize.y };
		position = ImGui::GetCursorScreenPos();
		boundaries = ImRect(position, position + size);
		mouseInTimeline = ImGui::IsMouseHoveringRect(position, position + size);

		laneOffset = (size.x * 0.5f) - ((NUM_LANES * laneWidth) * 0.5f);
		minOffset = size.y - 50;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->PushClipRect(boundaries.Min, boundaries.Max, true);
		drawList->AddRectFilled(boundaries.Min, boundaries.Max, 0xff202020);

		if (background.isDirty())
		{
			background.resize({ size.x, size.y });
			background.process(renderer);
		}
		else if (prevSize.x != size.x || prevSize.y != size.y)
		{
			background.resize({ size.x, size.y });
		}

		if (config.drawBackground)
		{
			const float bgWidth = static_cast<float>(background.getWidth());
			const float bgHeight = static_cast<float>(background.getHeight());
			ImVec2 bgPos{ position.x - (abs(bgWidth - size.x) / 2.0f),
				          position.y - (abs(bgHeight - size.y) / 2.0f) };
			drawList->AddImage((ImTextureID)background.getTextureID(), bgPos,
			                   bgPos + ImVec2{ bgWidth, bgHeight });
		}

		// Remember whether the last mouse click was in the timeline or not
		static bool clickedOnTimeline = false;
		if (ImGui::IsMouseClicked(0))
			clickedOnTimeline = mouseInTimeline;

		const bool pasting = context.pasteData.pasting;
		const ImGuiIO& io = ImGui::GetIO();
		// Get mouse position relative to timeline
		mousePos = io.MousePos - position;
		mousePos.y -= offset;
		if (mouseInTimeline && !UI::isAnyPopupOpen())
		{
			if (io.KeyCtrl)
			{
				setZoom(zoom + (io.MouseWheel * 0.1f));
			}
			else
			{
				float scrollAmount = io.MouseWheel * scrollUnit;
				offset += scrollAmount *
				          (io.KeyShift ? config.scrollSpeedShift : config.scrollSpeedNormal);
			}

			if (!isHoveringNote && !isHoldingNote && !insertingHold && !pasting &&
			    currentMode == TimelineMode::Select)
			{
				// Clicked inside timeline, the current mouse position is the first drag point
				if (ImGui::IsMouseClicked(0))
				{
					dragStart = mousePos;
					if (!io.KeyCtrl && !io.KeyAlt &&
					    !ImGui::IsPopupOpen(IMGUI_TITLE(ICON_FA_MUSIC, "notes_timeline")))
					{
						context.selectedNotes.clear();
						context.selectedHiSpeedChanges.clear();
					}
				}

				// Clicked and dragging inside the timeline
				if (clickedOnTimeline && ImGui::IsMouseDown(0) &&
				    ImGui::IsMouseDragPastThreshold(0, 10.0f))
					dragging = true;
			}
		}

		offset = std::max(offset, minOffset);
		updateScrollingPosition();

		// Selection rectangle
		// Draw selection rectangle after notes are rendered
		if (dragging && ImGui::IsMouseReleased(0) && !pasting)
		{
			// Calculate drag selection
			float left = std::min(dragStart.x, mousePos.x);
			float right = std::max(dragStart.x, mousePos.x);
			float top = std::min(dragStart.y, mousePos.y);
			float bottom = std::max(dragStart.y, mousePos.y);

			if (!io.KeyAlt && !io.KeyCtrl)
			{
				context.selectedNotes.clear();
				context.selectedHiSpeedChanges.clear();
			}

			float yThreshold = (notesHeight * 0.5f) + 2.0f;
			for (const auto& [id, note] : context.score.notes)
			{
				const bool layerHidden = context.score.layers.at(note.layer).hidden;
				if ((layerHidden || note.layer != context.selectedLayer) && !context.showAllLayers)
					continue;
				float x1 = laneToPosition(note.lane);
				float x2 = laneToPosition(note.lane + note.width);
				float y = -tickToPosition(note.tick);

				if (right > x1 && left < x2 &&
				    isWithinRange(y, top - yThreshold, bottom + yThreshold))
				{
					if (io.KeyAlt)
						context.selectedNotes.erase(id);
					else
						context.selectedNotes.insert(id);
				}
			}
			for (const auto& [id, hsc] : context.score.hiSpeedChanges)
			{
				float lx =
				    laneToPosition(MAX_LANE + context.score.metadata.laneExtension + 1) + 123;
				float rx =
				    laneToPosition(MAX_LANE + context.score.metadata.laneExtension + 1) + 180;
				float y = -tickToPosition(hsc.tick);

				if (right > lx && left < rx &&
				    (hsc.layer == context.selectedLayer || context.showAllLayers) &&
				    isWithinRange(y, top - yThreshold / 2, bottom + yThreshold / 2))
				{
					if (io.KeyAlt)
						context.selectedHiSpeedChanges.erase(id);
					else
						context.selectedHiSpeedChanges.insert(id);
				}
			}

			dragging = false;
		}

		const float x1 = getTimelineStartX();
		const float x2 = getTimelineEndX();
		const float exX1 = getTimelineStartX(context.score);
		const float exX2 = getTimelineEndX(context.score);

		// Draw solid background color
		drawList->AddRectFilled(
		    { exX1, position.y }, { x1, position.y + size.y },
		    Color::abgrToInt(std::clamp((int)(config.laneOpacity * 255), 0, 255), 0x1c, 0x1a,
		                     0x0f));
		drawList->AddRectFilled(
		    { x1, position.y }, { x2, position.y + size.y },
		    Color::abgrToInt(std::clamp((int)(config.laneOpacity * 255), 0, 255), 0x1c, 0x1a,
		                     0x1f));
		drawList->AddRectFilled(
		    { x2, position.y }, { exX2, position.y + size.y },
		    Color::abgrToInt(std::clamp((int)(config.laneOpacity * 255), 0, 255), 0x1c, 0x1a,
		                     0x0f));

		if (config.drawWaveform)
			drawWaveform(context);

		// Draw lanes
		// mod 从minline开始画 画到MAX_LANE
		for (int l = MIN_LANE; l <= MAX_LANE; ++l)
		{
			const int x = position.x + laneToPosition(l);
			const bool boldLane = !(l & 1);
			drawList->AddLine(ImVec2(x, position.y), ImVec2(x, position.y + size.y),
			                  boldLane ? divColor1 : divColor2,
			                  boldLane ? primaryLineThickness : secondaryLineThickness);
		}

		// Draw measures
		int firstTick = std::max(0, positionToTick(visualOffset - size.y));
		int lastTick = positionToTick(visualOffset);
		int measure = accumulateMeasures(firstTick, TICKS_PER_BEAT, context.score.timeSignatures);
		firstTick = measureToTicks(measure, TICKS_PER_BEAT, context.score.timeSignatures);

		int tsIndex = findTimeSignature(measure, context.score.timeSignatures);
		int ticksPerMeasure =
		    beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;
		int beatTicks = ticksPerMeasure / context.score.timeSignatures[tsIndex].numerator;
		int subdivision = TICKS_PER_BEAT / (division / 4);

		// Snap to the sub-division before the current measure to prevent the lines from jumping
		// around
		for (int tick = firstTick - (firstTick % subdivision); tick <= lastTick;
		     tick += subdivision)
		{
			const int y = position.y - tickToPosition(tick) + visualOffset;
			int currentMeasure =
			    accumulateMeasures(tick, TICKS_PER_BEAT, context.score.timeSignatures);

			// Time signature changes on current measure
			if (context.score.timeSignatures.find(currentMeasure) !=
			        context.score.timeSignatures.end() &&
			    currentMeasure != tsIndex)
			{
				tsIndex = currentMeasure;
				ticksPerMeasure =
				    beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;
				beatTicks = ticksPerMeasure / context.score.timeSignatures[tsIndex].numerator;

				// snap to sub-division again on time signature change
				tick = measureToTicks(currentMeasure, TICKS_PER_BEAT, context.score.timeSignatures);
				tick -= tick % subdivision;
			}

			// determine whether the tick is a beat relative to its measure's tick
			int measureTicks =
			    measureToTicks(currentMeasure, TICKS_PER_BEAT, context.score.timeSignatures);

			ImU32 color;
			ImU32 exColor;
			float thickness;
			if (!((tick - measureTicks) % beatTicks))
			{
				color = measureColor;
				exColor = exMeasureColor;
				thickness = primaryLineThickness;
			}
			else if (division >= 192)
			{
				continue;
			}
			else
			{
				color = divColor2;
				exColor = exDivColor2;
				thickness = secondaryLineThickness;
			}

			drawList->AddLine(ImVec2(exX1, y), ImVec2(x1, y), exColor, thickness);
			drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), color, thickness);
			drawList->AddLine(ImVec2(x2, y), ImVec2(exX2, y), exColor, thickness);
		}

		tsIndex = findTimeSignature(measure, context.score.timeSignatures);
		ticksPerMeasure = beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;

		// Overdraw one measure to make sure the measure string is always visible
		for (int tick = firstTick; tick < lastTick + ticksPerMeasure; tick += ticksPerMeasure)
		{
			if (context.score.timeSignatures.find(measure) != context.score.timeSignatures.end())
			{
				tsIndex = measure;
				ticksPerMeasure =
				    beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;
			}

			std::string measureStr = std::to_string(measure);
			const float txtPos =
			    exX1 - MEASURE_WIDTH - (ImGui::CalcTextSize(measureStr.c_str()).x * 0.5f);
			const int y = position.y - tickToPosition(tick) + visualOffset;

			drawList->AddLine(ImVec2(exX1 - MEASURE_WIDTH, y), ImVec2(exX2 + MEASURE_WIDTH, y),
			                  measureColor, primaryLineThickness);
			drawShadedText(drawList, ImVec2(txtPos, y), 26, measureTxtColor, measureStr.c_str());

			++measure;
		}

		// draw lanes
		for (int l = -context.score.metadata.laneExtension;
		     l <= NUM_LANES + context.score.metadata.laneExtension; ++l)
		{
			const int x = position.x + laneToPosition(l);
			const bool boldLane = !(l & 1);
			const bool outOfBounds = l < 0 || l > NUM_LANES;
			drawList->AddLine(ImVec2(x, position.y), ImVec2(x, position.y + size.y),
			                  outOfBounds ? boldLane ? exDivColor1 : exDivColor2
			                  : boldLane  ? divColor1
			                              : divColor2,
			                  boldLane ? primaryLineThickness : secondaryLineThickness);
		}

		hoverTick = snapTickFromPos(-mousePos.y);
		hoverLane = positionToLane(mousePos.x);
		hoveringNote = -1;
		isHoveringNote = false;

		// Draw cursor behind notes
		const float y = position.y - tickToPosition(context.currentTick) + visualOffset;
		const int triPtOffset = 6;
		const int triXPos = exX1 - (triPtOffset * 2);
		drawList->AddTriangleFilled(ImVec2(triXPos, y - triPtOffset),
		                            ImVec2(triXPos, y + triPtOffset),
		                            ImVec2(triXPos + (triPtOffset * 2), y), cursorColor);
		drawList->AddLine(ImVec2(exX1, y), ImVec2(exX2, y), cursorColor,
		                  primaryLineThickness + 1.0f);

		contextMenu(context);

		// Update hi-speed changes
		for (auto& [id, hiSpeed] : context.score.hiSpeedChanges)
		{
			if (hiSpeedControl(context, hiSpeed))
			{
				eventEdit.editId = id;
				eventEdit.editHiSpeed = hiSpeed.speed;
				eventEdit.type = EventType::HiSpeed;
				ImGui::OpenPopup("edit_event");
			}
		}

		// Update time signature changes
		for (auto& [measure, ts] : context.score.timeSignatures)
		{
			if (timeSignatureControl(
			        context.score, ts.numerator, ts.denominator,
			        measureToTicks(ts.measure, TICKS_PER_BEAT, context.score.timeSignatures),
			        !playing))
			{
				eventEdit.editId = measure;
				eventEdit.editTimeSignatureNumerator = ts.numerator;
				eventEdit.editTimeSignatureDenominator = ts.denominator;
				eventEdit.type = EventType::TimeSignature;
				ImGui::OpenPopup("edit_event");
			}
		}

		// Update bpm changes
		for (int index = 0; index < context.score.tempoChanges.size(); ++index)
		{
			Tempo& tempo = context.score.tempoChanges[index];
			if (bpmControl(context.score, tempo))
			{
				eventEdit.editId = index;
				eventEdit.editBpm = tempo.bpm;
				eventEdit.type = EventType::Bpm;
				ImGui::OpenPopup("edit_event");
			}
		}

		// Update waypoints
		for (int index = 0; index < context.score.waypoints.size(); ++index)
		{
			Waypoint& wp = context.score.waypoints[index];
			if (waypointControl(context.score, wp))
			{
				eventEdit.editId = index;
				eventEdit.editName = wp.name;
				eventEdit.type = EventType::Waypoint;
				ImGui::OpenPopup("edit_event");
			}
		}

		// Update song boundaries
		if (context.audio.isMusicInitialized())
		{
			int startTick = accumulateTicks(context.workingData.musicOffset / 1000, TICKS_PER_BEAT,
			                                context.score.tempoChanges);
			int endTick = accumulateTicks(context.audio.getMusicEndTime(), TICKS_PER_BEAT,
			                              context.score.tempoChanges);

			float x = getTimelineEndX(context.score);
			float y1 = position.y - tickToPosition(startTick) + visualOffset;
			float y2 = position.y - tickToPosition(endTick) + visualOffset;

			drawList->AddTriangleFilled({ x, y1 }, { x + 10, y1 }, { x + 10, y1 - 10 }, 0xFFCCCCCC);
			drawList->AddTriangleFilled({ x, y2 }, { x + 10, y2 }, { x + 10, y2 + 10 }, 0xFFCCCCCC);
		}

		feverControl(context.score, context.score.fever);

		// Update skill triggers
		for (const auto& [_, skill] : context.score.skills)
			skillControl(context.score, skill);

		eventEditor(context);
		updateNotes(context, edit, renderer);

		// Update cursor tick after determining whether a note is hovered
		// The cursor tick should not change if a note is hovered
		if (ImGui::IsMouseClicked(0) && !isHoveringNote && mouseInTimeline && !playing &&
		    !pasting && !UI::isAnyPopupOpen() && currentMode == TimelineMode::Select &&
		    ImGui::IsWindowFocused())
		{
			context.currentTick = hoverTick;
			lastSelectedTick = context.currentTick;
		}

		// Selection boxes
		for (id_t id : context.selectedNotes)
		{
			const Note& note = context.score.notes.at(id);
			if (!isNoteVisible(note, 0))
				continue;

			float x = position.x;
			float y = position.y - tickToPosition(note.tick) + visualOffset;

			float lanePos = note.lane;

			//mod 对于bell 和 ten按键 绘制选择框时向左移动一行
			if (note.getType() == NoteType::Tap && !note.isFlick())
			{
				lanePos -= note.width / 2.0f;
			}
			else if (note.getType() == NoteType::Damage && note.damageType == DamageType::Circle)
			{
				lanePos -= note.width / 2.0f;
			}

			//mod end

			ImVec2 p1{ x + laneToPosition(lanePos) - 3, y - (notesHeight * 0.5f) };
			ImVec2 p2{ x + laneToPosition(lanePos + note.width) + 3, y + (notesHeight * 0.5f) };

			drawList->AddRectFilled(p1, p2, 0x20f4f4f4, 2.0f, ImDrawFlags_RoundCornersAll);
			drawList->AddRect(p1, p2, 0xcccccccc, 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
		}

		if (dragging && !pasting)
		{
			float startX = std::min(position.x + dragStart.x, position.x + mousePos.x);
			float endX = std::max(position.x + dragStart.x, position.x + mousePos.x);
			float startY =
			    std::min(position.y + dragStart.y, position.y + mousePos.y) + visualOffset;
			float endY = std::max(position.y + dragStart.y, position.y + mousePos.y) + visualOffset;
			ImVec2 start{ startX, startY };
			ImVec2 end{ endX, endY };

			drawList->AddRectFilled(start, end, selectionColor1);
			drawList->AddRect(start, end, 0xbbcccccc, 0.2f, ImDrawFlags_RoundCornersAll, 1.0f);

			ImVec2 iconPos = ImVec2(position + dragStart);
			iconPos.y += visualOffset;
			if (io.KeyCtrl)
			{
				drawList->AddText(ImGui::GetFont(), 12, iconPos, 0xdddddddd, ICON_FA_PLUS_CIRCLE);
			}
			else if (io.KeyAlt)
			{
				drawList->AddText(ImGui::GetFont(), 12, iconPos, 0xdddddddd, ICON_FA_MINUS_CIRCLE);
			}
		}

		drawList->PopClipRect();

		// Status bar: playback controls, division, zoom, current time and rhythm
		ImGui::SetCursorPos(
		    ImVec2{ ImGui::GetStyle().WindowPadding.x,
		            size.y + UI::toolbarBtnSize.y + 4 + ImGui::GetStyle().WindowPadding.y });
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });

		if (UI::transparentButton(ICON_FA_BACKWARD, UI::btnSmall, true,
		                          context.currentTick > 0 && !playing))
			previousTick(context);

		ImGui::SameLine();
		if (UI::transparentButton(ICON_FA_STOP, UI::btnSmall, false))
			stop(context);

		ImGui::SameLine();
		if (UI::transparentButton(playing ? ICON_FA_PAUSE : ICON_FA_PLAY, UI::btnSmall))
			setPlaying(context, !playing);

		ImGui::SameLine();
		if (UI::transparentButton(ICON_FA_FORWARD, UI::btnSmall, true, !playing))
			nextTick(context);

		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		UI::divisionSelect(getString("division"), division, divisions,
		                   sizeof(divisions) / sizeof(int));

		//mod 添加LaneSnap选项
		ImGui::SameLine();
		int laneSnapModeInt = (uint8_t)laneSnapMode;
		ImGui::SetNextItemWidth(125);
		if (UI::inlineSelect(getString("lane_snap_mode"), laneSnapModeInt, laneSnapModes,
			(size_t)LaneSnapMode::LaneSnapModeMax))
		{
			laneSnapMode = (LaneSnapMode)laneSnapModeInt;
		}

		ImGui::SameLine();
		int snapModeInt = (uint8_t)snapMode;
		ImGui::SetNextItemWidth(125);
		if (UI::inlineSelect(getString("snap_mode"), snapModeInt, snapModes,
		                     (size_t)SnapMode::SnapModeMax))
		{
			snapMode = (SnapMode)snapModeInt;
		}


		static int gotoMeasure = 0;
		bool activated = false;

		ImGui::SameLine();
		ImGui::SetNextItemWidth(50);
		ImGui::InputInt("##goto_measure", &gotoMeasure, 0, 0, ImGuiInputTextFlags_AutoSelectAll);
		activated |= ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false);
		UI::tooltip(getString("goto_measure"));

		ImGui::SameLine();
		activated |= UI::transparentButton(ICON_FA_ARROW_RIGHT, UI::btnSmall);

		if (activated)
		{
			gotoMeasure = std::max(gotoMeasure, 0);
			scrollTimeline(
			    context, measureToTicks(gotoMeasure, TICKS_PER_BEAT, context.score.timeSignatures));
		}

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		if (UI::transparentButton(ICON_FA_MINUS, UI::btnSmall, false,
		                          playbackSpeed > minPlaybackSpeed))
			setPlaybackSpeed(context, playbackSpeed - 0.25f);

		ImGui::SameLine();
		UI::transparentButton(IO::formatString("%.0f%%", playbackSpeed * 100).c_str(),
		                      ImVec2{ ImGui::CalcTextSize("0000%").x, UI::btnSmall.y }, false,
		                      false);

		ImGui::SameLine();
		if (UI::transparentButton(ICON_FA_PLUS, UI::btnSmall, false,
		                          playbackSpeed < maxPlaybackSpeed))
			setPlaybackSpeed(context, playbackSpeed + 0.25f);

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();

		int currentMeasure =
		    accumulateMeasures(context.currentTick, TICKS_PER_BEAT, context.score.timeSignatures);
		const TimeSignature& ts =
		    context.score
		        .timeSignatures[findTimeSignature(currentMeasure, context.score.timeSignatures)];
		const Tempo& tempo = getTempoAt(context.currentTick, context.score.tempoChanges);
		id_t hiSpeed = findHighSpeedChange(context.currentTick, context.score.hiSpeedChanges,
		                                  context.selectedLayer);
		float speed = (hiSpeed == -1 ? 1.0f : context.score.hiSpeedChanges[hiSpeed].speed);

		/*std::string rhythmString = IO::formatString(
		    "  %02d:%02d:%02d  |  %d/%d  |  %g BPM  |  %gx", (int)time / 60, (int)time % 60,
		    (int)((time - (int)time) * 100), ts.numerator, ts.denominator, tempo.bpm, speed);*/
		std::string rhythmString;
		if (config.showTickInProperties)
		{
			rhythmString = IO::formatString(
				"  %02d:%02d:%03d  |  %dt  |  %d/%d  |  %g BPM  |  %gx",
				(int)time / 60, (int)time % 60, (int)((time - (int)time) * 1000),
				context.currentTick, ts.numerator, ts.denominator, tempo.bpm, speed);
		}
		else
		{
			rhythmString = IO::formatString(
				"  %02d:%02d:%03d  |  %d/%d  |  %g BPM  |  %gx",
				(int)time / 60, (int)time % 60, (int)((time - (int)time) * 1000),
				ts.numerator, ts.denominator, tempo.bpm, speed);
		}

		float _zoom = zoom;
		int controlWidth = ImGui::GetContentRegionAvail().x -
		                   ImGui::CalcTextSize(rhythmString.c_str()).x - (UI::btnSmall.x * 3);
		if (UI::zoomControl("zoom", _zoom, minZoom, 10, std::clamp(controlWidth, 120, 320)))
			setZoom(_zoom);

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		ImGui::Text("%s", rhythmString.c_str());

		updateScrollbar();

		updateNoteSE(context);

		timeLastFrame = time;
		if (playing)
		{
			time += ImGui::GetIO().DeltaTime * playbackSpeed;
			context.currentTick = accumulateTicks(time, TICKS_PER_BEAT, context.score.tempoChanges);

			float cursorY = tickToPosition(context.currentTick);
			if (config.followCursorInPlayback)
			{
				float timelineOffset = size.y * (1.0f - config.cursorPositionThreshold);
				if (cursorY >= offset - timelineOffset)
					visualOffset = offset = cursorY + timelineOffset;
			}
			else if (cursorY > offset)
			{
				visualOffset = offset = cursorY + size.y;
			}
		}
		else
		{
			time =
			    accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges);
		}
	}

	void ScoreEditorTimeline::updateNotes(ScoreContext& context, EditArgs& edit, Renderer* renderer)
	{
		// directxmath dies
		if (size.y < 10 || size.x < 10)
			return;

		Shader* shader = ResourceManager::shaders[0];
		shader->use();
		shader->setMatrix4("projection", camera.getOffCenterOrthographicProjection(
		                                     0, size.x, position.y, position.y + size.y));

		glEnable(GL_FRAMEBUFFER_SRGB);
		framebuffer->bind();
		framebuffer->clear();
		renderer->beginBatch();

		minNoteYDistance = INT_MAX;
		for (auto& [id, note] : context.score.notes)
		{
			const bool layerHidden = context.score.layers.at(note.layer).hidden;
			if (!isNoteVisible(note) || (layerHidden && !context.showAllLayers))
				continue;

			if (note.getType() == NoteType::Tap)
			{
				updateNote(context, edit, note);
				drawNote(note, renderer,
				         (context.showAllLayers || note.layer == context.selectedLayer)
				             ? noteTint
				             : otherLayerTint,
				         0, 0, context.showAllLayers || note.layer == context.selectedLayer);
			}
			if (note.getType() == NoteType::Damage)
			{
				updateNote(context, edit, note);
				drawCcNote(note, renderer,
				           (context.showAllLayers || note.layer == context.selectedLayer)
				               ? noteTint
				               : otherLayerTint,
				           0, 0, context.showAllLayers || note.layer == context.selectedLayer);
			}
		}

		for (auto& [id, hold] : context.score.holdNotes)
		{
			Note& start = context.score.notes.at(hold.start.ID);
			Note& end = context.score.notes.at(hold.end);

			const bool startLayerHidden = context.score.layers.at(start.layer).hidden;
			const bool endLayerHidden = context.score.layers.at(end.layer).hidden;
			if ((startLayerHidden || endLayerHidden) && !context.showAllLayers)
				continue;

			if (isNoteVisible(start))
				updateNote(context, edit, start);
			if (isNoteVisible(end))
				updateNote(context, edit, end);

			for (const auto& step : hold.steps)
			{
				Note& mid = context.score.notes.at(step.ID);
				if (isNoteVisible(mid))
					updateNote(context, edit, mid);
				if (skipUpdateAfterSortingSteps)
					break;
			}

			drawHoldNote(context.score.notes, hold, renderer, noteTint,
			             context.showAllLayers ? -1 : context.selectedLayer);
		}
		skipUpdateAfterSortingSteps = false;

		renderer->endBatch();
		renderer->beginBatch();

		const bool pasting = context.pasteData.pasting;
		if (pasting && mouseInTimeline && !playing)
		{
			context.pasteData.offsetTicks = hoverTick;
			context.pasteData.offsetLane = hoverLane;
			previewPaste(context, renderer);
			if (ImGui::IsMouseClicked(0))
				context.confirmPaste();
			else if (ImGui::IsMouseClicked(1))
				context.cancelPaste();
		}

		if (mouseInTimeline && !isHoldingNote && currentMode != TimelineMode::Select && !pasting &&
		    !playing && !UI::isAnyPopupOpen())
		{
			previewInput(context, edit, renderer);
			if (ImGui::IsMouseClicked(0) && hoverTick >= 0 && !isHoveringNote)
				executeInput(context, edit);

			if (insertingHold && !ImGui::IsMouseDown(0))
			{
				insertHold(context, edit);
				insertingHold = false;
			}
		}
		else
		{
			insertingHold = false;
		}

		renderer->endBatch();

		glDisable(GL_FRAMEBUFFER_SRGB);
		glDisable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddImage((void*)framebuffer->getTexture(), position, position + size);

		// draw hold step outlines
		for (const auto& data : drawSteps)
		{
			const bool layerHidden = context.score.layers.at(data.layer).hidden;
			if (layerHidden && !context.showAllLayers)
				continue;

			drawOutline(data, context.showAllLayers ? -1 : context.selectedLayer);
		}

		drawSteps.clear();
	}

	void ScoreEditorTimeline::previewPaste(ScoreContext& context, Renderer* renderer)
	{
		context.pasteData.offsetLane =
		    std::clamp(hoverLane - context.pasteData.midLane, (float)context.pasteData.minLaneOffset,
				(float)context.pasteData.maxLaneOffset);

		for (const auto notes : { context.pasteData.notes, context.pasteData.damages })
			for (const auto& [_, note] : notes)
				if (isNoteVisible(note, hoverTick))
				{
					if (note.getType() == NoteType::Tap)
						drawNote(note, renderer, hoverTint, hoverTick, context.pasteData.offsetLane,
						         context.showAllLayers || note.layer == context.selectedLayer);
					else if (note.getType() == NoteType::Damage)
						drawCcNote(note, renderer, hoverTint, hoverTick,
						           context.pasteData.offsetLane,
						           context.showAllLayers || note.layer == context.selectedLayer);
				}

		for (const auto& [_, hold] : context.pasteData.holds)
			drawHoldNote(context.pasteData.notes, hold, renderer, hoverTint, -1, hoverTick,
			             context.pasteData.offsetLane);

		for (const auto& [_, hsc] : context.pasteData.hiSpeedChanges)
			hiSpeedControl(context, hsc.tick + hoverTick, hsc.speed, -1);
	}

	/// <summary>
	/// 切换当前所选要插入的按键时调用
	/// 对应option窗口的设置
	/// </summary>
	/// <param name="score"></param>
	/// <param name="edit"></param>
	void ScoreEditorTimeline::updateInputNotes(const Score& score, EditArgs& edit)
	{
		//mod 如果为无轨模式则不取整
		float lane = laneFromCenterPosition(score, hoverLane, edit.noteWidth);
		if (laneSnapMode == LaneSnapMode::Relative)
		{
			lane = std::floorf(lane);
		}

		//int lane = laneFromCenterPosition(score, hoverLane, edit.noteWidth);

		int width = edit.noteWidth;
		int tick = hoverTick;
		inputNotes.tap.lane = lane + 1;
		//mod 直接限制死普通tap宽度为1
		inputNotes.tap.width = 1;
		inputNotes.tap.tick = tick;
		inputNotes.tap.flick =
		    currentMode == TimelineMode::InsertFlick ? edit.flickType : FlickType::None;
		inputNotes.tap.critical = currentMode == TimelineMode::MakeCritical;
		inputNotes.tap.friction = currentMode == TimelineMode::MakeFriction;
		//mod 直接在切换timelineMode的时候附加上值 如果是flick就允许改宽度
		if (currentMode != TimelineMode::InsertFlick)
		{
			inputNotes.tap.resizeAble = false;
		}
		else
		{
			inputNotes.tap.lane = lane;
			inputNotes.tap.resizeAble = true;
			inputNotes.tap.width = 3;
		}
		inputNotes.holdStep.lane = lane;
		inputNotes.holdStep.width = width;
		inputNotes.holdStep.tick = tick;

		if (insertingHold)
		{
			inputNotes.holdEnd.lane = lane;
			inputNotes.holdEnd.width = width;
			inputNotes.holdEnd.tick = tick;
		}
		else
		{
			inputNotes.holdStart.lane = lane;
			inputNotes.holdStart.width = width;
			inputNotes.holdStart.tick = tick;
		}

		if (inputNotes.damage.damageType == DamageType::Circle)
			inputNotes.damage.resizeAble = false;
		else
			inputNotes.damage.resizeAble = true;

		inputNotes.damage.lane = lane + 1;
		inputNotes.damage.width = 1;
		inputNotes.damage.tick = tick;
	}

	void ScoreEditorTimeline::insertEvent(ScoreContext& context, EditArgs& edit)
	{
		if (currentMode == TimelineMode::InsertBPM)
		{
			for (const auto& tempo : context.score.tempoChanges)
				if (tempo.tick == hoverTick)
					return;

			Score prev = context.score;
			context.score.tempoChanges.push_back({ hoverTick, edit.bpm });
			std::sort(context.score.tempoChanges.begin(), context.score.tempoChanges.end(),
			          [](const auto& a, const auto& b) { return a.tick < b.tick; });
			context.pushHistory("Insert BPM change", prev, context.score);
		}
		else if (currentMode == TimelineMode::InsertTimeSign)
		{
			int measure =
			    accumulateMeasures(hoverTick, TICKS_PER_BEAT, context.score.timeSignatures);
			if (context.score.timeSignatures.find(measure) != context.score.timeSignatures.end())
				return;

			Score prev = context.score;
			context.score.timeSignatures[measure] = { measure, edit.timeSignatureNumerator,
				                                      edit.timeSignatureDenominator };
			context.pushHistory("Insert time signature", prev, context.score);
		}
		else if (currentMode == TimelineMode::InsertHiSpeed)
		{
			for (const auto& [_, hs] : context.score.hiSpeedChanges)
				if (hs.tick == hoverTick && hs.layer == context.selectedLayer)
					return;

			Score prev = context.score;
			id_t id = getNextHiSpeedID();
			context.score.hiSpeedChanges[id] = { id, hoverTick, edit.hiSpeed,
				                                 context.selectedLayer };
			context.pushHistory("Insert hi-speed changes", prev, context.score);
		}
	}

	void ScoreEditorTimeline::previewInput(const ScoreContext& context, EditArgs& edit,
	                                       Renderer* renderer)
	{
		updateInputNotes(context.score, edit);
		switch (currentMode)
		{
		case TimelineMode::InsertLong:
			if (insertingHold)
			{
				drawHoldCurve(inputNotes.holdStart, inputNotes.holdEnd, edit.easeType, false,
				              renderer, hoverTint);
				//去掉首尾note
				//drawNote(inputNotes.holdStart, renderer, hoverTint);
				//drawNote(inputNotes.holdEnd, renderer, hoverTint);
			}
			else
			{
				drawNote(inputNotes.holdStart, renderer, hoverTint);
			}
			break;

		case TimelineMode::InsertLongMid:
			drawHoldMid(inputNotes.holdStep, edit.stepType, renderer, hoverTint);
			drawOutline(StepDrawData(inputNotes.holdStep, edit.stepType == HoldStepType::Skip
			                                                  ? StepDrawType::SkipStep
			                                                  : StepDrawType::NormalStep));
			break;

		case TimelineMode::InsertGuide:
		{
			StepDrawType color = static_cast<StepDrawType>(
			    static_cast<int>(StepDrawType::GuideNeutral) + static_cast<int>(edit.colorType));
			if (insertingHold)
			{
				float a1, a2;
				switch (edit.fadeType)
				{
				case FadeType::Out:
					a1 = 1.0f;
					a2 = 0.0f;
					break;
				case FadeType::In:
					a1 = 0.0f;
					a2 = 1.0f;
					break;
				default:
					a1 = 1.0f;
					a2 = 1.0f;
				}
				if (inputNotes.holdStart.tick > inputNotes.holdEnd.tick)
					std::swap(a1, a2);
				drawHoldCurve(inputNotes.holdStart, inputNotes.holdEnd, EaseType::Linear, true,
				              renderer, noteTint, 1, 0, a1, a2, edit.colorType);
				//drawOutline(StepDrawData(inputNotes.holdStart, color));
				//drawOutline(StepDrawData(inputNotes.holdEnd, color));
			}
			else
			{
				drawOutline(StepDrawData(inputNotes.holdStart, color));
			}
		}
		break;

		case TimelineMode::InsertBPM:
			bpmControl(context.score, edit.bpm, hoverTick, false);
			break;

		case TimelineMode::InsertTimeSign:
			timeSignatureControl(context.score, edit.timeSignatureNumerator,
			                     edit.timeSignatureDenominator, hoverTick, false);
			break;

		case TimelineMode::InsertHiSpeed:
			hiSpeedControl(context, hoverTick, edit.hiSpeed, -1);
			break;

		case TimelineMode::InsertDamage:
			drawCcNote(inputNotes.damage, renderer, hoverTint);
			break;

		default:
			drawNote(inputNotes.tap, renderer, hoverTint);
			break;
		}
	}

	void ScoreEditorTimeline::executeInput(ScoreContext& context, EditArgs& edit)
	{
		context.showAllLayers = false;
		switch (currentMode)
		{
		case TimelineMode::InsertTap:
		case TimelineMode::MakeCritical:
		case TimelineMode::MakeFriction:
		case TimelineMode::InsertFlick:
			insertNote(context, edit);
			break;

		case TimelineMode::InsertLong:
		case TimelineMode::InsertGuide:
			insertingHold = true;
			break;

		case TimelineMode::InsertLongMid:
		{
			int id = findClosestHold(context, hoverLane, hoverTick);
			if (id != -1)
				insertHoldStep(context, edit, id);
		}
		break;

		case TimelineMode::InsertBPM:
		case TimelineMode::InsertTimeSign:
		case TimelineMode::InsertHiSpeed:
			insertEvent(context, edit);
			break;

		case TimelineMode::InsertDamage:
			insertDamage(context, edit);
			break;

		default:
			break;
		}
	}

	void ScoreEditorTimeline::changeMode(TimelineMode mode, EditArgs& edit)
	{
		if (currentMode == mode)
		{
			if (mode == TimelineMode::InsertLongMid)
			{
				edit.stepType =
				    (HoldStepType)(((int)edit.stepType + 1) % (int)HoldStepType::HoldStepTypeCount);
			}
			else if (mode == TimelineMode::InsertFlick)
			{
				edit.flickType =
				    (FlickType)(((int)edit.flickType + 1) % (int)FlickType::FlickTypeCount);
				if (!(int)edit.flickType)
					edit.flickType = FlickType::Default;
			}
			else if (mode == TimelineMode::InsertGuide)
			{
				edit.colorType =
				    (GuideColor)(((int)edit.colorType + 1) % (int)GuideColor::GuideColorCount);
			}
		}

		currentMode = mode;
	}

	int ScoreEditorTimeline::findClosestHold(ScoreContext& context, int lane, int tick)
	{
		float xt = laneToPosition(lane);
		float yt = getNoteYPosFromTick(tick);

		for (const auto& [id, hold] : context.score.holdNotes)
		{
			const Note& start = context.score.notes.at(hold.start.ID);
			const Note& end = context.score.notes.at(hold.end);

			// No need to search holds outside the cursor's reach
			if (start.tick > tick || end.tick < tick)
				continue;

			// Do not take skip steps into account
			int s1{ -1 }, s2{ -1 };
			while (++s2 < hold.steps.size())
			{
				if (hold.steps[s2].type != HoldStepType::Skip)
					break;
			}

			if (isArrayIndexInBounds(s2, hold.steps))
			{
				// Getting here means we found a non-skip step
				if ((context.showAllLayers || start.layer == context.selectedLayer ||
				     context.score.notes.at(hold.steps[s2].ID).layer == context.selectedLayer) &&
				    isMouseInHoldPath(start, context.score.notes.at(hold.steps[s2].ID),
				                      hold.start.ease, xt, yt))
					return id;

				s1 = s2;
				while (++s2 < hold.steps.size())
				{
					if (hold.steps[s2].type != HoldStepType::Skip)
					{
						const Note& m1 = context.score.notes.at(hold.steps[s1].ID);
						const Note& m2 = context.score.notes.at(hold.steps[s2].ID);
						if ((context.showAllLayers || m1.layer == context.selectedLayer ||
						     m2.layer == context.selectedLayer) &&
						    isMouseInHoldPath(m1, m2, hold.steps[s1].ease, xt, yt))
							return id;

						s1 = s2;
					}
				}

				id_t nextId = hold.steps[s1].ID;
				if ((context.showAllLayers || start.layer == context.selectedLayer ||
				     context.score.notes.at(nextId).layer == context.selectedLayer) &&
				    isMouseInHoldPath(context.score.notes.at(nextId), end, hold.steps[s1].ease, xt,
				                      yt))
					return id;
			}
			else
			{
				// Hold consists of only skip steps or no steps at all

				if ((context.showAllLayers || start.layer == context.selectedLayer ||
				     end.layer == context.selectedLayer) &&
				    isMouseInHoldPath(start, end, hold.start.ease, xt, yt))
					return id;
			}
		}

		return -1;
	}

	bool ScoreEditorTimeline::noteControl(ScoreContext& context, const Note& note,
	                                      const ImVec2& pos, const ImVec2& sz, const char* id,
	                                      ImGuiMouseCursor cursor)
	{
		float minLane = MIN_LANE - context.score.metadata.laneExtension;
		float maxLane = MAX_LANE + context.score.metadata.laneExtension;
		// Do not process notes if the cursor is outside of the timeline
		// This fixes ui buttons conflicting with note "buttons"
		if (!mouseInTimeline && !isHoldingNote)
			return false;

		// Do not allow editing notes during playback
		if (playing)
			return false;

		ImGui::SetCursorScreenPos(pos);
		ImGui::InvisibleButton(id, sz);
		if (mouseInTimeline && ImGui::IsItemHovered() && !dragging)
			ImGui::SetMouseCursor(cursor);

		// Note clicked
		if (ImGui::IsItemActivated())
		{
			prevUpdateScore = context.score;
			ctrlMousePos = mousePos;
			holdLane = hoverLane;
			holdTick = hoverTick;
		}

		// Holding note
		if (ImGui::IsItemActive())
		{
			ImGui::SetMouseCursor(cursor);
			isHoldingNote = true;
			return true;
		}

		// Note released
		if (ImGui::IsItemDeactivated())
		{
			bool noChange = false;
			auto it = context.score.notes.find(holdingNote);
			if (it != context.score.notes.end())
				noChange = noteTransformOrigin.isSame(it->second);

			isHoldingNote = false;
			holdingNote = 0;

			if (!noChange)
			{
				std::unordered_set<int> sortHolds = context.getHoldsFromSelection();
				for (int id : sortHolds)
				{
					HoldNote& hold = context.score.holdNotes.at(id);
					Note& start = context.score.notes.at(id);
					Note& end = context.score.notes.at(hold.end);

					if (start.tick > end.tick)
					{
						std::swap(start.tick, end.tick);
						std::swap(start.lane, end.lane);
						std::swap(start.width, end.width);
					}

					if (hold.steps.size())
					{
						sortHoldSteps(context.score, hold);

						// Ensure hold steps are between the start and end
						Note& firstMid = context.score.notes.at(hold.steps[0].ID);
						if (start.tick > firstMid.tick)
						{
							std::swap(start.tick, firstMid.tick);
							std::swap(start.lane, firstMid.lane);
							start.lane = std::clamp(start.lane, minLane, maxLane - start.width + 1);
							firstMid.lane =
							    std::clamp(firstMid.lane, minLane, maxLane - firstMid.width + 1);
						}

						Note& lastMid =
						    context.score.notes.at(hold.steps[hold.steps.size() - 1].ID);
						if (end.tick < lastMid.tick)
						{
							std::swap(end.tick, lastMid.tick);
							std::swap(end.lane, lastMid.lane);
							lastMid.lane =
							    std::clamp(lastMid.lane, minLane, maxLane - lastMid.width + 1);
							end.lane = std::clamp(end.lane, minLane, maxLane - end.width + 1);
						}
					}

					sortHoldSteps(context.score, hold);
					skipUpdateAfterSortingSteps = true;
				}

				context.pushHistory("Update notes", prevUpdateScore, context.score);
			}
		}

		return false;
	}

	void ScoreEditorTimeline::updateNote(ScoreContext& context, EditArgs& edit, Note& note)
	{
		if (!(context.showAllLayers || context.selectedLayer == note.layer))
			return;
		const float minLane = MIN_LANE - context.score.metadata.laneExtension;
		const float maxLane = MAX_LANE + context.score.metadata.laneExtension;
		const float maxNoteWidth = MAX_NOTE_WIDTH + context.score.metadata.laneExtension * 2;

		// mod 保证tap框拖拽正确
		float lanePos = note.lane;
		if (note.getType() == NoteType::Tap && !note.isFlick())
		{
			lanePos -= note.width / 2.0f;
		}
		//圆形弹幕同理
		else if (note.getType() == NoteType::Damage && note.damageType == DamageType::Circle)
		{
			lanePos -= note.width / 2.0f;
		}

		const float btnPosY =
			position.y - tickToPosition(note.tick) + visualOffset - (notesHeight * 0.5f);
		float btnPosX = laneToPosition(lanePos) + position.x - 2.0f;

		ImVec2 pos{ btnPosX, btnPosY };
		ImVec2 noteSz{ laneToPosition(lanePos + note.width) + position.x + 2.0f - btnPosX,
			           notesHeight };
		ImVec2 sz{ noteControlWidth, notesHeight };

		//mod end

		const ImGuiIO& io = ImGui::GetIO();
		if (ImGui::IsMouseHoveringRect(pos, pos + noteSz, false) && mouseInTimeline)
		{
			isHoveringNote = true;

			float noteYDistance =
			    std::abs((btnPosY + notesHeight / 2 - visualOffset - position.y) - mousePos.y);
			if (noteYDistance < minNoteYDistance || io.KeyCtrl)
			{
				minNoteYDistance = noteYDistance;
				hoveringNote = note.ID;
				if (ImGui::IsMouseClicked(0) && !UI::isAnyPopupOpen())
				{
					if (!io.KeyCtrl && !io.KeyAlt && !context.isNoteSelected(note))
					{
						context.selectedNotes.clear();
						context.selectedHiSpeedChanges.clear();
					}

					context.selectedNotes.insert(note.ID);

					if (io.KeyAlt && context.isNoteSelected(note))
						context.selectedNotes.erase(note.ID);

					if (context.isNoteSelected(note))
					{
						holdingNote = note.ID;
						noteTransformOrigin = NoteTransform::fromNote(note);
					}
				}
			}
		}

		// Left resize
		ImGui::PushID(note.ID);
		if (noteControl(context, note, pos, sz, "L", ImGuiMouseCursor_ResizeEW))
		{
			int curLane = positionToLane(mousePos.x);
			int grabLane = std::clamp(positionToLane(ctrlMousePos.x), minLane, maxLane);
			int diff = curLane - grabLane;

			//mod 完全基于鼠标移动的diff
			float sizediff = positionToLane(mousePos.x) - positionToLane(ctrlMousePos.x);

			if (abs(sizediff) > 0)
			{
				bool canResize = !std::any_of(
				    context.selectedNotes.begin(), context.selectedNotes.end(),
				    [&context, diff, minLane, maxLane, maxNoteWidth](int id)
				    {
					    Note& n = context.score.notes.at(id);

						//--------Mod 如果按键被设为不能resize就直接false
						if (!n.resizeAble) return true;

					    int newLane = n.lane + diff;
					    int newWidth = n.width - diff;
					    return (newLane < minLane || newLane + newWidth - 1 > maxLane ||
					            !isWithinRange(newWidth, MIN_NOTE_WIDTH, maxNoteWidth));
				    });

				if (canResize)
				{
					ctrlMousePos.x = mousePos.x;
					for (id_t id : context.selectedNotes)
					{
						Note& n = context.score.notes.at(id);

						// mod snap或者无轨模式
						if (laneSnapMode == LaneSnapMode::Relative)
						{
							n.width = std::clamp(n.width - diff, (float)MIN_NOTE_WIDTH, maxNoteWidth);
							n.lane = std::clamp(n.lane + diff, minLane, maxLane - n.width + 1);
						}
						else
						{
							n.width = std::fmax(0, n.width - sizediff);
							n.lane = n.lane + sizediff;
						}
					}
				}
			}
		}

		pos.x += noteControlWidth;
		// account for <1 width by always having this be positive
		sz.x = std::max((laneWidth * note.width) + 4.0f - (noteControlWidth * 2.0f),
		                (noteControlWidth * 2.0f));

		// Move
		if (noteControl(context, note, pos, sz, "M", ImGuiMouseCursor_Hand))
		{
			float curLane = truncf(positionToLane(mousePos.x));
			float grabLane = truncf(std::clamp(positionToLane(ctrlMousePos.x), minLane, maxLane));
			int grabTick = snapTickFromPos(-ctrlMousePos.y);

			int laneDiff = curLane - grabLane;
			//mod 完全基于鼠标移动的diff
			float posDiff = positionToLane(mousePos.x) - positionToLane(ctrlMousePos.x);
			// Move X
			// 改为基于posDiff判断
			if (abs(posDiff) > 0)
			{
				isMovingNote = true;
				ctrlMousePos.x = mousePos.x;
				bool canMove =
				    !std::any_of(context.selectedNotes.begin(), context.selectedNotes.end(),
				                 [&context, laneDiff, minLane, maxLane](int id)
				                 {
					                 Note& n = context.score.notes.at(id);
					                 int newLane = n.lane + laneDiff;
					                 return (newLane < minLane || newLane + n.width - 1 > maxLane);
				                 });

				if (canMove)
				{
					for (id_t id : context.selectedNotes)
					{
						Note& n = context.score.notes.at(id);

						//根据是否snap移动
						if (laneSnapMode == LaneSnapMode::Relative)	n.lane = std::clamp(n.lane + laneDiff, minLane, maxLane - n.width + 1);
						else n.lane = n.lane + posDiff;
					}
				}
			}

			//Move Y
			int tickDiff = hoverTick - grabTick;
			if (abs(tickDiff) > 0)
			{
				isMovingNote = true;
				ctrlMousePos.y = mousePos.y;

				bool canMove =
				    !std::any_of(context.selectedNotes.begin(), context.selectedNotes.end(),
				                 [&context, tickDiff](int id)
				                 { return context.score.notes.at(id).tick + tickDiff < 0; });

				if (canMove)
				{
					switch (snapMode)
					{
					case SnapMode::Relative:
					{
						for (id_t id : context.selectedNotes)
						{
							Note& n = context.score.notes.at(id);
							n.tick = std::max(n.tick + tickDiff, 0);
						}
						break;
					}
					case SnapMode::Absolute:
					{
						int grabbingNoteTick = note.tick;
						int grabbingNoteTickSnapped =
						    roundTickDown(grabbingNoteTick + tickDiff, division);
						int actualDiff = grabbingNoteTickSnapped - grabbingNoteTick;

						for (id_t id : context.selectedNotes)
						{
							Note& n = context.score.notes.at(id);
							n.tick = std::max(n.tick + actualDiff, 0);
						}

						break;
					}
					case SnapMode::IndividualAbsolute:
					{
						std::vector<id_t> sortedSelectedNotes(context.selectedNotes.size());
						std::copy(context.selectedNotes.begin(), context.selectedNotes.end(),
						          sortedSelectedNotes.begin());
						std::sort(sortedSelectedNotes.begin(), sortedSelectedNotes.end(),
						          [&context](id_t a, id_t b) {
							          return context.score.notes.at(a).tick <
							                 context.score.notes.at(b).tick;
						          });

						for (int id : sortedSelectedNotes)
						{
							Note& n = context.score.notes.at(id);
							auto shiftedTick = n.tick + tickDiff;
							n.tick = std::max(roundTickDown(shiftedTick, division), 0);
						}

						break;
					}
					default:
						throw std::runtime_error("Invalid snap mode (Unreachable)");
					}
				}
			}
		}

		// Per note options here
		if (ImGui::IsItemDeactivated())
		{
			if (!isMovingNote && !context.selectedNotes.empty())
			{
				switch (currentMode)
				{
				case TimelineMode::InsertFlick:
					context.setFlick(FlickType::FlickTypeCount);
					break;

				case TimelineMode::MakeCritical:
					context.toggleCriticals();
					break;

				case TimelineMode::InsertLong:
					context.setEase(EaseType::EaseTypeCount);
					break;

				case TimelineMode::InsertLongMid:
					context.setStep(HoldStepType::HoldStepTypeCount);
					break;

				case TimelineMode::InsertGuide:
					context.setGuideColor(GuideColor::GuideColorCount);
					break;

				case TimelineMode::MakeFriction:
					context.toggleFriction();
					break;

				default:
					break;
				}
			}

			isMovingNote = false;
		}

		pos.x += sz.x;
		sz.x = noteControlWidth;

		// Right resize
		if (noteControl(context, note, pos, sz, "R", ImGuiMouseCursor_ResizeEW))
		{
			int grabLane = std::clamp(positionToLane(ctrlMousePos.x), minLane, maxLane);
			int curLane = positionToLane(mousePos.x);

			int diff = curLane - grabLane;
			//mod 同上
			float sizediff = positionToLane(mousePos.x) - positionToLane(ctrlMousePos.x);
			if (abs(sizediff) > 0)
			{
				bool canResize = !std::any_of(
				    context.selectedNotes.begin(), context.selectedNotes.end(),
				    [&context, diff, maxLane](int id)
				    {
					    Note& n = context.score.notes.at(id);

						//--------Mod 如果按键被设为不能resize就直接true（有取反）
						if (!n.resizeAble) return true;

					    int newWidth = n.width + diff;
					    return (newWidth < MIN_NOTE_WIDTH || n.lane + newWidth - 1 > maxLane);
				    });

				if (canResize)
				{
					ctrlMousePos.x = mousePos.x;
					for (id_t id : context.selectedNotes)
					{
						Note& n = context.score.notes.at(id);

						//mod 同上
						if (laneSnapMode == LaneSnapMode::Relative)
						{
							n.width = std::clamp(n.width + diff, (float)MIN_NOTE_WIDTH,
								maxNoteWidth - n.lane);
						}
						else
						{
							n.width = std::fmax(0, n.width + sizediff);
						}
					}
				}
			}
		}

		ImGui::PopID();
	}

	//mod 添加holdeventtype以便根据HoldType绘制
	void ScoreEditorTimeline::drawHoldCurve(const Note& n1, const Note& n2, EaseType ease,
	                                        bool isGuide, Renderer* renderer, const Color& tint_,
	                                        const int offsetTick, const int offsetLane,
	                                        const float startAlpha, const float endAlpha,
	                                        const GuideColor guideColor, const int selectedLayer, 
											const HoldEventType holdEventType, const std::string guideColorInHex)
	{
		//int texIndex{ noteTextures.holdPath };
		int texIndex{ noteTextures.guideColors };
		ZIndex zIndex{ ZIndex::HoldLine };
		if (isGuide)
		{
			zIndex = ZIndex::Guide;
			texIndex = noteTextures.guideColors;
		}

		if (texIndex == -1)
			return;

		auto tint = tint_;
		const Texture& pathTex = ResourceManager::textures[texIndex];
		//const int sprIndex = isGuide ? static_cast<int>(guideColor) : n1.critical ? 3 : 1;
		//mod
		int sprIndex = 1;
		if (isGuide) sprIndex = 8;
		else if (holdEventType == HoldEventType::Event_Laser) sprIndex = 5;
		else if (holdEventType == HoldEventType::Event_Warning) sprIndex = 4;
		else sprIndex = 0;

		if (!isArrayIndexInBounds(sprIndex, pathTex.sprites))
			return;

		const Sprite& spr = pathTex.sprites[sprIndex];

		float startX1 = laneToPosition(n1.lane + offsetLane);
		float startX2 = laneToPosition(n1.lane + n1.width + offsetLane);
		float startY = getNoteYPosFromTick(n1.tick + offsetTick);

		float endX1 = laneToPosition(n2.lane + offsetLane);
		float endX2 = laneToPosition(n2.lane + n2.width + offsetLane);
		float endY = getNoteYPosFromTick(n2.tick + offsetTick);

		int left = spr.getX() + holdCutoffX;
		int right = spr.getX() + spr.getWidth() - holdCutoffX;

		auto easeFunc = getEaseFunction(ease);
		float steps = std::max(5.0f, std::ceilf(abs((endY - startY)) / 10));
		for (int y = 0; y < steps; ++y)
		{
			Color inactiveTint = tint * otherLayerTint;
			const float percent1 = y / steps;
			const float percent2 = (y + 1) / steps;

			float xl1 = easeFunc(startX1, endX1, percent1) - 2;
			float xr1 = easeFunc(startX2, endX2, percent1) + 2;
			float y1 = lerp(startY, endY, percent1);
			float y2 = lerp(startY, endY, percent2);
			float xl2 = easeFunc(startX1, endX1, percent2) - 2;
			float xr2 = easeFunc(startX2, endX2, percent2) + 2;

			int z = (selectedLayer == -1 || ((y < steps / 2) ? n1 : n2).layer == selectedLayer)
			            ? (int)ZIndex::zCount
			            : 0;

			if (y2 <= 0)
				continue;

			// rest of hold no longer visible
			if (y1 > size.y + size.y + position.y + 100)
				break;

			// mod guildColor
			Color localTint;
			if (isGuide)
			{

				Color color = localTint.fromHex(guideColorInHex);

				localTint =
					selectedLayer == -1
					? color
					: (n1.layer == selectedLayer ? color: color * inactiveTint,
						n2.layer == selectedLayer ? color: color * inactiveTint);

				//localTint.a = tint.a;
			}
			else
			{
				localTint =
					selectedLayer == -1
					? noteTint
					: Color::lerp(n1.layer == selectedLayer ? noteTint : inactiveTint,
						n2.layer == selectedLayer ? noteTint : inactiveTint, percent1);

				localTint.a = tint.a * lerp(0.7, 1, lerp(startAlpha, endAlpha, percent1));
			}

			Vector2 p1{ xl1, y1 };
			Vector2 p2{ xl1 + holdSliceSize, y1 };
			Vector2 p3{ xl2, y2 };
			Vector2 p4{ xl2 + holdSliceSize, y2 };
			renderer->drawQuad(p1, p2, p3, p4, pathTex, left, left + holdSliceWidth, spr.getY(),
			                   spr.getY() + spr.getHeight(), localTint);
			p1.x = xl1 + holdSliceSize;
			p2.x = xr1 - holdSliceSize;
			p3.x = xl2 + holdSliceSize;
			p4.x = xr2 - holdSliceSize;
			renderer->drawQuad(p1, p2, p3, p4, pathTex, left + holdSliceWidth,
			                   right - holdSliceWidth, spr.getY(), spr.getY() + spr.getHeight(),
			                   localTint);
			p1.x = xr1 - holdSliceSize;
			p2.x = xr1;
			p3.x = xr2 - holdSliceSize;
			p4.x = xr2;
			renderer->drawQuad(p1, p2, p3, p4, pathTex, right - holdSliceWidth, right, spr.getY(),
			                   spr.getY() + spr.getHeight(), localTint);
		}
	}

	void ScoreEditorTimeline::drawInputNote(Renderer* renderer)
	{
		if (insertingHold)
		{
			drawHoldCurve(inputNotes.holdStart, inputNotes.holdEnd, EaseType::Linear, false,
			              renderer, noteTint);
			drawNote(inputNotes.holdStart, renderer, noteTint);
			drawNote(inputNotes.holdEnd, renderer, noteTint);
		}
		else
		{
			drawNote(currentMode == TimelineMode::InsertLong ? inputNotes.holdStart
			                                                 : inputNotes.tap,
			         renderer, hoverTint);
		}
	}

	void ScoreEditorTimeline::drawHoldNote(const std::unordered_map<id_t, Note>& notes,
	                                       const HoldNote& note, Renderer* renderer,
	                                       const Color& tint_, const int selectedLayer,
	                                       const int offsetTicks, const int offsetLane)
	{
		const Note& start = notes.at(note.start.ID);
		const Note& end = notes.at(note.end);
		const int length = abs(end.tick - start.tick);
		auto tint = tint_;
		if (note.steps.size())
		{
			static constexpr auto isSkipStep = [](const HoldStep& step)
			{ return step.type == HoldStepType::Skip; };
			int s1 = -1;
			int s2 = -1;

			if (length > 0)
			{
				for (int i = 0; i < note.steps.size(); ++i)
				{
					if (isSkipStep(note.steps[i]))
						continue;

					s2 = i;
					const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
					const Note& n2 = s2 == -1 ? end : notes.at(note.steps[s2].ID);
					const EaseType ease = s1 == -1 ? note.start.ease : note.steps[s1].ease;
					const float p1 = (n1.tick - start.tick) / (float)length;
					const float p2 = (n2.tick - start.tick) / (float)length;
					float a1, a2;
					if (!note.isGuide() || note.fadeType == FadeType::None)
					{
						a1 = 1;
						a2 = 1;
					}
					else if (note.fadeType == FadeType::In)
					{
						a1 = p1;
						a2 = p2;
					}
					else
					{
						a1 = 1 - p1;
						a2 = 1 - p2;
					}
					drawHoldCurve(n1, n2, ease, note.isGuide(), renderer, tint, offsetTicks,
					              offsetLane, a1, a2, note.guideColor, selectedLayer, note.holdEventType, note.colorInHex);

					s1 = s2;
				}

				const float p1 =
				    s1 == -1 ? 0 : (notes.at(note.steps[s1].ID).tick - start.tick) / (float)length;
				const float p2 = 1;
				float a1, a2;
				if (!note.isGuide() || note.fadeType == FadeType::None)
				{
					a1 = 1;
					a2 = 1;
				}
				else if (note.fadeType == FadeType::In)
				{
					a1 = p1;
					a2 = p2;
				}
				else
				{
					a1 = 1 - p1;
					a2 = 1 - p2;
				}

				const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
				const EaseType ease = s1 == -1 ? note.start.ease : note.steps[s1].ease;
				drawHoldCurve(n1, end, ease, note.isGuide(), renderer, tint, offsetTicks,
				              offsetLane, a1, a2, note.guideColor, selectedLayer, note.holdEventType, note.colorInHex);
			}

			s1 = -1;
			s2 = 1;

			if (noteTextures.notes == -1)
				return;

			const Texture& tex = ResourceManager::textures[noteTextures.notes];
			const Vector2 nodeSz{ notesHeight - 5, notesHeight - 5 };
			for (int i = 0; i < note.steps.size(); ++i)
			{
				const Note& n3 = notes.at(note.steps[i].ID);

				// find first non-skip step
				s2 = std::distance(note.steps.cbegin(),
				                   std::find_if(note.steps.cbegin() + std::max(s2, i),
				                                note.steps.cend(), std::not_fn(isSkipStep)));

				if (isNoteVisible(n3, offsetTicks))
				{
					if (drawHoldStepOutlines)
						drawSteps.emplace_back(StepDrawData{
						    n3.tick + offsetTicks, n3.lane + (float)offsetLane, n3.width,
						    note.steps[i].type == HoldStepType::Skip ? StepDrawType::SkipStep
						                                             : StepDrawType::NormalStep,
						    n3.layer });

					if (note.steps[i].type != HoldStepType::Hidden)
					{
						int sprIndex = getNoteSpriteIndex(n3);
						if (sprIndex > -1 && sprIndex < tex.sprites.size())
						{
							const Sprite& s = tex.sprites[sprIndex];
							Vector2 pos{ laneToPosition(n3.lane + offsetLane + (n3.width / 2.0f)),
								         getNoteYPosFromTick(n3.tick + offsetTicks) };

							if (isSkipStep(note.steps[i]))
							{
								// find the note before and after the skip step
								const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
								const Note& n2 =
								    s2 >= note.steps.size() ? end : notes.at(note.steps[s2].ID);

								// calculate the interpolation ratio based on the distance between
								// n1 and n2
								float ratio =
								    (float)(n3.tick - n1.tick) / (float)(n2.tick - n1.tick);
								const EaseType rEase =
								    s1 == -1 ? note.start.ease : note.steps[s1].ease;

								auto easeFunc = getEaseFunction(rEase);

								// interpolate the step's position
								float x1 = easeFunc(laneToPosition(n1.lane + offsetLane),
								                    laneToPosition(n2.lane + offsetLane), ratio);
								float x2 = easeFunc(laneToPosition(n1.lane + offsetLane + n1.width),
								                    laneToPosition(n2.lane + offsetLane + n2.width),
								                    ratio);
								pos.x = midpoint(x1, x2);
							}

							int z = (selectedLayer == -1
							             ? (int)ZIndex::zCount
							             : (n3.layer == selectedLayer ? (int)ZIndex::zCount : 0)) +
							        3;

							renderer->drawSprite(pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex,
							                     s.getX(), s.getX() + s.getWidth(), s.getY(),
							                     s.getY() + s.getHeight(),
							                     (selectedLayer == -1 || n3.layer == selectedLayer)
							                         ? tint
							                         : tint * otherLayerTint,
							                     z);
						}
					}
				}

				// update first interpolation point
				if (!isSkipStep(note.steps[i]))
					s1 = i;
			}
		}
		else
		{
			float a1, a2;
			if (!note.isGuide() || note.fadeType == FadeType::None)
			{
				a1 = 1;
				a2 = 1;
			}
			else if (note.fadeType == FadeType::In)
			{
				a1 = 0;
				a2 = 1;
			}
			else
			{
				a1 = 1;
				a2 = 0;
			}
			drawHoldCurve(start, end, note.start.ease, note.isGuide(), renderer, tint, offsetTicks,
			              offsetLane, a1, a2, note.guideColor, selectedLayer, note.holdEventType, note.colorInHex);
		}

		auto inactiveTint = tint * otherLayerTint;

		if (isNoteVisible(start, offsetTicks))
		{
			if (note.startType == HoldNoteType::Normal)
			{
				// 去掉Hold首尾的绘制
				/*drawNote(
				    start, renderer,
				    (selectedLayer == -1 || start.layer == selectedLayer) ? tint : inactiveTint,
				    offsetTicks, offsetLane, selectedLayer == -1 || start.layer == selectedLayer);*/
				//画空心框
				StepDrawType drawType = StepDrawType::SkipStep;
				drawSteps.push_back({ start.tick + offsetTicks, start.lane + (float)offsetLane,
									  start.width, drawType, start.layer });
			}
			else if (drawHoldStepOutlines)
			{
				/*StepDrawType drawType;
				if (note.startType == HoldNoteType::Guide)
				{
					drawType =
					    static_cast<StepDrawType>(static_cast<int>(StepDrawType::GuideNeutral) +
					                              static_cast<int>(note.guideColor));
				}
				else
				{
					drawType = start.critical ? StepDrawType::InvisibleHoldCritical
					                          : StepDrawType::InvisibleHold;
				}
				drawSteps.push_back({ start.tick + offsetTicks, start.lane + (float)offsetLane,
				                      start.width, drawType, start.layer });*/
				StepDrawType drawType = StepDrawType::SkipStep;
				drawSteps.push_back({ start.tick + offsetTicks, start.lane + (float)offsetLane,
									  start.width, drawType, start.layer });
			}
		}

		if (isNoteVisible(end, offsetTicks))
		{
			if (note.endType == HoldNoteType::Normal)
			{
				// 去掉Hold首尾的绘制
				/*drawNote(end, renderer,
				         (selectedLayer == -1 || end.layer == selectedLayer) ? tint : inactiveTint,
				         offsetTicks, offsetLane,
				         selectedLayer == -1 || start.layer == selectedLayer);*/
				StepDrawType drawType = StepDrawType::SkipStep;
				drawSteps.push_back({ end.tick + offsetTicks, end.lane + (float)offsetLane,
									  end.width, drawType, end.layer });
			}
			else if (drawHoldStepOutlines)
			{
				//StepDrawType drawType;
				/*if (note.startType == HoldNoteType::Guide)
				{
					drawType =
					    static_cast<StepDrawType>(static_cast<int>(StepDrawType::GuideNeutral) +
					                              static_cast<int>(note.guideColor));
				}
				else
				{
					drawType = start.critical ? StepDrawType::InvisibleHoldCritical
					                          : StepDrawType::InvisibleHold;
				}*/
				StepDrawType drawType = StepDrawType::SkipStep;
				drawSteps.push_back({ end.tick + offsetTicks, end.lane + (float)offsetLane,
				                      end.width, drawType, end.layer });
			}
		}
	}

	void ScoreEditorTimeline::drawHoldMid(Note& note, HoldStepType type, Renderer* renderer,
	                                      const Color& tint, const bool selectedLayer)
	{
		if (type == HoldStepType::Hidden || noteTextures.notes == -1)
			return;

		const Texture& tex = ResourceManager::textures[noteTextures.notes];
		int sprIndex = getNoteSpriteIndex(note);
		if (sprIndex < 0 || sprIndex >= tex.sprites.size())
			return;

		const Sprite& s = tex.sprites[sprIndex];

		// Center diamond
		Vector2 pos{ laneToPosition(note.lane + (note.width / 2.0f)),
			         getNoteYPosFromTick(note.tick) };
		Vector2 nodeSz{ notesHeight - 5, notesHeight - 5 };

		int z = (selectedLayer ? (int)ZIndex::zCount : 0) + 4;

		renderer->drawSprite(pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex, s.getX(),
		                     s.getX() + s.getWidth(), s.getY(), s.getY() + s.getHeight(), tint, z);
	}

	void ScoreEditorTimeline::drawOutline(const StepDrawData& data, int selectedLayer)
	{
		float x = position.x;
		float y = position.y - tickToPosition(data.tick) + visualOffset;

		ImVec2 p1{ x + laneToPosition(data.lane), y - (notesHeight * 0.15f) };
		ImVec2 p2{ x + laneToPosition(data.lane + data.width), y + (notesHeight * 0.15f) };
		auto fill = data.getFillColor();
		auto outline = data.getOutlineColor();



		if (selectedLayer != -1 && data.layer != -1 && selectedLayer != data.layer)
		{
			int fillAlpha = (fill & 0xFF000000) >> 24;
			fill = (fill & 0x00FFFFFF) | (fillAlpha / 2) << 24;
			int outlineAlpha = (outline & 0xFF000000) >> 24;
			outline = (outline & 0x00FFFFFF) | (outlineAlpha / 2) << 24;
		}

		ImGui::GetWindowDrawList()->AddRectFilled(p1, p2, fill, 1.0f, ImDrawFlags_RoundCornersAll);
		ImGui::GetWindowDrawList()->AddRect(p1, p2, outline, 1, ImDrawFlags_RoundCornersAll, 2);
	}

	void ScoreEditorTimeline::drawFlickArrow(const Note& note, Renderer* renderer,
	                                         const Color& tint, const int offsetTick,
	                                         const int offsetLane, const bool selectedLayer)
	{
		if (noteTextures.notes == -1)
			return;

		//mod 同drawnote
		int texName = -1;
		int sprIndex = -1;

		if (note.flick == FlickType::Right)
		{
			texName = noteTextures.flick_right;
			sprIndex = 0;
		}
		else if (note.flick == FlickType::Left)
		{
			texName = noteTextures.flick_left;
			sprIndex = 0;
		}
		else
		{
			texName = noteTextures.notes;
			sprIndex = getFlickArrowSpriteIndex(note);
		}

		const Texture& tex = ResourceManager::textures[texName];
		if (!isArrayIndexInBounds(sprIndex, tex.sprites))
			return;
		const Sprite& arrowS = tex.sprites[sprIndex];

		Vector2 pos{ 0, getNoteYPosFromTick(note.tick + offsetTick) };
		const float x1 = laneToPosition(note.lane + offsetLane);
		const float x2 = pos.x + laneToPosition(note.lane + note.width + offsetLane);
		pos.x = midpoint(x1, x2);
		pos.y += notesHeight * 0.7f; // Move the arrow up a bit

		// Notes wider than 6 lanes also use flick arrow size 6
		int sizeIndex = std::min((int)floor(note.width) - 1, 5);
		Vector2 size{ laneWidth * flickArrowWidths[sizeIndex],
			          notesHeight * flickArrowHeights[sizeIndex] };

		float sx1 = arrowS.getX();
		float sx2 = arrowS.getX() + arrowS.getWidth();
		//if (note.flick == FlickType::Right)
		//{
		//	// Flip arrow to point to the right
		//	sx1 = arrowS.getX() + arrowS.getWidth();
		//	sx2 = arrowS.getX();
		//}

		renderer->drawSprite(pos, 0.0f, size, AnchorType::MiddleCenter, tex, sx1, sx2,
		                     arrowS.getY(), arrowS.getY() + arrowS.getHeight(), tint, 2);
	}

	/// <summary>
	/// 绘制Note用
	/// </summary>
	void ScoreEditorTimeline::drawNote(const Note& note, Renderer* renderer, const Color& tint,
		const int offsetTick, const int offsetLane,
		const bool selectedLayer)
	{
		if (noteTextures.notes == -1)
			return;

		int texName = -1;
		int sprIndex = -1;

		//mod 根据按键类型加载自定义贴图 修改自getNoteSpriteIndex（）
		if (note.isHold())
		{
			texName = noteTextures.notes;
			sprIndex = getNoteSpriteIndex(note);
		}
		else if (!note.critical && note.flick == FlickType::None)
		{
			texName = noteTextures.bell;
			sprIndex = 0;
		}
		else if (note.isFlick())
		{
			texName = noteTextures.notes;
			sprIndex = getNoteSpriteIndex(note);
		}
		else if (note.critical && note.getType() != NoteType::HoldMid)
		{
			texName = noteTextures.ten;
			sprIndex = 0;
		}
		else if (note.getType() == NoteType::Damage)
		{
			texName = noteTextures.danmaku;
			sprIndex = 0;
		}
		else
		{
			texName = noteTextures.notes;
			sprIndex = getNoteSpriteIndex(note);
		}

		//const Texture& tex = ResourceManager::textures[noteTextures.notes];
		//const int sprIndex = getNoteSpriteIndex(note);
		//if (!isArrayIndexInBounds(sprIndex, tex.sprites))
		//	return;
		//const Sprite& s = tex.sprites[sprIndex];

		const Texture& tex = ResourceManager::textures[texName];
		if (!isArrayIndexInBounds(sprIndex, tex.sprites))
			return;
		const Sprite& s = tex.sprites[sprIndex];


		Vector2 pos{ laneToPosition(note.lane + offsetLane),
					 getNoteYPosFromTick(note.tick + offsetTick) };
		const Vector2 sliceSz(notesSliceSize, notesHeight);
		const AnchorType anchor = AnchorType::MiddleLeft;

		const float midLen =
			std::max(0.0f, (laneWidth * note.width) - (sliceSz.x * 2) + noteOffsetX + 5);
		const Vector2 midSz{ midLen, notesHeight };

		pos.x -= noteOffsetX;
		const int left = s.getX() + noteCutoffX;
		const int right = s.getX() + s.getWidth() - noteCutoffX;

		const int z = (selectedLayer ? (int)ZIndex::zCount : 0) + (int)ZIndex::Note;

		// MOD 绘制的时候只绘制单独的中间
		if (note.getType() == NoteType::Tap && !note.isFlick())
		{
			Vector2 pos{ laneToPosition(note.lane), getNoteYPosFromTick(note.tick + offsetTick) };
			renderer->drawSprite(pos, 0.0f, Vector2(40, 40), AnchorType::MiddleCenter, tex, 0, tint, z);
		}
		// 普通圆形弹幕同上
		else if (note.getType() == NoteType::Damage && note.damageType == DamageType::Circle)
		{
			Vector2 pos{ laneToPosition(note.lane), getNoteYPosFromTick(note.tick + offsetTick) };
			renderer->drawSprite(pos, 0.0f, Vector2(40, 40), AnchorType::MiddleCenter, tex, 0, tint, z);
		}
		else
		{
			// left slice
			renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex, left, left + noteSliceWidth, s.getY(),
				s.getY() + s.getHeight(), tint, z);
			pos.x += sliceSz.x;

			// Middle
			renderer->drawSprite(pos, 0.0f, midSz, anchor, tex, left + noteSliceWidth,
				left + noteSliceWidth + 1, s.getY(), s.getY() + s.getHeight(), tint,
				z);

			pos.x += midLen;

			// right slice
			renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex, right - noteSliceWidth, right,
				s.getY(), s.getY() + s.getHeight(), tint, z);
		}


		if (note.friction)
		{
			int frictionSprIndex = getFrictionSpriteIndex(note);
			if (isArrayIndexInBounds(frictionSprIndex, tex.sprites))
			{
				// Friction diamond is slightly smaller
				const Sprite& frictionSpr = tex.sprites[frictionSprIndex];
				const Vector2 nodeSz{ notesHeight * 0.8f, notesHeight * 0.8f };

				// diamond is always centered
				pos.x = midpoint(laneToPosition(note.lane + offsetLane),
					laneToPosition(note.lane + offsetLane + note.width));
				renderer->drawSprite(
					pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex, frictionSpr.getX(),
					frictionSpr.getX() + frictionSpr.getWidth(), frictionSpr.getY(),
					frictionSpr.getY() + frictionSpr.getHeight(), tint, z + 1);
			}
		}

		if (note.isFlick())
			drawFlickArrow(note, renderer, tint, offsetTick, offsetLane);


		//-------------------------------------------------- By Cursor 绘制text 不为1时绘制

		if (note.extraSpeed != 1)
		{
			std::string extraSpeedStr = IO::formatString("%.2fx", note.extraSpeed);

			float textX = position.x + laneToPosition(note.lane + offsetLane + note.width) - ImGui::CalcTextSize(extraSpeedStr.c_str()).x + 20;
			float textY = position.y - tickToPosition(note.tick + offsetTick) + visualOffset - (notesHeight * 0.5f) + 15;

			ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 15.0f, ImVec2(textX, textY), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), extraSpeedStr.c_str());
		}
	}

	void ScoreEditorTimeline::drawCcNote(const Note& note, Renderer* renderer, const Color& tint,
	                                     const int offsetTick, const int offsetLane,
	                                     const bool selectedLayer)
	{
		if (noteTextures.notes == -1)
			return;

		int texName = -1;
		int sprIndex = -1;

		//mod 根据按键类型加载自定义贴图 修改自getNoteSpriteIndex（）
		if (note.damageType == DamageType::Circle)
		{
			switch (note.damageDirection)
			{
				case DamageDirection::Left:
				{
					texName = noteTextures.danmaku_left;
					break;
				}
				case DamageDirection::Right:
				{
					texName = noteTextures.danmaku_right;
					break;
				}
				case DamageDirection::Middle:
				{
					texName = noteTextures.danmaku_center;
					break;
				}
				default:
				{
					texName = noteTextures.danmaku;
					break;
				}
			}
			sprIndex = 0;
		}
		else
		{
			texName = noteTextures.ccNotes;
			sprIndex = 0;
		}

		const Texture& tex = ResourceManager::textures[texName];
		if (!isArrayIndexInBounds(sprIndex, tex.sprites))
			return;
		const Sprite& s = tex.sprites[sprIndex];

		Vector2 pos{ laneToPosition(note.lane + offsetLane),
			 getNoteYPosFromTick(note.tick + offsetTick) };
		const Vector2 sliceSz(notesSliceSize, notesHeight);
		const AnchorType anchor = AnchorType::MiddleLeft;

		const float midLen =
			std::max(0.0f, (laneWidth * note.width) - (sliceSz.x * 2) + noteOffsetX + 5);
		const Vector2 midSz{ midLen, notesHeight };

		pos.x -= noteOffsetX;
		const int left = s.getX() + noteCutoffX;
		const int right = s.getX() + s.getWidth() - noteCutoffX;

		const int z = (selectedLayer ? (int)ZIndex::zCount : 0) + 1;

		if (note.damageType == DamageType::Circle)
		{
			Vector2 pos{ laneToPosition(note.lane), getNoteYPosFromTick(note.tick + offsetTick) };
			renderer->drawSprite(pos, 0.0f, Vector2(40, 40), AnchorType::MiddleCenter, tex, 0, tint, z);
		}
		else
		{
			// left slice
			renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex, left, left + noteSliceWidth, s.getY(),
				s.getY() + s.getHeight(), tint, z);
			pos.x += sliceSz.x;

			// middle
			renderer->drawSprite(pos, 0.0f, midSz, anchor, tex, left + noteSliceWidth,
				right - noteSliceWidth, s.getY(), s.getY() + s.getHeight(), tint, z);
			pos.x += midLen;

			// right slice
			renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex, right - noteSliceWidth, right,
				s.getY(), s.getY() + s.getHeight(), tint, z);
		}

		if (note.friction)
		{
			int frictionSprIndex = getFrictionSpriteIndex(note);
			if (frictionSprIndex >= 0 && frictionSprIndex < tex.sprites.size())
			{
				// friction diamond is slightly smaller
				const Sprite& frictionSpr = tex.sprites[frictionSprIndex];
				const Vector2 nodeSz{ notesHeight * 0.8f, notesHeight * 0.8f };

				// diamond is always centered
				pos.x = midpoint(laneToPosition(note.lane + offsetLane),
				                 laneToPosition(note.lane + offsetLane + note.width));
				renderer->drawSprite(
				    pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex, frictionSpr.getX(),
				    frictionSpr.getX() + frictionSpr.getWidth(), frictionSpr.getY(),
				    frictionSpr.getY() + frictionSpr.getHeight(), tint, z + 1);
			}
		}

		if (note.isFlick())
			drawFlickArrow(note, renderer, tint, offsetTick, offsetLane);

		//-------------------------------------------------- By Cursor 绘制text 不为1时绘制

		if (note.extraSpeed != 1)
		{
			std::string extraSpeedStr = IO::formatString("%.2fx", note.extraSpeed);

			float textX = position.x + laneToPosition(note.lane + offsetLane + note.width) - ImGui::CalcTextSize(extraSpeedStr.c_str()).x + 20;
			float textY = position.y - tickToPosition(note.tick + offsetTick) + visualOffset - (notesHeight * 0.5f) + 15;

			ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 15.0f, ImVec2(textX, textY), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), extraSpeedStr.c_str());
		}
	}

	bool ScoreEditorTimeline::bpmControl(const Score& score, const Tempo& tempo)
	{
		return bpmControl(score, tempo.bpm, tempo.tick, !playing);
	}

	bool ScoreEditorTimeline::bpmControl(const Score& score, float bpm, int tick, bool enabled)
	{
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineEndX(score) + (15 * dpiScale),
			         position.y - tickToPosition(tick) + visualOffset };
		return eventControl(getTimelineEndX(score), pos, tempoColor,
		                    IO::formatString("%g BPM", bpm).c_str(), enabled);
	}

	bool ScoreEditorTimeline::timeSignatureControl(const Score& score, int numerator,
	                                               int denominator, int tick, bool enabled)
	{
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineEndX(score) + (78 * dpiScale),
			         position.y - tickToPosition(tick) + visualOffset };
		return eventControl(getTimelineEndX(score), pos, timeColor,
		                    IO::formatString("%d/%d", numerator, denominator).c_str(), enabled);
	}

	bool ScoreEditorTimeline::skillControl(const Score& score, const SkillTrigger& skill)
	{
		return skillControl(score, skill.tick, !playing);
	}

	bool ScoreEditorTimeline::skillControl(const Score& score, int tick, bool enabled)
	{
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineStartX(score) - (50 * dpiScale),
			         position.y - tickToPosition(tick) + visualOffset };
		return eventControl(getTimelineStartX(score), pos, skillColor, getString("skill"), enabled);
	}

	bool ScoreEditorTimeline::feverControl(const Score& score, const Fever& fever)
	{
		return feverControl(score, fever.startTick, true, !playing) ||
		       feverControl(score, fever.endTick, false, !playing);
	}

	bool ScoreEditorTimeline::feverControl(const Score& score, int tick, bool start, bool enabled)
	{
		if (tick < 0)
			return false;

		std::string txt = "FEVER";
		txt.append(start ? ICON_FA_CARET_UP : ICON_FA_CARET_DOWN);

		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineStartX(score) - (108 * dpiScale),
			         position.y - tickToPosition(tick) + visualOffset };
		return eventControl(getTimelineStartX(score), pos, feverColor, txt.c_str(), enabled);
	}

	bool ScoreEditorTimeline::hiSpeedControl(const ScoreContext& context,
	                                         const HiSpeedChange& hiSpeed)
	{
		return hiSpeedControl(context, hiSpeed.tick, hiSpeed.speed, hiSpeed.layer,
		                      context.selectedHiSpeedChanges.find(hiSpeed.ID) !=
		                          context.selectedHiSpeedChanges.end());
	}

	bool ScoreEditorTimeline::hiSpeedControl(const ScoreContext& context, int tick, float speed,
	                                         int layer, bool selected)
	{
		std::string txt =
		    (layer == -1 || context.selectedLayer == layer)
		        ? IO::formatString("%.2fx", speed)
		        : IO::formatString("%.2fx (%s)", speed, context.score.layers[layer].name.c_str());
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineEndX(context.score) +
			             (((layer == -1 || context.showAllLayers || context.selectedLayer == layer)
			                   ? 123
			                   : 180) *
			              dpiScale),
			         position.y - tickToPosition(tick) + visualOffset };
		bool enabled = layer == -1 || context.showAllLayers || context.selectedLayer == layer;
		auto color = enabled ? speedColor : inactiveSpeedColor;

		return eventControl(
		    getTimelineEndX(context.score), pos,
		    selected ? ImGui::ColorConvertFloat4ToU32(generateHighlightColor(
		                   generateHighlightColor(ImGui::ColorConvertU32ToFloat4(color))))
		             : color,
		    txt.c_str(), enabled);
	}

	bool ScoreEditorTimeline::waypointControl(const Score& score, const Waypoint& waypoint)
	{
		return waypointControl(score, waypoint.name, waypoint.tick);
	}
	bool ScoreEditorTimeline::waypointControl(const Score& score, std::string name, int tick)
	{
		if (tick < 0)
			return false;

		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineStartX(score) - (176 * dpiScale),
			         position.y - tickToPosition(tick) + visualOffset };
		return eventControl(getTimelineStartX(score), pos, waypointColor, name.c_str(), true);
	}

	void ScoreEditorTimeline::eventEditor(ScoreContext& context)
	{
		ImGui::SetNextWindowSize(ImVec2(250, -1), ImGuiCond_Always);
		if (ImGui::BeginPopup("edit_event"))
		{
			std::string editLabel{ "edit_" };
			editLabel.append(eventTypes[(int)eventEdit.type]);
			ImGui::Text(getString(editLabel));
			ImGui::Separator();

			if (eventEdit.type == EventType::Bpm)
			{
				if (!isArrayIndexInBounds(eventEdit.editId, context.score.tempoChanges))
				{
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}

				UI::beginPropertyColumns();

				Tempo& tempo = context.score.tempoChanges[eventEdit.editId];
				UI::addFloatProperty(getString("bpm"), eventEdit.editBpm, "%g");
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					Score prev = context.score;
					tempo.bpm = std::clamp(eventEdit.editBpm, MIN_BPM, MAX_BPM);

					context.pushHistory("Change tempo", prev, context.score);
				}
				UI::endPropertyColumns();

				// cannot remove the first tempo change
				if (tempo.tick != 0)
				{
					ImGui::Separator();
					if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
					{
						ImGui::CloseCurrentPopup();
						Score prev = context.score;
						context.score.tempoChanges.erase(context.score.tempoChanges.begin() +
						                                 eventEdit.editId);
						context.pushHistory("Remove tempo change", prev, context.score);
					}
				}
			}
			else if (eventEdit.type == EventType::TimeSignature)
			{
				if (context.score.timeSignatures.find(eventEdit.editId) ==
				    context.score.timeSignatures.end())
				{
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}

				UI::beginPropertyColumns();
				if (UI::timeSignatureSelect(eventEdit.editTimeSignatureNumerator,
				                            eventEdit.editTimeSignatureDenominator))
				{
					Score prev = context.score;
					TimeSignature& ts = context.score.timeSignatures[eventEdit.editId];
					ts.numerator = std::clamp(abs(eventEdit.editTimeSignatureNumerator),
					                          MIN_TIME_SIGNATURE, MAX_TIME_SIGNATURE_NUMERATOR);
					ts.denominator = std::clamp(abs(eventEdit.editTimeSignatureDenominator),
					                            MIN_TIME_SIGNATURE, MAX_TIME_SIGNATURE_DENOMINATOR);

					context.pushHistory("Change time signature", prev, context.score);
				}
				UI::endPropertyColumns();

				// cannot remove the first time signature
				if (eventEdit.editId != 0)
				{
					ImGui::Separator();
					if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
					{
						ImGui::CloseCurrentPopup();
						Score prev = context.score;
						context.score.timeSignatures.erase(eventEdit.editId);
						context.pushHistory("Remove time signature", prev, context.score);
					}
				}
			}
			else if (eventEdit.type == EventType::HiSpeed)
			{
				if (context.score.hiSpeedChanges.find(eventEdit.editId) ==
				    context.score.hiSpeedChanges.end())
				{
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}

				UI::beginPropertyColumns();
				UI::addFloatProperty(getString("hi_speed_speed"), eventEdit.editHiSpeed, "%g");
				HiSpeedChange& hiSpeed = context.score.hiSpeedChanges[eventEdit.editId];
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					Score prev = context.score;
					hiSpeed.speed = eventEdit.editHiSpeed;

					context.pushHistory("Change hi-speed", prev, context.score);
				}
				UI::endPropertyColumns();

				ImGui::Separator();
				if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
				{
					ImGui::CloseCurrentPopup();
					Score prev = context.score;
					context.score.hiSpeedChanges.erase(eventEdit.editId);
					context.pushHistory("Remove hi-speed change", prev, context.score);
				}
			}
			else if (eventEdit.type == EventType::Waypoint)
			{
				if (eventEdit.editId >= context.score.waypoints.size())
				{
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}

				UI::beginPropertyColumns();
				UI::addStringProperty(getString("waypoint_name"), eventEdit.editName);
				Waypoint& waypoint = context.score.waypoints[eventEdit.editId];
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					Score prev = context.score;
					waypoint.name = eventEdit.editName;

					context.pushHistory("Change waypoint", prev, context.score);
				}
				UI::endPropertyColumns();

				ImGui::Separator();
				if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
				{
					ImGui::CloseCurrentPopup();
					Score prev = context.score;
					context.score.waypoints.erase(context.score.waypoints.begin() +
					                              eventEdit.editId);
					context.pushHistory("Remove waypint", prev, context.score);
				}
			}
			ImGui::EndPopup();
		}
	}

	bool ScoreEditorTimeline::isMouseInHoldPath(const Note& n1, const Note& n2, EaseType ease,
	                                            float x, float y)
	{
		float xStart1 = laneToPosition(n1.lane);
		float xStart2 = laneToPosition(n1.lane + n1.width);
		float xEnd1 = laneToPosition(n2.lane);
		float xEnd2 = laneToPosition(n2.lane + n2.width);
		float y1 = getNoteYPosFromTick(n1.tick);
		float y2 = getNoteYPosFromTick(n2.tick);

		if (!isWithinRange(y, y1, y2))
			return false;

		auto easeFunc = getEaseFunction(ease);
		float percent = (y - y1) / (y2 - y1);
		float x1 = easeFunc(xStart1, xEnd1, percent);
		float x2 = easeFunc(xStart2, xEnd2, percent);

		return isWithinRange(x, std::min(x1, x2), std::max(x1, x2));
	}

	void ScoreEditorTimeline::previousTick(ScoreContext& context)
	{
		if (playing || context.currentTick == 0)
			return;

		context.currentTick = std::max(
		    roundTickDown(context.currentTick, division) - (TICKS_PER_BEAT / (division / 4)), 0);
		lastSelectedTick = context.currentTick;
		focusCursor(context, Direction::Down);
	}

	void ScoreEditorTimeline::nextTick(ScoreContext& context)
	{
		if (playing)
			return;

		context.currentTick =
		    roundTickDown(context.currentTick, division) + (TICKS_PER_BEAT / (division / 4));
		lastSelectedTick = context.currentTick;
		focusCursor(context, Direction::Up);
	}

	int ScoreEditorTimeline::roundTickDown(int tick, int division)
	{
		return std::max(tick - (tick % (TICKS_PER_BEAT / (division / 4))), 0);
	}

	void ScoreEditorTimeline::insertNote(ScoreContext& context, EditArgs& edit)
	{
		Score prev = context.score;

		Note newNote = inputNotes.tap;
		newNote.ID = Note::getNextID();
		newNote.layer = context.selectedLayer;

		context.score.notes[newNote.ID] = newNote;
		context.pushHistory("Insert note", prev, context.score);
	}

	void ScoreEditorTimeline::insertHold(ScoreContext& context, EditArgs& edit)
	{
		Score prev = context.score;

		Note holdStart = inputNotes.holdStart;
		holdStart.ID = Note::getNextID();
		holdStart.layer = context.selectedLayer;

		Note holdEnd = inputNotes.holdEnd;
		holdEnd.ID = Note::getNextID();
		holdEnd.parentID = holdStart.ID;
		holdEnd.layer = context.selectedLayer;

		if (holdStart.tick == holdEnd.tick)
		{
			holdEnd.tick += TICKS_PER_BEAT / (static_cast<float>(division) / 4);
		}
		else if (holdStart.tick > holdEnd.tick)
		{
			std::swap(holdStart.tick, holdEnd.tick);
			std::swap(holdStart.width, holdEnd.width);
			std::swap(holdStart.lane, holdEnd.lane);
		}

		HoldNoteType holdType =
		    currentMode == TimelineMode::InsertGuide ? HoldNoteType::Guide : HoldNoteType::Normal;
		context.score.notes[holdStart.ID] = holdStart;
		context.score.notes[holdEnd.ID] = holdEnd;
		context.score.holdNotes[holdStart.ID] = { {
			                                          holdStart.ID,
			                                          HoldStepType::Normal,
			                                          edit.easeType,
			                                      },
			                                      {},
			                                      holdEnd.ID,
			                                      holdType,
			                                      holdType,
												  //mod 额外hold属性
												  edit.holdEventType,
												  edit.colorsetID,
												  edit.highlight,
												  edit.colorInHex,
												  //mod end
			                                      edit.fadeType,
			                                      edit.colorType };
		context.pushHistory("Insert hold", prev, context.score);
	}

	void ScoreEditorTimeline::insertHoldStep(ScoreContext& context, EditArgs& edit, int holdId)
	{
		// make sure the hold data exists
		if (context.score.holdNotes.find(holdId) == context.score.holdNotes.end())
			return;

		// make sure the parent hold note exists
		if (context.score.notes.find(holdId) == context.score.notes.end())
			return;

		Score prev = context.score;

		HoldNote& hold = context.score.holdNotes[holdId];
		Note holdStart = context.score.notes[holdId];

		Note holdStep = inputNotes.holdStep;
		holdStep.ID = Note::getNextID();
		holdStep.critical = holdStart.critical;
		holdStep.parentID = holdStart.ID;
		holdStep.layer = context.selectedLayer;

		context.score.notes[holdStep.ID] = holdStep;

		hold.steps.push_back(
		    { holdStep.ID, hold.isGuide() ? HoldStepType::Hidden : edit.stepType, edit.easeType });

		// sort steps in-case the step is inserted before/after existing steps
		sortHoldSteps(context.score, hold);
		context.pushHistory("Insert hold step", prev, context.score);
	}

	void ScoreEditorTimeline::insertDamage(ScoreContext& context, EditArgs& edit)
	{
		Score prev = context.score;

		Note newNote = inputNotes.damage;
		newNote.ID = Note::getNextID();
		newNote.layer = context.selectedLayer;

		context.score.notes[newNote.ID] = newNote;
		context.pushHistory("Insert damage", prev, context.score);
	}

	void ScoreEditorTimeline::debug(ScoreContext& context)
	{
		ImGui::Text("Window position: (%.2f, %.2f)\nWindow size: (%.2f, %.2f)", position.x,
		            position.y, size.x, size.y);
		ImGui::Separator();

		ImGui::Text("Mouse position: (%.2f, %.2f)\nMouse in timeline: %s", mousePos.x, mousePos.y,
		            boolToString(mouseInTimeline));
		ImGui::Separator();

		ImGui::Text("Minimum offset: %g\nCurrent offset: %g\nMaximum offset: %g", minOffset, offset,
		            maxOffset);
		ImGui::Separator();

		if (mouseInTimeline)
		{
			ImGui::Text("Hover lane: %d\nHover tick: %d", hoverLane, hoverTick);
		}
		else
		{
			ImGui::TextDisabled("Hover lane: --\nHover tick: --");
		}

		ImGui::Text("Last selected tick : %d", lastSelectedTick);
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Hover Note", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Hovering note ID: %llu", hoveringNote);
			ImGui::Text("Holding note ID: %llu", holdingNote);

			auto it = context.score.notes.find(hoveringNote);
			if (it != context.score.notes.end())
			{
				const Note& note = it->second;
				ImGui::Text("ID: %llu\nType: %hhu\nTick: %d\nLane: %f\nWidth: %f\nCritical: "
				            "%s\nFriction: %s\nFlick: %s",
				            note.ID, static_cast<uint8_t>(note.getType()), note.tick, note.lane,
				            note.width, boolToString(note.critical), boolToString(note.friction),
				            flickTypes[static_cast<int>(note.flick)]);
			}
			else
			{
				// Prevent window scrolling from jumping around
				ImGui::TextDisabled("ID: -\nType: -\nTick: -\nLane: -\nWidth: -\nCritical: "
				                    "-\nFriction: -\nFlick: -");
			}
		}
	}

	ScoreEditorTimeline::ScoreEditorTimeline()
	{
		framebuffer = std::make_unique<Framebuffer>(1920, 1080);
		playbackSpeed = 1.0f;

		background.load(config.backgroundImage.empty()
		                    ? (Application::getAppDir() + "res\\textures\\default.png")
		                    : config.backgroundImage);
		background.setBrightness(0.67);

		timelineInstance = this;
	}

	void ScoreEditorTimeline::setPlaybackSpeed(ScoreContext& context, float speed)
	{
		playbackSpeed = std::clamp(speed, minPlaybackSpeed, maxPlaybackSpeed);
		context.audio.setPlaybackSpeed(playbackSpeed, time);
	}

	void ScoreEditorTimeline::setPlaying(ScoreContext& context, bool state)
	{
		if (playing == state)
			return;

		playing = state;
		if (playing)
		{
			playStartTime = time;
			context.audio.seekMusic(time);
			context.audio.playMusic(time);
			context.audio.setLastPlaybackTime(time);
			context.audio.syncAudioEngineTimer();
		}
		else
		{
			if (config.returnToLastSelectedTickOnPause)
			{
				context.currentTick = lastSelectedTick;
				offset =
				    std::max(minOffset, tickToPosition(context.currentTick) +
				                            (size.y * (1.0f - config.cursorPositionThreshold)));
			}

			context.audio.stopSoundEffects(false);
			context.audio.stopMusic();
		}
	}

	void ScoreEditorTimeline::stop(ScoreContext& context)
	{
		playing = false;
		time = lastSelectedTick = context.currentTick = 0;
		offset = std::max(minOffset, tickToPosition(context.currentTick) +
		                                 (size.y * (1.0f - config.cursorPositionThreshold)));

		context.audio.stopSoundEffects(false);
		context.audio.stopMusic();
	}

	void ScoreEditorTimeline::updateNoteSE(ScoreContext& context)
	{
		if (!playing)
			return;

		static auto singleNoteSEFunc = [&context, this](const Note& note, float notePlayTime)
		{
			bool playSE = true;
			if (note.getType() == NoteType::Hold)
			{
				playSE = context.score.holdNotes.at(note.ID).startType == HoldNoteType::Normal;
			}
			else if (note.getType() == NoteType::HoldEnd)
			{
				playSE = context.score.holdNotes.at(note.parentID).endType == HoldNoteType::Normal;
			}

			if (playSE)
			{
				std::string_view se = getNoteSE(note, context.score);
				std::string key = std::to_string(note.tick) + "-" + se.data();
				if (!se.empty() && (playingNoteSounds.find(key) == playingNoteSounds.end()))
				{
					context.audio.playSoundEffect(se.data(), notePlayTime, -1, time);
					playingNoteSounds.insert(key);
				}
			}
		};

		static auto holdNoteSEFunc = [&context, this](const Note& note, float startTime)
		{
			int endTick = context.score.notes.at(context.score.holdNotes.at(note.ID).end).tick;
			float endTime = accumulateDuration(endTick, TICKS_PER_BEAT, context.score.tempoChanges);

			float adjustedEndTime = endTime - playStartTime + audioOffsetCorrection;
			context.audio.playSoundEffect(note.critical ? SE_CRITICAL_CONNECT : SE_CONNECT,
			                              startTime, adjustedEndTime, time);
		};

		playingNoteSounds.clear();
		for (const auto& [id, note] : context.score.notes)
		{
			float noteTime =
			    accumulateDuration(note.tick, TICKS_PER_BEAT, context.score.tempoChanges);
			float notePlayTime = noteTime - playStartTime;
			float offsetNoteTime = noteTime - (audioLookAhead * playbackSpeed);

			if (offsetNoteTime >= timeLastFrame && offsetNoteTime < time)
			{
				singleNoteSEFunc(note, notePlayTime - audioOffsetCorrection);
				if (note.getType() == NoteType::Hold &&
				    !context.score.holdNotes.at(note.ID).isGuide())
					holdNoteSEFunc(note, notePlayTime - audioOffsetCorrection);
			}
			else if (time == playStartTime)
			{
				// Playback just started
				if (noteTime >= time && offsetNoteTime < time)
					singleNoteSEFunc(note, notePlayTime);

				// Playback started mid-hold
				if (note.getType() == NoteType::Hold &&
				    !context.score.holdNotes.at(note.ID).isGuide())
				{
					int endTick =
					    context.score.notes.at(context.score.holdNotes.at(note.ID).end).tick;
					float endTime =
					    accumulateDuration(endTick, TICKS_PER_BEAT, context.score.tempoChanges);
					if ((noteTime - time) <= audioLookAhead && endTime > time)
						holdNoteSEFunc(note, std::max(0.0f, notePlayTime));
				}
			}
		}
	}

	void ScoreEditorTimeline::drawWaveform(ScoreContext& context)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		constexpr ImU32 waveformColorL = 0x80646464;
		constexpr ImU32 waveformColorR = 0x80585858;

		// Ideally this should be calculated based on the current BPM
		const double secondsPerPixel = waveformSecondsPerPixel / zoom;
		const double durationSeconds = context.waveformL.durationInSeconds;
		const double musicOffsetInSeconds = context.workingData.musicOffset / 1000.0f;

		const float timelineMidPosition = midpoint(getTimelineStartX(), getTimelineEndX());

		for (size_t index = 0; index < 2; index++)
		{
			const bool rightChannel = index == 1;
			const Audio::WaveformMipChain& waveform =
			    rightChannel ? context.waveformR : context.waveformL;
			if (waveform.isEmpty())
				continue;

			const ImU32 waveformColor = rightChannel ? waveformColorR : waveformColorL;
			const Audio::WaveformMip& mip = waveform.findClosestMip(secondsPerPixel);

			for (int y = visualOffset - size.y; y < visualOffset; y += 1)
			{
				int tick = positionToTick(y);

				// Small accuracy loss by converting to ticks but shouldn't be too noticeable
				const double secondsAtPixel =
				    accumulateDuration(tick, TICKS_PER_BEAT, context.score.tempoChanges) -
				    musicOffsetInSeconds;
				const bool outOfBounds =
				    secondsAtPixel < 0 || secondsAtPixel > waveform.durationInSeconds;

				float amplitude =
				    std::max(waveform.getAmplitudeAt(mip, secondsAtPixel, secondsPerPixel), 0.0f);
				float barValue = outOfBounds ? 0.0f : (amplitude * std::min(laneWidth * 6, 180.0f));
				float rectYPosition = floorf(position.y + visualOffset - y);
				// WARNING: A thickness of 0.5 or less does not draw with integrated graphics
				// (optimization? limitation?)

				float timelineMidPosition = midpoint(getTimelineStartX(), getTimelineEndX());
				ImVec2 rect1(timelineMidPosition, rectYPosition);
				ImVec2 rect2(timelineMidPosition +
				                 (std::max(0.75f, barValue) * (rightChannel ? 1 : -1)),
				             rectYPosition + 0.75f);
				drawList->AddRectFilled(rect1, rect2, waveformColor);
			}
		}
	}

	void ScoreEditorTimeline::scrollTimeline(ScoreContext& context, const int tick)
	{
		context.currentTick = tick;
		offset = std::max(minOffset, tickToPosition(context.currentTick) +
		                                 (size.y * (1.0f - config.cursorPositionThreshold)));
	}
} // namespace MikuMikuWorld
