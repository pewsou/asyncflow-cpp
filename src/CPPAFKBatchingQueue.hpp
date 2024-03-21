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

#ifndef CPPAFKBatchingQueue_h
#define CPPAFKBatchingQueue_h
/**
 @class ASFKBatchingQueue
 @brief provides queueing functionality for batches.
 @discussion any call enqueues exactly one object (batch) which in turn can contain single object or collection. Each dequeuing call returns exactly one object from the earliest batch until it is exgausted; after that next batch is tapped. Main reason for such behavior: blocking calls. If blocking mode is disabled then calls do the same as casts. Otherwise, call will block until all of its batch is consumed. Batch size management supported.
 */
template <typename T> class CPPAFKBatchingQueue : public CPPAFKQueue<T>
{
protected:
    std::atomic<CPPAFKQSize_t> batchLimitUpper;
    std::atomic<CPPAFKQSize_t> batchLimitLower;
    std::atomic<CPPAFKQSize_t> netCount;
    CPPAFKPriv_WrapBQ<T>* tempWBQ;
    std::deque<CPPAFKPriv_WrapBQ<T>*> itsBQ;
    std::deque<CPPAFKPriv_WrapBQ<T>*> itsRelaxingQ;
    
    void _reset(){
        batchLimitUpper=std::numeric_limits<std::uint64_t>::max();
        batchLimitLower=std::numeric_limits<std::uint64_t>::min();
        this->paused=false;
        netCount=0;
        this->minQSize=0;
        this->maxQSize=std::numeric_limits<CPPAFKQSize_t>::max();

        this->condPredR=false;
        this->condPredW=false;
        this->cvR.notify_all();
        this->cvW.notify_all();
        _clearRelaxQ();

    }
    void _sendToRelaxation(CPPAFKPriv_WrapBQ<T>* wrap){
        this->itsRelaxingQ.push_back(wrap);
    };
    void _clearRelaxQ(){
        std::uint64_t c=this->itsRelaxingQ.size();
        this->lock.lock();
        while(c!=0){
            CPPAFKPriv_WrapBQ<T>* wrap=this->itsRelaxingQ.front();
            this->itsRelaxingQ.pop_front();
            if(wrap->discarded.load())
            {
                delete wrap;
            }
            else
            {
                this->itsRelaxingQ.push_back(wrap);
            }
            --c;
        }
        this->lock.unlock();
    }
public:
    CPPAFKBatchingQueue<T>():CPPAFKQueue<T>(){
        _reset();
        this->tempWBQ = new CPPAFKPriv_WrapBQ<T>();
    }
    CPPAFKBatchingQueue<T>(bool blkR):CPPAFKQueue<T>(blkR){
        _reset();
        this->tempWBQ = new CPPAFKPriv_WrapBQ<T>();
        this->blockingR=blkR;
        this->blockingW=false;
    }
#ifdef CPPAFK_USE_STR_OBJ_NAME
    CPPAFKBatchingQueue<T>(std::string name, bool blkR):CPPAFKQueue<T>(name, blkR){
        _reset();
        this->tempWBQ = new CPPAFKPriv_WrapBQ<T>();
        this->blockingR=blkR;
        this->blockingW=false;
    }
#endif
    ~CPPAFKBatchingQueue(){
        reset();
        delete this->tempWBQ;
        
    }
    bool setMaxQSize(CPPAFKQSize_t size){
        WASFKLog(CPPAFK_STR_UNSUPPORTED_OP);
        return false;
    }
    bool setMinQSize(CPPAFKQSize_t size){
        WASFKLog(CPPAFK_STR_UNSUPPORTED_OP);
        return false;
    }
    bool setUpperBatchLimit(std::uint64_t limit){
        bool r=true;
        if(limit < batchLimitLower.load()){
            r=false;
            WASFKLog(CPPAFK_STR_Q_ULIMIT_VIOLATION);
            return r;
        }
        batchLimitUpper=limit;
        return r;
        
    }
    bool setLowerBatchLimit(std::uint64_t limit){
        bool r=true;
        if(limit > batchLimitUpper.load()){
            r=false;
            WASFKLog(CPPAFK_STR_Q_LLIMIT_VIOLATION);
            return r;
        }
        batchLimitLower=limit;
        return r;
    }
    void reset(){
        purge();
        batchLimitUpper=std::numeric_limits<std::uint64_t>::max();
        batchLimitLower=std::numeric_limits<std::uint64_t>::min();
        if(this->tempWBQ != std::nullptr_t()){
            this->tempWBQ->many.clear();
        }
        _reset();
    }
    void relax(){
        _clearRelaxQ();
    }
    void purge(){
        std::lock_guard<std::mutex> guard(this->lock);
        if(this->blockingR){
            std::unique_lock<std::mutex> lk(this->cvLockR);
            this->condPredR=true;
            lk.unlock();
            this->cvR.notify_all();
        }

        for(typename std::deque<CPPAFKPriv_WrapBQ<T>*>::iterator it=this->itsBQ.begin();
            this->itsBQ.end() != it;++it){
            
            if(this->blockingW){
                {
                    std::unique_lock<std::mutex> lk(((CPPAFKPriv_WrapBQ<T>*)(*it))->cvLockW);
                    ((CPPAFKPriv_WrapBQ<T>*)(*it))->condPredW=true;
                    
                }
                ((CPPAFKPriv_WrapBQ<T>*)(*it))->cvW.notify_all();
                this->lock.lock();
                _sendToRelaxation(((CPPAFKPriv_WrapBQ<T>*)(*it)));
                this->lock.unlock();
            }
  

        }
        
        this->itsBQ.clear();

        this->netCount=0;

    }
    bool castObject(T item, CPPAFKExecutionParams * ex){
        _clearRelaxQ();

        CPPAFKPriv_WrapBQ<T>* wrap=new CPPAFKPriv_WrapBQ<T>;
        wrap->many.push_back(item);
        this->lock.lock();
        this->itsBQ.push_back(wrap);
        this->netCount.fetch_add(1);
        this->lock.unlock();
        this->cvR.notify_all();
        wrap->discarded=true;
            return true;

    };
    bool castVector(std::vector<T>& data, CPPAFKExecutionParams * ex){
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }
    bool callObject(T item, CPPAFKExecutionParams * ex){
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;

    };
    bool callVector(std::vector<T>& data, CPPAFKExecutionParams * ex){
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }
#pragma mark - casting with accumulation
    CPPAFKQSize_t candidateCount(){
        CPPAFKQSize_t csize=0;
        if(tempWBQ != std::nullptr_t()){
            csize=tempWBQ->many.size();
        }
        return csize;
    }
    
    /**
     @brief adds single object to cumulative batch. The batch will not be added to queue until it is finalized.
     @discussion if resulting batch size exceeds the established upper bound, the operation will fail. If queue is (going to be) oversized - operation will be cancelled.
     @param obj the dictionary.
     @return true for success, NO otherwise.
     */
    bool castObjectToBatch(T obj){
        _clearRelaxQ();
        bool tval=false;
        if(obj){
            this->lock.lock();
           
            tempWBQ->many.push_back(obj);
            this->lock.unlock();
            tval=true;
        }
        return tval;
    }
    /**
     @brief finalizes cumulative batch. The batch will be added to queue and new cumulative batch will be created.
     @param force if NO, then cumulative batch size will be examined against minimum size and, if less than the minumum or greater than maximum - operation will fail. Otherwise the batch will be appended to queue disregarding the size.
     @return true for success, NO otherwise.
     */
    bool commitBatch(bool force)
    {
        _clearRelaxQ();
        bool res=false;
        
        this->lock.lock();
        if(force==true){
            
        }
        else{
            if(batchLimitLower > tempWBQ->many.size() || batchLimitUpper < tempWBQ->many.size()){
                this->lock.unlock();
                WASFKLog(CPPAFK_STR_Q_BTCH_LIMIT_VIOLATION);
                return false;
            }
        }
        
        if(tempWBQ && tempWBQ->many.size() > 0){
            this->itsBQ.push_back(tempWBQ);
            netCount.fetch_add(tempWBQ->many.size());
            tempWBQ=new CPPAFKPriv_WrapBQ<T> ;
            res=true;
        }
        this->lock.unlock();
        return res;
    }
    /**
     @brief resets cumulative batch. The batch will be cleared form all accumulated objects.
     @return true for success, NO otherwise.
     */
    bool resetBatch(){
        _clearRelaxQ();
        bool res=false;
        std::lock_guard<std::mutex> guard(this->lock);
        
        if(tempWBQ != std::nullptr_t() && tempWBQ->many.size() >0){
            tempWBQ->many.clear();// removeAllObjects];
            res=true;
        }
        
        return res;
    }
    
    void queueFromBatchingQueue(CPPAFKBatchingQueue<T>* otherq){
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
    }
    /*!
     @brief pauses reading from queue for batches. After invocation, reading method calls will return all items belonging to batch. When entire batch is read, next reading calls will return nil, until 'resume' method invoked.
     */
    void pause(){
        _clearRelaxQ();
        this->paused=true;
    }
    
    /*!
     @brief resumes reading from queue for batches.
     */
    void resume(){
        _clearRelaxQ();
        this->paused=false;
    }
    
    /*!
     @brief returns number of batches in queue.
     */
    CPPAFKQSize_t batchCount(){
        _clearRelaxQ();
        std::lock_guard<std::mutex> guard(this->lock);
        CPPAFKQSize_t qc=this->itsBQ.size();
        return qc;
    }
    /*!
     @brief pulls earliest batch's contents as array.
     */
    void pullBatchAsArray(std::vector<T>& result, bool& res)
    {
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        
    }
    
    T    pull(bool& success)
    {
        _clearRelaxQ();
        
        if(this->paused.load()){
            success=false;
            return T();
        }
        this->lock.lock();
        
        if(this->netCount > this->minQSize){
            CPPAFKPriv_WrapBQ<T>* item=this->itsBQ.front();
            
            if(item->many.size() > 0)
            {
                T subitem = item->many.front();
                item->many.pop_front();
                this->netCount.fetch_sub(1);
                if(item->many.size()==0){
                    itsBQ.pop_front();
                    if(item->waitUntilRead){
                        std::unique_lock<std::mutex> lk(item->cvLockW);
                        (item)->condPredW=true;
                        lk.unlock();
                        (item)->cvW.notify_all();
                        
                    }
                   _sendToRelaxation(item);
                }
                success=true;
                this->lock.unlock();

                return subitem;
            }
            else{
                this->lock.unlock();
            }
        }
        else{
            this->lock.unlock();
            if(this->blockingR.load())
            {
                std::unique_lock<std::mutex> lk(this->cvLockR);
                this->condPredR=false;
                this->cvR.wait(lk, [this]{
                    this->lock.lock();
                    this->condPredR=this->netCount > this->minQSize ? true:false;
                    this->lock.unlock();
                    return this->condPredR.load();
                });
                lk.unlock();
            }
        }
        success=false;
        
        return T();
        
    }
    
    CPPAFKQSize_t size(){
        _clearRelaxQ();
        CPPAFKQSize_t res=0;
        std::lock_guard<std::mutex> guard(this->lock);
        res=this->netCount;
        return res;
    };
    bool isEmpty(){
        _clearRelaxQ();
        bool res=false;
        std::lock_guard<std::mutex> guard(this->lock);
        res=itsBQ.empty();
        return res;
    };
};


#endif /* CPPAFKBatchingQueue_h */
