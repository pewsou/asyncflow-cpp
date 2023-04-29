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

//  Created by Boris Vigman on 15/02/2019.
//  Copyright © 2019-2023 Boris Vigman. All rights reserved.
//

#ifndef CPPAFKBase_hpp
#define CPPAFKBase_hpp

#define CPPAFK_VERSION "0.0.1"

#define CPPAFK_ENABLE_MAILBOX 1

#define CPPAFK_NUM_OF_HW_THREADS (-1)

#define CPPAFK_DEBUG_ENABLED 1
#define CPPAFK_MSG_WARNING_ENABLED 1

#define CPPAFK_USE_STR_OBJ_NAME 1

#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <cstdint>
#include <limits>
#include <deque>
#include <condition_variable>
#include <random>
#include <iostream>
#include <sstream>
#include <functional>
#include <map>

#define CPPAFK_STR_UNSUPPORTED_OP "Operation not supported"
#define CPPAFK_STR_Q_ULIMIT_VIOLATION "new upper limit is not greater than lower limit"
#define CPPAFK_STR_Q_LLIMIT_VIOLATION "new lower limit is not less than upper limit"
#define CPPAFK_STR_MISCONFIG_OP "operation not confgured properly"

#ifdef CPPAFK_MSG_DEBUG_ENABLED
#define DASFKLog(str) {std::cout<<"DEBUG "<<__FILE__<<" : "<<__LINE__<<" : "<<__FUNCTION__<<" : "<<str<<std::endl;}

#define DASFKLog_2(str,arg) {std::cout<<"DEBUG "<<__FILE__<<" : "<<__LINE__<<" : "<<__FUNCTION__<<" : "<<str<<arg<<std::endl;}
#else
#define DASFKLog(str)

#define DASFKLog_2(str,arg)
#endif

#ifdef CPPAFK_MSG_WARNING_ENABLED
#define WASFKLog(str) {std::cout<<"WARNING "<<__FILE__<<" : "<<__LINE__<<" : "<<__FUNCTION__<<" : "<<str<<std::endl;}

#define WASFKLog_2(str,arg) {std::cout<<"WARNING "<<__FILE__<<" : "<<__LINE__<<" : "<<__FUNCTION__<<" : "<<str<<arg<<std::endl;}
#else
#define WASFKLog(str)

#define WASFKLog_2(str)
#endif

#ifdef CPPAFK_MSG_COMMON_ENABLED
#define COMASFKLog(str) {std::cout<<"COMMON "<<__FILE__<<" : "<<__LINE__<<" : "<<__FUNCTION__<<" : "<<str<<std::endl;}

#define COMASFKLog_2(str,arg) {std::cout<<"COMMON "<<__FILE__<<" : "<<__LINE__<<" : "<<__FUNCTION__<<" : "<<str<<arg<<std::endl;}
#else
#define COMASFKLog(str)
#define COMASFKLog_2(str,arg)
#endif

namespace AsyncFlowKit{
    
    typedef std::string     tCPPAFK_SESSION_ID_TYPE;
    typedef std::uint64_t   CPPAFKQSize_t;
    
    typedef enum enumCPPAFK_BLOCKING_CALL_MODE{
        /**
         @brief AFK_BC_NO_BLOCK stands for no blocking allowed - calls on session in this mode will be rejected.
         */
        AFK_BC_NO_BLOCK,
        /**
         @brief AFK_BC_CONTINUOUS stands for continuous mode - once processing of blocked batch started, first element of the next batch will be fetched for processing immediately after the last element of previous batch. I.e. processing of the next batch will not wait until the previous batch has been fully processed.
         */
        AFK_BC_CONTINUOUS,
        /**
         @brief AFK_BC_EXCLUSIVE means that processing of the next batch will start only after the previous batch was fully processed.
         */
        AFK_BC_EXCLUSIVE
    } eCPPAFKBlockingCallMode;
    
    
    typedef enum enumCPPAFKPipelineExecutionStatus{
        eAFK_ES_HAS_MORE=0,
        eAFK_ES_HAS_NONE,
        eAFK_ES_WAS_CANCELLED,
        eAFK_ES_SKIPPED_MAINT
    } eCPPAFKThreadpoolExecutionStatus;
    
    /**
     @brief Unique identifier. Zero value is invalid.
     */
    typedef union uCPPAFKNumId{
        std::uint64_t num64;
        std::uint32_t lowhi[2];
        uCPPAFKNumId(){num64=0;};
    } CPPAFKNumId;
    
    typedef std::function<void()> CPPAFKProgressRoutine_t;

    typedef std::function<void()> CPPAFKExecutableRoutineSummary_t;

    typedef std::function<void()> CPPAFKExecutableRoutine_t;

    typedef std::function<void(CPPAFKNumId)> CPPAFKCancellationRoutine_t;
    typedef std::function<void()> CPPAFKExpirationRoutine_t;

    class CPPAFKExpirationCondition{
        public:
        void exec();
    };
    
    typedef std::function<void()> CPPAFKOnPauseNotification_t;
    //CPPAFKOnPauseNotification onPauseProc;
    
    struct CPPASFKReturnInfo {
        double totalSessionTime;
        double totalProcsTime;
        double returnStatsProcsElapsedSec;
        double returnStatsSessionElapsedSec;
        std::uint64_t totalProcsCount;
        std::uint64_t totalSessionsCount;
        std::string returnCodeDescription;
        std::string returnResult;
        tCPPAFK_SESSION_ID_TYPE returnSessionId;
        bool returnCodeSuccessful;
    };
    struct CPPAFKConfigParams{
        CPPAFKProgressRoutine_t progressProc;
        CPPAFKExecutableRoutineSummary_t summaryRoutine;
        std::vector<CPPAFKExecutableRoutine_t> procs;
        CPPAFKCancellationRoutine_t cancellationProc;
        CPPAFKExpirationCondition expCondition;
        CPPAFKOnPauseNotification_t onPauseProc;
        //@public ASFKPreBlockingRoutine preBlock;
    };

