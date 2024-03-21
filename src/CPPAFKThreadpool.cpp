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

//  Copyright © 2019-2024 Boris Vigman. All rights reserved.
//
//

#include "CPPAFKBase.hpp"

#include <iostream>
#include <thread>
#include <pthread.h>

#define CPPAFK_NUM_OF_HW_THREADS (-1)

#if defined (__linux__)

#elif defined(__APPLE__) && defined(__MACH__)
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
typedef struct cpu_set {
    uint32_t    count;
} cpu_set_t;

static inline void
CPU_ZERO(cpu_set_t *cs) { cs->count = 0; }

static inline void
CPU_SET(int num, cpu_set_t *cs) { cs->count |= (1 << num); }

static inline int
CPU_ISSET(int num, cpu_set_t *cs) { return (cs->count & (1 << num)); }

int pthread_setaffinity_np(pthread_t thread, size_t cpu_size,
                           cpu_set_t *cpu_set)
{
    thread_port_t mach_thread;
    int core = 0;
    
    for (core = 0; core < 8 * cpu_size; core++) {
        if (CPU_ISSET(core, cpu_set)) break;
    }
    thread_affinity_policy_data_t policy = { core };
    mach_thread = pthread_mach_thread_np(thread);
    thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
                      (thread_policy_t)&policy, 1);
    return 0;
}
#endif

namespace AsyncFlowKit{
void CPPAFKThreadpoolState::reassignProcs(CPPAFKThreadpoolConfig& tpc, CPPAFKQSize_t reqThreads){

    long proc=0;
    tpcfg.requiredThreadsCount=reqThreads;
    tpc.actualThreadsCount=numCPUs;
    
    CPPAFKThreadpoolConfigRange tcr;
    if(tpc.requiredThreadsCount==0){
        for(proc=0;proc<tpc.actualThreadsCount;++proc){
            tcr.lowBound=0;
            tcr.length=0;
            vectProc2Bounds[proc]=tcr;
        }
        return;
    }
    
    if(tpc.requiredThreadsCount<tpc.actualThreadsCount){
        tpc.share=tpc.actualThreadsCount/tpc.requiredThreadsCount;
        tpc.residue=tpc.actualThreadsCount%tpc.requiredThreadsCount;
        
        for(proc=0;proc<tpc.actualThreadsCount;++proc){
            tcr=vectProc2Bounds[proc];
            tcr.length=1;
            tcr.lowBound=(proc) % tpc.requiredThreadsCount;
            vectProc2Bounds[proc]=tcr;
        }
    }
    else{
        tpc.share = tpc.requiredThreadsCount / tpc.actualThreadsCount;
        tpc.residue = tpc.requiredThreadsCount%tpc.actualThreadsCount;
        long residue=tpc.residue;
        long lb=0;
        for(proc=0;proc<tpc.actualThreadsCount;++proc){
            tcr.lowBound=lb;
            tcr.length=tpc.share;
            if(residue>0){
                tcr.length+=1;
                residue--;
            }
            lb+=tcr.length;
            
            vectProc2Bounds[proc]=tcr;
        }
    }
}
void CPPAFKThreadpoolState::cloneSessionsMap2Vector(){
    std::unordered_map<std::uint64_t, CPPAFKThreadpoolSession*>::iterator iter;
    onlineSessions.clear();
    for (iter=allSessions.begin(); iter != allSessions.end(); ++iter) {
        if(iter->second->isPaused() == false){
            onlineSessions.push_back(iter->second);
        }
    }
}

void tworker( CPPAFKThreadpoolSize_t selector,  CPPAFKThreadpoolState* state);

