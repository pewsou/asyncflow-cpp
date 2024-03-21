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
namespace AsyncFlowKit{
void CPPAFKControlBlock::cancel(){
    abortByCaller=true;
}
void CPPAFKControlBlock::flushRequested(bool flush){
    flushed=flush;
}

bool CPPAFKControlBlock::flushRequested(){
    return flushed;
}
bool CPPAFKControlBlock::cancellationRequestedByStarter(){
    bool cr=abortByCaller;
    return cr;
}
bool CPPAFKControlBlock::cancellationRequestedByCallback(){
    bool b=abortByCallback;
    return b;
}
void CPPAFKControlBlock::setPaused(bool yesno){
    paused=yesno;
}
bool CPPAFKControlBlock::isPaused(){
    return paused;
}
bool CPPAFKControlBlock::setProgressRoutine(CPPAFKProgressRoutine_t progress){
    if(progress){
        itsLock.lock();
        itsProgressProc=progress;
        itsLock.unlock();
        return true;
    }
    return false;
}
void CPPAFKControlBlock::reset(){

    abortByCallback=false;
    abortByCaller=false;

}

bool CPPAFKControlBlock::cancellationRequested(){
    bool b=abortByCallback|abortByCaller;
    return b;
}

CPPAFKNumId CPPAFKControlBlock::getParentObjectId(){
    return parentId;
}
CPPAFKProgressRoutine_t CPPAFKControlBlock::getProgressRoutine(){
    itsLock.lock();
    CPPAFKProgressRoutine_t p=itsProgressProc;
    itsLock.unlock();
    return p;
}
void CPPAFKControlBlock::stop(){
    abortByCallback=true;
}
    
}
