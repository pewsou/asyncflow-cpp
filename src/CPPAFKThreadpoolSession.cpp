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

namespace AsyncFlowKit {
    void CPPAFKThreadpoolSession::_resetPriorityQueue(){
        while (!pq.empty()) {
            pq.pop();
        }
    }
    bool CPPAFKThreadpoolSession::cancellationRequested(){
        return cblk->cancellationRequested();
    }
    bool CPPAFKThreadpoolSession::cancellationRequestedByCallback(){
        return cblk->cancellationRequestedByCallback();
    }
    bool CPPAFKThreadpoolSession::cancellationRequestedByStarter(){
        return cblk->cancellationRequestedByStarter();
    }

    void CPPAFKThreadpoolSession::flush(CPPAFKGarbageCollector& gc){
        itsLock.lock();
        cblk->flushRequested(true);
        if(queueZero.size()>0 && dataQueues.size()>0){
            dataQueues[0]->queueFromQueue(&queueZero);
            queueZero.reset();
            
        }
        itsLock.unlock();
    }
    void CPPAFKThreadpoolSession::pause(){
        itsLock.lock();
        cblk->setPaused(true);
        paused=true;
        itsLock.unlock();
    }
    void CPPAFKThreadpoolSession::resume(){
        itsLock.lock();
        cblk->setPaused(false);
        paused=false;
        itsLock.unlock();
    }
    bool CPPAFKThreadpoolSession::isPaused(){
        return paused;
    }

    CPPAFKQSize_t CPPAFKThreadpoolSession::getDataItemsCount(){
        return 0;
    }
    bool CPPAFKThreadpoolSession::isBusy(){
        return busyCount.load()>0;
    }
    void CPPAFKThreadpoolSession::deploy(){
        itsLock.lock();
        _reconfigureDataqueues();
        itsLock.unlock();
    }
    eCPPAFKThreadpoolExecutionStatus CPPAFKThreadpoolSession::select(CPPAFKThreadpoolSize_t, CPPAFKTPSessionCallback_t&, CPPAFKGarbageCollector& ){
        return eAFK_ES_HAS_NONE;
    }

    CPPAFKControlBlock* CPPAFKThreadpoolSession::getControlBlock(){
        return cblk;
    }
    void CPPAFKThreadpoolSession::_adoptDataFromZeroQueue(){
        if(queueZero.size()>0 && dataQueues.size()>0){
            dataQueues[0]->queueFromQueue(&queueZero);
            sASFKPrioritizedQueueItem qin;
            qin.queueId=0;
            qin.priority=queueZero.size();
            if(qin.priority>0){
                pq.push(qin);
            }
            queueZero.reset();
            
        }
    }

    bool CPPAFKThreadpoolSession::_switchRoutinesHandlers(CPPAFKThreadpoolSize_t& indR){
        bool res=false;
        if(routinesHndReload){
            hotReplacementIndexRoutW += 1;
            hotReplacementIndexRoutW %= 2;
            hotReplacementIndexRoutR += 1;
            hotReplacementIndexRoutR %= 2;
            routinesHndReload=false;
            
            res= true;
        }
        indR=hotReplacementIndexRoutR;
        return res;
    }
    bool CPPAFKThreadpoolSession::_switchSummaryHandlers(CPPAFKThreadpoolSize_t& indR){
        bool res=false;
        if(sumHndReload){
            hotReplacementIndexSumW += 1;
            hotReplacementIndexSumW %= 2;
            hotReplacementIndexSumR += 1;
            hotReplacementIndexSumR %= 2;
            sumHndReload=false;
            res = true;
        }
        indR = hotReplacementIndexSumR;
        return res;
    }
    bool CPPAFKThreadpoolSession::_switchCancellationHandlers(CPPAFKThreadpoolSize_t& indR){
        bool res=false;
        if(cancelHndReload){
            hotReplacementIndexCancW += 1;
            hotReplacementIndexCancW %= 2;
            hotReplacementIndexCancR += 1;
            hotReplacementIndexCancR %= 2;
            cancelHndReload=false;
            res = true;
        }
        indR = hotReplacementIndexCancR;
        return res;
    }

