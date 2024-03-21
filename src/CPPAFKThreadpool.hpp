/*
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as
 published by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.
 
 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//  Created by Boris Vigman.
//  Copyright © 2019-2024 Boris Vigman. All rights reserved.

#ifndef CPPAFKThreadpool_h
#define CPPAFKThreadpool_h

struct CPPAFKThreadpoolConfig{
    CPPAFKQSize_t share;
    CPPAFKQSize_t residue;
    CPPAFKQSize_t actualThreadsCount;
    CPPAFKQSize_t requiredThreadsCount;
};

struct CPPAFKThreadpoolConfigRange{
    CPPAFKThreadpoolSize_t lowBound;
    CPPAFKThreadpoolSize_t length;
};

typedef  std::function<bool(CPPAFKNumId)> CPPAFKTPSessionCallback_t;

struct sASFKPrioritizedQueueItem{
    CPPAFKQSize_t priority;
    CPPAFKQSize_t queueId;
};
class CPPAFKComparePriorities {
public:
    bool operator()(sASFKPrioritizedQueueItem& pq1, sASFKPrioritizedQueueItem& pq2)
    {
        if (pq1.priority <= pq2.priority) return true;
        return false;
    }
};

struct CPPAFK_PrivSharedPrimitives{
    std::mutex lkMutex1;
};

class CPPAFKThreadpoolSession;
struct CPPAFKGarbageCollector{
    std::atomic<bool> active;
    CPPAFKQueue<CPPAFKBasicDataObjectWrap*> userDataCollection;
    CPPAFKQueue<CPPAFKPriv_WrapBQ<CPPAFKBasicDataObjectWrap*>*> wrapperCollection;
    CPPAFKQueue<CPPAFKThreadpoolSession*> killedSessions;
    CPPAFKGarbageCollector(){
        active=false;
    }
    void dispose();
    void disposeAll();
};

#include "CPPAFKThreadpoolQueue.hpp"

class CPPAFKThreadpoolSession:public CPPAFKBase{
protected:
    CPPAFKBasicDataObjectWrap itsTerminalObj;
    
    std::priority_queue<sASFKPrioritizedQueueItem, std::vector<sASFKPrioritizedQueueItem>, CPPAFKComparePriorities> pq;
    void _resetPriorityQueue();
    void _adoptDataFromZeroQueue();
    
    std::atomic<CPPAFKQSize_t> busyCount;
    CPPAFKThreadpoolSize_t hotReplacementIndexCancR;
    CPPAFKThreadpoolSize_t hotReplacementIndexCancW;
    
    CPPAFKThreadpoolSize_t hotReplacementIndexRoutR;
    CPPAFKThreadpoolSize_t hotReplacementIndexRoutW;
    
    CPPAFKThreadpoolSize_t hotReplacementIndexSumR;
    CPPAFKThreadpoolSize_t hotReplacementIndexSumW;
    
    
    std::vector<CPPAFKCancellationRoutine_t> cancelHnds;
    std::vector<std::vector<CPPAFKExecUnitWrap>> routinesHnds;
    std::vector<CPPAFKExecutableRoutineSummary_t> sumHnds;
    
    CPPAFKThreadpoolQueue queueZero;
    
    std::vector<CPPAFKThreadpoolQueue*> dataQueues;
    
    CPPAFKCancellationRoutine_t internalCancellationHandler;
    CPPAFKExpirationRoutine_t expirationSummary;
    CPPAFKExpirationCondition* excond;
    
    std::mutex itsLock;
    CPPAFKControlBlock* cblk;
    CPPAFKGarbageCollector& itsGC;
    
    std::atomic<bool> cancelHndReload;;
    std::atomic<bool> routinesHndReload;;
    std::atomic<bool> sumHndReload;;
    std::atomic<bool> paused;
    std::atomic<bool> isStopped;
    std::atomic<bool> cancelled;
    eCPPAFKRunPriorityML runprioML;
    eCPPAFKRunPriorityDOR runprioDOR;
    
    bool autodelete;
    
    
    void _invokeCancellationHandler(CPPAFKCancellationRoutine_t cru, CPPAFKNumId identity);
    bool _switchCancellationHandlers(CPPAFKThreadpoolSize_t&);
    bool _switchRoutinesHandlers(CPPAFKThreadpoolSize_t&);
    bool _switchSummaryHandlers(CPPAFKThreadpoolSize_t&);
    virtual void _reconfigureDataqueues()=0;
    bool _setRoutines(std::vector<CPPAFKExecUnitWrap>& ps);
public:
    std::atomic<eCPPAFKBlockingCallMode> callMode;
    CPPAFKThreadpoolSession(bool autodel, CPPAFKGarbageCollector& gc):itsGC(gc){
        cancelHnds.resize(2);
        cancelHnds[CPPAFK_HRI_PRIMARY]=std::nullptr_t();
        cancelHnds[CPPAFK_HRI_SECONDARY]=std::nullptr_t();
        cancelHndReload=false;
        hotReplacementIndexCancW=CPPAFK_HRI_PRIMARY;
        hotReplacementIndexCancR=CPPAFK_HRI_SECONDARY;
        
        hotReplacementIndexRoutW = CPPAFK_HRI_PRIMARY;
        hotReplacementIndexRoutR = CPPAFK_HRI_SECONDARY;
        routinesHnds.resize(2);
        routinesHndReload = false;;
        
        hotReplacementIndexSumW = CPPAFK_HRI_PRIMARY;;
        hotReplacementIndexSumR = CPPAFK_HRI_SECONDARY;
        sumHnds.resize(2);
        sumHnds[CPPAFK_HRI_PRIMARY]=std::nullptr_t();
        sumHnds[CPPAFK_HRI_SECONDARY]=std::nullptr_t();
        sumHndReload=false;
        
        paused = false;
        autodelete=autodel;
        callMode = AFK_BC_NO_BLOCK;
        cblk = new CPPAFKControlBlock();
        cancelled=false;
        isStopped=false;
        excond=std::nullptr_t();

        internalCancellationHandler=std::nullptr_t();
        expirationSummary=std::nullptr_t();
        busyCount=0;
        runprioML=AFK_RUN_LAMBDA_ONLY;
        runprioDOR=AFK_RUNR_STORED_ROUTINE_ONLY;
    }
    
    virtual CPPAFKThreadpoolSize_t getRoutinesCount()=0;
    CPPAFKControlBlock* getControlBlock();
    void setRunPriorityML(eCPPAFKRunPriorityML rp){
        runprioML=rp;
    };
    void setRunPriorityML(eCPPAFKRunPriorityDOR rp){
        runprioDOR=rp;
    };
    bool setExpirationSummary(CPPAFKExpirationRoutine_t sum);
    bool setCancellationHandler(CPPAFKCancellationRoutine_t cru);
    void setInternalCancellationHandler(CPPAFKCancellationRoutine_t cru);
    bool setProgressRoutine(CPPAFKProgressRoutine_t progress);
    void setExpirationCondition(CPPAFKExpirationCondition* trop);
    bool setSummary(CPPAFKExecutableRoutineSummary_t sum);
    virtual bool setRoutines(std::vector<CPPAFKExecUnitWrap>& sum);
    virtual void deploy();
    bool cancellationRequested();
    bool cancellationRequestedByCallback();
    bool cancellationRequestedByStarter();
    virtual void flush(CPPAFKGarbageCollector&);
    void pause();
    void resume();
    virtual void cancel()=0;
    virtual CPPAFKQSize_t getDataItemsCount();
    virtual bool isBusy();
    bool isCancelled();
    bool isPaused();
    virtual bool postDataAsVector(CPPAFK_PrivSharedPrimitives& shprim,std::vector<CPPAFKBasicDataObjectWrap*>& data, bool blk)=0;
    virtual bool postData(CPPAFK_PrivSharedPrimitives& shprim, CPPAFKBasicDataObjectWrap* data, bool blk)=0;

    virtual eCPPAFKThreadpoolExecutionStatus select(CPPAFKThreadpoolSize_t, CPPAFKTPSessionCallback_t&, CPPAFKGarbageCollector&  )=0;
    virtual ~CPPAFKThreadpoolSession(){
        flush(itsGC);
        delete cblk;
    }
};

struct CPPAFKThreadpoolState{
    
    std::condition_variable cvTsh;
    std::mutex cvmutexTsh;
    std::vector<CPPAFKThreadpoolSession*> onlineSessions;
    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>  allSessions;
    CPPAFKGarbageCollector gc;
    CPPAFKThreadpoolConfig tpcfg;
    CPPAFK_PrivSharedPrimitives sharedPrimitives;
    std::vector<CPPAFKThreadpoolConfigRange> vectProc2Bounds;
    std::uint16_t numCPUs;
    std::atomic<std::uint16_t> shutdownCont;
    std::atomic<bool> toShutdown;
    std::atomic<bool> shutdownComplete;
    CPPAFKThreadpoolState(){
        numCPUs = 1;
        toShutdown=false;
        shutdownComplete=false;
        shutdownCont=0;
    };
    ~CPPAFKThreadpoolState(){
        sharedPrimitives.lkMutex1.lock();
        onlineSessions.clear();
        allSessions.clear();
        vectProc2Bounds.clear();
        sharedPrimitives.lkMutex1.unlock();
    };
    void reassignProcs(CPPAFKThreadpoolConfig& tpc, CPPAFKQSize_t reqThreads);
    void cloneSessionsMap2Vector();
};

/**
 *  @brief Main global threadpool.
 */
