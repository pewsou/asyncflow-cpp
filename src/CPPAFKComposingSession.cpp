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

#include "CPPAFKBase.hpp"

namespace AsyncFlowKit {
    void CPPAFKComposingSession::_reconfigureDataqueues(){
        for (CPPAFKThreadpoolSize_t i=0; i<dataQueues.size(); ++i) {
            dataQueues[i]->reset();
            delete dataQueues[i];
        }
        dataQueues.clear();
        CPPAFKThreadpoolSize_t t=routinesHnds[hotReplacementIndexRoutR].size();
        if(t>0){
            dataQueues.push_back(new CPPAFKThreadpoolQueueHyb(callMode));
        }
    }
    void CPPAFKComposingSession::_flush(CPPAFKGarbageCollector& gc){
        itsLock.lock();

        bool res=true;
        while (1) {
            CPPAFKBasicDataObjectWrap* t = queueZero.pull(res);
            if(res==true){
                if(autodelete){
                    itsGC.userDataCollection.castObject(t, std::nullptr_t());
                }
            }
            else{
                break;
            }
        }
        std::vector<CPPAFKThreadpoolQueue*>::iterator it = dataQueues.begin();
        for (; it!=dataQueues.end(); ++it)
        {
            bool success=true;
            while (1)
            {
                CPPAFKBasicDataObjectWrap* t = (*it)->pull(success);
                if(success)
                {
                    busyCount.fetch_sub(1);
                    if(autodelete){
                        itsGC.userDataCollection.castObject(t, std::nullptr_t());
                    }
                }
                else{
                    break;
                }
                
            }
            while(1){
                CPPAFKPriv_WrapBQ<CPPAFKBasicDataObjectWrap*>* t=(*it)->getResidual();
                if(t==std::nullptr_t()){
                    break;
                }
                itsGC.wrapperCollection.castObject(t, std::nullptr_t());
                
            }
            
            (*it)->unoccupy();
        }
        cblk->flushRequested(false);
        itsLock.unlock();
        
    }
    void CPPAFKComposingSession::flush(CPPAFKGarbageCollector& gc){
        cblk->flushRequested(true);
        _flush(gc);
    }
    void CPPAFKComposingSession::cancel(){
        cblk->cancel();
    }
    CPPAFKQSize_t CPPAFKComposingSession::getDataItemsCount(){
        itsLock.lock();
        CPPAFKQSize_t s=0;
        s += queueZero.size();
        std::vector<CPPAFKThreadpoolQueue*>::iterator it = dataQueues.begin();
        for (; it!=dataQueues.end(); ++it)
        {
            s += (*it)->size();
        }
        itsLock.unlock();
        return s;
    }
    bool CPPAFKComposingSession::isBusy(){
        return busyCount.load()>0? true: false;
    }