    struct CPPAFKExecutionParams:public CPPAFKConfigParams{

        std::function<void()> preBlock;
    };
    
    struct CPPAFKSessionConfigParams:public CPPAFKConfigParams{
        //@public ASFKOnPauseNotification onPauseProc;
        eCPPAFKBlockingCallMode blockCallMode;
        CPPAFKSessionConfigParams(){
            blockCallMode=AFK_BC_NO_BLOCK;
        }
    };
    
    
    struct cmpNumericalId64 {
        bool operator()(const CPPAFKNumId& a, const CPPAFKNumId& b) const {
            return a.num64 < b.num64;
        }
    };
    
    struct CPPAFKUtilities{
        static CPPAFKNumId createRandomNum();
        static std::chrono::time_point<std::chrono::system_clock> getTimePoint();
        static std::uint64_t getTimeInterval(std::chrono::time_point<std::chrono::system_clock>& start,std::chrono::time_point<std::chrono::system_clock>& end);
    };
    
    class CPPAFKControlBlock {
    protected:
        std::atomic<std::uint64_t> itsResPosition;
        std::uint64_t totalProcessors;
        std::mutex itsLock;
        CPPAFKProgressRoutine_t itsProgressProc;
        std::atomic<std::uint64_t> indexSecondary;
        std::atomic< bool> paused;
        std::atomic< bool> flushed;
        std::atomic< bool> abortByCallback;
        std::atomic< bool> abortByCaller;
        std::atomic< bool> abortByInternal;
        CPPAFKNumId parentId;
        CPPAFKNumId sessionPrimId;
        CPPAFKNumId sessionSecId;
        
        
    public:
        CPPAFKControlBlock(){
            parentId=CPPAFKUtilities::createRandomNum();;
            sessionPrimId=CPPAFKUtilities::createRandomNum();;
            sessionSecId=CPPAFKUtilities::createRandomNum();;
        };

        CPPAFKControlBlock(CPPAFKNumId parent,CPPAFKNumId prim,CPPAFKNumId sec){
            parentId=parent;
            sessionPrimId=prim;
            sessionSecId=sec;
        };
        void setProgressRoutine(CPPAFKProgressRoutine_t progress);
        bool cancellationRequested();
        bool cancellationRequestedByCallback();
        bool cancellationRequestedByStarter();
        bool isStopped();
        bool isPaused();
        void cancel();
        void reset();
        void stop();
        void flushRequested(bool flush);
        bool flushRequested();
        void setPaused(bool yesno);
        CPPAFKNumId getCurrentSessionId();
        CPPAFKNumId getParentObjectId();
        CPPAFKProgressRoutine_t getProgressRoutine();
    };
    
    /**
     */
    class CPPAFKBase{
        CPPAFKNumId itsNumId;
#ifdef CPPAFK_USE_STR_OBJ_NAME
        std::string itsStrId;
#endif
        bool isCancellationRequested();
        CPPAFKControlBlock* refreshCancellationData();
        void registerCtrlBlock(CPPAFKControlBlock* cblk);
        CPPAFKControlBlock* newCtrlBlock();
        CPPAFKControlBlock* newCtrlBlock(CPPAFKNumId sessionId,CPPAFKNumId subId);
        void forgetAllCtrlBlocks();
        void forgetCtrlBlock(CPPAFKNumId sessionId);
        CPPAFKControlBlock* getControlBlockWithId(CPPAFKNumId blkId);
        void setProgressRoutine(CPPAFKProgressRoutine_t prog);
    protected:
        std::map<CPPAFKNumId, CPPAFKControlBlock*, cmpNumericalId64> ctrlblocks;
        std::mutex lkNonLocal;
        
#ifdef CPPAFK_USE_STR_OBJ_NAME
        CPPAFKBase(std::string name){
            itsNumId=CPPAFKUtilities::createRandomNum();
            itsStrId=name;
        }
        CPPAFKBase(){
            itsNumId=CPPAFKUtilities::createRandomNum();
            std::ostringstream oss;
            oss << itsNumId.num64;
            itsStrId = oss.str();
        }
        std::string getItsName(){return itsStrId;};

#else
        CPPAFKBase(){
            itsNumId=CPPAFKUtilities::createRandomNum();
        }
#endif
    public:
        std::string getVersion();
        CPPAFKNumId getItsId(){return itsNumId;};
        static std::string getVersionStatic();
        virtual ~CPPAFKBase(){}
    };
    
    template <typename T> class CPPAFKLinearFlow: public CPPAFKBase{
    protected:
        CPPAFKLinearFlow<T>():CPPAFKBase() {};
#ifdef CPPAFK_USE_STR_OBJ_NAME
        CPPAFKLinearFlow<T>(std::string name):CPPAFKBase(name){};
   
#endif
    public:
        
        bool cast(T, CPPAFKExecutionParams*);
        bool call(T, CPPAFKExecutionParams*);
        
        T pull(bool& success);
        
    };
    
    /**
     *  @brief Basic data object for processing.
     */
    struct CPPAFKBasicDataObjectWrap{
        
    };
    /**
     *  @brief Basic execution unit.
     */
    struct CPPAFKExecUnitWrap{
        /**
         *  @brief method to be called when object is to perform some calculation.
         */
        void exec();
    };

    template<typename T> class CPPAFKSessionFlow:public CPPAFKLinearFlow<T>{
    public:
        CPPAFKSessionFlow<T>():CPPAFKLinearFlow<T>(){
        
        };
#ifdef CPPAFK_USE_STR_OBJ_NAME
        CPPAFKSessionFlow<T>(std::string name):CPPAFKLinearFlow<T>(){ };
#endif
    };
    