class CPPAFKThreadpool{

    std::vector<std::thread> threads;
    CPPAFKThreadpoolState itsState;
    std::atomic<CPPAFKQSize_t> countPaused;
protected:
    
    void _shutdown();
    bool _cancelAll();
    bool _pauseAll();
    bool _resumeAll();
    void _flushAll();
    bool _isBusy();
    CPPAFKQSize_t _runningSessionsCount();
    CPPAFKQSize_t _pausedSessionsCount();
public:
    static CPPAFKThreadpool* getInstance();
    /**
     @brief shuts down the whole threadpool, leaves all sessions intact. After the call, no thread should be active.
     */
    static void shutdown();
    /**
     @brief cancels and removes all sessions with all their data.
     */
    static bool cancelAllSessions();
    /**
     @brief pauses all sessions.
     */
    static bool pauseAllSessions();
    /**
     @brief resumes all paused sessions.
     */
    static bool resumeAllSessions();
    /**
     @brief flushes data only from all sessions.
     */
    static void flushAllSessions();
    static bool isBusy();
    static CPPAFKQSize_t runningSessionsCount();
    static CPPAFKQSize_t pausedSessionsCount();
    
protected:
    void createThreads(CPPAFKThreadpoolSize_t numProcs);
    bool setProgressRoutine(CPPAFKNumId sessionId, CPPAFKProgressRoutine_t rprogress);
    bool setExRoutines(CPPAFKNumId sessionId, std::vector<CPPAFKExecutableRoutine_t>& routines);
    bool setCancellationRoutine(CPPAFKNumId sessionId, CPPAFKCancellationRoutine_t rcanc);
    bool setSummaryRoutine(CPPAFKNumId sessionId, CPPAFKExecutableRoutineSummary_t rsum);
    bool setExpirationRoutine(CPPAFKNumId sessionId, CPPAFKExpirationRoutine_t rexp);
    bool postDataAsVector(std::vector<CPPAFKBasicDataObjectWrap*>& data, CPPAFKNumId sessionId, bool blk);
    bool postData(CPPAFKBasicDataObjectWrap* data, CPPAFKNumId sessionId, bool blk);
    CPPAFKThreadpoolSession* getThreadpoolSessionWithId(CPPAFKNumId sessionId);
    bool addSession(CPPAFKThreadpoolSession* aseq, CPPAFKNumId sessionId);
public:
    CPPAFKThreadpool();
    ~CPPAFKThreadpool();
    