    void CPPAFKGarbageCollector::dispose(){
        for(CPPAFKQSize_t t=0;t<CPPAFK_DATA_RELEASING_BATCH_SIZE;++t)
        {
            bool success=false;
            CPPAFKBasicDataObjectWrap* ow=userDataCollection.pull(success);
            if(success)
            {
                delete ow;
            }
            else{
                break;
            }
        }
        for(CPPAFKQSize_t t=0;t<CPPAFK_DATA_RELEASING_BATCH_SIZE;++t)
        {
            bool success=false;
            CPPAFKPriv_WrapBQ<CPPAFKBasicDataObjectWrap*>* wc=wrapperCollection.pull(success);
            if(success)
            {
                delete wc;
            }
            else{
                break;
            }
        }
        for(CPPAFKQSize_t t=0;t<CPPAFK_DATA_RELEASING_BATCH_SIZE;++t)
        {
            bool success=false;
            CPPAFKThreadpoolSession* ts=killedSessions.pull(success);
            if(success)
            {
                if (ts->isCancelled() ) {
                    delete ts;
                }
            }
            else{
                break;
            }
        }
    }
    void CPPAFKGarbageCollector::disposeAll(){
        for(CPPAFKQSize_t t=0;;++t)
        {
            bool success=false;
            CPPAFKBasicDataObjectWrap* ow=userDataCollection.pull(success);
            if(success)
            {
                delete ow;
            }
            else{
                break;
            }
        }
        for(CPPAFKQSize_t t=0;;++t)
        {
            bool success=false;
            CPPAFKPriv_WrapBQ<CPPAFKBasicDataObjectWrap*>* wc=wrapperCollection.pull(success);
            if(success)
            {
                delete wc;
            }
            else{
                break;
            }
        }
        for(CPPAFKQSize_t t=0;;++t)
        {
            bool success=false;
            CPPAFKThreadpoolSession* ts=killedSessions.pull(success);
            if(success)
            {
                if (ts->isCancelled() ) {
                    delete ts;
                }
                
            }
            else{
                break;
            }
        }
    }
CPPAFKThreadpool* CPPAFKThreadpool::getInstance()
{
    static CPPAFKThreadpool s;
    return &s;
}

CPPAFKThreadpool::CPPAFKThreadpool()
{
    countPaused=0;
    createThreads(CPPAFK_NUM_OF_HW_THREADS);
}
CPPAFKThreadpool::~CPPAFKThreadpool()
{
    //std::uint32_t tc=threads.size();
    for (; threads.size()>0;) {
        threads[threads.size()-1].join();
        threads.pop_back();
    }
    countPaused=0;
}

void CPPAFKThreadpool::shutdown(){
    CPPAFKThreadpool::getInstance()->_shutdown();
}
bool CPPAFKThreadpool::cancelAllSessions(){
    return CPPAFKThreadpool::getInstance()->_cancelAll();
}
bool CPPAFKThreadpool::pauseAllSessions(){
    return CPPAFKThreadpool::getInstance()->_pauseAll();
};
bool CPPAFKThreadpool::resumeAllSessions(){
    return CPPAFKThreadpool::getInstance()->_resumeAll();
};
void CPPAFKThreadpool::flushAllSessions(){
    CPPAFKThreadpool::getInstance()->_flushAll();
}

bool CPPAFKThreadpool::setProgressRoutineInSession(CPPAFKNumId sessionId, CPPAFKProgressRoutine_t rprogress){
    bool result=false;
    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId.num64);
    
    if(iter != itsState.allSessions.end()){
        result = iter->second->setProgressRoutine(rprogress);
    }
    itsState.sharedPrimitives.lkMutex1.unlock();
    
