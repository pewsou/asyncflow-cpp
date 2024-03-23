# asyncflow-cpp
Cpp version of Asynchronous Flow Kit (https://github.com/pewsou/asyncflow-objc).

C++ 2011 compatible. Tested with OS X 10.12.

Currently available facilities:

*Threadpool*: 

can create and run in parallel virtually unlimited number of isolated sessions. Each session stores set of functions.

Session types available:

Composition - parallel execution pattern mimicking Single Instruction Multiple Data approach.
There is stored array of functions, F0...Fn .
Each supplied data item Dk is processed in next manner: Fn(...F2(F1(F0(Dk)))).

*Queues*: 

simple FIFO-style queues that provide size boundaries, upper and lower. Additionaly queues may perform blocking calls, i.e. wait for first insert in queue or wait for first extraction from the queue.

*Batching queues*:
    
FIFO-style queues that may aggregate inserted data in batches.


# How it works?
Short manual for Threadpool

create session configuration
    #import "CPPAFKBase.hpp"
    AsyncFlowKit::CPPAFKSessionConfigParams scparams;
    std::string name="123";
    scparams.sessionName=name;
    AsyncFlowKit::CPPAFKExecUnitWrap w2;

create function for data processing

    w2.runL=[=](AsyncFlowKit::CPPAFKBasicDataObjectWrap* in, AsyncFlowKit::CPPAFKControlBlock& cb, AsyncFlowKit::CPPAFKNumId sessionId)->AsyncFlowKit::CPPAFKBasicDataObjectWrap*{
        /* do whatever desired */
        return in;
    };

add function to configuration 
    
    scparams.routines.push_back(w2);

create session with this configuration.

    AsyncFlowKit::CPPAFKThreadpool* executor=AsyncFlowKit::CPPAFKThreadpool::getInstance();
    AsyncFlowKit::CPPAFKNumId session_id = executor->createComposingSession(&scparams);

Once session created, we may feed data into it; in this case - asynchronously:

    AsyncFlowKit::CPPAFKBasicDataObjectWrap dobj;
    bool castres=executor->castObjectForSession(session_id,&dobj,  std::nullptr_t());

Also, we may pause this session:

    bool castres=executor->pauseSession(session_id);

Resume:

    bool castres=executor->resumeSession(session_id);

Flush from this session all the data waiting for processing:

    bool castres=executor->flushSession(session_id);

And even destroy the session:

    bool castres=executor->cancelSession(session_id);

At this point the session will be completely eliminated.

## Short manual for queues:
create queue:

    AsyncFlowKit::CPPAFKQueue<double>* pfq=new AsyncFlowKit::CPPAFKQueue<double>("queue1",false);

set its minimal size - data cannot be extracted if the size of the queue is below this value

    bool r=pfq->setMinQSize(2);

set maximum size - data cannot be inserted if the queue's size is beyond this value

    bool r=pfq->setMaxQSize(10);

Insert data asynchronously - return immediately, no matter if insertion succeeded.

    bool r=pfq->castObject(1.0, std::nullptr_t());

extract item:

    bool success=false;
    double item0=pfq->pull(success);

## Short manual for batching queue:
Create queue:

    AsyncFlowKit::CPPAFKBatchingQueue<double>* abq=new AsyncFlowKit::CPPAFKBatchingQueue<double>("122",true);;

insert item - it will create batch of 1

    bool b=abq->castObjectToBatch(1.5);

Create another batch:

    bool b=abq->castObjectToBatch(1.5);
    b=abq->castObjectToBatch(1.6);
    cres=abq->commitBatch(true);

Now we have have FIFO queue with 2 batches:

first batch contains 1 item

second batch contains 2 items.

Now we will extract item from first batch
    bool opsucc;
    double x=abq->pull(opsucc);

And then from the second:

    x=abq->pull(opsucc);
    x=abq->pull(opsucc);

There are no more batches in the queue!

## Upcoming:

Filtering queue: FIFO-style queue, that allows to silently discard items by different criteria.

Mailbox - embedded facility for asynchronous message exchange.

Contact:

Bugreport: https://chat.whatsapp.com/B6rj5jGLcV1J1noUtla4vd

[![Twitter URL](https://img.shields.io/twitter/url/https/twitter.com/bvprojs.svg?style=social&label=Follow%20%40bvprojs)](https://twitter.com/bvprojs)