    bool isPausedSession(CPPAFKNumId sessionId);
    bool isBusySession(CPPAFKNumId sessionId);
    void getThreadpoolSessionsList(std::vector<CPPAFKNumId>& out);
    CPPAFKNumId createComposingSession(CPPAFKSessionConfigParams* separams);
    
    CPPAFKQSize_t totalSessionsCount();
    
    /**
     Queries number of data items waiting for processng in the given session

     @param sessionId session Id
     @return number of pending items
     */
    CPPAFKQSize_t itemsCountForSession(CPPAFKNumId sessionId);
    /**
     @brief delete all data items waiting for processing in the given session.
     @param sessionId session Id
     @return true if succeeded, false otherwise.
     */
    bool flushSession(CPPAFKNumId sessionId);
    /**
     @brief delete given session; remove all data.
     */
    bool cancelSession(CPPAFKNumId sessionId);
    /**
     @brief temporarily suspend the processing in given session.
     */
    bool pauseSession(CPPAFKNumId sessionId);
    /**
     @brief resume suspended session.
     */
    bool resumeSession(CPPAFKNumId sessionId);
    
    /**
     @brief performing non-blocking invocation for given session; the method will return immediately after starting the processing.
     */
    bool castObjectForSession(CPPAFKNumId sessionId, CPPAFKBasicDataObjectWrap* obj,CPPAFKExecutionParams* params);
    /**
     @brief performing blocking invocation for given session; the method will return once the object ended processing.
     */
    bool callObjectForSession(CPPAFKNumId sessionId, CPPAFKBasicDataObjectWrap* obj,CPPAFKExecutionParams* params);
    /**
     @brief performing non-blocking invocation for given session; the method will return immediately after starting the processing.
     */
    bool castVectorForSession(CPPAFKNumId sessionId, std::vector<CPPAFKBasicDataObjectWrap*>& objs,CPPAFKExecutionParams* params);
    /**
     @brief performing blocking invocation for given session; the method will return once all vector members ended processing.
     */
    bool callVectorForSession(CPPAFKNumId sessionId, std::vector<CPPAFKBasicDataObjectWrap*>& objs,CPPAFKExecutionParams* params);
    /**
     @brief setting processing routines for given session; the routines will be applied to provided data items.
     */
    bool setRoutinesInSession(CPPAFKNumId sessionId, std::vector<CPPAFKExecUnitWrap>& ps);
    /**
     @brief setting progress report routine for given session; the routine should be called from any or all processing routine(s).
     */
    bool setProgressRoutineInSession(CPPAFKNumId sessionId, CPPAFKProgressRoutine_t progress);
    /**
     @brief setting cancellation routine for given session; the routine will be called, once the session is terminated.
     */
    bool setCancellationRoutineInSession(CPPAFKNumId sessionId, CPPAFKCancellationRoutine_t cru);
    /**
     @brief setting summary routine for given session; the routine will be called, for each fully processed item.
     */
    bool setExecSummaryRoutineInSession(CPPAFKNumId sessionId, CPPAFKExecutableRoutineSummary_t esum);

};

