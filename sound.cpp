#define SDL_AUDIO_CALLBACK(name) void name(void *udata, unsigned char *stream, int len)
typedef SDL_AUDIO_CALLBACK(sdl_audio_callback);

#define AUDIO_MONO 1
#define AUDIO_STEREO 2

struct WavFile {
    unsigned int size;
    unsigned char *data;
    char *fileName;
};

struct PlayingSound {
    WavFile *wavFile;
    unsigned int bytesAt;
    
    bool active;
    float volume; //percent of original volume
    
    //NOTE(ollie): This is the next sound to play if it's looped or continues to next segment of song etc. 
    PlayingSound *nextSound;
    
    //NOTE(ollie): The playing sounds is a big linked list, so this is pointing to the sound that's next into the list
    struct PlayingSound *next;

    bool shouldEnd;
    
    //NOTE: For 3d sounds
    bool attenuate; //for if we lerp from the listner to the sound position
    float3 location;

    //NOTE(ollie): Radius for sound. Inside inner radius, volume at 1, then lerp between inner & outer. 
    float innerRadius;
    float outerRadius;
    //

};

typedef struct {
    //NOTE: Playing sounds lists
    PlayingSound *playingSoundsFreeList;
    PlayingSound *playingSounds;
    //
    
    //For 3d sounds
    // float3 listenerLocation;
    
} EasySound_SoundState;

static EasySound_SoundState *globalSoundState;

PlayingSound *getPlaySound(WavFile *wavFile) {
    PlayingSound *result = globalSoundState->playingSoundsFreeList;
    if(result) {
        globalSoundState->playingSoundsFreeList = result->next;
    } else {
        result = (PlayingSound *)malloc(sizeof(PlayingSound));
        memset(result, 0, sizeof(PlayingSound));
    }
    assert(result);
    
    result->active = true;
    result->volume = 1.0f;
    result->bytesAt = 0;
    result->wavFile = wavFile;
    result->nextSound = 0;
    result->shouldEnd = false;
    
    return result;
}

PlayingSound *playSound(WavFile *wavFile) {
    PlayingSound *result = globalSoundState->playingSoundsFreeList;
    if(result) {
        globalSoundState->playingSoundsFreeList = result->next;
    } else {
        result = (PlayingSound *)malloc(sizeof(PlayingSound));
        memset(result, 0, sizeof(PlayingSound));
    }
    assert(result);
    
    //add to playing sounds. 
    result->next = globalSoundState->playingSounds;
    globalSoundState->playingSounds = result;
    
    result->active = true;
    result->volume = 1.0f;
    result->bytesAt = 0;
    result->wavFile = wavFile;
    result->nextSound = 0;
    result->shouldEnd = false;
    
    return result;
}

void loadWavFile(WavFile *result, char *fileName, SDL_AudioSpec *audioSpec) {
    int desiredChannels = audioSpec->channels;
    
    SDL_AudioSpec* outputSpec = SDL_LoadWAV(fileName, audioSpec, &result->data, &result->size);
    result->fileName = fileName;
    
    ///NOTE: Upsample to Stereo if mono sound
    if(outputSpec->channels != desiredChannels) {
        assert(outputSpec->channels == AUDIO_MONO);
        assert(audioSpec->channels != desiredChannels);
        audioSpec->channels = desiredChannels;
        
        
        unsigned int newSize = 2 * result->size;
        //assign double the data 
        unsigned char *newData = (unsigned char *)calloc(sizeof(unsigned char)*newSize, 1); 
        //TODO :SIMD this 
        Uint16 *samples = (Uint16 *)result->data;
        Uint16 *newSamples = (Uint16 *)newData;
        int sampleCount = result->size/sizeof(Uint16);
        for(int i = 0; i < sampleCount; ++i) {
            Uint16 value = samples[i];
            int sampleAt = 2*i;
            newSamples[sampleAt] = value;
            newSamples[sampleAt + 1] = value;
        }
        result->size = newSize;
        SDL_FreeWAV(result->data);
        result->data = newData;
    }
    /////////////
    
    if(outputSpec) {
        assert(audioSpec->freq == outputSpec->freq);
        assert(audioSpec->format = outputSpec->format);
        assert(audioSpec->channels == outputSpec->channels);   
        assert(audioSpec->samples == outputSpec->samples);
    } else {
        fprintf(stderr, "Couldn't open wav file: %s\n", SDL_GetError());
        assert(!"couldn't open file");
    }

    assert(result->data);
}