    return result;
}
bool CPPAFKThreadpool::setRoutinesInSession(CPPAFKNumId sessionId, std::vector<CPPAFKExecUnitWrap>& routines){
    bool result=false;
    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId.num64);
    
    if(iter != itsState.allSessions.end()){
        result = iter->second->setRoutines(routines);
    }
    itsState.sharedPrimitives.lkMutex1.unlock();
    
    return result;
}
bool CPPAFKThreadpool::setCancellationRoutineInSession(CPPAFKNumId sessionId, CPPAFKCancellationRoutine_t rcanc){
    bool result=false;
    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId.num64);
    
    if(iter != itsState.allSessions.end()){
        result = iter->second->setCancellationHandler(rcanc);
    }
    itsState.sharedPrimitives.lkMutex1.unlock();
    
    return result;
}
bool CPPAFKThreadpool::setExecSummaryRoutineInSession(CPPAFKNumId sessionId, CPPAFKExecutableRoutineSummary_t rsum){
    bool result=false;
    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId.num64);
    
    if(iter != itsState.allSessions.end()){
        result = iter->second->setSummary(rsum);
    }
    itsState.sharedPrimitives.lkMutex1.unlock();
    
    return result;
}
bool CPPAFKThreadpool::setExpirationRoutine(CPPAFKNumId sessionId, CPPAFKExpirationRoutine_t rexp){
    bool result=false;
    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId.num64);
    
    if(iter != itsState.allSessions.end()){
        result = iter->second->setExpirationSummary(rexp);
    }
    itsState.sharedPrimitives.lkMutex1.unlock();
    
    return result;
}
    CPPAFKNumId CPPAFKThreadpool::createComposingSession(CPPAFKSessionConfigParams* separams){
        CPPAFKNumId numid;
        if(separams == std::nullptr_t()){
            return numid;
        }
        //decode params
        eCPPAFKBlockingCallMode blkMode=separams->blockCallMode;
        bool dataMode=true;
#ifdef CPPAFK_USE_STR_OBJ_NAME
        std::string name=separams->sessionName;
#endif
        //create session
        CPPAFKComposingSession* se=new CPPAFKComposingSession(blkMode, dataMode, separams->autodelete, itsState.gc
#ifdef CPPAFK_USE_STR_OBJ_NAME
                                                              , name
#endif
                                                              );
        
        se->setSummary(separams->summaryRoutine);
        se->setProgressRoutine(separams->progressRoutine);
        se->setRoutines(separams->routines);
        se->setCancellationHandler(separams->cancellationRoutine);
        se->setInternalCancellationHandler([&](CPPAFKNumId numId){

        });
        se->setExpirationSummary(separams->expSummary);
        se->setExpirationCondition(separams->expCondition);
        //prepareSession;
        bool res=addSession(se, se->getControlBlock()->getSessionId());
        if(!res){
            delete se;
            DASFKLog("Could not create session");
            return numid;
        }

        numid=se->getControlBlock()->getSessionId();
        return numid;
    }
    bool CPPAFKThreadpool::castObjectForSession(CPPAFKNumId sessionId, CPPAFKBasicDataObjectWrap* obj,
                                              CPPAFKExecutionParams* params){
        if( sessionId.num64 == 0){
            return false;
        }
        return postData(obj, sessionId, false);
    }
    bool CPPAFKThreadpool::callObjectForSession(CPPAFKNumId sessionId, CPPAFKBasicDataObjectWrap* obj,CPPAFKExecutionParams* params){
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        
        return false;
    }
    
    bool CPPAFKThreadpool::castVectorForSession(CPPAFKNumId sessionId, std::vector<CPPAFKBasicDataObjectWrap*>& objs,CPPAFKExecutionParams* params){
        if( objs.size() == 0 || sessionId.num64 == 0){
            return false;
        }
        return postDataAsVector(objs, sessionId, false);
    }
    bool CPPAFKThreadpool::callVectorForSession(CPPAFKNumId sessionId, std::vector<CPPAFKBasicDataObjectWrap*>& objs,CPPAFKExecutionParams* params){
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }
    
bool CPPAFKThreadpool::isBusy(){
    return CPPAFKThreadpool::getInstance()->_isBusy();
}
CPPAFKQSize_t CPPAFKThreadpool::runningSessionsCount(){
    return CPPAFKThreadpool::getInstance()->_runningSessionsCount();
}
CPPAFKQSize_t CPPAFKThreadpool::pausedSessionsCount(){
    return CPPAFKThreadpool::getInstance()->_pausedSessionsCount();
}
void CPPAFKThreadpool::_shutdown(){
    itsState.toShutdown=true;
    std::unique_lock<std::mutex> lk1(itsState.cvmutexTsh);
    itsState.cvTsh.wait(lk1,[this]{
        return itsState.shutdownComplete.load();
    });

    lk1.unlock();

}