struct CPPAFKPriv_Wrapper{
    CPPAFKBasicDataObjectWrap *execDataItem;
    CPPAFKExecUnitWrap *execUnit;
};

/*!
 @brief ASFKBatchingQueue2
 @discussion internal use. Main purpose: providing 'call' methods without unblocking thread waiting to read.
 */
template <typename T> class CPPAFKBatchingQueue2 : public CPPAFKBatchingQueue<T>
{
protected:
    std::deque<CPPAFKPriv_WrapBQ<T>*> deferred;
public:
    CPPAFKPriv_WrapBQ<T>* pullDeferred(){
        CPPAFKPriv_WrapBQ<T>* t=std::nullptr_t();
        this->lock.lock();
        if(deferred.size()>0){
            CPPAFKPriv_WrapBQ<T>* t=deferred.front();
            t->condPredW=true;
            t->discarded=true;
            t->cvW.notify_all();
            deferred.pop_front();
        }
        
        this->lock.unlock();
        return t;
    }
    virtual void flush(CPPAFKGarbageCollector& gc)
    {
        this->lock.lock();
        while (deferred.size()>0) {
            CPPAFKPriv_WrapBQ<T>* t=deferred.front();
            t->condPredW=true;
            t->discarded=true;
            t->cvW.notify_all();
            deferred.pop_front();
            gc.wrapperCollection.castObject(t, std::nullptr_t());
        }
        
        
        this->lock.unlock();
    };
    
    CPPAFKBatchingQueue2<T>(bool blkW){
        this->blockingW=blkW;
    }
    
    T   pullAndBatchStatus(std::int64_t& itemsLeft, bool& endBatch, bool& success)
    {
        endBatch=false;
        this->lock.lock();
        if(this->deferred.size() == 0)
        {
            if(this->itsBQ.size()>0)
            {
                CPPAFKPriv_WrapBQ<T>* item=this->itsBQ.front();
                if(item->many.size() > 0)
                {
                    T subitem=item->many.front();
                    item->many.pop_front();
                    this->netCount.fetch_sub(1);
                    if(item->many.size() == 0){
                        this->itsBQ.pop_front();
                        endBatch=true;
                        if(this->blockingW.load()){
                            this->deferred.push_back(item);
                        }
                        else{
                            {
                                std::unique_lock<std::mutex> lk(item->cvLockW);

                                item->condPredW=true;
                                lk.unlock();
                                
                            }
                            item->cvW.notify_all();
                            this->_sendToRelaxation(item);
                        }
                    }
                    success = true;
                    this->lock.unlock();
                    
                    return subitem;
                }
                
            }
            
        }
        else{
            
        }
        
        this->lock.unlock();
        return T();
    }
    bool castObject(T item, CPPAFKExecutionParams * ex)
    {
        CPPAFKPriv_WrapBQ<T>* wrap=new CPPAFKPriv_WrapBQ<T>;
        wrap->many.push_back(item);
        this->lock.lock();
        this->itsBQ.push_back(wrap);
        this->netCount.fetch_add(1);
        this->lock.unlock();
        return true;
    }
    bool castVector(std::vector<T>& items, CPPAFKExecutionParams * ex)
    {
        CPPAFKPriv_WrapBQ<T>* wrap=new CPPAFKPriv_WrapBQ<T>;
        typename std::vector<T>::iterator it = items.begin();
        for(;it != items.end();++it){
            wrap->many.push_back(*it);
        }
        
        this->lock.lock();
        this->itsBQ.push_back(wrap);
        this->netCount.fetch_add(1);
        this->lock.unlock();
        return true;
    }
    
#pragma mark - Blocking interface
    bool callObject(T& item, CPPAFKExecutionParams * params)
    {
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }
    bool callVector(std::vector<T>& items, CPPAFKExecutionParams * params)
    {
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }
    virtual void releaseFirst()
    {
        this->lock.lock();
        
        if(this->deferred.size()>0)
        {
            CPPAFKPriv_WrapBQ<T>* wrap = this->deferred.front();
  
            
            {
                std::unique_lock<std::mutex> lk(wrap->cvLockW);
                wrap->condPredW=true;
                lk.unlock();
            }
            wrap->cvW.notify_all();
            this->deferred.pop_front();
            this->_sendToRelaxation(wrap);
        }
        this->lock.unlock();
    }
    virtual void releaseAll()
    {
        typename std::deque<CPPAFKPriv_WrapBQ<T>*>::iterator it;
        this->lock.lock();
        for(it=this->deferred.begin();it!=this->deferred.end();++it){
            {
                std::unique_lock<std::mutex> lk(((CPPAFKPriv_WrapBQ<T>*)(*it))->cvLockW);
                ((CPPAFKPriv_WrapBQ<T>*)(*it))->condPredW=true;
            }
            ((CPPAFKPriv_WrapBQ<T>*)(*it))->cvW.notify_all();
            this->_sendToRelaxation(((CPPAFKPriv_WrapBQ<T>*)(*it)));
        }
        
        this->deferred.clear();
        this->lock.unlock();
    };
};
/*!
 @brief ASFKBatchingQueue3
 @discussion internal use. Main purpose: providing 'call' methods without unblocking thread waiting to read.
 */