    bool CPPAFKThreadpoolSession::_setRoutines(std::vector<CPPAFKExecUnitWrap>& ps){
        bool res=false;
        routinesHnds[hotReplacementIndexRoutW].clear();
        
        std::uint64_t n=ps.size();
        if(n>0){
            std::vector<CPPAFKExecUnitWrap>::iterator it=ps.begin();
            for(;it!=ps.end();++it){
                routinesHnds[hotReplacementIndexRoutW].push_back(*it);
            }
            
            res=true;
        }
        routinesHndReload=true;
        return res;
    }
    bool CPPAFKThreadpoolSession::setCancellationHandler(CPPAFKCancellationRoutine_t cru){
        if(cru){
            COMASFKLog("ASFKPipelineSession: Setting Cancellation Routine Operator");
            itsLock.lock();
            cancelHnds[hotReplacementIndexCancW]=cru;
            cancelHndReload=true;
            
            itsLock.unlock();
            return true;
        }
        return false;
    }
    void CPPAFKThreadpoolSession::setInternalCancellationHandler(CPPAFKCancellationRoutine_t cru){
        if(cru){
            COMASFKLog("ASFKPipelineSession: Setting Cancellation Routine Operator");
            itsLock.lock();

            internalCancellationHandler=cru;
            
            itsLock.unlock();
        }
    }
    void CPPAFKThreadpoolSession::setExpirationCondition(CPPAFKExpirationCondition* trop){
        if(trop){
            COMASFKLog("ASFKPipelineSession: Setting Expiration Operator");
            itsLock.lock();
            if(excond!=std::nullptr_t())
            {
                delete excond;
            }
            //excond=nil;
            excond=trop;
            itsLock.unlock();
        }
    }
    
    bool CPPAFKThreadpoolSession::setSummary(CPPAFKExecutableRoutineSummary_t sum){
        if(sum){
            itsLock.lock();

            sumHnds[hotReplacementIndexSumW] = sum;
            sumHndReload=true;
          
            itsLock.unlock();
            return true;
        }else{
            WASFKLog("Pass Summary proc is undefined; not stored");
            return false;
        }
    }
    
    bool CPPAFKThreadpoolSession::setRoutines(std::vector<CPPAFKExecUnitWrap>& ps){
        bool res = false;
        itsLock.lock();
        res = _setRoutines(ps);
        
        itsLock.unlock();
        if(res == false){
            WASFKLog("Routines undefined; not stored");
        }
        
        return res;
    }
    
    bool CPPAFKThreadpoolSession::setProgressRoutine(CPPAFKProgressRoutine_t progress){
        if(progress){
            itsLock.lock();
            cblk->setProgressRoutine(progress);
            itsLock.unlock();
            return true;
        }
        else{
            WASFKLog("Progress proc is undefined; not stored");
            return false;
        }
    }
    
    bool CPPAFKThreadpoolSession::setExpirationSummary(CPPAFKExpirationRoutine_t sum){
        if(sum){
            itsLock.lock();
            expirationSummary=sum;
            
            itsLock.unlock();
            return true;
        }else{
            WASFKLog("Expiraion Summary proc is undefined; not stored");
            return false;
        }
    }
    bool CPPAFKThreadpoolSession::isCancelled(){
        return cancelled;
    }
    void CPPAFKThreadpoolSession::_invokeCancellationHandler(CPPAFKCancellationRoutine_t cru, CPPAFKNumId identity){
        bool tval=false;

        
        if(cancelled.compare_exchange_strong(tval,true))
        {
            DASFKLog_2("Cancellation on the way, session %@",identity);
            cru(identity);
        }
    }

}