    eCPPAFKThreadpoolExecutionStatus CPPAFKComposingSession::select(CPPAFKThreadpoolSize_t ii,CPPAFKTPSessionCallback_t& callback, CPPAFKGarbageCollector& gc){
        itsLock.lock();
        bool tval=true;
        if(paused.compare_exchange_strong(tval,false)){
            std::vector<CPPAFKThreadpoolQueue*>::iterator it = dataQueues.begin();
            std::vector<CPPAFKThreadpoolQueue*>::iterator end = dataQueues.end();
            for (;it != end;++it ) {
                (*it)->unoccupy();
            }
        }
        _adoptDataFromZeroQueue();
        
        if(isStopped.load()){
            itsLock.unlock();
            return eAFK_ES_WAS_CANCELLED;
        }
        if(
           getControlBlock()->cancellationRequestedByCallback() ||
           getControlBlock()->cancellationRequestedByStarter()
     
           )
        {
            _resetPriorityQueue();
            {
            CPPAFKThreadpoolSize_t indCancR;
            _switchCancellationHandlers(indCancR);
            
            itsLock.unlock();
            flush(gc);
            cancel();
            _invokeCancellationHandler(cancelHnds[indCancR], cblk->getSessionId());
            }

            DASFKLog_2("Cancelling... Pt 0, session %lld",itsNumId.num64);

            return eAFK_ES_WAS_CANCELLED;
        }
        if(busyCount<=0){
            CPPAFKExpirationRoutine_t expirproc=expirationSummary;
            CPPAFKExpirationCondition* trp=excond;

            if(trp){
                trp->setSample(busyCount);

                if(trp->allConditionsMet()){
                    itsLock.unlock();
                    DASFKLog_2("<1> Expiring session %lld" ,itsNumId.num64);
                    flush(gc);
                    cancel();
                    if(expirationSummary != std::nullptr_t()){
                        expirationSummary();
                    }

                    internalCancellationHandler(cblk->getSessionId());

                    return eAFK_ES_WAS_CANCELLED;
                }
            }
            
        }

        itsLock.unlock();
        
        while(1){
            if(getControlBlock()->cancellationRequestedByCallback() ||
               getControlBlock()->cancellationRequestedByStarter()
               ){
                itsLock.lock();
                _resetPriorityQueue();
                {
                CPPAFKThreadpoolSize_t indCancR;
                _switchCancellationHandlers(indCancR);
                
                itsLock.unlock();
                flush(gc);
                cancel();
                _invokeCancellationHandler(cancelHnds[indCancR],cblk->getSessionId());
                }

                DASFKLog_2("[1] Cancelling... session %lld",itsNumId.num64);
                break;
            }
            if( getControlBlock()->flushRequested() ){
                _flush(gc);
                
                DASFKLog_2("[2] Flushing... session %lld",itsNumId.num64);
            }
            CPPAFKThreadpoolQueue* q=std::nullptr_t();
            itsLock.lock();
            
                CPPAFKThreadpoolSize_t indRoutR=0;
                bool switched=_switchRoutinesHandlers(indRoutR);
                if(switched){
                    _reconfigureDataqueues();
                    _resetPriorityQueue();
                }
            
            if(dataQueues.size()>0){
                q=dataQueues[0];
            }
            itsLock.unlock();
            
            std::int64_t itemIndex=-1;

            CPPAFKPriv_QStatusPacket qstat;
            
            if(q != std::nullptr_t()){
                CPPAFKBasicDataObjectWrap* result=q->pullAndOccupyWithId(ii, itemIndex, qstat);
                
                if(qstat.validrv)
                {
                    itsLock.lock();
                    CPPAFKExpirationRoutine_t expirproc=expirationSummary;
                    CPPAFKExpirationCondition* trp=excond;
                    CPPAFKBasicDataObjectWrap* r0=result;
                    itsLock.unlock();

                    q->unoccupy();
                    
                    for (std::vector<CPPAFKExecUnitWrap>::iterator it=routinesHnds[indRoutR].begin(); it != routinesHnds[indRoutR].end();++it)
                    {
                        
                        switch (runprioML) {
                            case AFK_RUN_LAMBDA_ONLY:
                            {
                                CPPAFKExecutableRoutine_t* eproc=&(it->runL);
                                if(eproc == std::nullptr_t()){
                                    break;
                                }
                                result = (*eproc)(r0, *cblk, cblk->getSessionId());
                                if(autodelete){
                                    if(result != r0){
                                        if(r0 != std::nullptr_t()){
                                            gc.userDataCollection.castObject(r0, std::nullptr_t());
                                        }
                                        r0=result;
                                    }
                                }
                            }
                                break;
                            case AFK_RUN_METHOD_ONLY:
                            {
                                result=it->runM(r0, *cblk, cblk->getSessionId());
                                if(autodelete){
                                    if(result != r0){
                                        if(r0 != std::nullptr_t()){
                                            gc.userDataCollection.castObject(r0, std::nullptr_t());
                                        }
                                        r0=result;
                                    }
                                }
                            }
                                break;
                            case AFK_RUN_LAMBDA_THEN_METHOD:
                            {
                                CPPAFKExecutableRoutine_t* eproc=&(it->runL);
                                if(eproc == std::nullptr_t()){
                                    break;
                                }
                                result=(*eproc)(r0, *cblk, cblk->getSessionId());
                                if(autodelete){
                                    if(result != r0){
                                        if(r0 != std::nullptr_t()){
                                            gc.userDataCollection.castObject(r0, std::nullptr_t());
                                        }
                                        r0=result;
                                    }
                                }
                                
                                result=it->runM(r0, *cblk, cblk->getSessionId());
                                if(autodelete){
                                    if(result != r0){
                                        if(r0 != std::nullptr_t()){
                                            gc.userDataCollection.castObject(r0, std::nullptr_t());
                                        }
                                        r0=result;
                                    }
                                }
                            }
                                break;
                            case AFK_RUN_METHOD_THEN_LAMBDA:
                            {
                                result=it->runM(r0, *cblk, cblk->getSessionId());
                                if(autodelete){
                                    if(result != r0){
                                        if(r0 != std::nullptr_t()){
                                            gc.userDataCollection.castObject(r0, std::nullptr_t());
                                        }
                                        r0=result;
                                    }
                                }
                                
                                CPPAFKExecutableRoutine_t* eproc=&(it->runL);
                                if(eproc == std::nullptr_t()){
                                    break;
                                }
                                result = (*eproc)(r0, *cblk, cblk->getSessionId());
                                if(autodelete){
                                    if(result != r0){
                                        if(r0 != std::nullptr_t()){
                                            gc.userDataCollection.castObject(r0, std::nullptr_t());
                                        }
                                        r0=result;
                                    }
                                }
                            }
                                break;
                            default:
                                break;
                        }
                        
                        if(cblk->cancellationRequestedByCallback() || cblk->cancellationRequestedByStarter()){
                            if(autodelete && result != std::nullptr_t()){
                                gc.userDataCollection.castObject(result, std::nullptr_t());
                            }
                            itsLock.lock();
                            
                            {
                                CPPAFKThreadpoolSize_t indCancR;
                                _switchCancellationHandlers(indCancR);

                                itsLock.unlock();
                                flush(gc);
                                cancel();
                                if(cancelHnds[indCancR] != std::nullptr_t()){
                                    cancelHnds[indCancR](cblk->getSessionId());
                                }
                            }

                            DASFKLog_2("Cancelling... Pt 2, session %lld",itsNumId.num64);
                            busyCount=0;
                            return eAFK_ES_WAS_CANCELLED;
                        }
                        
                    }

                    if(cblk->cancellationRequestedByCallback() ||
                       cblk->cancellationRequestedByStarter()){
                        if(autodelete && result != std::nullptr_t()){
                            gc.userDataCollection.castObject(result, std::nullptr_t());
                        }
                        itsLock.lock();
                        _resetPriorityQueue();
                        q->unoccupy();

                        {
                            CPPAFKThreadpoolSize_t indCancR;
                            _switchCancellationHandlers(indCancR);
                            
                            itsLock.unlock();
                            flush(gc);
                            cancel();
                            _invokeCancellationHandler(cancelHnds[indCancR], cblk->getSessionId());
                            if(cancelHnds[indCancR] != std::nullptr_t()){
                                cancelHnds[indCancR](cblk->getSessionId());
                            }
                        }

                        DASFKLog_2("[3] Cancelling... , session %lld",itsNumId.num64);
                        break;
                    }
                    
                    if(cblk->flushRequested()){
                        if(autodelete){
                            if(result != std::nullptr_t()){
                                if(autodelete){
                                    gc.userDataCollection.castObject(result, std::nullptr_t());
                                }
                                result=std::nullptr_t();
                            }
                        }
                        
                        q->unoccupy();
                        _flush(gc);
                    }
                    
                    //id res = result;
                    {
                        r0=result;
                        itsLock.lock();
                        CPPAFKThreadpoolSize_t indSumR;
                        _switchSummaryHandlers(indSumR);
                        itsLock.unlock();
                        if(sumHnds[indSumR] != std::nullptr_t()){
                            result=sumHnds[indSumR](r0, *cblk, cblk->getSessionId());
                            if(autodelete){
                                if(result == r0){
                                    if(r0 != std::nullptr_t()){
                                        gc.userDataCollection.castObject(r0, std::nullptr_t());
                                    }
                                }
                                else{
                                    if(r0 != std::nullptr_t()){
                                        gc.userDataCollection.castObject(r0, std::nullptr_t());
                                    }
                                    if(result != std::nullptr_t()){
                                        gc.userDataCollection.castObject(result, std::nullptr_t());
                                    }
                                }
                            }
                            
                        }
                        else{
                            if(autodelete && result != std::nullptr_t()){
                                gc.userDataCollection.castObject(result, std::nullptr_t());
                            }
                        }
                        
                    }
                    
                    busyCount.fetch_sub(1);
                    
                    if(qstat.endbatch && qstat.called){

                        ((CPPAFKThreadpoolQueueHyb*)dataQueues[0]) ->_releaseBlocked();
                        
                    }
                    if(trp){
                        trp->setSample(busyCount);
                        if(trp->someConditionsMet())
                        {
                            DASFKLog_2("<2> Expiring session %lld",itsNumId.num64);
                            flush(gc);
                            cancel();
                            if(expirationSummary != std::nullptr_t()){
                                expirationSummary();
                            }
                            internalCancellationHandler(cblk->getSessionId());
                            break;
                        }
                    }
                    
                }
                else
                {
                    if(qstat.empty){
                        break;
                    }
                }
            }
            else{
                break;
            }
            
        }
        
        return eAFK_ES_HAS_NONE;
    }

