
#define MemoryBarrier()  __sync_synchronize()
#define ReadWriteBarrier() { SDL_CompilerBarrier(); }

typedef void ThreadWorkFuncType(void *Data);

struct ThreadWork
{
    ThreadWorkFuncType *FunctionPtr;
    void *Data;
    bool Finished;
};

struct ThreadsInfo {
    int currentThreadId;
    SDL_sem *Semaphore;
     //NOTE: This is 9.6megabytes - not sure if this is alright. It's so the main thread never waits to put work on the queue. 
     //       It could be smaller if the ao mask calucations were batched into jobs as when there are alot it's most likely the whole chunk that's being calculated. 
     //       So there could be two versions - one where just one offs are caluated when they go to draw & one when the chunk is created
    ThreadWork WorkQueue[400000];
    SDL_atomic_t IndexToTakeFrom;
    SDL_atomic_t IndexToAddTo;
};

//TODO(ollie): Make safe for threads other than the main thread to add stuff
void pushWorkOntoQueue(ThreadsInfo *Info, ThreadWorkFuncType *WorkFunction, void *Data) { //NOT THREAD SAFE. OTHER THREADS CAN'T ADD TO QUEUE
    for(;;)
    {
        int OnePastTheHead = (Info->IndexToAddTo.value + 1) % arrayCount(Info->WorkQueue);
        if(Info->WorkQueue[Info->IndexToAddTo.value].Finished && OnePastTheHead != Info->IndexToTakeFrom.value)
        {
            ThreadWork *Work = Info->WorkQueue + Info->IndexToAddTo.value; 
            Work->FunctionPtr = WorkFunction;
            Work->Data = Data;
            Work->Finished = false;
            
            MemoryBarrier();
            ReadWriteBarrier();
            
            ++Info->IndexToAddTo.value %= arrayCount(Info->WorkQueue);
            
            MemoryBarrier();
            ReadWriteBarrier();
            
            SDL_SemPost(Info->Semaphore);
            break;
        }
        else
        {   
            //NOTE(ollie): Queue is full
            // assert(false);
        }
    }
}

ThreadWork *GetWorkOffQueue(ThreadsInfo *Info, ThreadWork **WorkRetrieved)
{
    *WorkRetrieved = 0;
    
    int OldValue = Info->IndexToTakeFrom.value;
    if(OldValue != Info->IndexToAddTo.value)
    {
        uint32_t NewValue = (OldValue + 1) % arrayCount(Info->WorkQueue);
        
        if(SDL_AtomicCAS(&Info->IndexToTakeFrom, OldValue, NewValue) == SDL_TRUE)
        {
            *WorkRetrieved = Info->WorkQueue + OldValue;
            assert(*WorkRetrieved);
        }
    }    
    return *WorkRetrieved;
}


void doThreadWork(ThreadsInfo *Info, int id)
{
    ThreadWork *Work;
    while(GetWorkOffQueue(Info, &Work))
    {
        Work->FunctionPtr(Work->Data);
        assert(!Work->Finished);
        
        MemoryBarrier();
        ReadWriteBarrier();
        
        Work->Finished = true;
    }
}

int platformThreadEntryPoint(void *Info_) {
    ThreadsInfo *Info = (ThreadsInfo *)Info_;

    int id = Info->currentThreadId;
    
    for(;;) {
        doThreadWork(Info, id);
        SDL_SemWait(Info->Semaphore);
    }
    
}

bool isWorkFinished(ThreadsInfo *Info)
{
    bool Result = true;
    for(uint32_t WorkIndex = 0;
        WorkIndex < arrayCount(Info->WorkQueue);
        ++WorkIndex)
    {
        Result &= Info->WorkQueue[WorkIndex].Finished;
        if(!Result) { break; }
    }
    
    return Result;
}


void waitForWorkToFinish(ThreadsInfo *Info)
{
    while(!isWorkFinished(Info))
    {
        doThreadWork(Info, 0);        
    }
}

void initThreadQueue(ThreadsInfo *threadsInfo) {
    //NOTE: Clear data to zero
    memset(threadsInfo, 0, sizeof(ThreadsInfo));

    int numberOfProcessors = SDL_GetCPUCount();
            
    int numberOfUnusedProcessors = (numberOfProcessors - 1); //NOTE(oliver): minus one to account for the one we are on
    
    threadsInfo->Semaphore = SDL_CreateSemaphore(0);
    
    for(int WorkIndex = 0;
        WorkIndex < arrayCount(threadsInfo->WorkQueue);
        ++WorkIndex)
    {
        threadsInfo->WorkQueue[WorkIndex].Finished = true;
    }
    
    SDL_Thread *Threads[12];
    int threadCount = 0;

    int CoreCount = MathMin(numberOfUnusedProcessors, arrayCount(Threads));

    for(int CoreIndex = 0;
        CoreIndex < CoreCount;
        ++CoreIndex)
    {
        threadsInfo->currentThreadId = CoreIndex;
        assert(threadCount < arrayCount(Threads));
        Threads[threadCount++] = SDL_CreateThread(platformThreadEntryPoint, "", threadsInfo);
    }
            
}