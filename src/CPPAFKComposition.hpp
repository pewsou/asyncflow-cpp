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

#ifndef CPPAFKComposition_h
#define CPPAFKComposition_h
#include <queue>

class CPPAFKComposingSession:public CPPAFKThreadpoolSession{
protected:
    friend CPPAFKThreadpool;
    bool datamode;
    void _flush(CPPAFKGarbageCollector& );
#ifdef CPPAFK_USE_STR_OBJ_NAME
    std::string itsName;
#endif
    void _reconfigureDataqueues();
public:
    CPPAFKComposingSession(
                           eCPPAFKBlockingCallMode blkmode,
                           bool datamodeOn,
                           bool autodel,
                           CPPAFKGarbageCollector& gc
    #ifdef CPPAFK_USE_STR_OBJ_NAME
                           , std::string& name
    #endif
                           ):CPPAFKThreadpoolSession(autodel, gc){
    #ifdef CPPAFK_USE_STR_OBJ_NAME
        itsName=name;
    #endif
        callMode=blkmode;
        datamode=datamodeOn;
    }
    ~CPPAFKComposingSession(){
        itsLock.lock();
        routinesHnds[CPPAFK_HRI_PRIMARY].clear();
        routinesHnds[CPPAFK_HRI_SECONDARY].clear();
        itsLock.unlock();
    }
    
    void cancel();
    void flush(CPPAFKGarbageCollector& gc);
    CPPAFKQSize_t getDataItemsCount();
    bool isBusy();
    bool setRoutines(std::vector<CPPAFKExecUnitWrap>& ps);
    bool postData(CPPAFK_PrivSharedPrimitives& shprim,CPPAFKBasicDataObjectWrap* data, bool blk);
    bool postDataAsVector(CPPAFK_PrivSharedPrimitives& shprim, std::vector<CPPAFKBasicDataObjectWrap*>& data, bool blk);
    bool postExecUnitAsVector(std::vector<CPPAFKBasicDataObjectWrap>& data, bool blk);
    eCPPAFKThreadpoolExecutionStatus select(CPPAFKThreadpoolSize_t, CPPAFKTPSessionCallback_t&, CPPAFKGarbageCollector& );
    void _invokeCancellationHandler(CPPAFKCancellationRoutine_t cru, CPPAFKNumId identity);
    CPPAFKThreadpoolSize_t getRoutinesCount();
};

#endif /* CPPAFKComposition_h */
