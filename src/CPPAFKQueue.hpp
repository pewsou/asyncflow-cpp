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

#ifndef CPPAFKQueue_h
#define CPPAFKQueue_h
/**
 @brief Definition of common queue.
 @discussion supports blocking and non-blocking read/write operations; queue size management.
 */
template <typename T> class CPPAFKQueue: public CPPAFKLinearFlow<T>
{
    friend class CPPAFKQueue<T>;
protected:
    std::mutex lock;
    std::mutex cvLockR; //LEU15625885
    std::mutex cvLockW; //LEU15625885
    std::atomic<bool> condPredR;
    std::atomic<bool> condPredW;
    std::atomic<bool> blockingR;//LEU15625885
    std::atomic<bool> blockingW;//LEU15625885
    std::atomic<bool> paused;
    std::atomic<CPPAFKQSize_t> maxQSize;
    std::atomic<CPPAFKQSize_t> minQSize;
    std::condition_variable cvW; //LEU15625885
    std::condition_variable cvR; //LEU15625885
    std::deque<T> itsQ;
    virtual void _postSet(){};
    virtual void _releaseAll(){
        if(blockingR){
            //LEU15625885
            std::unique_lock<std::mutex> lk(cvLockR);
            condPredR=true;
            lk.unlock();
            cvR.notify_all();
        }
        if(blockingW)
        {
            std::unique_lock<std::mutex> lk(cvLockW);
            condPredW=true;
            lk.unlock();
            cvW.notify_all();
        }
        condPredW=false;
    }
    virtual void _reset(){
        std::lock_guard<std::mutex> guard(lock);
        itsQ.clear();
        _releaseAll();
        _init();
    }
    void _init(){
        minQSize=0;
        maxQSize=std::numeric_limits<CPPAFKQSize_t>::max();
        paused=false;
        blockingR=false;
        blockingW=false;
        condPredR=false;
        condPredW=false;
    }
public:
    CPPAFKQueue<T>():CPPAFKLinearFlow<T>(){
        _init();
    }
    CPPAFKQueue<T>(bool blkR):CPPAFKLinearFlow<T>(){
        _init();
        blockingR=blkR;
    }
#ifdef CPPAFK_USE_STR_OBJ_NAME
    CPPAFKQueue<T>(std::string name, bool blkR):CPPAFKLinearFlow<T>(name){
        _init();
        blockingR=blkR;
    }
#endif
    ~CPPAFKQueue<T>(){reset();}
    void begin(){
        lock.lock();
    }

    void commit(){
        lock.unlock();
    }

    /**
     @brief Sets maximum queue size.
     @discussion when the queue size reaches this value any further enqueing operation will not increase it.
     @param size required maximum size.
     @return YES if the update was successful, NO otherwise.
     */
    bool setMaxQSize(CPPAFKQSize_t size){
        bool r=true;
        lock.lock();
        if(size < minQSize.load()){
            lock.unlock();
            r=false;
            
            WASFKLog("new upper limit is not greater than lower limit");
        }
        else{
            maxQSize=size;
            lock.unlock();
            _postSet();
        }
        
        return r;
    }
    /**
     @brief Sets minimum queue size.
     @discussion when the queue size reaches this value any further enqueing operation will not decrease it.
     @param size required minimum size.
     @return YES if the update was successul, NO otherwise.
     */
    bool setMinQSize(CPPAFKQSize_t size){
        bool r=true;
        std::lock_guard<std::mutex> guard(lock);
        if(size > maxQSize.load()){
            r=false;
            WASFKLog("new lower limit is not less than upper limit");
        }
        else{
            minQSize=size;
        }
        
        return r;
    }
    
    /**
     @brief replaces contents of receiver by contents of another queue.
     */
    bool queueFromQueue(CPPAFKQueue<T>* otherq){
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }
    /**
     @brief puts contents of another queue at head of receiver's queue. Not available in blocking mode.
     */
    bool prepend(CPPAFKQueue<T>* otherq)
    {
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
       
    }
    
    bool prepend(T obj)
    {
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }

    virtual bool castQueue(CPPAFKQueue<T>* otherq, CPPAFKExecutionParams* ex)
    {
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }
    virtual bool castObject(T item, CPPAFKExecutionParams* ex)
    {
        std::lock_guard<std::mutex> guard(lock);
        
        bool insert=itsQ.size() + 1 <= maxQSize?true:false;
        if(insert){
            blockingW=false;
            itsQ.push_back(item);
            if(blockingR){
                //LEU15625885
                std::unique_lock<std::mutex> lk(cvLockR);
                condPredR=true;
                lk.unlock();
                cvR.notify_one();
            }
        }
            
        return insert;
    }
    virtual bool castVector(std::vector<T>& data, CPPAFKExecutionParams * ex)
    {
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }
    virtual bool callQueue(CPPAFKQueue<T>* otherq, CPPAFKExecutionParams* ex)
    {
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }
    virtual bool callObject(T item, CPPAFKExecutionParams* ex)
    {
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        return false;
    }
    virtual bool callVector(std::vector<T>& data, CPPAFKExecutionParams* ex)
    {
        WASFKLog(CPPAFK_STR_UNAVAIL_VER_OP);
        
        return false;
    }
    virtual T    pull(bool& success)
    {
        if(paused){
            success=false;
            return T();
        }
        if(blockingR){
            //LEU15625885
            lock.lock();
            CPPAFKQSize_t c=itsQ.size();
            lock.unlock();
            if(c <= minQSize){
                std::unique_lock<std::mutex> lk(cvLockR);
                condPredR=false;
                cvR.wait(lk, [this,c]{
                    lock.lock();
                    condPredR=itsQ.size()>minQSize?true:false;
                    lock.unlock();
                    return condPredR.load();
                });
                lk.unlock();
                cvR.notify_one();
            }
            else{
                condPredR=true;
            }
        }
        std::lock_guard<std::mutex> guard(lock);
        if(itsQ.size() > minQSize){
            T item = itsQ.front();
            itsQ.pop_front();
            if(blockingW){
                //LEU15625885
                std::unique_lock<std::mutex> lk(cvLockW);
                condPredW=true;
                lk.unlock();
                cvW.notify_one();
            }
            success=true;
            return item;
        }
        else{
            success=false;
            return T();
        }
    }
    
    virtual CPPAFKQSize_t size()
    {
        CPPAFKQSize_t res=0;
        std::lock_guard<std::mutex> guard(lock);
        res=itsQ.size();
        return res;
    };
    virtual bool isEmpty(){
        bool res=false;
        std::lock_guard<std::mutex> guard(lock);
        res=itsQ.empty();
        return res;
    };
    bool isBlockingRead(){return blockingR.load();};
    bool isBlockingWrite(){return blockingW.load();};
    /**
     @brief pauses reading from queue. When paused, the queue will return nil on any reading attempt.
     */
    void pause(){ paused = true;}
    /**
     @brief resumes reading from queue.
     */
    void resume(){ paused = false;}
    /**
     @brief deletes all accumulated data and resets configuration.
     @discussion removes queue contents and resets configuration data to defaults.
     */
    virtual void reset(){
        _reset();
    }
    /**
     @brief deletes all accumulated data.
     @discussion removes queue contents only.
     */
    virtual void purge()
    {
        std::lock_guard<std::mutex> guard(lock);
        _releaseAll();
    };
 
};

#endif /* CPPAFKQueue_h */