    void CPPAFKComposingSession::_invokeCancellationHandler(CPPAFKCancellationRoutine_t cru, CPPAFKNumId identity){

        {
            DASFKLog_2("Cancellation on the way, session %@",identity);
            if(cru!=std::nullptr_t()){
                cru(identity);
            }
            cancelled=true;
        }
    }
    std::uint64_t CPPAFKComposingSession::getRoutinesCount(){
        return 0;
    }
    bool CPPAFKComposingSession::setRoutines(std::vector<CPPAFKExecUnitWrap>& ps){
        itsLock.lock();
        _setRoutines(ps);
        itsLock.unlock();
        return true;
    }
    bool CPPAFKComposingSession::postData(CPPAFK_PrivSharedPrimitives& shprim,CPPAFKBasicDataObjectWrap* data, bool blk){
        bool res=false;
        if(cblk->flushRequested()==true){
            return res;
        }
        itsLock.lock();
        if(dataQueues.size()==0){
            queueZero.castObject(data, std::nullptr_t(), 0);
            cblk->flushRequested(false);
            itsLock.unlock();
            
            return res;
        }
        if(blk)
        {
            //Unavailable in this version
        }
        {
            res=dataQueues[0]->castObject(data,std::nullptr_t(),0);
            
        }

        itsLock.unlock();
        return res;
    }
    bool CPPAFKComposingSession::postDataAsVector(CPPAFK_PrivSharedPrimitives& shprim,std::vector<CPPAFKBasicDataObjectWrap*>& data, bool blk)
    {
        bool res=false;
        if(cblk->flushRequested()==true){
            return res;
        }
        if(data.size()==0){
            if(blk){
                //Unavailable in this version
            }
            cblk->flushRequested(false);
            return res;
        }
        itsLock.lock();
        if(dataQueues.size()==0){
            queueZero.castVector(data, std::nullptr_t(),0);
            itsLock.unlock();
            if(blk){
                //Unavailable in this version
            }
            return res;
        }
        
        if(blk){
            //Unavailable in this version
        }
        {
            res = dataQueues[0]->castVector(data,std::nullptr_t(),0);
        }

        itsLock.unlock();
        return res;
    }
    bool postExecUnitAsVector(std::vector<CPPAFKBasicDataObjectWrap>& data, bool blk){
        return true;
    }

}
