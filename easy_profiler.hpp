/*
How to use: 

DEBUG_TIME_BLOCK(); //put this at the begining of a scope like a fiunction 
DEBUG_TIME_BLOCK_NAMED(name); //this is if you want to profile just a block

*/
#define EASY_PROFILER_ON 1


// SPDX-License-Identifier: GPL-2.0
u64 _rdtsc(void)
{
    u64 val;

    /*
     * According to ARM DDI 0487F.c, from Armv8.0 to Armv8.5 inclusive, the
     * system counter is at least 56 bits wide; from Armv8.6, the counter
     * must be 64 bits wide.  So the system counter could be less than 64
     * bits wide and it is attributed with the flag 'cap_user_time_short'
     * is true.
     */
    asm volatile("mrs %0, cntvct_el0" : "=r" (val));

    return val;
}

typedef enum {
	EASY_PROFILER_PUSH_SAMPLE,
	EASY_PROFILER_POP_SAMPLE,
} EasyProfile_SampleType;


struct EasyProfile_TimeStamp {
	EasyProfile_SampleType type;

	//NOTE: Key
	u32 lineNumber;
	char *fileName;
	char *functionName;	
	//

	s64 startTimeCounter; //from query performance counter
	float totalTime; //milli seconds

	u64 timeStamp;
	u64 beginTimeStamp;
} ;

#define EASY_PROFILER_SAMPLES_INCREMENT 24000

struct EasyProfile_FrameData {
	//NOTE(ollie): The array to store the frames data

	u32 timeStampAt; //this index into the array
	u32 dataCount; //number of samples in data
	EasyProfile_TimeStamp *data;
	
	//NOTE(ollie): This are the times we take percentages of
	u64 beginCountForFrame;
	u64 countsForFrame;

	s64 beginTimeCountForFrame; //From query performance counter
	float millisecondsForFrame;

};

///////////////////////************* All the frame data for the profiler************////////////////////

#define EASY_PROFILER_FRAMES 64
struct EasyProfiler_State{
	int frameAt;
	int framesFilled;

	EasyProfile_FrameData frames[EASY_PROFILER_FRAMES];

	bool queuePause; //We have to pause at the end of a profiler frame in order to match up PUSH/POP samples
	EasyProfile_FrameData *lookingAtSingleFrame;
};

static inline EasyProfiler_State *EasyProfiler_initProfilerState() {
	//NOTE(ollie): This is a pretty massive struct, not sure what the consequences are 
	u32 bytes = sizeof(EasyProfiler_State);
	EasyProfiler_State *result = (EasyProfiler_State *)malloc(bytes);

	//NOTE(ollie): Clear the array to zero. Note: the data * in each frame has to be zero & datacount has to be zero
	memset(result, 0, bytes);

	//just check this is zero
	assert(result->frames[0].data == 0);
	assert(result->frames[0].dataCount == 0);

	result->frameAt = 0;
	result->framesFilled = 0;

	result->lookingAtSingleFrame = 0;
	result->queuePause = false;

	///////////////////////************ Initialize the array to zero *************////////////////////
	for(int i = 0; i < arrayCount(result->frames); i++) {
		EasyProfile_FrameData *frameData = &result->frames[result->frameAt];

		frameData->timeStampAt = 0;
		frameData->beginCountForFrame = 0;
		frameData->countsForFrame = 0;
	}

	////////////////////////////////////////////////////////////////////

	DEBUG_global_ProfilePaused = false;

	return result;
}

static EasyProfiler_State *DEBUG_globalEasyEngineProfilerState;// = &DEBUG_globalEasyEngineProfilerState_;