void CPPAFKThreadpool::createThreads(CPPAFKThreadpoolSize_t numProcs)
{
    if(numProcs == (CPPAFKThreadpoolSize_t)(-1) || numProcs == 0)
    {
        numProcs = std::thread::hardware_concurrency();
    }
    COMASFKLog_2("Cores found:",numProcs);
    itsState.numCPUs=numProcs;
    itsState.tpcfg.actualThreadsCount=itsState.numCPUs;
    itsState.tpcfg.requiredThreadsCount=itsState.numCPUs;
    itsState.tpcfg.residue=itsState.tpcfg.requiredThreadsCount%itsState.tpcfg.actualThreadsCount;
    itsState.tpcfg.share=itsState.tpcfg.requiredThreadsCount/itsState.tpcfg.actualThreadsCount;
    
    threads.clear();
    
    for (unsigned int i=0; i<numProcs; ++i) {
        threads.push_back((std::thread(tworker,i,&itsState)));
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int retcode = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpuset), &cpuset);
        itsState.shutdownCont.fetch_add(1);
        if (retcode != 0) {
            std::cerr << "Error while calling pthread_setaffinity_np() : " << retcode << "\n";
        }
    }
    
}
CPPAFKQSize_t CPPAFKThreadpool::_runningSessionsCount(){
    itsState.sharedPrimitives.lkMutex1.lock();
    CPPAFKQSize_t cc=itsState.onlineSessions.size();
    itsState.sharedPrimitives.lkMutex1.unlock();
    return (cc);
}
CPPAFKQSize_t CPPAFKThreadpool::_pausedSessionsCount(){
    itsState.sharedPrimitives.lkMutex1.lock();
    CPPAFKThreadpoolSize_t cc=countPaused;
    itsState.sharedPrimitives.lkMutex1.unlock();
    return (cc);
}
bool CPPAFKThreadpool::isPausedSession(CPPAFKNumId sessionId){
    bool result=false;
    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId.num64);
    
    if(iter != itsState.allSessions.end()){

        result = iter->second->isPaused();
    }
    itsState.sharedPrimitives.lkMutex1.unlock();
    
    return result;
}
bool CPPAFKThreadpool::_isBusy(){
    bool result=false;
    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    for (iter=itsState.allSessions.begin(); iter!= itsState.allSessions.end(); ++iter) {
        result |=iter->second->isBusy();
        if(result==true){
            break;
        }
    }
    itsState.sharedPrimitives.lkMutex1.unlock();
    return result;
}
bool CPPAFKThreadpool::isBusySession(CPPAFKNumId sessionId){
    bool result=false;
    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId.num64);
    
    if(iter != itsState.allSessions.end()){
        CPPAFKThreadpoolSession* ss=iter->second;
        result = ss->isBusy();;
    }
    itsState.sharedPrimitives.lkMutex1.unlock();
    return result;
}
void CPPAFKThreadpool::getThreadpoolSessionsList(std::vector<CPPAFKNumId>& out){

    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    for (iter=itsState.allSessions.begin(); iter != itsState.allSessions.end(); ++iter) {
        CPPAFKNumId n;
        n.num64=iter->first;
        out.push_back(n);
    }
    itsState.sharedPrimitives.lkMutex1.unlock();
}
CPPAFKQSize_t CPPAFKThreadpool::totalSessionsCount(){
    itsState.sharedPrimitives.lkMutex1.lock();
    CPPAFKThreadpoolSize_t c=itsState.allSessions.size();
    itsState.sharedPrimitives.lkMutex1.unlock();
    return c;
}

CPPAFKQSize_t CPPAFKThreadpool::itemsCountForSession(CPPAFKNumId sessionId){
    CPPAFKThreadpoolSize_t result=0;
    
    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId.num64);
    
    if(iter != itsState.allSessions.end()){
        CPPAFKThreadpoolSession* ss=iter->second;
        result = ss->getDataItemsCount();;
    }
    itsState.sharedPrimitives.lkMutex1.unlock();
    
    return result;
}
bool CPPAFKThreadpool::flushSession(CPPAFKNumId sessionId){
    bool res=false;
    //COMASFKLog_2("Flushing session ",sessionId.num64);
    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId.num64);
    
    if(iter != itsState.allSessions.end()){
        CPPAFKThreadpoolSession* ss=iter->second;
        ss->flush(itsState.gc);;
        res=true;
    }
    itsState.sharedPrimitives.lkMutex1.unlock();

    return res;
}

void CPPAFKThreadpool::_flushAll(){
    COMASFKLog("ASFKGlobalThreadpool: Flushing all sessions");
    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    for (iter=itsState.allSessions.begin(); iter != itsState.allSessions.end(); ++iter) {
        CPPAFKThreadpoolSession* ss=iter->second;
        ss->flush(itsState.gc);;
    }
    itsState.sharedPrimitives.lkMutex1.unlock();
}
 
