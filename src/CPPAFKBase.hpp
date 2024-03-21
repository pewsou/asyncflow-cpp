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

#ifndef CPPAFKBase_hpp
#define CPPAFKBase_hpp

#define CPPAFK_VERSION "0.1.0-A"

#define CPPAFK_NUM_OF_HW_THREADS (-1)

#define CPPAFK_DEBUG_ENABLED 1
#define CPPAFK_MSG_WARNING_ENABLED 1
#define CPPAFK_USE_STR_OBJ_NAME 1
#define CPPAFK_MSG_COMMON_ENABLED 1
#define CPPAFK_DATAWRAP_VIRTUAL_ENABLED 1
#define CPPAFK_DATA_RELEASING_BATCH_SIZE 1000

#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <cstdint>
#include <limits>
#include <deque>
#include <queue>
#include <condition_variable>
#include <random>
#include <iostream>
#include <sstream>
#include <functional>
#include <map>
#include <unordered_map>

#define CPPAFK_STR_UNSUPPORTED_OP "Operation not supported"
#define CPPAFK_STR_Q_ULIMIT_VIOLATION "new upper limit is not greater than lower limit"
#define CPPAFK_STR_Q_LLIMIT_VIOLATION "new lower limit is not less than upper limit"
#define CPPAFK_STR_Q_LIMIT_VIOLATION "queue size limits violated"
#define CPPAFK_STR_Q_BTCH_LIMIT_VIOLATION "batch size limits violated"
#define CPPAFK_STR_MISCONFIG_OP "operation not confgured properly"
#define CPPAFK_STR_UNAVAIL_VER_OP "unavailable in this version"

#define CPPAFK_PRIVSYM_OBJ_RELEASE_SAMPLE_SIZE 1000

#ifdef CPPAFK_DATAWRAP_VIRTUAL_ENABLED
#define CPPAFK_VIRTUAL virtual
#else
#define CPPAFK_VIRTUAL
#endif


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

#define WASFKLog_2(str,arg)
#endif

#ifdef CPPAFK_MSG_ERROR_ENABLED
#define EASFKLog(str) {std::cout<<"ERROR "<<__FILE__<<" : "<<__LINE__<<" : "<<__FUNCTION__<<" : "<<str<<std::endl;}

#define EASFKLog_2(str,arg) {std::cout<<"ERROR "<<__FILE__<<" : "<<__LINE__<<" : "<<__FUNCTION__<<" : "<<str<<arg<<std::endl;}
#else
#define EASFKLog(str)

#define EASFKLog_2(str,arg)
#endif

#ifdef CPPAFK_MSG_COMMON_ENABLED
#define COMASFKLog(str) {std::cout<<"COMMON "<<__FILE__<<" : "<<__LINE__<<" : "<<__FUNCTION__<<" : "<<str<<std::endl;}

#define COMASFKLog_2(str,arg) {std::cout<<"COMMON "<<__FILE__<<" : "<<__LINE__<<" : "<<__FUNCTION__<<" : "<<str<<"|"<<arg<<std::endl;}
#else
#define COMASFKLog(str)
#define COMASFKLog_2(str,arg)
#endif

#define CPPAFK_DATABLOCK_PROC_PARAMS CPPAFKBasicDataObjectWrap* in, CPPAFKControlBlock& cb, CPPAFKNumId sessionId

#define CPPAFK_HRI_PRIMARY 0
#define CPPAFK_HRI_SECONDARY 1

namespace AsyncFlowKit{
    
