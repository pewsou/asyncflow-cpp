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
//  Copyright © 2019-2023 Boris Vigman. All rights reserved.

#include "CPPAFKBase.hpp"

namespace AsyncFlowKit {
    void CPPAFKComposingSession::cancel(){
        
    }
    CPPAFKQSize_t CPPAFKComposingSession::getDataItemsCount(){
        return 0;
    }
    bool CPPAFKComposingSession::isBusy(){
        return false;
    }
    
    void CPPAFKComposingSession::select(std::uint64_t ii,CPPAFKTPSessionCallback_t& callback){
        
    }
    
    void CPPAFKComposingSession::_invokeCancellationHandler(CPPAFKCancellationRoutine_t cru, CPPAFKNumId identity){
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
    std::uint64_t CPPAFKComposingSession::getRoutinesCount(){
        return 0;
    }
    void CPPAFKComposingSession::replaceRoutinesWithArray(std::vector<CPPAFKExecutableRoutine_t>& ps){
        itsLock.lock();
        //[lock lock];
        procs.clear();
        //[procs removeAllObjects];
        for (std::uint64_t i=0; i<ps.size(); ++i) {
            this->procs.push_back(ps[i]);
        }
        for (std::uint64_t i=0; i<dataQueues.size(); ++i) {
            dataQueues[i]->reset();
        }

        itsLock.unlock();
    }
    bool CPPAFKComposingSession::postDataAsVector(std::vector<CPPAFKBasicDataObjectWrap>& data, bool blk)
    {
        return true;
    }

}