bool CPPAFKThreadpool::cancelSession(CPPAFKNumId sessionId){
    DASFKLog_2("Cancelling session with ID %@",sessionId);
    bool res=false;
    std::unordered_map<std::uint64_t,CPPAFKThreadpoolSession*>::iterator allsiter;
    
    itsState.sharedPrimitives.lkMutex1.lock();
    allsiter=itsState.allSessions.find(sessionId.num64);
    
    if(allsiter != itsState.allSessions.end()){
        CPPAFKThreadpoolSession* ss=allsiter->second;
        if(ss->isPaused()){
            countPaused.fetch_sub(1);
        }
        ss->cancel();
        itsState.allSessions.erase(allsiter);
        itsState.gc.killedSessions.castObject(ss,std::nullptr_t());
        
        res=true;
    }

    itsState.sharedPrimitives.lkMutex1.unlock();
    return res;
}
bool CPPAFKThreadpool::_cancelAll(){
    WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
    return false;
}
bool CPPAFKThreadpool::pauseSession(CPPAFKNumId sessionId){
    bool res=false;
    DASFKLog_2("Pausing session ",sessionId);
    itsState.sharedPrimitives.lkMutex1.lock();
    std::unordered_map<std::uint64_t, CPPAFKThreadpoolSession*>::iterator iter=itsState.allSessions.find(sessionId.num64);
    if(iter != itsState.allSessions.end()){
        CPPAFKThreadpoolSession* ss=iter->second;
        ss->pause();
        countPaused.fetch_add(1);
        res=true;
    }
    itsState.sharedPrimitives.lkMutex1.unlock();
    return res;
}
bool CPPAFKThreadpool::_pauseAll(){
    bool res=false;
    DASFKLog("Pausing all sessions");
    std::unordered_map<std::uint64_t, CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    if(itsState.allSessions.size()>0){
        for (iter=itsState.allSessions.begin(); iter!=itsState.allSessions.end(); ++iter) {
            iter->second->pause();
            countPaused.fetch_add(1);
        }
        res=true;
    }
    
    itsState.sharedPrimitives.lkMutex1.unlock();
    DASFKLog("All sessions paused");
    return res;
}
bool CPPAFKThreadpool::resumeSession(CPPAFKNumId sessionId) {
    DASFKLog_2("Resuming session ",sessionId);
    bool res=false;
    itsState.sharedPrimitives.lkMutex1.lock();
    std::unordered_map<std::uint64_t, CPPAFKThreadpoolSession*>::iterator iter=itsState.allSessions.find(sessionId.num64);
    // objectForKey:sessionId];
    if(iter != itsState.allSessions.end()){
        CPPAFKThreadpoolSession* ss=iter->second;
        ss->resume();
        countPaused.fetch_sub(1);
        res=true;

    }
    itsState.sharedPrimitives.lkMutex1.unlock();
    return res;
}
bool CPPAFKThreadpool::_resumeAll(){
    bool res=false;
    DASFKLog("Resuming all sessions");
    std::unordered_map<std::uint64_t, CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    if(itsState.allSessions.size()>0){
        for (iter=itsState.allSessions.begin(); iter!=itsState.allSessions.end(); ++iter) {
            iter->second->resume();
            countPaused.fetch_sub(1);
        }
        res=true;
    }

    itsState.sharedPrimitives.lkMutex1.unlock();
    DASFKLog("ASFKGlobalThreadpool: All sessions resumed");
    return res;
}
    
