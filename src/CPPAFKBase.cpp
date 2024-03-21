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
//

#include "CPPAFKBase.hpp"
#include <chrono>
namespace AsyncFlowKit{
CPPAFKNumId CPPAFKUtilities::createRandomNum(){
    CPPAFKNumId n;
    std::random_device randdev;
    std::mt19937_64 rng(randdev());
    std::uniform_int_distribution<std::mt19937_64::result_type> dist(0,std::numeric_limits<uint64_t>::max());
    n.num64 = dist(rng);
    return n;

};
std::chrono::time_point<std::chrono::system_clock> CPPAFKUtilities::getTimePoint(){
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    return now;
    
};
std::uint64_t CPPAFKUtilities::getTimeInterval(std::chrono::time_point<std::chrono::system_clock>& start,std::chrono::time_point<std::chrono::system_clock>& end){
    
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
};
std::string CPPAFKBase::getVersionStatic()
{
    return std::string(CPPAFK_VERSION);
}
std::string CPPAFKBase::getVersion()
{
    return std::string(CPPAFK_VERSION);
}
bool CPPAFKBase::isCancellationRequested(){
    return true;
}
CPPAFKControlBlock* CPPAFKBase::refreshCancellationData(){
    
    return std::nullptr_t();
}
void CPPAFKBase::registerCtrlBlock(CPPAFKControlBlock* cblk){
    if(cblk){
        DASFKLog_2("INFO: Registering session ",cblk->sessionId);
        lkNonLocal. lock();
        itsCtrlBlocks[cblk->getSessionId()]=cblk;
        lkNonLocal.unlock();
    }
}
CPPAFKControlBlock* CPPAFKBase::newCtrlBlock(){
    DASFKLog("Adding session");
    CPPAFKControlBlock* b=new CPPAFKControlBlock(itsNumId,CPPAFKUtilities::createRandomNum(),CPPAFKUtilities::createRandomNum());
     registerCtrlBlock(b);
    
    return b;
}
CPPAFKControlBlock* CPPAFKBase::newCtrlBlock(CPPAFKNumId sessionId,CPPAFKNumId subId){
    COMASFKLog("Adding sub-session");
    CPPAFKControlBlock* b=new CPPAFKControlBlock(itsNumId,sessionId,subId);;
    registerCtrlBlock(b);
    
    return b;
}
void CPPAFKBase::forgetAllCtrlBlocks(){
    DASFKLog(" Unregistering all control blocks");
    lkNonLocal.lock();
    std::map<CPPAFKNumId, CPPAFKControlBlock*, cmpNumericalId64>::iterator iter;
    for (iter=itsCtrlBlocks.begin(); iter!=itsCtrlBlocks.end(); ++iter) {
        delete iter->second;
    }
    itsCtrlBlocks.clear();
    lkNonLocal.unlock();
}
void CPPAFKBase::forgetCtrlBlock(CPPAFKControlBlock* cb){
    if(cb != std::nullptr_t())
    {
        lkNonLocal.lock();
        delete cb;
        lkNonLocal.unlock();
    }
    else{
            WASFKLog(" Failed to forget control block because it was not found");
        }
}
void CPPAFKBase::forgetCtrlBlock(CPPAFKNumId sessionId){
    DASFKLog_2(" Forgetting session ",sessionId);
    lkNonLocal.lock();
    std::map<CPPAFKNumId,CPPAFKControlBlock*,cmpNumericalId64>::iterator iter;
    iter=itsCtrlBlocks.find(sessionId);
    if(iter != itsCtrlBlocks.end()){
        CPPAFKControlBlock* cb=iter->second;
        itsCtrlBlocks.erase(iter);
        delete cb;
        lkNonLocal.unlock();
    }
    else{
        lkNonLocal.unlock();
        WASFKLog(" Failed to forget control block because it was not found");
    }
}
CPPAFKControlBlock* CPPAFKBase::getControlBlockWithId(CPPAFKNumId blkId){
    lkNonLocal.lock();
    std::map<CPPAFKNumId,CPPAFKControlBlock*,cmpNumericalId64>::iterator iter;
    iter=itsCtrlBlocks.find(blkId);
    if(iter != itsCtrlBlocks.end()){
        CPPAFKControlBlock* r=iter->second;
        lkNonLocal.unlock();
        return r;
    }

    return std::nullptr_t();;
}


}
