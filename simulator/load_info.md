Information on the load/store lifecycle. Specifically relating to the construction of requests and packets.

Focus is on loads.

## Fetch
No special behavior for loads.

## Decode
No special behavior for loads.

## Rename
- Contitionally stall on loads/stores based on the LQ/SQ respectively, depending whether free entries are available.

- Serialize instructions are handled here -- they are forced to wait in Rename until the ROB is empty.
  - We don't have to have any handling for serialize instructions then; they wouldn't enter the LQ/SQ until they have serialized.

- Counts the number of loads and stores in progress
  - We should NOT increment the loads in progress statistic: these statistics are used to determine the number of free LQ/SQ entries

## IEW
- Calls `ldstQueue.tick()` before anything else (see [lsq:tick](#lsq-tick))
- For squashed loads/stores: tell rename they've been processed and drop them.
- Blocks on LQ/SQ full if a load/store is found (respectively).
  - Great for us; another thing *not* to handle.
- Atomic stores are inserted into ldstQueue and immediately `setCanCommit`.
- loads go to [lsq:insertLoad](#lsq-insertload).
- stores go to [lsq:insertStore](#lsq-insertstore).
  - if storeconditional, immediately `setCanCommit` and don't add to iq
  - add to iq if not conditional
- Add to IQ
- [Execute instructions](#execute-insts)
- [Writeback instructions](#writeback-insts)
- schedule insts for next cycle
- Use left over bandwidth to:
    - writebackstores
    - attempt to issue loads for silent stores
- commit stores and loads
    - if instruction seqNum is less than youngest instruction, commit load (remove from LQ)
    - set canWB for stores (skip here if store is waiting for load -- it's less overhead/cleaner); breaks out of loop if instruction seqNum is greater than youngest instruction

## LSQ: tick()
- Re-Issues loads which got blocked on the per-cycle load ports limit.
    - Calls Inst_Queue `cacheUnblocked()` if all load ports were marked used, but the cache has since unblocked.
    - *Should* be fine for us: this is called before we attempt to issue loads -- giving priority to anything in the `retryMemInsts` list. We just have to make sure our load attempts don't make it here.

## LSQ: insertLoad()
- LQEntry is LSQEntry (`using LQEntry = LSQEntry`)
- Get tid, then pass to LSQUnit
  1. Grow load queue.
  2. `inst->sqIT = storeQueue.end()`. This iterator is used as the starting
     point (from here to head) for store/load forwarding
  3. Set the Queue back to the inst.
  4. Set the *load queue index* `inst->lqIdx = loadQueue.tail()`.
  5. Set the *load queue iterator* `load_inst->lqIt = loadQueue.getIterator(load_inst->lqIdx)`.
  6. Increment number of loads in LQ

## LSQ: insertStore()
- Get tid, then pass to LSQUnit
    1. Grow store queue.
    2. Set *store queue index* `store_inst->sqIdx = storeQueue.tail()`
    3. Set *load queue index* `store_inst->lqIdx = loadQueue.moduloAdd(loadQueue.tail(), 1)`. *If this is a store, why are we setting load queue index?*
    4. Set *load queue iterator* `store_inst->lqIt = loadQueue.end()`.
        - This iterator will be used when checking for memory ordering violations
        - We may be able to use this for (silent stores) LQ value forwarding!
    5. set storequeue back to this inst `storeQueue.back().set(store_inst)`

## Execute Insts
- if squashed, `setExecuted` and `setCanCommit`
- if `isMemRef`, calculate address
- [call](#execute-load) `executeLoad` for loads
    - if delayed translation: `deferMemInst(load_inst)`
- [call](#execute-store) `executeStore` for stores
    - if delayed translation: `deferMemInst(store_inst)`
    - fault could indicate lack of mem req; send it to commit without completing
    - likely so that it can later cause a squash
    - `setExecuted` and `instToCommit`

## Execute Store
* Get tid, then pass to LSQUnit
  1. Get store index from instruction
  2. Initiate Acc `store_inst->initiateAcc()`
      - calculates effective address
      - call `pick(Data, 2, dataSize)` ???
      - call writeMemTiming in memhelpers.hh if `NoFault`. (for `St::` type)
      - eventually ends up at `cpu->pushRequest`; which leads to [lsq:pushRequest](#lsq-pushrequest)
  3. returns `NoFault` if translation delayed
  4. handles predicates
  5. fault on PC ???
  6. Check for memory order violations; starting at `store_inst->lqIt`

## Execute Load
* Get tid, then pass to LSQUnit
    1. InitiateAcc() `load_fault = inst->initiateAcc()`
      - call initiateMemRead in BaseDynInst. (for `Ld::` type)
          1. assert byte enable *is* empty **or** byte enable size is equal to dataSize
          2. `cpu->pushRequest` with `this` as the instruction pointer, `ld=true`,
            `data=nullptr`, `res=nullptr`, `amo_op=nullptr`.
          3. calls lsq's `pushRequest`: [lsq:pushRequest](#lsq-pushrequest).
    2. handle MemAccPredicate
    3. Return `NoFault` if `isTranslationDelayed()`
    4. handle partial faults (return `NoFault`)
    5. handle predication and faults (`instToCommit` and conditionally `setExecuted`)
    6. check violations with LQ (only if checkloads)
* I think `initiateAcc` is the brunt of it and the rest is really just error handling.
* We should be mindful of the faults returned -- it's likely that we need to handle some (eg cache misses cause delay?)

## LSQ pushRequest
1. Get tid using the instruction context
    - we can just get the TID directly?
    - Actually, instruction context should be fine since we're using the store
      inst
2. Determine whether burst is needed using `transferNeedsBurst` (address, size,
    and cacheLineSize)
3. if `translationStarted`, inst has a saved req
4. if not `translationStarted`, req is either a `singleDataRequest` or a
    `splitDataRequest`; depending on `needs_burst`
5. sets `byeEnable` in the request
6. sets the instruction's request `inst->setRequest()`
7. sets the request's taskId `req->taskId(cpu->taskId())`
8. Reset inst's fault `inst->getFault() = NoFault`
9. InitiateTranslation on the request (*BELOW IS SINGLEDATAREQUEST PROCESS ONLY*)
    1. Can only be called if `_requests.size() == 0`
    2. Calls `addRequest`; request is only added if `byte_enable.empty()` **or**
       any elements in `byte_enable` are active
    3. If request added to `_requests`, set `instSeqNum`, set `taskId`, set
       `translationStarted=true`, set state `Translation`, set flags
       `TranslationStarted`, set instruction's request to `this`
    4. Send fragment to translation (sends to `TLB`)
        - ***CAUTION*** also sends `TLB::Read` or `TLB::Write`
        - Translation can be `finish` or `markDelayed`
10. If translation is complete
    1. set instruction's effective addresss using req's vaddr
    2. set instruction's size
    3. attempt `cpu->read` or `cpu->write` (depending on `isLoad`)
        - [cpu->read](#cpu-read)
        - [cpu->write](#cpu-write)
        - only if `isMemAccessRequired`

## SingleDataRequest
- child class of LSQRequest
- port is `thread[tid]` and points to an LSQUnit
- address is effective address as calculated in `initiateAcc`
- constructor is parent constructor: sets `isLoad`, `WBStore`, and `isAtomic`
    flags; *and* `install()`s the request
    - `install()` sets the request in the LQ/SQ as
        `loadQueue[_inst->lqIdx].setRequest(this)`
    - ***We need to be careful of this***. If called naively, it *will*
        overwrite the request of something in the loadqueue (or possibly
        segfault if that entry is null)

## cpu->read()
- Leads to `read()` in LSQUnit
- See this step for rescheduling of memory instructions (is this something we
    ever need to do? Seems like no...)
1. get load_req out of LQ using idx
2. get the load instruction
3. Set the load_req's req to the req from pushRequest (either newly created or
   loaded from `inst->savedReq`)
    - Seems redundant after install?
4. check for store forwarding, using the sqIt as the starting point
    - skipping logic for now
    - @pradyumna TODO
5. Allocate memory in the load instruction (memData)
6. Setup sender state
7. ***buildPackets()***
8. ***sendPacketToCache()***
9. if not sent, `blockMemInst`
    - We should do this for the stores if they cause stalls while waiting for
      silent load

## cpu->write()
- leads to `write()` in LSQUnit
- stores are never rescheduled - makes sense, they either commit or not
1. `setRequest` using SQ index (request is set to the request from `pushRequest`)
2. set SQ size (get from req)
3. Copy data into the SQ
    - This is what we'll compare with! `storeQueue[store_idx].data()`
4. always returns no fault

## Writeback Insts
1. if `!isSquashed` and `isExecuted` and `NoFault`, wake dependents and mark registers as ready
    - We shouldn't fall into this category until after silencing/not

## Other things to check/watch out for
- inst->isSquashed()
  - Don't attempt to build packets or issue loads for squashed instructions
- `block(tid)`: should we call this if the head of the SQ is attempting to be silent, but it's waiting for the load value?
  - Need to also handle unblocking.
- AMO == atomic memory operation. Our amo_functors will always be nullptr
    because we're not touching atomic operations.
- by *not* calling `initiateAcc`, the ***effective address is wrong***!
  - we **should** be alright, if we get it from the request. The req address is set in pushRequest which is called with the calculated EA
  - `EA = SegBase + bits(scale * Index + Base + disp, addressSize * 8 - 1, 0)`
- interesting note in writebackInsts:
    > strictly ordered loads are sent to commit without executing because commit tells the LSQ when to execute the load
    - should we be looking at this to replicate this mechanism?
