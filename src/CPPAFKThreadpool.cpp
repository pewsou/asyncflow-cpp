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

//  Copyright © 2019-2023 Boris Vigman. All rights reserved.
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
    std::map<CPPAFKNumId, CPPAFKThreadpoolSession*>::iterator iter;
    onlineSessions.clear();
    for (iter=allSessions.begin(); iter != allSessions.end(); ++iter) {
        if(iter->second->isPaused() == false){
            onlineSessions.push_back(iter->second);
        }
    }
}
CPPAFKPriv_EndingTerm & CPPAFKPriv_EndingTerm::getInstance (){
    static CPPAFKPriv_EndingTerm et;
    return et;
}

void tworker( std::uint32_t selector,  CPPAFKThreadpoolState* state);


CPPAFKThreadpool* CPPAFKThreadpool::getInstance()
{
    static CPPAFKThreadpool s;
    return &s;
}

CPPAFKThreadpool::CPPAFKThreadpool()
{
    createThreads(CPPAFK_NUM_OF_HW_THREADS);
}
CPPAFKThreadpool::~CPPAFKThreadpool()
{
    
}
void CPPAFKThreadpool::_reassignProcs(CPPAFKThreadpoolConfig& tpc){
    
}
void CPPAFKThreadpool::createThreads(std::uint32_t numProcs)
{
    if(numProcs == (std::uint32_t)(-1) || numProcs == 0)
    {
        numProcs = std::thread::hardware_concurrency();
    }
    
    itsState.numCPUs=numProcs;
    
    threads.clear();
    
    for (unsigned int i=0; i<numProcs; ++i) {
        threads.push_back((std::thread(tworker,i,&itsState)));
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int retcode = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpuset), &cpuset);
        if (retcode != 0) {
            std::cerr << "Erroroneous call pthread_setaffinity_np() : " << retcode << "\n";
        }
    }
    for (std::thread& t : threads) {
        t.join();
    }
    
}
CPPAFKQSize_t CPPAFKThreadpool::runningSessionsCount(){
    itsState.lkMutex1.lock();
    CPPAFKQSize_t cc=itsState.onlineSessions.size();
    itsState.lkMutex1.unlock();
    return (cc);
}
std::uint64_t CPPAFKThreadpool::pausedSessionsCount(){
    itsState.lkMutex1.lock();
    std::uint64_t cc=itsState.allSessions.size()-itsState.onlineSessions.size();
    itsState.lkMutex1.unlock();
    return (cc);
}
bool CPPAFKThreadpool::isPausedSession(CPPAFKNumId sessionId){
    bool result=false;
    std::map<CPPAFKNumId,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.lkMutex1.lock();
    //iter=itsState.allSessions.find(sessionId);
    
    if(iter != itsState.allSessions.end()){
        //CPPAFKThreadpoolSession* ss=iter->second;
        result = iter->second->isPaused();
    }
    itsState.lkMutex1.unlock();
    
    return result;
}
bool CPPAFKThreadpool::isBusySession(CPPAFKNumId sessionId){
    bool result=false;
    std::map<CPPAFKNumId,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId);
    
    if(iter != itsState.allSessions.end()){
        CPPAFKThreadpoolSession* ss=iter->second;
        result = ss->isBusy();;
    }
    itsState.lkMutex1.unlock();
    return result;
}
void CPPAFKThreadpool::getThreadpoolSessionsList(std::vector<CPPAFKNumId>& out){

    std::map<CPPAFKNumId,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.lkMutex1.lock();
    for (iter=itsState.allSessions.begin(); iter != itsState.allSessions.end(); ++iter) {
        out.push_back(iter->first);
    }
    itsState.lkMutex1.unlock();
}
CPPAFKQSize_t CPPAFKThreadpool::totalSessionsCount(){
    itsState.lkMutex1.lock();
    std::uint64_t c=itsState.allSessions.size();
    itsState.lkMutex1.unlock();
    return c;
}
CPPAFKQSize_t CPPAFKThreadpool::itemsCountForSession(CPPAFKNumId sessionId){
    std::uint64_t result=0;
    
    std::map<CPPAFKNumId,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId);
    
    if(iter != itsState.allSessions.end()){
        CPPAFKThreadpoolSession* ss=iter->second;
        result = ss->getDataItemsCount();;
    }
    itsState.lkMutex1.unlock();
    
    return result;
}
void CPPAFKThreadpool::flushSession(CPPAFKNumId sessionId){
    COMASFKLog_2("Flushing session ",sessionId);
    std::map<CPPAFKNumId,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId);
    
    if(iter != itsState.allSessions.end()){
        CPPAFKThreadpoolSession* ss=iter->second;
        ss->flush();;
    }
    itsState.lkMutex1.unlock();

    COMASFKLog_2("flushed session ",sessionId);
}
void CPPAFKThreadpool::flushAll(){
    COMASFKLog("ASFKGlobalThreadpool: Flushing all sessions");
    std::map<CPPAFKNumId,CPPAFKThreadpoolSession*>::iterator iter;
    itsState.lkMutex1.lock();
    for (iter=itsState.allSessions.begin(); iter != itsState.allSessions.end(); ++iter) {
        CPPAFKThreadpoolSession* ss=iter->second;
        ss->flush();;
    }
    itsState.lkMutex1.unlock();
}

 
void CPPAFKThreadpool::cancelSession(CPPAFKNumId sessionId){
    DASFKLog_2("Cancelling session with ID %@",sessionId);

    std::map<CPPAFKNumId,CPPAFKThreadpoolSession*>::iterator allsiter;
    
    itsState.lkMutex1.lock();
    allsiter=itsState.allSessions.find(sessionId);
    
    if(allsiter != itsState.allSessions.end()){
        CPPAFKThreadpoolSession* ss=allsiter->second;
        ss->cancel();
//        result = true;
        itsState.killedSessions.push_back(ss);
        itsState.allSessions.erase(allsiter);// removeObjectForKey:sessionId];

    }

    itsState.lkMutex1.unlock();
    DASFKLog_2("Session to be cancelled: ",sessionId);
}
void CPPAFKThreadpool::cancelAll(){
    DASFKLog(@"Cancelling all sessions");
    std::map<CPPAFKNumId, CPPAFKThreadpoolSession*>::iterator iter;
    itsState.lkMutex1.lock();
    for (iter=itsState.allSessions.begin(); iter!=itsState.allSessions.end(); ++iter) {
        iter->second->cancel();
        itsState.killedSessions.push_back(iter->second);
    }

    itsState.tpcfg.actualThreadsCount=0;
    CPPAFKThreadpoolConfig tpc=itsState.tpcfg;
    _reassignProcs(tpc);
    itsState.vectProc2Bounds.clear();
    itsState.vectProc2Bounds.resize(itsState.tpcfg.actualThreadsCount);
    itsState.onlineSessions.clear();
    

    itsState.allSessions.clear();// = [NSMutableDictionary new];
    itsState.lkMutex1.unlock();
    DASFKLog("ASFKGlobalThreadpool: All sessions should be cancelled");
}
void CPPAFKThreadpool::pauseSession(CPPAFKNumId sessionId){
    DASFKLog_2("Pausing session ",sessionId);
    itsState.lkMutex1.lock();
    std::map<CPPAFKNumId, CPPAFKThreadpoolSession*>::iterator iter=itsState.allSessions.find(sessionId);
    // objectForKey:sessionId];
        if(iter != itsState.allSessions.end()){
            CPPAFKThreadpoolSession* ss=iter->second;
            ss->pause();
            itsState.cloneSessionsMap2Vector();

        }
        itsState.lkMutex1.unlock();
    DASFKLog_2("Paused session ",sessionId);
}
void CPPAFKThreadpool::pauseAll(){
    DASFKLog("Pausing all sessions");
    std::map<CPPAFKNumId, CPPAFKThreadpoolSession*>::iterator iter;
    itsState.lkMutex1.lock();
    for (iter=itsState.allSessions.begin(); iter!=itsState.allSessions.end(); ++iter) {
        iter->second->pause();
    }
    itsState.cloneSessionsMap2Vector();

    itsState.lkMutex1.unlock();
    DASFKLog("All sessions paused");
}
void CPPAFKThreadpool::resumeSession(CPPAFKNumId sessionId) {
    DASFKLog_2("Resuming session ",sessionId);
    itsState.lkMutex1.lock();
    std::map<CPPAFKNumId, CPPAFKThreadpoolSession*>::iterator iter=itsState.allSessions.find(sessionId);
    // objectForKey:sessionId];
    if(iter != itsState.allSessions.end()){
        CPPAFKThreadpoolSession* ss=iter->second;
        ss->resume();
        itsState.cloneSessionsMap2Vector();

    }
    itsState.lkMutex1.unlock();
    DASFKLog_2("Resumed session ",sessionId);
}
void CPPAFKThreadpool::resumeAll(){
    DASFKLog("Resuming all sessions");
    std::map<CPPAFKNumId, CPPAFKThreadpoolSession*>::iterator iter;
    itsState.lkMutex1.lock();
    for (iter=itsState.allSessions.begin(); iter!=itsState.allSessions.end(); ++iter) {
        iter->second->resume();
    }
    itsState.cloneSessionsMap2Vector();
    //[lkMutexL2 unlock];
    itsState.lkMutex1.unlock();
    DASFKLog("ASFKGlobalThreadpool: All sessions resumed");
}
    