void tworker( CPPAFKThreadpoolSize_t selector, CPPAFKThreadpoolState* state ) {
    CPPAFKQSize_t i=0;

    std::function<bool(CPPAFKNumId)> clbMain=[state](CPPAFKNumId sessionId){
        DASFKLog_2("Stopping session ",sessionId);

        state->sharedPrimitives.lkMutex1.lock();
        CPPAFKThreadpoolConfig tpc1=state->tpcfg;
        state->reassignProcs(tpc1,state->tpcfg.actualThreadsCount);
        state->sharedPrimitives.lkMutex1.unlock();

        return true;
    };

        CPPAFKQSize_t ii=i;
        CPPAFKThreadpoolSize_t selectedSlot=0;
        while(1)
        {
            if(state->toShutdown.load()==true){
                state->shutdownCont.fetch_sub(1);
                if(state->shutdownCont.load()==0){
                    std::unique_lock<std::mutex> lk1(state->cvmutexTsh);
                    state->shutdownComplete=true;

                    state->gc.disposeAll();

                    lk1.unlock();
                    
                }
                state->cvTsh.notify_one();
                return;
            }
            
            bool tval=false;
            if(state->gc.active.compare_exchange_strong(tval,true)){
                state->gc.dispose();
                state->gc.active=false;
            }
            
            state->sharedPrimitives.lkMutex1.lock();
            CPPAFKThreadpoolConfig tpc=state->tpcfg;
            CPPAFKThreadpoolConfigRange tcr;

            if(state->vectProc2Bounds.size() != state->tpcfg.actualThreadsCount){
                state->vectProc2Bounds.clear();
                state->vectProc2Bounds.resize(state->tpcfg.actualThreadsCount);
            }
            
            state->reassignProcs(tpc,state->onlineSessions.size());
            tcr=state->vectProc2Bounds[ii];
            if(tcr.length==0 ||
                state->onlineSessions.size()==0){
                state->sharedPrimitives.lkMutex1.unlock();
                continue;
            }
            selectedSlot=(selectedSlot+1);
            if(tcr.lowBound<=selectedSlot &&
               tcr.length+tcr.lowBound>selectedSlot){
            }
            else{
                selectedSlot=tcr.lowBound;
            }
            if(selector >= state->onlineSessions.size()){
                state->sharedPrimitives.lkMutex1.unlock();
                continue;
            }
            
            CPPAFKThreadpoolSession* ss=state->onlineSessions[selector];//objectAtIndex:selectedSlot];
            if(ss->cancellationRequested()){
                CPPAFKThreadpoolConfig tpc1=state->tpcfg;
                state->cloneSessionsMap2Vector();
                state->reassignProcs(tpc1,state->onlineSessions.size());
                state->sharedPrimitives.lkMutex1.unlock();

                continue;
            }
            
            state->sharedPrimitives.lkMutex1.unlock();
            if(ss != std::nullptr_t() && ss->isPaused()==false)
            {
                ss->select(ii, clbMain,state->gc);
            }
        }
}
bool CPPAFKThreadpool::addSession(CPPAFKThreadpoolSession* aseq, CPPAFKNumId identity){
    bool res=false;
    std::unordered_map<std::uint64_t, CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    iter = itsState.allSessions.find(identity.num64);
    if(aseq != std::nullptr_t() && iter==itsState.allSessions.end()){
        itsState.allSessions[identity.num64]=aseq;
        itsState.onlineSessions.push_back(aseq);
        aseq->deploy();
        res=true;
    }
    
    itsState.sharedPrimitives.lkMutex1.unlock();
    return res;
}
CPPAFKThreadpoolSession* CPPAFKThreadpool::getThreadpoolSessionWithId(CPPAFKNumId identity){
    CPPAFKThreadpoolSession* ss=std::nullptr_t();
    std::unordered_map<std::uint64_t, CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    iter=itsState.allSessions.find(identity.num64);
    if(iter != itsState.allSessions.end()){
        ss=iter->second;
    }
    itsState.sharedPrimitives.lkMutex1.unlock();
    return ss;
}
bool CPPAFKThreadpool::postDataAsVector(std::vector<CPPAFKBasicDataObjectWrap*>& data, CPPAFKNumId sessionId, bool blk){
#ifdef CPPAFK_MSG_DEBUG_ENABLED
    if(blk){
        DASFKLog("Performing blocking call");
    }
    else{
        DASFKLog("Performing non-blocking call");
    }
#endif
    bool res=false;
    std::unordered_map<std::uint64_t, CPPAFKThreadpoolSession*>::iterator iter;
    itsState.sharedPrimitives.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId.num64);
    if(iter != itsState.allSessions.end()){
        CPPAFKThreadpoolSession* ss=iter->second;
        if(blk)
        {
            //Unavailable in this version
        }
        {
            res=ss->postDataAsVector(itsState.sharedPrimitives, data,false);
            itsState.sharedPrimitives.lkMutex1.unlock();
        }
    }
    else{
            itsState.sharedPrimitives.lkMutex1.unlock();
            EASFKLog_2("session not found: ",sessionId);
        }
    return res;
}
bool CPPAFKThreadpool::postData(CPPAFKBasicDataObjectWrap* data, CPPAFKNumId sessionId, bool blk){
#ifdef CPPAFK_MSG_DEBUG_ENABLED
        if(blk){
            DASFKLog("Performing blocking call");
        }
        else{
            DASFKLog("Performing non-blocking call");
        }
#endif
        bool res=false;
        std::unordered_map<std::uint64_t, CPPAFKThreadpoolSession*>::iterator iter;
        itsState.sharedPrimitives.lkMutex1.lock();
        iter=itsState.allSessions.find(sessionId.num64);
        if(iter != itsState.allSessions.end()){
            CPPAFKThreadpoolSession* ss=iter->second;
            if(blk)
            {
                //Unavailable in this version
            }
            {
                res=ss->postData(itsState.sharedPrimitives, data,false);
                itsState.sharedPrimitives.lkMutex1.unlock();
            }
        }
        else{
            itsState.sharedPrimitives.lkMutex1.unlock();
            EASFKLog_2("session not found: ",sessionId);
        }
        return res;
    }

};