#define initAudioSpec(audioSpec, frequency) initAudioSpec_(audioSpec, frequency, audioCallback)

void initAudioSpec_(SDL_AudioSpec *audioSpec, int frequency, sdl_audio_callback *callback) {
    /* Set the audio format */
    audioSpec->freq = frequency;
    audioSpec->format = AUDIO_S16;
    audioSpec->channels = AUDIO_STEREO;
    audioSpec->samples = 4096; 
    audioSpec->callback = callback;
    audioSpec->userdata = NULL;
    assert(callback);
}

bool initAudio(SDL_AudioSpec *audioSpec) {
    bool successful = true;
    SDL_AudioDeviceID id = SDL_OpenAudioDevice(NULL, 0, audioSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
    /* Open the audio device, forcing the desired format */
    if (id == 0) {
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        successful = false;
    } else {
        SDL_PauseAudioDevice(id, 0);
    }
    return successful;
}

SDL_AUDIO_CALLBACK(audioCallback) {
    
    SDL_memset(stream, 0, len);

    int sampleCount = len / 2;

    float *tempMem = (float *)malloc(sampleCount*sizeof(float));
    SDL_memset(tempMem, 0, sampleCount*sizeof(float));
    
    for(PlayingSound **soundPrt = &globalSoundState->playingSounds;
        *soundPrt; 
        ) {
        bool advancePtr = true;
        PlayingSound *sound = *soundPrt;

        if(sound->active) {
            unsigned char *samples = sound->wavFile->data + sound->bytesAt;
            int remainingBytes = sound->wavFile->size - sound->bytesAt;
            
            assert(remainingBytes >= 0);
            
            unsigned int bytesToWrite = (remainingBytes < len) ? remainingBytes: len;

            int samplesToWrite = bytesToWrite / 2;

            //NOTE: This is the Audio Mixer - it sould do everything in float then sample down
            for(int i = 0; i < samplesToWrite; i++) {
                float a = tempMem[i];
                float b = (float)(((Uint16 *)samples)[i]);
                tempMem[i] = a + b;
            }
          
            sound->bytesAt += bytesToWrite;
            
            if(sound->bytesAt >= sound->wavFile->size) {
                assert(sound->bytesAt == sound->wavFile->size);
                if(sound->nextSound) {
                    //TODO: Allow the remaining bytes to loop back round and finish the full duration 
                    sound->active = false;
                    sound->bytesAt = 0;
                    sound->nextSound->active = true;

                    *sound = *sound->nextSound;
                } else {
                    sound->active = false;
                    //remove from linked list
                    advancePtr = false;
                    *soundPrt = sound->next;
                    sound->next = globalSoundState->playingSoundsFreeList;
                    globalSoundState->playingSoundsFreeList = sound;
                }
            }
        }

        if(sound->shouldEnd) {
            advancePtr = false;
            *soundPrt = sound->next;
            sound->next = globalSoundState->playingSoundsFreeList;
            globalSoundState->playingSoundsFreeList = sound;
        }
        
        if(advancePtr) {
            soundPrt = &((*soundPrt)->next);
        }
    }

    Uint16 *s = (Uint16 *)stream;
    for(int i = 0 ; i < sampleCount; ++i) {
        s[i] = (Uint16)tempMem[i];
    }

    free(tempMem);
}