////////////////////////////////////////////////////////////////////
static u64 EasyProfile_AddSample(u32 lineNumber, char *fileName, char *functionName, u64 timeStamp, u64 beginTimeStamp, s64 startTimeCounter, float totalTimeMilliseconds, EasyProfile_SampleType type, bool isOverallFrame) {
	EasyProfiler_State *state = DEBUG_globalEasyEngineProfilerState;
	assert(state);

	assert(state->frameAt < EASY_PROFILER_FRAMES);
	EasyProfile_FrameData *frame = &state->frames[state->frameAt];

	
	
	///////////////////////************ Setup the datum for the frame i.e. the overall size of the frame *************////////////////////
	if(isOverallFrame) {
		if(type == EASY_PROFILER_PUSH_SAMPLE) {
			assert(timeStamp == beginTimeStamp);
			frame->beginCountForFrame = beginTimeStamp;
			frame->beginTimeCountForFrame = startTimeCounter;
		} else {
			assert(type == EASY_PROFILER_POP_SAMPLE);
			frame->countsForFrame = timeStamp;
			frame->millisecondsForFrame = totalTimeMilliseconds;
		}
	}


	///////////////////////************ Actually add the sample to the data set *************////////////////////

	//NOTE(ollie): Expand the array
	if(frame->timeStampAt >= frame->dataCount) {
		assert(frame->timeStampAt == frame->dataCount);

		u32 newCount = frame->dataCount + EASY_PROFILER_SAMPLES_INCREMENT;
		EasyProfile_TimeStamp *newArray = (EasyProfile_TimeStamp *)malloc(newCount*sizeof(EasyProfile_TimeStamp));		
 		
 		if(frame->dataCount > 0) {
 			//NOTE(ollie): Copy data over to new array
 			assert(frame->data);
			memcpy(newArray, frame->data, frame->dataCount*sizeof(EasyProfile_TimeStamp));	
		}

		//assign the new array & count
		frame->dataCount = newCount;
		frame->data = newArray;

	}

	assert(frame->timeStampAt < frame->dataCount);
	EasyProfile_TimeStamp *sample = frame->data + frame->timeStampAt++;

	memset(sample, 0, sizeof(EasyProfile_TimeStamp));

	sample->type = type;

	sample->lineNumber = lineNumber;
	sample->fileName = fileName;
	sample->functionName = functionName;	

	sample->startTimeCounter = startTimeCounter;
	sample->totalTime = totalTimeMilliseconds;

	sample->timeStamp = timeStamp;
	sample->beginTimeStamp = beginTimeStamp;

	return timeStamp;
}

class EasyProfileBlock {
	public: 
	u32 lineNumber;
	char *fileName;
	char *functionName;
	bool isOverallFrame;	
	u64 startTimeStamp; 
	s64 startTimeSecondsCount; //This is the performance counter result. We have to convert it to actually make sense. See easy_time.h  

	bool wasPaused;

	EasyProfileBlock(u32 lineNumber, char *fileName, char *functionName, bool isOverallFrame) {
		if(!DEBUG_global_ProfilePaused) {
			// u64 ts = __rdtsc(); //NOTE: WINDOWS version
			u64 ts = _rdtsc();
			s64 startTimeSecondsCount = EasyTime_GetTimeCount(); //Since unix epoch I guess?
			//We just duplicate the ts & timeStart since these are used for the total time & is only for when we POP a sample (not PUSH)
			EasyProfile_AddSample(lineNumber, fileName, functionName, ts, ts, startTimeSecondsCount, 0, EASY_PROFILER_PUSH_SAMPLE, isOverallFrame);

			this->startTimeStamp = ts;
			this->lineNumber = lineNumber;
			this->fileName = fileName;
			this->functionName = functionName;	
			this->isOverallFrame = isOverallFrame;
			this->startTimeSecondsCount = startTimeSecondsCount;

		} else {
			this->isOverallFrame = false;
		}

		this->wasPaused = DEBUG_global_ProfilePaused;
	}