template <typename T> class CPPAFKBatchingQueue3 : public CPPAFKBatchingQueue2<T>
{
public:
    CPPAFKBatchingQueue3<T>(bool blkW):CPPAFKBatchingQueue2<T>(blkW){
        
    }
    void releaseFirst()
    {
        std::lock_guard<std::mutex> guard(this->lock);
        
        if(this->deferred.size() > 0)
        {
            CPPAFKPriv_WrapBQ<T>* wrap = this->deferred.front();
            {
                std::unique_lock<std::mutex> lk(wrap->cvLockW);
                wrap->condPredW=true;
                lk.unlock();
            }
            wrap->cvW.notify_all();
            this->deferred.pop_front();
            
            this->_sendToRelaxation (wrap);
            
        }

    }
    
    T   pullAndBatchStatus(std::uint64_t& itemsLeft, bool& endBatch, bool& success)
    {
        endBatch=false;
        this->lock.lock();
        if(this->itsBQ.size()>0)
        {
            CPPAFKPriv_WrapBQ<T>* item=this->itsBQ.front();
            if(item->many.size() > 0)
            {
                T subitem=item->many.front();
                item->many.pop_front();
                this->netCount.fetch_sub(1);
                if(item->many.size() == 0){
                    this->itsBQ.pop_front();
                    endBatch=true;
                    if(this->blockingW.load()){
                        this->deferred.push_back(item);
                    }
                    else{
                        std::unique_lock<std::mutex> lk(item->cvLockW);
                        item->condPredW=true;
                        lk.unlock();
                        item->cvW.notify_all();
                        _sendToRelaxation (item);
                    }
                }
                this->lock.unlock();
                return subitem;
            }
            
        }
        
        this->lock.unlock();
        return T();
    }
    
};