    typedef std::string     tCPPAFK_SESSION_ID_TYPE;
    typedef std::int64_t    CPPAFKQSize_t;
    typedef std::uint64_t   CPPAFKThreadpoolSize_t;
    typedef std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> TimePtNSec_t;
    typedef std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::milliseconds> TimePtMSec_t;
    typedef std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::microseconds> TimePtMkSec_t;
    typedef std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::seconds> TimePtSec_t;
    typedef enum enumCPPAFK_ORDERING{
        AFK_ORD_BEFORE,
        AFK_ORD_AFTER,
        AFK_ORD_SAME,
        AFK_ORD_UNKNOWN
    } eCPPAFKOrdering;
    typedef enum enumCPPAFK_TIMESCALE{
        AFK_TMS_NANO,
        AFK_TMS_MICRO,
        AFK_TMS_MILLI,
        AFK_TMS_SEC
    } eCPPAFKtimescale;
    typedef enum enumCPPAFK_BLOCKING_CALL_MODE{
        /**
         @brief AFK_BC_NO_BLOCK stands for no blocking allowed - calls on session in this mode will be rejected.
         */
        AFK_BC_NO_BLOCK,
        /**
         @brief AFK_BC_WAIT_FOR_FIRST stands for interleaving mode - once processing of blocked batch started, first element of the next batch will be fetched for processing immediately after the last element of previous batch started processing. I.e. processing of the next batch will not wait until the previous batch has been fully processed.
         */
        AFK_BC_WAIT_FOR_FIRST,
        /**
         @brief AFK_BC_WAIT_FOR_LAST means that processing of the next batch will start only after the previous batch has been fully processed, i.e. the last item in batch was processed.
         */
        AFK_BC_WAIT_FOR_LAST
    } eCPPAFKBlockingCallMode;
    
    
    typedef enum enumCPPAFKPipelineExecutionStatus{
        eAFK_ES_HAS_NONE=0,
        eAFK_ES_HAS_MORE,
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
    
    struct CPPAFKUtilities{
        static CPPAFKNumId createRandomNum();
        static std::chrono::time_point<std::chrono::system_clock> getTimePoint();
        static std::uint64_t getTimeInterval(std::chrono::time_point<std::chrono::system_clock>& start,std::chrono::time_point<std::chrono::system_clock>& end);
    };
    
    struct CPPAFKRange{
        std::int64_t location;
        std::int64_t length;
        CPPAFKRange(){
            location=0;
            length=0;
        }
    };
    
    /**
     @brief custom routine for handlig progress report
     @param current current value
     @param outOf final value to be achieved
     @param stage index of stage where the report takes place
     */
    typedef std::function<void(std::uint64_t current, std::uint64_t outOf, std::uint64_t stage)> CPPAFKProgressRoutine_t;
    
    class CPPAFKControlBlock
    {
    protected:
        std::mutex itsLock;
        CPPAFKProgressRoutine_t itsProgressProc;
        std::uint64_t totalProcessors;
        std::atomic<CPPAFKThreadpoolSize_t> itsResPosition;
        std::atomic<CPPAFKThreadpoolSize_t> indexSecondary;
        CPPAFKNumId parentId;
        CPPAFKNumId sessionPrimId;
        CPPAFKNumId sessionSecId;
        std::atomic< bool> paused;
        std::atomic< bool> flushed;
        std::atomic< bool> abortByCallback;
        std::atomic< bool> abortByCaller;
        std::atomic< bool> abortByInternal;
        
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
        CPPAFKNumId getSessionId(){return sessionPrimId;};
        bool setProgressRoutine(CPPAFKProgressRoutine_t progress);
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
        void updateProgressStatus(std::uint64_t current, std::uint64_t outOf, std::uint64_t stage);
        CPPAFKNumId getParentObjectId();
        CPPAFKProgressRoutine_t getProgressRoutine();
    };
    
    class CPPAFKBasicDataObjectWrap;
    typedef std::function<CPPAFKBasicDataObjectWrap* (CPPAFK_DATABLOCK_PROC_PARAMS)> CPPAFKExecutableRoutine_t;
    struct CPPAFKExecUnitWrap{
        CPPAFK_VIRTUAL CPPAFKBasicDataObjectWrap* runM(CPPAFK_DATABLOCK_PROC_PARAMS){return std::nullptr_t();};
        CPPAFKExecutableRoutine_t runL;
        CPPAFKExecUnitWrap(){runL=std::nullptr_t();}
        CPPAFK_VIRTUAL ~CPPAFKExecUnitWrap(){ };
    };
    
    
    /**
     *  @brief Basic data object for processing.
     */
    struct CPPAFKBasicDataObjectWrap{
    public:
        bool runnable;
        bool isTerminal(){return false;}
        CPPAFKExecUnitWrap execUnit;
        CPPAFKBasicDataObjectWrap(){
            runnable=false;
        };
        
        CPPAFK_VIRTUAL ~CPPAFKBasicDataObjectWrap(){

        };
    };
    
    typedef enum enumCPPAFKRunPriorityML{
        AFK_RUN_METHOD_ONLY=0,
        AFK_RUN_LAMBDA_ONLY,
        AFK_RUN_LAMBDA_THEN_METHOD,
        AFK_RUN_METHOD_THEN_LAMBDA
    } eCPPAFKRunPriorityML;
    typedef enum enumCPPAFKRunPriorityDR{
        AFK_RUNR_STORED_ROUTINE_ONLY=0,
        AFK_RUNR_DATAOBJ_ONLY,
        AFK_RUNR_ROUTINE_THEN_DATAOBJ,
        AFK_RUNR_DATAOBJ_THEN_ROUTINE
    } eCPPAFKRunPriorityDOR;
    typedef std::function<CPPAFKBasicDataObjectWrap* (CPPAFK_DATABLOCK_PROC_PARAMS)> CPPAFKExecutableRoutineSummary_t;
    

    typedef std::function<void( CPPAFKNumId sid)> CPPAFKCancellationRoutine_t;
    typedef std::function<void()> CPPAFKExpirationRoutine_t;

    typedef std::function<void()> CPPAFKOnPauseNotification_t;
    
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
    

#include "CPPAFKCondition.hpp"

    struct CPPAFKConfigParams{
        CPPAFKProgressRoutine_t progressRoutine;
        CPPAFKExecutableRoutineSummary_t summaryRoutine;
        std::vector<CPPAFKExecUnitWrap> routines;
        CPPAFKCancellationRoutine_t cancellationRoutine;
        CPPAFKCancellationRoutine_t internalCancellationRoutine;
        CPPAFKExpirationCondition* expCondition;
        CPPAFKOnPauseNotification_t onPauseProc;
        CPPAFKExpirationRoutine_t expSummary;
        bool autodelete;
        CPPAFKConfigParams(){
            summaryRoutine=std::nullptr_t();
            progressRoutine=std::nullptr_t();
            cancellationRoutine=std::nullptr_t();
            expCondition=std::nullptr_t();
            onPauseProc=std::nullptr_t();
            expSummary=std::nullptr_t();
            autodelete=false;
        };
    };
    struct CPPAFKExecutionParams:public CPPAFKConfigParams{
        std::function<void()> preBlock;
    };
    
    struct CPPAFKSessionConfigParams:public CPPAFKConfigParams{
        std::string sessionName;
        eCPPAFKBlockingCallMode blockCallMode;
        std::vector<CPPAFKExecUnitWrap> routines;
        eCPPAFKRunPriorityML rpML;
        eCPPAFKRunPriorityDOR rpDOR;
        CPPAFKSessionConfigParams(){
            blockCallMode=AFK_BC_NO_BLOCK;
            sessionName="";
            rpML=AFK_RUN_LAMBDA_ONLY;
            rpDOR=AFK_RUNR_STORED_ROUTINE_ONLY;
        }
    };
    
    
    struct cmpNumericalId64 {
        bool operator()(const CPPAFKNumId& a, const CPPAFKNumId& b) const {
            return a.num64 < b.num64;
        }
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
        
        CPPAFKControlBlock* newCtrlBlock();
        CPPAFKControlBlock* newCtrlBlock(CPPAFKNumId sessionId,CPPAFKNumId subId);
        
        CPPAFKControlBlock* getControlBlockWithId(CPPAFKNumId blkId);
    protected:
        void registerCtrlBlock(CPPAFKControlBlock* cblk);
        void forgetAllCtrlBlocks();
        void forgetCtrlBlock(CPPAFKNumId sessionId);
        void forgetCtrlBlock(CPPAFKControlBlock* cb);
        std::map<CPPAFKNumId, CPPAFKControlBlock*, cmpNumericalId64> itsCtrlBlocks;
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


    
#include "CPPAFKQueue.hpp"
    
    template <typename T> struct CPPAFKPriv_WrapBQ{
        std::condition_variable cvW;
        std::mutex cvLockW;
        std::atomic<bool> condPredW;
        std::atomic<bool> discarded;
        std::deque<T> many;
        bool waitUntilRead; //LEU15625885
        CPPAFKPriv_WrapBQ(){
            waitUntilRead=false;
            condPredW=false;
            discarded=false;
        };
        void _release(){
            condPredW=false;
            cvW.notify_all();
            discarded=true;
        }
        ~CPPAFKPriv_WrapBQ(){
            int x=0;
            ++x;
        }
    };
    
    typedef enum enumQSpecificity{
        ASFK_E_QSPEC_NONE,
        ASFK_E_QSPEC_REG,
        ASFK_E_QSPEC_BAT
    } eQSpecificity;
    
    typedef std::deque<std::pair<eQSpecificity,CPPAFKThreadpoolSize_t>> CPPAFKQMapper_t;


#include "CPPAFKBatchingQueue.hpp"
#include "CPPAFKThreadpool.hpp"
    
#include "CPPAFKComposition.hpp"
    
};



#endif /* CPPAFKBase_hpp */