void tworker( std::uint32_t selector, CPPAFKThreadpoolState* state ) {
    //__block NSMutableArray* blocks=[NSMutableArray array];
    
    CPPAFKQSize_t i=0;
//    for (i=0; i<tpcfg.actualThreadsCount;++i)
//    {
    std::function<bool(CPPAFKNumId)> clbMain=[state](CPPAFKNumId sessionId){
        DASFKLog_2("Stopping session ",sessionId);

        //[self _cancelSessionInternally:identity];
        CPPAFKThreadpoolConfig tpc1=state->tpcfg;
        state->lkMutex1.lock();
        state->reassignProcs(tpc1,state->onlineSessions.size());
        state->lkMutex1.unlock();
        //[lkMutexL1 unlock];
        return true;
    };

        std::uint32_t ii=i;
        std::uint32_t selectedSlot=0;
        while(1)
        {
            CPPAFKThreadpoolConfig tpc=state->tpcfg;
            CPPAFKThreadpoolConfigRange tcr;
            state->lkMutex1.lock();

            if(state->vectProc2Bounds.size() != state->tpcfg.actualThreadsCount){
                state->vectProc2Bounds.clear();
                state->vectProc2Bounds.resize(state->tpcfg.actualThreadsCount);
            }
            
            state->reassignProcs(tpc,state->onlineSessions.size());
            tcr=state->vectProc2Bounds[ii];
            if(tcr.length==0 ||
               state->onlineSessions.size()==0){
                state->lkMutex1.unlock();
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
                state->lkMutex1.unlock();
                continue;
            }
            
            CPPAFKThreadpoolSession* ss=state->onlineSessions[selector];//objectAtIndex:selectedSlot];
            if(ss->cancellationRequested()){
                CPPAFKThreadpoolConfig tpc1=state->tpcfg;
                
                state->cloneSessionsMap2Vector();
                state->reassignProcs(tpc1,state->onlineSessions.size());
                state->lkMutex1.unlock();
                //[lkMutexL1 unlock];
                continue;
            }
            
            state->lkMutex1.unlock();
            if(ss != std::nullptr_t())
            {
                ss->select(ii, clbMain);

            }
        }
}
bool CPPAFKThreadpool::addSession(CPPAFKThreadpoolSession* aseq, CPPAFKNumId identity){
    bool res=false;
    std::map<CPPAFKNumId, CPPAFKThreadpoolSession*>::iterator iter;
    itsState.lkMutex1.lock();
    iter = itsState.allSessions.find(identity);
    if(aseq != std::nullptr_t() && iter==itsState.allSessions.end()){
        itsState.allSessions[identity]=aseq;

        itsState.onlineSessions.push_back(aseq);
        res=true;
    }
    
    itsState.lkMutex1.unlock();
    return res;
}
CPPAFKThreadpoolSession* CPPAFKThreadpool::getThreadpoolSessionWithId(CPPAFKNumId identity){
    CPPAFKThreadpoolSession* ss=std::nullptr_t();
    std::map<CPPAFKNumId, CPPAFKThreadpoolSession*>::iterator iter;
    itsState.lkMutex1.lock();
    iter=itsState.allSessions.find(identity);
    if(iter != itsState.allSessions.end()){
        ss=iter->second;
    }
    itsState.lkMutex1.unlock();
    return ss;
}
bool CPPAFKThreadpool::postDataAsVector(std::vector<CPPAFKBasicDataObjectWrap>& data, CPPAFKNumId sessionId, bool blk){
#ifdef CPPAFK_MSG_DEBUG_ENABLED
    if(blk){
        DASFKLog("Performing blocking call");
    }
    else{
        DASFKLog("Performing non-blocking call");
    }
#endif
    bool res=false;
    std::map<CPPAFKNumId, CPPAFKThreadpoolSession*>::iterator iter;
    itsState.lkMutex1.lock();
    iter=itsState.allSessions.find(sessionId);
    if(iter != itsState.allSessions.end()){
        CPPAFKThreadpoolSession* ss=iter->second;
        if(ss->callMode == eCPPAFKBlockingCallMode::AFK_BC_NO_BLOCK && blk){
            COMASFKLog(CPPAFK_STR_MISCONFIG_OP);
        }
        else{
            res=ss->postDataAsVector(data,blk);
        }
    }
    
    itsState.lkMutex1.unlock();

    return res;
}
};