class CPPAFKThreadpoolQueueHyb :public CPPAFKThreadpoolQueue
{
    std::deque<std::pair<eQSpecificity, std::int64_t>> qmapper;
    std::atomic<eQSpecificity> lastItem;
    CPPAFKBatchingQueue2<CPPAFKBasicDataObjectWrap*>* blkQ;
    CPPAFKBatchingQueue<CPPAFKBasicDataObjectWrap*>* regQ;
    void _reset(){
        _releaseBlockedAll();
    }
public:
    void flush(CPPAFKGarbageCollector& gc)
    {    };
    virtual CPPAFKPriv_WrapBQ<CPPAFKBasicDataObjectWrap*>*  getResidual()
    {
        CPPAFKPriv_WrapBQ<CPPAFKBasicDataObjectWrap*>* t = std::nullptr_t();
        if(blkQ != std::nullptr_t()){
            t = blkQ->pullDeferred();
        }
        
        return t;
    };
    
    void _begin(){
        this->lock.lock();
    };
    void _commit(){
        this->lock.unlock();
    };
    
    CPPAFKThreadpoolQueueHyb(eCPPAFKBlockingCallMode blockingMode){
        this->blockingW = false;
        this->regQ = new CPPAFKBatchingQueue<CPPAFKBasicDataObjectWrap*>();
        if(blockingMode == AFK_BC_WAIT_FOR_LAST){
            blkQ = new CPPAFKBatchingQueue2<CPPAFKBasicDataObjectWrap*>(true);
            this->blockingW=true;
        }
        else if(blockingMode == AFK_BC_WAIT_FOR_FIRST){
            blkQ = new CPPAFKBatchingQueue3<CPPAFKBasicDataObjectWrap*>(true);
            this->blockingW=true;
        }
        else{
            blkQ = std::nullptr_t();
        }
    }
    
