#define DRAW_PROFILER_MIN_BAR_WIDTH 2


static inline float2 easyInput_mouseToResolution(float2 mouseP_01, float2 resolution) {
	float2 result = make_float2(mouseP_01.x*resolution.x, mouseP_01.y*resolution.y);
	return result;
}

static inline float2 easyInput_mouseToResolution_originLeftBottomCorner(float2 mouseP_01, float2 resolution) {
	float2 result = make_float2(mouseP_01.x*resolution.x, (mouseP_01.y)*resolution.y);
	return result;
}


static void EasyProfile_DrawGraph(Renderer *renderer, GameState *gameState, EasyProfile_ProfilerDrawState *drawState, float dt, float aspectRatio_yOverX, float2 mouseP_01) {
	assert(renderer);
	assert(gameState);
	assert(drawState);

	DEBUG_TIME_BLOCK()
	EasyProfiler_State *state = DEBUG_globalEasyEngineProfilerState;

	float4 colors[] = { make_float4(0.31, 0.77, 0.97, 1),   // Sky Blue
		make_float4(1.00, 0.70, 0.00, 1),   // Amber
		make_float4(0.51, 0.78, 0.52, 1),   // Light Green
		make_float4(0.90, 0.45, 0.45, 1),   // Soft Red
		make_float4(0.58, 0.46, 0.80, 1),   // Deep Purple
		make_float4(0.30, 0.71, 0.67, 1),   // Teal
		make_float4(0.86, 0.91, 0.46, 1),   // Lime
		make_float4(1.00, 0.54, 0.40, 1),   // Orange
		make_float4(0.69, 0.74, 0.77, 1),   // Grey
		make_float4(0.94, 0.38, 0.57, 1),   // Pink 
		};

	//NOTE(ollie): We render things to a fake resolution then it just scales when it goes out to the viewport. 
	float fuaxWidth = 1920;
	float2 resolution = make_float2(fuaxWidth, fuaxWidth*aspectRatio_yOverX);
	float screenRelativeSize = 1.0f;
	float fontSize = 1.2f;
	float zOffset = 1;
	
	if(drawState->firstFrame) {
		drawState->firstFrame = false;
	}

	if(state->framesFilled > 0) { //don't output anything on the first frames worth

		float backdropHeight = (state->lookingAtSingleFrame) ? resolution.y : 0.6f*resolution.y;
		float backdropY = 0;

		if(drawState->openTimer >= 0) {
			drawState->openTimer += dt;

			float val = drawState->openTimer / 0.4f;

			if(drawState->openState == EASY_PROFILER_DRAW_OPEN) {
				backdropY = lerp(-backdropHeight, 0, make_lerpTValue(val));	
			} else {
				backdropY = lerp(0, -backdropHeight, make_lerpTValue(val));
			}

			if(val >= 1.0f) {
				drawState->openTimer = -1;
			}
		} 

		if(drawState->openState == EASY_PROFILER_DRAW_OPEN || drawState->openTimer >= 0) {

			///////////////////////************ Draw the backdrop *************////////////////////
			Rect2f backDropRect = make_rect2f_min_dim(0, backdropY, resolution.x, backdropHeight);

			float2 center = rect2f_getCenter(backDropRect);
			pushRect(renderer,  make_float3(center.x, center.y, zOffset), get_scale_rect2f(backDropRect), make_float4(0.15, 0.17, 0.20, 1));
			
			////////////////////////////////////////////////////////////////////
			
			if(gameState->keys.keys[KEY_ESCAPE] == MOUSE_BUTTON_PRESSED && state->lookingAtSingleFrame) {
				state->lookingAtSingleFrame = 0;
			}

			float graphWidth = 0.8f*resolution.x;
			int level = 0;

			if(drawState->drawType == EASY_PROFILER_DRAW_OVERVIEW) {
				if(state->lookingAtSingleFrame) {
					assert(DEBUG_global_ProfilePaused);//has to be paused

					///////////////////////*********** Scroll bar**************////////////////////
					
					float4 color = make_float4(0.72, 0.37, 0.26, 1);   // Classic copper red;
					float xMin = 0.1f*resolution.x;
					float xMax = 0.9f*resolution.x;

					float handleY = 0.06f*resolution.y;

					float handleWidth = (1.0f / drawState->zoomLevel)*graphWidth;
					
					if(handleWidth < 0.01*resolution.x) {
						handleWidth = 0.01*resolution.x;
					}
					float handleHeight = 0.04f*resolution.y;

					Rect2f r = make_rect2f_min_dim(xMin + drawState->xOffset*graphWidth, handleY + backdropY, handleWidth, handleHeight);

					if(in_rect2f_bounds(r, easyInput_mouseToResolution_originLeftBottomCorner(mouseP_01, resolution))) {
						color = make_float4(1, 1, 1, 1);
						if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
							drawState->holdingScrollBar = true;
							//TODO:
							// drawState->grabOffset = easyInput_mouseToResolution_originLeftBottomCorner(keyStates, resolution).x - (drawState->xOffset*graphWidth + xMin);
							assert(drawState->grabOffset >= 0); //xOffset is the start of the handle
						}					
					} 

					if(drawState->holdingScrollBar) {
						color = make_float4(0, 1, 0, 1);

						float goalPos = ((easyInput_mouseToResolution_originLeftBottomCorner(mouseP_01, resolution).x - drawState->grabOffset) - xMin) / graphWidth;
						//update position
						drawState->xOffset = lerp(drawState->xOffset, goalPos, make_lerpTValue(0.4f));
					}

					///////////////////////************* Clamp the scroll bar between the range ************////////////////////
					//NOTE(ollie): Clamp the min range
					if(drawState->xOffset < 0) {
						drawState->xOffset = 0.0f;
					} 

					float maxPercent = ((graphWidth - handleWidth) / graphWidth);
					//NOTE(ollie): Clamp the max range
					if(drawState->xOffset > maxPercent) {
						drawState->xOffset = maxPercent; 
					} 

					//NOTE(ollie): Update the offset when we scroll & the handle changes size
					if(drawState->xOffset*graphWidth + handleWidth > graphWidth) {
						drawState->xOffset = (graphWidth - handleWidth) / graphWidth;
					}

					///////////////////////************* Get the scroll bar percent ************////////////////////

					float dividend = (graphWidth - handleWidth);
					if(dividend == 0) {
						drawState->scrollPercent = 0.0f;
					} else {
						drawState->scrollPercent = drawState->xOffset*graphWidth / dividend;	
					}

					////////////////////////////////////////////////////////////////////
					

					if(gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED) {
						drawState->holdingScrollBar = false;
					}

					///////////////////////************* Draw the scroll bar handle ************////////////////////
					//recalculate the handle again
					r = make_rect2f_min_dim(xMin + drawState->xOffset*graphWidth, handleY + backdropY, handleWidth, handleHeight);

					float2 center = get_centre_rect2f(r);
					pushRect(renderer,  make_float3(center.x, center.y, zOffset), get_scale_rect2f(r), color);
					
					////////////////////////////////////////////////////////////////////

					float defaultGraphWidth = graphWidth; 
					graphWidth *= drawState->zoomLevel;

					EasyProfile_FrameData *frame = state->lookingAtSingleFrame;

					///////////////////////************ Draw infomration for frame*************////////////////////

					float xAt = 0.05*resolution.x;
					float yAt = resolution.y - 2*gameState->mainFont.fontHeight;

					char *formatString = "Frame Information:\nTotal Milliseconds for frame: %f";

					char frameInfoString[1028];
					snprintf(frameInfoString, arrayCount(frameInfoString), formatString, state->lookingAtSingleFrame->millisecondsForFrame);
					
					renderText(renderer, &gameState->mainFont, frameInfoString, make_float2(xAt, yAt), fontSize, make_float4(0.7, 0.7, 0.7, 1)); 

					yAt -= 3*gameState->mainFont.fontHeight;

					////////////////////////////////////////////////////////////////////

					for(int i = 0; i < frame->timeStampAt; ++i) {

						EasyProfile_TimeStamp ts = frame->data[i];

						if(ts.type == EASY_PROFILER_PUSH_SAMPLE) {
							level++;
						} else {

							//NOTE: This was to stop you scrolling in too fast, but decided to get rid of it
							//(1.0f / drawState->zoomLevel)*
							drawState->zoomLevel += 0.01*gameState->scrollDp*MathMaxf(dt, 1.0f / 30.0f);

							if(drawState->zoomLevel < 1.0f) { drawState->zoomLevel = 1.0f; }
							if(drawState->zoomLevel > 10000.0f) { drawState->zoomLevel = 1000.0f; }

							assert(ts.type == EASY_PROFILER_POP_SAMPLE);
							assert(level > 0);
							
							float percent = (float)ts.timeStamp / (float)frame->countsForFrame;
							assert(percent <= 1.0f);

							float percentAcross = (float)(ts.beginTimeStamp - frame->beginCountForFrame) / (float)frame->countsForFrame; 

							float barWidth = percent*graphWidth;
							float barHeight = 0.05f*resolution.y;

							//NOTE(ollie): If the bar can actually been seen do we draw it 
							if(barWidth > DRAW_PROFILER_MIN_BAR_WIDTH) 
							{ //pixels in a fuax resolution projection

								float expandedSize = (graphWidth - defaultGraphWidth);
								float xStart = 0.1f*resolution.x - drawState->scrollPercent*expandedSize;
								float yStart = 0.1f*resolution.y;

								Rect2f r = make_rect2f_min_dim(xStart + percentAcross*graphWidth, yStart + level*barHeight + backdropY, barWidth, barHeight);

								float4 color = colors[i % arrayCount(colors)];
								if(in_rect2f_bounds(r, easyInput_mouseToResolution_originLeftBottomCorner(mouseP_01, resolution))) {

									///////////////////////*********** Draw the name of it **************////////////////////
									char digitBuffer[1028] = {};
									sprintf(digitBuffer, "%d", ts.lineNumber);

									assert(ts.timeStamp >= 0);
									assert(ts.totalTime >= 0);
									
									char *formatString = "File name: %s\n\nFunction name: %s\nLine number: %d\nMilliseonds: %f\nTotal Cycles: %lu\n";
									int stringLengthToAlloc = snprintf(0, 0, formatString, ts.fileName, ts.functionName, ts.lineNumber, ts.totalTime, ts.timeStamp) + 1; //for null terminator, just to be sure
									
									char *strArray = pushArray(&globalPerFrameArena, stringLengthToAlloc, char);

									snprintf(strArray, stringLengthToAlloc, formatString, ts.fileName, ts.functionName, ts.lineNumber, ts.totalTime, ts.timeStamp); //for null terminator, just to be sure

									renderText(renderer, &gameState->mainFont, strArray, make_float2(xAt, yAt), fontSize, make_float4(1, 1, 1, 1)); 

									////////////////////////////////////////////////////////////////////

									color = make_float4(1, 1, 1, 1);
								}

								float2 center = get_centre_rect2f(r);
								pushRect(renderer,  make_float3(center.x, center.y, zOffset), get_scale_rect2f(r), color);

								if(drawState->hotIndex == i) {

								}
							}

							level--;
						}
					} 

				} else {
					EasyProfile_FrameData *frame = state->lookingAtSingleFrame;

					//NOTE(ollie): Do a while loop since it is a ring buffer
					//NOTE(ollie):  state->frameAt will be the array is the array we are currently writing in to, so we want to not show this one.  
					s32 index = 0;
					assert(state->frameAt < EASY_PROFILER_FRAMES); 
					if(state->framesFilled < EASY_PROFILER_FRAMES) {
						//We haven't yet filled up the buffer so we will start at 0
						index = 0;
					} else {
						index = (s32)state->frameAt + 1;
						if(index >= EASY_PROFILER_FRAMES) {
							//Loop back to the end of the buffer
							index = 0;
						}
					}

					///////////////////////*********** Loop through first and accumulate the total **************////////////////////
					s64 totalTime = 0;
					s64 beginTime = state->frames[index].beginCountForFrame;
					int tempIndex = index;
					while(tempIndex != state->frameAt) {
						EasyProfile_FrameData *frame = &state->frames[tempIndex];
						////////////////////////////////////////////////////////////////////

						totalTime += frame->countsForFrame;

						////////////////////////////////////////////////////////////////////					
						tempIndex++;
						if(tempIndex == state->framesFilled) {
							if(state->framesFilled < EASY_PROFILER_FRAMES) {
								assert(state->framesFilled == state->frameAt);
								tempIndex = state->frameAt;
							} else {
								assert(tempIndex != state->frameAt);
								tempIndex = 0;
							}
						}
					}
					////////////////////////////////////////////////////////////////////

					float xStart = 0.1f*resolution.x;
					
					
					while(index != state->frameAt) {
						EasyProfile_FrameData *frame = &state->frames[index];
						float4 color = colors[index % arrayCount(colors)];

						float percent = (float)frame->countsForFrame / (float)totalTime;
						assert(frame->beginCountForFrame >= beginTime);
						float percentForStart = (float)(frame->beginCountForFrame - beginTime) / (float)totalTime;

						Rect2f r = make_rect2f_min_dim(xStart + percentForStart*graphWidth, 0.1f*resolution.y + backdropY, percent*graphWidth, 0.1f*resolution.y);
						
						if(in_rect2f_bounds(r, easyInput_mouseToResolution_originLeftBottomCorner(mouseP_01, resolution)) ) {
							color = make_float4(1, 1, 1, 1);

							///////////////////////*********** Draw milliseconds of it **************////////////////////
							char strBuffer[1028] = {};
							snprintf(strBuffer, arrayCount(strBuffer), "Milliseconds for frame: %fms", frame->millisecondsForFrame);

							float xAt = easyInput_mouseToResolution_originLeftBottomCorner(mouseP_01, resolution).x;
							float yAt = easyInput_mouseToResolution_originLeftBottomCorner(mouseP_01, resolution).y + backdropY;

							renderText(renderer, &gameState->mainFont, strBuffer, make_float2(10, 40), fontSize, make_float4(1, 1, 1 ,1)); 
							////////////////////////////////////////////////////////////////////

							if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
								state->lookingAtSingleFrame = frame;
								state->queuePause = true;
							}
						}

						float2 center = get_centre_rect2f(r);
						pushRect(renderer,  make_float3(center.x, center.y, zOffset), get_scale_rect2f(r), color);

						index++;
						if(index == state->framesFilled) {
							if(state->framesFilled < EASY_PROFILER_FRAMES) {
								assert(state->framesFilled == state->frameAt);
								index = state->frameAt;
							} else {
								assert(index != state->frameAt);
								index = 0;
							}
						}
					}
				}
			} else {

			}
		}
	}

	// drawState->lastMouseP = easyInput_mouseToResolution_originLeftBottomCorner(keyStates, resolution);

	if(gameState->keys.keys[KEY_1] == MOUSE_BUTTON_PRESSED && gameState->keys.keys[KEY_SHIFT] == MOUSE_BUTTON_DOWN) {
		drawState->openTimer = 0;	
		
		if(drawState->openState == EASY_PROFILER_DRAW_OPEN) {
			drawState->openState = EASY_PROFILER_DRAW_CLOSED;
		} else if(drawState->openState == EASY_PROFILER_DRAW_CLOSED) {
			drawState->openState = EASY_PROFILER_DRAW_OPEN;
		} 
	}
}