	~EasyProfileBlock() {
		if(!this->wasPaused) {
			// u64 ts = __rdtsc(); //NOTE: Windows version
			u64 ts = _rdtsc();
			s64 timeAt = EasyTime_GetTimeCount();
			assert(timeAt >= this->startTimeSecondsCount);
			assert(ts >= startTimeStamp);
			EasyProfile_AddSample(lineNumber, fileName, functionName, ts - startTimeStamp, this->startTimeStamp, timeAt, EasyTime_GetMillisecondsElapsed(timeAt, this->startTimeSecondsCount), EASY_PROFILER_POP_SAMPLE, isOverallFrame);
		}
	}
};


static inline void EasyProfile_MoveToNextFrame(bool hotKeyWasPressed) {

	EasyProfiler_State *state = DEBUG_globalEasyEngineProfilerState;
	assert(state);

	///////////////////////*********** Use the Pause hot key *************////////////////////
	
	//NOTE(ollie): It has to be here since we know all PUSH samples are matched with POP samples
	if(hotKeyWasPressed) {
	    DEBUG_global_ProfilePaused = !DEBUG_global_ProfilePaused;
	    
	    if(!DEBUG_global_ProfilePaused) {
	    	state->lookingAtSingleFrame = 0;
	    }
	} 

	///////////////////////************* Update if anyone wanted to pause during the frame ************////////////////////

	if(state->queuePause) {
		DEBUG_global_ProfilePaused = true;
	}

	state->queuePause = false;

	////////////////////////////////////////////////////////////////////


	if(!DEBUG_global_ProfilePaused) {
		
		///////////////////////************ Move to the next frame if we aren't paused *************////////////////////

		//NOTE(ollie): We advanced the index & count, & warp the index if it is overflown 
		state->frameAt++;
		state->framesFilled++;

		if(state->framesFilled >= EASY_PROFILER_FRAMES) {
			//make sure the frames filled is equal to the value. Not really needed.
			state->framesFilled = EASY_PROFILER_FRAMES;
		}

		if(state->frameAt >= EASY_PROFILER_FRAMES) {
			assert(state->frameAt == EASY_PROFILER_FRAMES);
			state->frameAt = 0;
		}

		EasyProfile_FrameData *frameData = &state->frames[state->frameAt];

		frameData->timeStampAt = 0;
		frameData->beginCountForFrame = 0;
		frameData->countsForFrame = 0;
		frameData->beginTimeCountForFrame = 0;
		frameData->millisecondsForFrame = 0;
	}
}
#if EASY_PROFILER_ON
#define DEBUG_TIME_BLOCK() EasyProfileBlock pb_##__FILE__##__LINE__##__FUNCTION__(__LINE__, __FILE__, (char *)__FUNCTION__, false); 
#define DEBUG_TIME_BLOCK_FOR_FRAME_BEGIN(varName, functionName) DEBUG_globalEasyEngineProfilerState = EasyProfiler_initProfilerState(); EasyProfileBlock *varName = new EasyProfileBlock(__LINE__, __FILE__, functionName, true); 
#define DEBUG_TIME_BLOCK_FOR_FRAME_START(varName, functionName) varName = new EasyProfileBlock(__LINE__, __FILE__, functionName, true); 
#define DEBUG_TIME_BLOCK_FOR_FRAME_END(varName, hotKeyWasPressed) delete(varName); EasyProfile_MoveToNextFrame(hotKeyWasPressed); 
#define DEBUG_TIME_BLOCK_NAMED(name) EasyProfileBlock pb_##__FILE__##__LINE__##__FUNCTION__(__LINE__, __FILE__, name, false);
#else 
#define DEBUG_TIME_BLOCK() //just compile out 
#define DEBUG_TIME_BLOCK_NAMED(name)
#define DEBUG_TIME_BLOCK_FOR_FRAME_BEGIN(varName, functionName)
#define DEBUG_TIME_BLOCK_FOR_FRAME_START(varName, functionName)
#define DEBUG_TIME_BLOCK_FOR_FRAME_END(varName, hotKeyWasPressed)
#endif