    virtual ~CPPAFKThreadpoolQueueHyb(){
        if(blkQ != std::nullptr_t()){
            blkQ->reset();
            delete blkQ;
        }
        regQ->reset();
        delete regQ;
    }
    void _releaseBlocked(){
        if(blkQ != std::nullptr_t()){
            blkQ->releaseFirst();
        }
    }
    void _releaseBlockedAll(){
        if(blkQ != std::nullptr_t()){
            blkQ->releaseAll();
        }
    }
    CPPAFKQSize_t size(){
        this->lock.lock();
        CPPAFKQSize_t r=this->_size();
        this->lock.unlock();
        return r;
    }
    CPPAFKQSize_t _size(){
        CPPAFKQSize_t r=0;
        if(blkQ != std::nullptr_t()){
            r=blkQ->size();
        }
        r=this->regQ->size()+r;
        return r;
    }
    bool castObject(CPPAFKBasicDataObjectWrap* item,CPPAFKExecutionParams* ex, std::int64_t index){
        this->deqIndexes.push_back(index);
        this->lock.lock();
        qmapper.push_back(std::make_pair(ASFK_E_QSPEC_REG, 1));
        this->regQ->castObject(item,std::nullptr_t());
        this->lock.unlock();
        
        return true;
    }
    bool castVector(std::vector<CPPAFKBasicDataObjectWrap*>& data,CPPAFKExecutionParams* ex, std::int64_t index){

        if(data.size()>0){
            this->lock.lock();
            qmapper.push_back(std::make_pair(ASFK_E_QSPEC_REG, data.size()));
            this->regQ->castVector(data,ex);
            this->lock.unlock();
        }
        return true;
    }
    
    bool callObject(CPPAFKBasicDataObjectWrap* item,CPPAFKExecutionParams* ex, std::int64_t index){
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }
    bool callVector(std::vector<CPPAFKBasicDataObjectWrap*>& data,CPPAFKExecutionParams* ex, std::int64_t index){
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }
    
    CPPAFKBasicDataObjectWrap*   pullAndOccupyWithId(std::int64_t itsid, std::int64_t& itemIndex, CPPAFKPriv_QStatusPacket& qstat){
        itemIndex=-1;
        qstat.endbatch=false;
        qstat.validrv=false;
        this->lock.lock();
        if(this->occupant>=0 && this->occupant!=itsid){
            qstat.empty=true;
            std::int64_t bc=0;
            if(blkQ != std::nullptr_t()){
                bc=blkQ->size();
            }
            if(this->regQ->size() + bc > 0){
                qstat.empty=false;
            }
            this->lock.unlock();
            return std::nullptr_t();
        }
        
        CPPAFKBasicDataObjectWrap* item=std::nullptr_t();
        
        if(qmapper.size()>0){
            std::pair<eQSpecificity, std::int64_t> items=qmapper.front();
            if(items.first==ASFK_E_QSPEC_REG){
                if(lastItem!=ASFK_E_QSPEC_REG){
                    lastItem=ASFK_E_QSPEC_REG;
                    
                }
                if(this->regQ->size()>0){
                    bool success=false;
                    item=this->regQ->pull(success);
                    if(success){
                        itemIndex=items.second;
                        items.second--;
                        if(items.second>0){
                            qmapper.front().second=items.second;
                        }
                        else{
                            qmapper.pop_front();
                        }
                        qstat.validrv=true;
                    }
                    
                }
            }
            else
                if(blkQ != std::nullptr_t()){
                    bool success=false;
                    std::int64_t lib=0;
                    if(lastItem!=ASFK_E_QSPEC_BAT){
                        lastItem=ASFK_E_QSPEC_BAT;
                    }
                    bool endb=false;
                    item=blkQ->pullAndBatchStatus(lib,endb,success);
                    
                    if(success){
                        qstat.validrv=true;
                        qstat.called=true;
                        itemIndex=items.second;
                        items.second--;
                        if(items.second>0){
                            qmapper.front().second=items.second;
                        }
                        else{
                            qmapper.pop_front();
                        }
                        
                    }
                    qstat.endbatch=endb;

                }
            
            if (qstat.validrv) {
                std::int64_t blkc=0;
                if(blkQ != std::nullptr_t()){
                    blkc=blkQ->size();
                }
                if(this->regQ->size()+blkc > 0){
                    this->occupant=itsid;
                    qstat.empty=false;
                }
                else{
                    this->occupant=-1;
                    qstat.empty=true;
                    qstat.endbatch=true;
                }
            }
            else{
                this->occupant=-1;
                qstat.empty=true;
                qstat.endbatch=true;
                
            }
            
        }
        else{
            lastItem=ASFK_E_QSPEC_NONE;
            qstat.empty=true;
            qstat.endbatch=true;
        }
        this->lock.unlock();
        return item;
    }
        
};
#endif /* CPPAFKThreadpool_h */