    /**
     @brief Definition of common queue.
     @discussion supports blocking and non-blocking read/write operations; queue size management.
     */
    template <typename T> class CPPAFKQueue: public CPPAFKLinearFlow<T>{
        friend class CPPAFKQueue<T>;
    protected:
        std::mutex lock;
        std::mutex cvLockR;
        std::mutex cvLockW;
        std::atomic<bool> condPredR;
        std::atomic<bool> condPredW;
        std::atomic<bool> blocking;
        std::atomic<bool> paused;
        std::atomic<CPPAFKQSize_t> maxQSize;
        std::atomic<CPPAFKQSize_t> minQSize;
        std::condition_variable cvW;
        std::condition_variable cvR;
        std::deque<T> itsQ;
    public:
        CPPAFKQueue<T>():CPPAFKLinearFlow<T>(){
            minQSize=0;
            maxQSize=std::numeric_limits<CPPAFKQSize_t>::max();
            paused=false;
            blocking=false;
            condPredR=false;
            condPredW=false;
        }
        CPPAFKQueue<T>(bool blk):CPPAFKLinearFlow<T>(){
            minQSize=0;
            maxQSize=std::numeric_limits<CPPAFKQSize_t>::max();
            //condPredR=NO;
            //condPredW=NO;
            paused=false;
            blocking=blk;
            condPredR=false;
            condPredW=false;
        }
#ifdef CPPAFK_USE_STR_OBJ_NAME
        CPPAFKQueue<T>(std::string name, bool blk):CPPAFKLinearFlow<T>(name){
            minQSize=0;
            maxQSize=std::numeric_limits<CPPAFKQSize_t>::max();
            paused=false;
            blocking=blk;
            condPredR=false;
            condPredW=false;
        }
#endif
        ~CPPAFKQueue<T>(){reset();}
        /**
         @brief Sets maximum queue size.
         @discussion when the queue size reaches this value any further enqueing operation will not increase it.
         @param size required maximum size.
         @return YES if the update was successful, NO otherwise.
         */
        bool setMaxQSize(CPPAFKQSize_t size){
            bool r=true;
            std::lock_guard<std::mutex> guard(lock);
            if(size < minQSize.load()){
                r=false;
                WASFKLog("new upper limit is not greater than lower limit");
            }
            else{
                maxQSize=size;
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
        void queueFromQueue(CPPAFKQueue<T>* otherq){
            if(blocking==false && otherq){
                std::lock_guard<std::mutex> guard0(otherq->lock);
                std::lock_guard<std::mutex> guard1(lock);

                itsQ.clear();
                typename std::deque<T>::iterator It;
                for ( It = otherq->itsQ.begin(); It != otherq->itsQ.end(); It++ ){
                    itsQ.push_back(It);
                }

            }
        }
        /**
         @brief puts contents of another queue at head of receiver's queue. Not available in blocking mode.
         */
        bool prepend(CPPAFKQueue<T>* otherq)
        {
            if(blocking==false && otherq){
                std::lock_guard<std::mutex> guard0(otherq->lock);
                std::lock_guard<std::mutex> guard1(lock);
                typename std::deque<T>::iterator It;
                for ( It = otherq->itsQ.begin(); It != otherq->itsQ.end(); It++ ){
                    itsQ.push_front(It);
                }
            }

            return true;
        }

        bool prepend(T obj){
            if(blocking==false ){
                std::lock_guard<std::mutex> guard1(lock);
                itsQ.clear();
                typename std::deque<T>::iterator It;
                itsQ.push_front(obj);
            }
        }
        //-(BOOL) prependFromQueue:(ASFKQueue*)otherq
        virtual bool castQueue(CPPAFKQueue<T>* otherq, CPPAFKExecutionParams* ex){
            return false;
        }
        virtual bool castObject(T item, CPPAFKExecutionParams* ex){
            //if(item)
            {
                std::lock_guard<std::mutex> guard(lock);
                bool insert=itsQ.size() + 1 <= maxQSize?true:false;
                if(insert){
                    itsQ.push_back(item);
                    if(blocking){
                        std::unique_lock<std::mutex> lk(cvLockR);
                        condPredR=true;
                        lk.unlock();
                        cvR.notify_one();
                    }
                }
                
                return insert;
            }
            return false;
        }
        virtual bool callQueue(CPPAFKQueue<T>* otherq, CPPAFKExecutionParams* ex){
            return false;
        }
        virtual bool callObject(T item, CPPAFKExecutionParams* ex){
            {
                std::lock_guard<std::mutex> guard(lock);
                bool insert=itsQ.size() + 1 <= maxQSize?true:false;
                if(insert){
                    itsQ.push_back(item);
                    if(blocking){
                        {
                            std::unique_lock<std::mutex> lk(cvLockR);
                            condPredR=true;
                            lk.unlock();
                            cvR.notify_one();
                        }
                        {
                            std::unique_lock<std::mutex> lk(cvLockW);
                            condPredW=false;
                            cvW.wait(lk,[this]{return condPredW.load();});
                            lk.unlock();

                        }
                    }
                }
                return insert;
            }
            
            return false;
        }
        
        virtual T    pull(bool& success)
        {
            if(paused){
                success=false;
                return T();
            }
            if(blocking){
                lock.lock();
                std::uint64_t c=itsQ.size();
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
                if(blocking){
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
        
        CPPAFKQSize_t size(){
            CPPAFKQSize_t res=0;
            std::lock_guard<std::mutex> guard(lock);
            res=itsQ.size();
        };
        bool isEmpty(){
            bool res=false;
            std::lock_guard<std::mutex> guard(lock);
            res=itsQ.empty();
        };
        bool isBlocking(){return blocking.load();};
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
            std::lock_guard<std::mutex> guard(lock);
            itsQ.clear();
            if(blocking){
                std::unique_lock<std::mutex> lk(cvLockR);
                {
                    std::unique_lock<std::mutex> lk(cvLockR);
                    condPredR=true;
                    lk.unlock();
                    cvR.notify_all();
                }

                
                {
                    std::unique_lock<std::mutex> lk(cvLockW);
                    condPredW=true;
                    lk.unlock();
                    cvW.notify_all();
                }

            }
        }
        /**
         @brief deletes all accumulated data.
         @discussion removes queue contents only.
         */
        virtual void purge(){
            std::lock_guard<std::mutex> guard(lock);
            itsQ.clear();
            if(blocking){
                std::unique_lock<std::mutex> lk(cvLockW);
                condPredW=true;
                lk.unlock();
                cvW.notify_all();
            }
            condPredW=false;
        };
        

    };
    

    class CPPAFKPriv_EndingTerm{
    public:
        CPPAFKPriv_EndingTerm & getInstance() ;
    };
    template <typename T> struct CPPAFKPriv_WrapBQ{
        std::condition_variable cvW;
        std::mutex cvLockW;
        std::atomic<bool> wcPred;
        std::deque<T> many;
        CPPAFKPriv_WrapBQ(){wcPred=false;};
    };
    
    enum eQSpecificity{
        ASFK_E_QSPEC_NONE,
        ASFK_E_QSPEC_REG,
        ASFK_E_QSPEC_BAT
    };
    typedef std::deque<std::pair<eQSpecificity,std::uint64_t>> CPPAFKQMapper_t;
    template <typename T> class CPPAFKThreadpoolQueue : public CPPAFKQueue<T>
    {
    protected:
        std::deque<std::int64_t> deqIndexes;
        std::int64_t occupant;;
    
    //-(void) queueFromThreadpoolQueue:(ASFKThreadpoolQueue*)queue;
    //-(void) queueFromItem:(id)item;
        T   pullAndOccupyWithId(std::int64_t itsid, bool& empty, std::int64_t& itemIndex, CPPAFKPriv_EndingTerm* term, bool& validrv){
            itemIndex=-1;
            
            this->lock.lock();
            if(occupant>=0 && occupant!=itsid){
                empty=true;
                if(this->itsQ.size()>0){
                    empty=false;
                }
                this->lock.unlock();;
                validrv=false;
                return T();
            }
            T item;
            //id item=[q firstObject];
            if (this->itsQ.size()>0) {
                item=this->itsQ.front();
                itemIndex=deqIndexes.front();
                deqIndexes.pop_front();
                //[q removeObjectAtIndex:0];
                this->itsQ.popFront();
                if(this->itsQ.size()>0){
                    occupant=itsid;
                    empty=false;
                }
                else{
                    occupant=-1;
                    empty=true;
                }
            }
            else{
                occupant=-1;
                empty=true;
            }
            this->lock.unlock();;
            return item;
            
        }
        bool castObject(T item,CPPAFKExecutionParams* ex, std::int64_t index){
            
        }
        void unoccupyWithId(std::int64_t itsid){
            
        }
        void unoccupy(){
            
        }
    
    };
    
    template <typename T> class CPPAFKThreadpoolQueueHyb :public CPPAFKThreadpoolQueue<T>{
    public:
        CPPAFKNumId itsSig;
    
    //-(id)   initWithBlkMode:(eASFKBlockingCallMode) blockingMode;
        void _releaseBlocked(){
            
        }
        void _releaseBlockedAll(){
            
        }
    };
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
    void _reset(){
        batchLimitUpper=std::numeric_limits<std::uint64_t>::max();
        batchLimitLower=std::numeric_limits<std::uint64_t>::min();
        this->paused=false;
        netCount=0;
        this->minQSize=0;
        this->maxQSize=std::numeric_limits<CPPAFKQSize_t>::max();
        //this->blocking=false;
        this->condPredR=false;
        this->condPredW=false;
        this->tempWBQ=std::nullptr_t();
    }
public:
    CPPAFKBatchingQueue<T>():CPPAFKQueue<T>(){
        _reset();
    }
    CPPAFKBatchingQueue<T>(bool blk):CPPAFKQueue<T>(blk){
        _reset();
        this->blocking=blk;
    }
#ifdef CPPAFK_USE_STR_OBJ_NAME
    CPPAFKBatchingQueue<T>(std::string name, bool blk):CPPAFKQueue<T>(name, blk){
        _reset();
        this->blocking=blk;
    }
#endif
    ~CPPAFKBatchingQueue(){
        reset();
        
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
        //WASFKLog("new lower limit is not less than upper limit");
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
        delete this->tempWBQ;
        //this->tempWBQ = new CPPAFKPriv_WrapBQ<T>();
    }
    _reset();
}
void purge(){
    std::lock_guard<std::mutex> guard(this->lock);
    if(this->blocking){
        for(typename std::deque<CPPAFKPriv_WrapBQ<T>*>::iterator it=this->itsBQ.begin();
            this->itsBQ.end() != it;++it){
            
            //if(((CPPAFKPriv_WrapBQ<T>*)(*it))->cvW)
            //{
                std::unique_lock<std::mutex> lk(((CPPAFKPriv_WrapBQ<T>*)(*it))->cvLockW);
                ((CPPAFKPriv_WrapBQ<T>*)(*it))->wcPred=true;
                lk.unlock();
                ((CPPAFKPriv_WrapBQ<T>*)(*it))->cvW.notify_all();

            delete *it;
        }
        this->itsQ.clear();
    }
    //[q removeAllObjects];
    this->netCount=0;
    this->condPredR=false;
    //[lock unlock];;
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
 @discussion if resulting batch size exceeds the established upper bound, the operation will fail.
 @param obj the dictionary.
 @return true for success, NO otherwise.
 */
    bool castObjectToBatch(T obj){
        bool tval=false;
        if(obj){
            //std::lock_guard<std::mutex> guard(this->mlock);
            this->lock.lock();
            if(batchLimitUpper.load() < tempWBQ->many.size() + 1){
                this->lock.unlock();
                WASFKLog(CPPAFK_STR_Q_ULIMIT_VIOLATION);
                return false;
            }
            
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
        bool res=false;
        this->lock.lock();
        if(force==true){
            
        }
        else{
            if(batchLimitLower > tempWBQ->many.size() || batchLimitUpper < tempWBQ->many.size()){
                this->lock.unlock();
                WASFKLog(CPPAFK_STR_Q_LLIMIT_VIOLATION);
                return false;
            }
        }
        
        if(tempWBQ && tempWBQ->many.size() > 0){
            this->itsBQ.push_back(tempWBQ);
            //[q addObject:tempWBQ];
            netCount.fetch_add(tempWBQ->many.size());
            tempWBQ=new CPPAFKPriv_WrapBQ<T> ;
            res=true;
        }
        this->lock.unlock();
        return res;
    }
/**
 @brief resets cumulative batch. The batch will be cleared form all accumulated objects.
 */
    bool resetBatch(){
        bool res=false;
        std::lock_guard<std::mutex> guard(this->lock);
        
        if(tempWBQ != std::nullptr_t() && tempWBQ->many.size() >0){
            tempWBQ->many.clear();// removeAllObjects];
            res=true;
        }
        
        return res;
    }


void queueFromBatchingQueue(CPPAFKBatchingQueue<T>* otherq);
/*!
 @brief pauses reading from queue for batches. After invocation, reading method calls will return all items belonging to batch. When entire batch is read, next reading calls will return nil, until 'resume' method invoked.
 */
    void pause(){
        this->paused=true;
    }

/*!
 @brief resumes reading from queue for batches.
 */
    void resume(){
        this->paused=false;
    }

/*!
 @brief returns number of batches in queue.
 */
    CPPAFKQSize_t batchCount(){
        std::lock_guard<std::mutex> guard(this->lock);
        std::uint64_t qc=this->itsQ.size();
        return qc;
    }
/*!
 @brief pulls earliest batch's contents as array.
 */
    void pullBatchAsArray(std::vector<T>& result){
        if(this->paused.load()){
            return;
        }
        if(this->blocking.load())
        {
            std::unique_lock<std::mutex> lk(this->cvLockW);
            this->condPredR=false;
            this->cvW.wait(lk, [this]{
                this->lock.lock();
                this->condPredR=this->itsQ.size() > this->minQSize?true:false;
                this->lock.unlock();
                return this->condPredR.load();
            });
            lk.unlock();

        }
        
        CPPAFKPriv_WrapBQ<T>* subitem=std::nullptr_t();
        //if(paused==NO){
        this->lock.lock();
        std::uint64_t c=this->itsQ.size();
        if(c > 0)
        {
            CPPAFKPriv_WrapBQ<T>* item=this->itsQ.front();
            if(item)
            {
                typename std::deque<T>::iterator it = ((CPPAFKPriv_WrapBQ<T>*)item)->many.begin();
                for (; it != ((CPPAFKPriv_WrapBQ<T>*)item)->many.end(); ++it) {
                    result.push_back(*it);
                }
                ((CPPAFKPriv_WrapBQ<T>*)item)->many.clear();
                
                //if(subitem){
                    if(this->blocking.load())
                    {
                        std::unique_lock<std::mutex> lk(this->cvLockW);
                        ((CPPAFKPriv_WrapBQ<T>*)(*it))->wcPred=true;
                        lk.unlock();
                        ((CPPAFKPriv_WrapBQ<T>*)(*it))->cvW.notify_all();
                        

                    }
                    this->netCount.fetch_sub(c);
                    //[q removeObjectAtIndex:0];
                    this->itsQ.pop_front();
                //}
        }
        }
        this->lock.unlock();
        
        //    }
        //    else{
        //
        //    }
        
        //    return nil;
    }
};

/*!
 @brief ASFKBatchingQueue2
 @discussion internal use. Main purpose: providing 'call' methods without unblocking thread waiting to read.
 */
template <typename T> class CPPAFKBatchingQueue2 : public CPPAFKBatchingQueue<T>
{
protected:
    std::deque<CPPAFKPriv_WrapBQ<T>*> deferred;
public:
    T   pullAndBatchStatus(std::int64_t& itemsLeft, bool& endBatch, CPPAFKPriv_EndingTerm** term){
        //id subitem=nil;
        *term=std::nullptr_t();
        endBatch=false;
        this->lock.lock();
        if(this->deferred.size() == 0)
        {
            if(this->itsQ.size()>0)
            {
                CPPAFKPriv_WrapBQ<T>* item=this->itsQ.front();
                if(item->many.size() > 0)
                {
                    T subitem=item->many.front();
                    item->many.pop_front();
                    
                    this->netCount.fetch_sub(1);
                    if(item->many.size() == 0){
                        this->itsQ.pop_front();
                        //[q removeObjectAtIndex:0];
                        *term=CPPAFKPriv_EndingTerm::getInstance();
                        endBatch=true;
                        if(this->blocking.load()){
                            this->deferred.push_back(item);
                        }
                        else{
                            delete item;
                        }
                    } //VERIFY CORRECTNESS
                    this->lock.unlock();
                    return subitem;
                }
                
            }
            
        }
        else{
            
        }
        
        this->lock.unlock();
        return T();
    }
bool castObject(T item, CPPAFKExecutionParams * ex){
        if(item){
            CPPAFKPriv_WrapBQ<T>* wrap=new CPPAFKPriv_WrapBQ<T>;
            //wrap->single=item;
            wrap->many.push_back(item);
            this->lock.lock();
            this->itsQ.push_back(wrap);
            this->netCount.fetch_add(1);
            this->lock.unlock();
            return true;
        }
        return false;
    }

    
#pragma mark - Blocking interface
bool callObject(T& item, CPPAFKExecutionParams * params){
        if(this->blocking){
            if(item){
                CPPAFKPriv_WrapBQ<T>* wrap= new CPPAFKPriv_WrapBQ<T>;
                wrap->many.push_back(item);
                this->lock.lock();
                this->itsQ.push_back(wrap);
                this->netCount.fetch_add(1);
                this->lock.unlock();
                
                std::unique_lock<std::mutex> lk(this->cvLockW);
                if(params){
                    params->preBlock();
                }
                wrap->cvLockW.wait(lk,[wrap]{return wrap->wcPred.load();});
                lk.unlock();

                return true;
            }
        }
        else{
            return castObject(item, params);
        }
        return false;
    }
    
virtual void releaseFirst(){
        this->lock.lock();
        
        if(this->deferred.size()>0)
        {
            CPPAFKPriv_WrapBQ<T>* wrap = this->deferred.front();
            
            std::unique_lock<std::mutex> lk(this->cvLockW);
            wrap->wcPred=true;
            lk.unlock();
            wrap->cvW.notify_all();
            this->deferred.pop_front();
            delete wrap;
        }
        this->lock.unlock();
    }
virtual void releaseAll()
{
    typename std::deque<CPPAFKPriv_WrapBQ<T>*>::iterator it;
    this->lock.lock();
    for(it=this->deferred.begin();it!=this->deferred.end();++it){
        std::unique_lock<std::mutex> lk(this->cvLockW);
        ((CPPAFKPriv_WrapBQ<T>*)(*it))->wcPred=true;
        lk.unlock();
        ((CPPAFKPriv_WrapBQ<T>*)(*it))->cvW.notify_all();
        delete ((CPPAFKPriv_WrapBQ<T>*)(*it));
    }
    
    this->deferred.clear();
    this->lock.unlock();
};
};
/*!
 @brief ASFKBatchingQueue3
 @discussion internal use. Main purpose: providing 'call' methods without unblocking thread waiting to read.
 */
template <typename T> class CPPAFKBatchingQueue3 : public CPPAFKBatchingQueue2<T>
{
public:
    void releaseFirst()
    {
        std::lock_guard<std::mutex> guard(this->lock);
        
        if(this->deferred.size() > 0)
        {
            CPPAFKPriv_WrapBQ<T>* wrap = this->deferred.front();
            //[wrap->writecond lock];
            std::unique_lock<std::mutex> lk(wrap->cvLockW);
            wrap->wcPred=true;
            lk.unlock();
            wrap->cvW.notify_all();
            //[wrap->writecond broadcast];
            //[wrap->writecond unlock];
            //[deferred removeObjectAtIndex:0];
            this->deferred.pop_front();
            delete wrap;
        }
        //[lock unlock];
    }
    
    T   pullAndBatchStatus(std::uint64_t& itemsLeft, bool& endBatch, CPPAFKPriv_EndingTerm** term)
    {
            //id subitem=nil;
        *term=std::nullptr_t();
        endBatch=false;
        this->lock.lock();
        if(this->itsQ.size()>0)
        {
            CPPAFKPriv_WrapBQ<T>* item=this->itsQ.front();
            if(item->many.size() > 0)
            {
                T subitem=item->many.front();
                item->many.pop_front();
                //[((ASFKPriv_WrapBQ*)item)->many removeObjectAtIndex:0];
                this->netCount.fetch_sub(1);
                if(item->many.size() == 0){
                    this->itsQ.pop_front();
                    //[q removeObjectAtIndex:0];
                    *term=CPPAFKPriv_EndingTerm::getInstance();
                    endBatch=true;
                    if(this->blocking.load()){
                        this->deferred.push_back(item);
                    }
                    else{
                        delete item;
                    }
                } //VERIFY CORRECTNESS
                this->lock.unlock();
                return subitem;
            }
            
        }
        
            this->lock.unlock();
            return T();
    }
};

    template <typename T> class CPPAFKScheduler: public CPPAFKBase
    {
    protected:
        std::mutex lock;
        std::vector<CPPAFKQueue<T>*> sources;
        std::atomic<bool> paused;
    public:
        CPPAFKScheduler<T>():CPPAFKBase(){
            paused=false;
        }
#ifdef CPPAFK_USE_STR_OBJ_NAME
        CPPAFKScheduler<T>(std::string name):CPPAFKBase(name){};
#endif
        /**
         @brief pauses .
         */
        void pause(){ paused = true;}
        /**
         @brief resumes .
         */
        void resume(){ paused = false;}
        
        virtual T pull(bool& success)=0;
        
        bool replaceQueues(std::vector<CPPAFKQueue<T>*> qs){
            bool result=false;
            if(qs.size()>0){
                lock.lock();
                sources.clear();
                for(typename std::vector<CPPAFKQueue<T>*>::iterator it=qs.begin();it != qs.end();++it){
                    //CPPAFKQueue<T>* t=it;
                    if((*it)->isBlocking()){
                        WASFKLog("Blocking queues cannot be used with scheduler");
                        break;
                    }
                }
                lock.unlock();
                result=true;
            }
            else{
                sources.clear();
                result=true;
            }
            return result;
        }
        bool addQueue(CPPAFKQueue<T>* q){
            bool result=false;
            if(q != std::nullptr_t()){
                lock.lock();
                if(q->isBlocking){
                    WASFKLog("Blocking queues cannot be used with scheduler");
                }
                else{
                    sources.push_back(q);
                    result=true;
                }
                lock.unlock();
            }
            return result;
        }
        virtual void reset(){
            lock.lock();
            sources.clear();
            lock.unlock();
        }
        virtual void purge(){
            lock.lock();
            sources.clear();
            lock.unlock();
        }
        virtual ~CPPAFKScheduler<T>(){
            reset();
        };
    };
    
    template <typename T> class CPPAFKSchedulerNaiveRR: public CPPAFKScheduler<T>
    {
    protected:
        std::uint64_t lastIndex;
    public:
        CPPAFKSchedulerNaiveRR<T>():CPPAFKScheduler<T>(){
            lastIndex=0;
        };
#ifdef CPPAFK_USE_STR_OBJ_NAME
        CPPAFKSchedulerNaiveRR<T>(std::string name):CPPAFKScheduler<T>(name){
            lastIndex=0;
        }
#endif
        
        T pull(bool& success)
        {
            if(this->paused.load()){
                success=false;
                return T();
            }
            std::lock_guard<std::mutex> guard(this->lock);
            if(this->sources.size()>0){
                do
                {
                    bool success0=false;
                    T obj=this->sources[lastIndex]->pull(success0);
                    lastIndex=(lastIndex+1) % this->sources.size();
                    if(success0){
                        return obj;
                    }
                } while(lastIndex!=0);
            }
            
            return T();
        }
        void reset(){
            this->lock.lock();
            this->sources.clear();
            lastIndex=0;
            this->lock.unlock();
        }
        void purge(){
            this->lock.lock();
            this->sources.clear();
            lastIndex=0;
            this->lock.unlock();
        }
        ~CPPAFKSchedulerNaiveRR<T>(){
            reset();
        };
    };
    
    struct CPPAFKThreadpoolConfig{
        CPPAFKQSize_t share;
        CPPAFKQSize_t residue;
        CPPAFKQSize_t actualThreadsCount;
        CPPAFKQSize_t requiredThreadsCount;
    };
    
    struct CPPAFKThreadpoolConfigRange{
        long lowBound;
        long length;
    };
    
    typedef  std::function<bool(CPPAFKNumId)> CPPAFKTPSessionCallback_t;
    
    class CPPAFKThreadpoolSession:public CPPAFKBase{
    protected:
        std::vector<CPPAFKQueue<CPPAFKBasicDataObjectWrap>*> dataQueues;
        CPPAFKCancellationRoutine_t cancellationHandler;
        CPPAFKExpirationRoutine_t expirationSummary;
        CPPAFKExpirationCondition* excond;
        CPPAFKExecutableRoutineSummary_t passSummary;
        std::vector<CPPAFKExecutableRoutine_t> procs;
        std::mutex itsLock;
        CPPAFKControlBlock* cblk;
        std::atomic<bool> paused;
        std::atomic<bool> isStopped;
        std::atomic<bool> cancelled;
        void _invokeCancellationHandler(CPPAFKCancellationRoutine_t cru, CPPAFKNumId identity);
    public:
        std::atomic<eCPPAFKBlockingCallMode> callMode;
        CPPAFKThreadpoolSession():CPPAFKBase(){
            paused = false;
            callMode = AFK_BC_NO_BLOCK;
            cblk = new CPPAFKControlBlock();
            cancelled=false;
            isStopped=false;
            excond=std::nullptr_t();
        }
        
        virtual std::uint64_t getRoutinesCount()=0;
        CPPAFKControlBlock* getControlBlock();
        
        void setExpirationSummary(CPPAFKExpirationRoutine_t sum);
        void setCancellationHandler(CPPAFKCancellationRoutine_t cru);
        void setProgressRoutine(CPPAFKProgressRoutine_t progress);
        void setExpirationCondition(CPPAFKExpirationCondition* trop);
        void setSummary(CPPAFKExecutableRoutineSummary_t sum);
        bool cancellationRequested();
        bool cancellationRequestedByCallback();
        bool cancellationRequestedByStarter();
        void flush();
        void pause();
        void resume();
        virtual void cancel();
        virtual CPPAFKQSize_t getDataItemsCount();
        virtual bool isBusy();
        bool isPaused(){return paused;};
        virtual void replaceRoutinesWithArray(std::vector<CPPAFKExecutableRoutine_t>& ps)=0;
        virtual bool postDataAsVector(std::vector<CPPAFKBasicDataObjectWrap>& data, bool blk)=0;
        //virtual bool postExecUnitAsVector(std::vector<CPPAFKExecUnitWrap>&,bool)=0;
        virtual void select(std::uint64_t, CPPAFKTPSessionCallback_t&)=0;
        virtual ~CPPAFKThreadpoolSession(){
            delete cblk;
        }
    };
    
    struct CPPAFKThreadpoolState{
        std::uint16_t numCPUs;
        //std::map<CPPAFKNumId,CPPAFKThreadpoolSession*> runningSessions;
        std::vector<CPPAFKThreadpoolSession*> onlineSessions;
        std::vector<CPPAFKThreadpoolSession*>  killedSessions;
        //std::map<CPPAFKNumId,CPPAFKThreadpoolSession*>  pausedSessions;
        std::map<CPPAFKNumId,CPPAFKThreadpoolSession*,cmpNumericalId64>  allSessions;
        CPPAFKThreadpoolConfig tpcfg;
        std::mutex lkMutex1;
        std::vector<CPPAFKThreadpoolConfigRange> vectProc2Bounds;
        
        CPPAFKThreadpoolState(){ numCPUs = 1; };
        void reassignProcs(CPPAFKThreadpoolConfig& tpc, CPPAFKQSize_t reqThreads);
        void cloneSessionsMap2Vector();
    };
    /**
     *  @brief Main global threadpool.
     */
    class CPPAFKThreadpool{
        std::vector<std::thread> threads;
        CPPAFKThreadpoolState itsState;
    public:
        static CPPAFKThreadpool* getInstance();
        //private:
        CPPAFKThreadpool();
        ~CPPAFKThreadpool();
        void createThreads(std::uint32_t numProcs);
        CPPAFKQSize_t runningSessionsCount();
        CPPAFKQSize_t pausedSessionsCount();
        bool isPausedSession(CPPAFKNumId sessionId);
        bool isBusySession(CPPAFKNumId sessionId);
        void getThreadpoolSessionsList(std::vector<CPPAFKNumId>& out);
        bool addSession(CPPAFKThreadpoolSession* aseq, CPPAFKNumId identity);
        CPPAFKThreadpoolSession* getThreadpoolSessionWithId(CPPAFKNumId identity);
        CPPAFKQSize_t totalSessionsCount();
        CPPAFKQSize_t itemsCountForSession(CPPAFKNumId sessionId);
        void flushSession(CPPAFKNumId sessionId);
        void flushAll();
        void cancelSession(CPPAFKNumId sessionId);
        void cancelAll();
        void pauseSession(CPPAFKNumId sessionId);
        void resumeSession(CPPAFKNumId sessionId);
        void pauseAll();
        void resumeAll();
        bool setProgressRoutine(CPPAFKProgressRoutine_t progress);
        
        bool isBusy();
        
        bool postDataAsVector(std::vector<CPPAFKBasicDataObjectWrap>& data, CPPAFKNumId sessionId, bool blk);
        //bool postExecUnitAsVector(std::vector<CPPAFKExecUnitWrap>& data, CPPAFKNumId sessionId, bool blk, bool dataModeOn);
        
    private:
        void _reassignProcs(CPPAFKThreadpoolConfig& tpc);
    };
    
    struct CPPAFKPriv_Wrapper{
        CPPAFKBasicDataObjectWrap *execDataItem;
        CPPAFKExecUnitWrap *execUnit;
    };
    
    class CPPAFKComposingSession:public CPPAFKThreadpoolSession{
    protected:
        eCPPAFKBlockingCallMode itsBlockingMode;
        bool datamode;
        
    public:
        CPPAFKComposingSession(eCPPAFKBlockingCallMode blkmode, bool datamodeOn):CPPAFKThreadpoolSession(){
            itsBlockingMode=blkmode;
            datamode=datamodeOn;
        }
        ~CPPAFKComposingSession(){ }
        void cancel();
        CPPAFKQSize_t getDataItemsCount();
        bool isBusy();
        void replaceRoutinesWithArray(std::vector<CPPAFKExecutableRoutine_t>& ps);
        bool postDataAsVector(std::vector<CPPAFKBasicDataObjectWrap>& data, bool blk);
        //bool postExecUnitAsVector(std::vector<CPPAFKExecUnitWrap>&,bool);
        void select(std::uint64_t, CPPAFKTPSessionCallback_t&);
        void _invokeCancellationHandler(CPPAFKCancellationRoutine_t cru, CPPAFKNumId identity);
        std::uint64_t getRoutinesCount();
    };
    
    class CPPAFKPar: public CPPAFKBase{
    protected:
        CPPAFKThreadpool* globalTPool;
        std::map<CPPAFKNumId,CPPAFKControlBlock*> itsCtrlBlocks;
    public:
        CPPAFKPar();
        ~CPPAFKPar();
        std::uint64_t itemsCountForSession(CPPAFKNumId sessionId);
        std::uint64_t totalSessionsCount();
        bool isPausedSession(CPPAFKNumId sessionId);
        
        bool isBusySession(CPPAFKNumId sessionId);
        
        std::uint64_t getRunningSessionsCount();
        std::uint64_t getPausedSessionsCount();
        virtual bool cast(CPPAFKBasicDataObjectWrap, CPPAFKExecutionParams*)=0;
        virtual bool call(CPPAFKBasicDataObjectWrap, CPPAFKExecutionParams*)=0;
        void cancelAll();
        
        void cancelSession(CPPAFKNumId sessionId);
        
        void flushAll();
        
        void flushSession(CPPAFKNumId sessionId);
        
        void pauseAll();
        
        void pauseSession(CPPAFKNumId sessionId);
        
        void resumeAll();
        
        void resumeSession(CPPAFKNumId sessionId);
        
        void getSessions(std::vector<CPPAFKComposingSession*>& sessions);
#ifdef CPPAFK_USE_STR_OBJ_NAME
        virtual CPPAFKNumId createSession(CPPAFKSessionConfigParams* exparams, std::string& sid)=0;
#endif
        virtual CPPAFKNumId createSession(CPPAFKSessionConfigParams* exparams)=0;
        
    };
    
    class CPPAFKCompositionPar:public CPPAFKPar{
    protected:
        CPPAFKComposingSession* _createNewSessionWithId(CPPAFKNumId sessionId, eCPPAFKBlockingCallMode blkMode, bool dataModeOn);
        bool _decodeParams(CPPAFKSessionConfigParams* params);
    public:
        CPPAFKCompositionPar();
        ~CPPAFKCompositionPar();
#ifdef CPPAFK_USE_STR_OBJ_NAME
        CPPAFKNumId createSession(CPPAFKSessionConfigParams* exparams, std::string& sid);
#endif
        CPPAFKNumId createSession(CPPAFKSessionConfigParams* exparams);
        bool cast(CPPAFKBasicDataObjectWrap, CPPAFKExecutionParams*);
        bool call(CPPAFKBasicDataObjectWrap, CPPAFKExecutionParams*);
    };

    
};
#ifdef CPPAFK_ENABLE_MAILBOX
#include "CPPAFKMailbox.hpp"
#endif

#endif /* CPPAFKBase_hpp */
