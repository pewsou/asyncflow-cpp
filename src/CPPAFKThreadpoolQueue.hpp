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

#ifndef CPPAFKThreadpoolQueue_h
#define CPPAFKThreadpoolQueue_h

struct CPPAFKPriv_QStatusPacket{
    bool empty;
    bool endbatch;
    bool called;
    bool validrv;
    CPPAFKPriv_QStatusPacket(){
        empty=false;
        endbatch=false;
        validrv=false;
        called=false;
    }
    
};
class CPPAFKThreadpoolQueue : public CPPAFKQueue<CPPAFKBasicDataObjectWrap*>
{
public:
    std::deque<std::int64_t> deqIndexes;
    std::int64_t occupant;;
    
    virtual void flush(CPPAFKGarbageCollector&)
    {    };
    virtual CPPAFKPriv_WrapBQ<CPPAFKBasicDataObjectWrap*>*  getResidual(){return std::nullptr_t();};

virtual    CPPAFKBasicDataObjectWrap*  pullAndOccupyWithId(std::int64_t itsid, std::int64_t& itemIndex, CPPAFKPriv_QStatusPacket& qstat){
    
        itemIndex=-1;
    
        this->lock.lock();
        if(occupant>=0 && occupant!=itsid){
            qstat.empty=true;
            if(this->itsQ.size()>0){
                qstat.empty=false;
            }
            this->lock.unlock();;
            qstat.validrv=false;
            return std::nullptr_t();
        }
    CPPAFKBasicDataObjectWrap* item=std::nullptr_t();

        if (this->itsQ.size()>0) {
            item=this->itsQ.front();
            itemIndex=deqIndexes.front();
            deqIndexes.pop_front();

            this->itsQ.pop_front();
            qstat.validrv=true;
            if(this->itsQ.size()>0){
                occupant=itsid;
                
                qstat.empty=false;
            }
            else{
                occupant=-1;
                qstat.empty=true;
            }
        }
        else{
            occupant=-1;
            qstat.empty=true;
        }
        this->lock.unlock();;
        return item;
        
    }
    CPPAFKBasicDataObjectWrap* pull(bool& success){
        std::int64_t index=0;
        CPPAFKPriv_QStatusPacket qstat;
        success=false;
        CPPAFKBasicDataObjectWrap* t = pullAndOccupyWithId(-1, index, qstat);
        if(t!=std::nullptr_t()){
            success=true;
        }
        return t;
    }
    void unoccupyWithId(std::int64_t itsid){
        this->lock.lock();
        if(occupant==itsid){
            occupant=-1;
        }
        this->lock.unlock();
    }
    void unoccupy(){
        this->lock.lock();
        occupant=-1;
        this->lock.unlock();
    }
    virtual bool castObject(CPPAFKBasicDataObjectWrap* item,CPPAFKExecutionParams* ex, std::int64_t index){
        this->lock.lock();
        deqIndexes.push_back(index);
        this->itsQ.push_back(item);
        this->lock.unlock();
        return true;
    }
    virtual bool callObject(CPPAFKBasicDataObjectWrap* item,CPPAFKExecutionParams* ex, std::int64_t index){
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }
    virtual bool castVector(std::vector<CPPAFKBasicDataObjectWrap*>& data,CPPAFKExecutionParams* ex, std::int64_t index){
        std::int64_t t=0;
        for (std::vector<CPPAFKBasicDataObjectWrap*>::iterator it=data.begin(); it != data.end(); ++it) {
            deqIndexes.push_back(t);
            t++;
        }
        
        return CPPAFKQueue<CPPAFKBasicDataObjectWrap*>::castVector(data, ex);
    }
    virtual bool callVector(std::vector<CPPAFKBasicDataObjectWrap*>& data,CPPAFKExecutionParams* ex, std::int64_t index){
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        
        return false;
    }
    void reset(){
        this->_reset();
        this->deqIndexes.clear();
    }

};


#endif /* CPPAFKThreadpoolQueue_h */
