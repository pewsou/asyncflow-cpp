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

namespace AsyncFlowKit {
    bool CPPAFKThreadpoolSession::cancellationRequested(){
        return cblk->cancellationRequested();
    }
    bool CPPAFKThreadpoolSession::cancellationRequestedByCallback(){
        return cancellationRequestedByCallback();
    }
    bool CPPAFKThreadpoolSession::cancellationRequestedByStarter(){
        return cblk->cancellationRequestedByStarter();
    }

    void CPPAFKThreadpoolSession::flush(){
        
    }
    void CPPAFKThreadpoolSession::pause(){
        itsLock.lock();
        cblk->setPaused(true);
        itsLock.unlock();
    }
    void CPPAFKThreadpoolSession::resume(){
        itsLock.lock();
        cblk->setPaused(false);
        itsLock.unlock();
    }
    void CPPAFKThreadpoolSession::cancel(){
        
    }
    CPPAFKQSize_t CPPAFKThreadpoolSession::getDataItemsCount(){
        return 0;
    }
    bool CPPAFKThreadpoolSession::isBusy(){
        return false;
    }

    void CPPAFKThreadpoolSession::select(std::uint64_t ii,CPPAFKTPSessionCallback_t& callback){
        
    }

    CPPAFKControlBlock* CPPAFKThreadpoolSession::getControlBlock(){
        return cblk;
    }
    void CPPAFKThreadpoolSession::setCancellationHandler(CPPAFKCancellationRoutine_t cru){
        if(cru){
            COMASFKLog("ASFKPipelineSession: Setting Cancellation Routine Operator");
            itsLock.lock();
            
            //cancellationHandler=nil;
            cancellationHandler=cru;
            
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
    void CPPAFKThreadpoolSession::setSummary(CPPAFKExecutableRoutineSummary_t sum){
        if(sum){
            itsLock.lock();
            
            passSummary=sum;
            
            itsLock.unlock();
        }else{
            WASFKLog("Pass Summary proc is undefined; not stored");
        }
    }
    void CPPAFKThreadpoolSession::setProgressRoutine(CPPAFKProgressRoutine_t progress){
        if(progress){
            itsLock.lock();
            cblk->setProgressRoutine(progress);
            itsLock.unlock();
        }
        else{
            WASFKLog("Progress proc is undefined; not stored");
        }
    }
    void CPPAFKThreadpoolSession::setExpirationSummary(CPPAFKExpirationRoutine_t sum){
        if(sum){
            itsLock.lock();
            expirationSummary=sum;
            itsLock.unlock();
        }else{
            WASFKLog("Expiraion Summary proc is undefined; not stored");
        }
    }
    void CPPAFKThreadpoolSession::_invokeCancellationHandler(CPPAFKCancellationRoutine_t cru, CPPAFKNumId identity){
        bool tval=false;
//        if(cru==nil){
//            return;
//        }
        
        if(cancelled.compare_exchange_strong(tval,true))
        {
            DASFKLog_2("Cancellation on the way, session %@",identity);
            cru(identity);
        }
    }

}


