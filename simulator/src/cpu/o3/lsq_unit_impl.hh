
/*
 * Copyright (c) 2010-2014, 2017-2019 ARM Limited
 * Copyright (c) 2013 Advanced Micro Devices, Inc.
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2004-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CPU_O3_LSQ_UNIT_IMPL_HH__
#define __CPU_O3_LSQ_UNIT_IMPL_HH__

#include "arch/generic/debugfaults.hh"
#include "arch/locked_mem.hh"
#include "base/str.hh"
#include "config/the_isa.hh"
#include "cpu/checker/cpu.hh"
#include "cpu/o3/lsq.hh"
#include "cpu/o3/lsq_unit.hh"
#include "debug/Activity.hh"
#include "debug/IEW.hh"
#include "debug/LSQUnit.hh"
#include "debug/LSQ.hh"
#include "debug/SilentStores.hh"
#include "debug/O3PipeView.hh"
#include "mem/packet.hh"
#include "mem/request.hh"

template<class Impl>
LSQUnit<Impl>::WritebackEvent::WritebackEvent(const DynInstPtr &_inst,
        PacketPtr _pkt, LSQUnit *lsq_ptr)
    : Event(Default_Pri, AutoDelete),
      inst(_inst), pkt(_pkt), lsqPtr(lsq_ptr)
{
    assert(_inst->savedReq);
    _inst->savedReq->writebackScheduled();
}

template<class Impl>
void
LSQUnit<Impl>::WritebackEvent::process()
{
    assert(!lsqPtr->cpu->switchedOut());

    lsqPtr->writeback(inst, pkt);

    assert(inst->savedReq);
    inst->savedReq->writebackDone();
    delete pkt;
}

template<class Impl>
const char *
LSQUnit<Impl>::WritebackEvent::description() const
{
    return "Store writeback";
}

template <class Impl>
bool
LSQUnit<Impl>::recvTimingResp(PacketPtr pkt)
{
    auto senderState = dynamic_cast<LSQSenderState*>(pkt->senderState);
    LSQRequest* req = senderState->request();
    assert(req != nullptr);
    bool ret = true;

    if (!(req->isSlstLoad(pkt) == pkt->isSLST())) {
        panic("SLST not set properly");
    }
    // if (req->isSlstLoad(pkt)) {
    if (pkt->isSLST()) {
         slstLoadsInFlight--;
    }

    /* Check that the request is still alive before any further action. */
    if (senderState->alive()) {
        ret = req->recvTimingResp(pkt);
    } else {
        senderState->outstanding--;
    }
    return ret;

}

template<class Impl>
void
LSQUnit<Impl>::completeDataAccess(PacketPtr pkt, uint32_t pkt_type)
{
    /* pkt_type:
     *  0: silent store load packet
     *  1: store packet from potentially silent store
     *  2: normal store or load
     *  3: NOT USED
     *  4: split data request, should have no effect.
     */
    LSQSenderState *state = dynamic_cast<LSQSenderState *>(pkt->senderState);
    DynInstPtr inst = state->inst;

    DPRINTF(SilentStores, "[sn:%lli]: Completing data access\n", inst->seqNum);

    cpu->ppDataAccessComplete->notify(std::make_pair(inst, pkt));

    /* Notify the sender state that the access is complete (for ownership
     * tracking). */
    state->complete();

    assert(!cpu->switchedOut());
    if (!inst->isSquashed()) {
        DPRINTF(SilentStores, "[sn:%lli]: not squashed\n", inst->seqNum);
        if (state->needWB) {
            DPRINTF(SilentStores, "[sn:%lli]: needs wb\n", inst->seqNum);
            // Only loads, store conditionals and atomics perform the writeback
            // after receving the response from the memory
            assert(inst->isLoad() || inst->isStoreConditional() ||
                   inst->isAtomic());

            writeback(inst, state->request()->mainPacket());
            if (inst->isStore() || inst->isAtomic()) {
                auto ss = dynamic_cast<SQSenderState*>(state);
                ss->writebackDone();
                completeStore(ss->idx);
            }

        } else if (inst->isStore()) {
            auto store_idx = dynamic_cast<SQSenderState*>(state)->idx;

            DPRINTF(SilentStores, "[sn:%lli]: Completing %s\n", inst->seqNum,
                    pkt_type == 0 ? "Silent-Store Load" : "Store");

            if (store_idx->dropLoad() && pkt_type == 0) {
                DPRINTF(SilentStores, "[sn:%lli] Dropping load packet"
                        "(%i in flight)\n", inst->seqNum, slstLoadsInFlight);

                store_idx->isWaitingForLoad() = false;
                return;
            }

            if (store_idx->isWaitingForLoad()) {
                assert(!store_idx->dropLoad() && pkt_type == 0);

                DPRINTF(SilentStores, "[sn:%lli] Received load packet "
                        "(%i in flight)\n", inst->seqNum, slstLoadsInFlight);
                store_idx->isWaitingForLoad() = false;

                store_idx->request()->clearSent();

                store_idx->hasValue() = true;
                return;
            }

            // assert(pkt_type == 1 || pkt_type == 2 || pkt_type == 4);
            assert(pkt_type != 0);

            // This is a regular store (i.e., not store conditionals and
            // atomics), so it can complete without writing back
            completeStore(store_idx);
        } else {
            DPRINTF(SilentStores, "[sn:%lli]: no wb, not st\n", inst->seqNum);
        }
    } else if (inst->isStore()) {  // squashed; handle packet count
        auto store_idx = dynamic_cast<SQSenderState*>(state)->idx;

        // if Received a packet for a squashed store, it _had_ to be a
        // silent-store load; write packets cannot be sent/received unless the
        // instruction is certain to retire
        assert(store_idx->isWaitingForLoad() && pkt_type == 0);

        DPRINTF(SilentStores, "[sn:%lli] Received load packet for squashed"
                "(%i in flight)\n", inst->seqNum, slstLoadsInFlight);
    }
}

template <class Impl>
LSQUnit<Impl>::LSQUnit(uint32_t lqEntries, uint32_t sqEntries)
    : lsqID(-1), storeQueue(sqEntries+1), loadQueue(lqEntries+1),
      loads(0), stores(0), storesToWB(0), slstLoadsInFlight(0),
      performSlstLoads(true), cacheBlockMask(0), stalled(false),
      isStoreBlocked(false), storeInFlight(false), hasPendingRequest(false),
      pendingRequest(nullptr) {
}

template<class Impl>
void
LSQUnit<Impl>::init(O3CPU *cpu_ptr, IEW *iew_ptr, DerivO3CPUParams *params,
        LSQ *lsq_ptr, unsigned id)
{
    lsqID = id;

    cpu = cpu_ptr;
    iewStage = iew_ptr;

    lsq = lsq_ptr;

    DPRINTF(LSQUnit, "Creating LSQUnit%i object.\n",lsqID);

    depCheckShift = params->LSQDepCheckShift;
    checkLoads = params->LSQCheckLoads;
    needsTSO = params->needsTSO;

    resetState();
}


template<class Impl>
void
LSQUnit<Impl>::resetState()
{
    loads = stores = storesToWB = slstLoadsInFlight = 0;
    performSlstLoads = true;

    storeWBIt = storeQueue.begin();

    retryPkt = NULL;
    memDepViolator = NULL;

    stalled = false;

    cacheBlockMask = ~(cpu->cacheLineSize() - 1);
}

template<class Impl>
std::string
LSQUnit<Impl>::name() const
{
    if (Impl::MaxThreads == 1) {
        return iewStage->name() + ".lsq";
    } else {
        return iewStage->name() + ".lsq.thread" + std::to_string(lsqID);
    }
}

template<class Impl>
void
LSQUnit<Impl>::regStats()
{
    // TODO add silent store stats
    lsqForwLoads
        .name(name() + ".forwLoads")
        .desc("Number of loads that had data forwarded from stores");

    invAddrLoads
        .name(name() + ".invAddrLoads")
        .desc("Number of loads ignored due to an invalid address");

    lsqSquashedLoads
        .name(name() + ".squashedLoads")
        .desc("Number of loads squashed");

    lsqIgnoredResponses
        .name(name() + ".ignoredResponses")
        .desc("Number of memory responses ignored because the instruction is squashed");

    lsqMemOrderViolation
        .name(name() + ".memOrderViolation")
        .desc("Number of memory ordering violations");

    lsqSquashedStores
        .name(name() + ".squashedStores")
        .desc("Number of stores squashed");

    invAddrSwpfs
        .name(name() + ".invAddrSwpfs")
        .desc("Number of software prefetches ignored due to an invalid address");

    lsqBlockedLoads
        .name(name() + ".blockedLoads")
        .desc("Number of blocked loads due to partial load-store forwarding");

    lsqRescheduledLoads
        .name(name() + ".rescheduledLoads")
        .desc("Number of loads that were rescheduled");

    lsqCacheBlocked
        .name(name() + ".cacheBlocked")
        .desc("Number of times an access to memory failed due to the cache being blocked");
}

template<class Impl>
void
LSQUnit<Impl>::setDcachePort(MasterPort *dcache_port)
{
    dcachePort = dcache_port;
}

template<class Impl>
void
LSQUnit<Impl>::drainSanityCheck() const
{
    for (int i = 0; i < loadQueue.capacity(); ++i)
        assert(!loadQueue[i].valid());

    assert(storesToWB == 0);
    assert(slstLoadsInFlight == 0);
    assert(!retryPkt);
}

template<class Impl>
void
LSQUnit<Impl>::takeOverFrom()
{
    resetState();
}

template <class Impl>
void
LSQUnit<Impl>::insert(const DynInstPtr &inst)
{
    assert(inst->isMemRef());

    assert(inst->isLoad() || inst->isStore() || inst->isAtomic());

    if (inst->isLoad()) {
        insertLoad(inst);
    } else {
        insertStore(inst);
    }

    inst->setInLSQ();
}

template <class Impl>
void
LSQUnit<Impl>::insertLoad(const DynInstPtr &load_inst)
{
    assert(!loadQueue.full());
    assert(loads < loadQueue.capacity());

    DPRINTF(LSQUnit, "Inserting load PC %s, idx:%i [sn:%lli]\n",
            load_inst->pcState(), loadQueue.tail(), load_inst->seqNum);

    /* Grow the queue. */
    loadQueue.advance_tail();

    load_inst->sqIt = storeQueue.end();

    assert(!loadQueue.back().valid());
    loadQueue.back().set(load_inst);
    load_inst->lqIdx = loadQueue.tail();
    load_inst->lqIt = loadQueue.getIterator(load_inst->lqIdx);

    ++loads;
}

template <class Impl>
void
LSQUnit<Impl>::insertStore(const DynInstPtr& store_inst)
{
    // Make sure it is not full before inserting an instruction.
    assert(!storeQueue.full());
    assert(stores < storeQueue.capacity());

    DPRINTF(LSQUnit, "Inserting store PC %s, idx:%i [sn:%lli]\n",
            store_inst->pcState(), storeQueue.tail(), store_inst->seqNum);
    storeQueue.advance_tail();

    store_inst->sqIdx = storeQueue.tail();
    store_inst->lqIdx = loadQueue.moduloAdd(loadQueue.tail(), 1);
    store_inst->lqIt = loadQueue.end();

    storeQueue.back().set(store_inst);

    ++stores;
}

template <class Impl>
typename Impl::DynInstPtr
LSQUnit<Impl>::getMemDepViolator()
{
    DynInstPtr temp = memDepViolator;

    memDepViolator = NULL;

    return temp;
}

template <class Impl>
unsigned
LSQUnit<Impl>::numFreeLoadEntries()
{
        //LQ has an extra dummy entry to differentiate
        //empty/full conditions. Subtract 1 from the free entries.
        DPRINTF(LSQUnit, "LQ size: %d, #loads occupied: %d\n",
                1 + loadQueue.capacity(), loads);
        return loadQueue.capacity() - loads;
}

template <class Impl>
unsigned
LSQUnit<Impl>::numFreeStoreEntries()
{
        //SQ has an extra dummy entry to differentiate
        //empty/full conditions. Subtract 1 from the free entries.
        DPRINTF(LSQUnit, "SQ size: %d, #stores occupied: %d\n",
                1 + storeQueue.capacity(), stores);
        return storeQueue.capacity() - stores;

 }

template <class Impl>
void
LSQUnit<Impl>::checkSnoop(PacketPtr pkt)
{
    // Should only ever get invalidations in here
    assert(pkt->isInvalidate());

    DPRINTF(LSQUnit, "Got snoop for address %#x\n", pkt->getAddr());

    for (int x = 0; x < cpu->numContexts(); x++) {
        ThreadContext *tc = cpu->getContext(x);
        bool no_squash = cpu->thread[x]->noSquashFromTC;
        cpu->thread[x]->noSquashFromTC = true;
        TheISA::handleLockedSnoop(tc, pkt, cacheBlockMask);
        cpu->thread[x]->noSquashFromTC = no_squash;
    }

    if (loadQueue.empty() && slstLoadsInFlight == 0)
        return;

    if (!loadQueue.empty()) {
        auto iter = loadQueue.begin();

        Addr invalidate_addr = pkt->getAddr() & cacheBlockMask;

        DynInstPtr ld_inst = iter->instruction();
        assert(ld_inst);
        LSQRequest *req = iter->request();

        // Check that this snoop didn't just invalidate our lock flag
        if (ld_inst->effAddrValid() &&
                req->isCacheBlockHit(invalidate_addr, cacheBlockMask)
                && ld_inst->memReqFlags & Request::LLSC)
            TheISA::handleLockedSnoopHit(ld_inst.get());

        bool force_squash = false;

        while (++iter != loadQueue.end()) {
            ld_inst = iter->instruction();
            assert(ld_inst);
            req = iter->request();
            if (!ld_inst->effAddrValid() || ld_inst->strictlyOrdered())
                continue;

            DPRINTF(LSQUnit, "-- inst [sn:%lli] to pktAddr:%#x\n",
                    ld_inst->seqNum, invalidate_addr);

            if (force_squash ||
                    req->isCacheBlockHit(invalidate_addr, cacheBlockMask)) {
                if (needsTSO) {
                    // If we have a TSO system, as all loads must be ordered with
                    // all other loads, this load as well as *all* subsequent loads
                    // need to be squashed to prevent possible load reordering.
                    force_squash = true;
                }
                if (ld_inst->possibleLoadViolation() || force_squash) {
                    DPRINTF(LSQUnit, "Conflicting load at addr %#x [sn:%lli]\n",
                            pkt->getAddr(), ld_inst->seqNum);

                    // Mark the load for re-execution
                    ld_inst->fault = std::make_shared<ReExec>();
                    req->setStateToFault();
                } else {
                    DPRINTF(LSQUnit, "HitExternal Snoop for addr %#x [sn:%lli]\n",
                            pkt->getAddr(), ld_inst->seqNum);

                    // Make sure that we don't lose a snoop hitting a LOCKED
                    // address since the LOCK* flags don't get updated until
                    // commit.
                    if (ld_inst->memReqFlags & Request::LLSC)
                        TheISA::handleLockedSnoopHit(ld_inst.get());

                    // If a older load checks this and it's true
                    // then we might have missed the snoop
                    // in which case we need to invalidate to be sure
                    ld_inst->hitExternalSnoop(true);
                }
            }
        }
    }

    if (slstLoadsInFlight == 0) {
        return;
    }

    // // check for invalidations coming in to silent stores
    // auto st_iter = storeQueue.begin();
    // DPRINTF(SilentStores, "Invalidation: Scanning SQ\n");
    //
    // while (st_iter != storeQueue.end()) {
    //     if (!(st_iter->hasValue() || st_iter->isWaitingForLoad())) {
    //         st_iter++;
    //         continue;
    //     }
    //     // invalidate packet info
    //     Addr req_s = pkt->getAddr() & cacheBlockMask;
    //     auto req_e = req_s + pkt->getSize();
    //     DPRINTF(SilentStores, "Invalidation: Got candidate store info\n");
    //     // candidate store info
    //     int store_size = st_iter->size();
    //     auto st_s = st_iter->instruction()->physEffAddr;
    //     auto st_e = st_s + store_size;
    //
    //     if (st_s == 0 || st_e == 0
    //             || store_size == 0) {
    //         // unresolved store address -- can't have issued slst load
    //         st_iter++;
    //         continue;
    //     }
    //
    //     bool store_has_lower_limit = req_s >= st_s;
    //     bool store_has_upper_limit = req_e <= st_e;
    //     bool lower_inv_has_store_part = req_s < st_e;
    //     bool upper_inv_has_store_part = req_e > st_s;
    //
    //     auto coverage = AddrRangeCoverage::NoAddrRangeCoverage;
    //
    //     if (store_has_lower_limit && store_has_upper_limit) {
    //         const auto& store_req = st_iter->request()->mainRequest();
    //         coverage = store_req->isMasked() ?
    //             AddrRangeCoverage::PartialAddrRangeCoverage :
    //             AddrRangeCoverage::FullAddrRangeCoverage;
    //         DPRINTF(SilentStores, "%s [sn:%lli]: invalidation has partial "
    //                 "or full addr range coverage\n",
    //                 st_iter->instruction()->seqNum);
    //
    //     } else if (
    //             ((store_has_lower_limit && lower_inv_has_store_part) ||
    //              (store_has_upper_limit && upper_inv_has_store_part) ||
    //              (lower_inv_has_store_part && upper_inv_has_store_part)) ||
    //             ((store_has_lower_limit || upper_inv_has_store_part) &&
    //              (store_has_upper_limit || lower_inv_has_store_part))) {
    //         coverage = AddrRangeCoverage::PartialAddrRangeCoverage;
    //         DPRINTF(SilentStores, "%s [sn:%lli]: invalidation has partial "
    //                 "address coverage\n", st_iter->instruction()->seqNum);
    //     }
    //
    //     // invalidation overlap found: ensure the loaded value is not used
    //     if (coverage == AddrRangeCoverage::FullAddrRangeCoverage ||
    //             coverage == AddrRangeCoverage::PartialAddrRangeCoverage) {
    //         st_iter->dropLoad() = true;
    //         st_iter->hasValue() = false;
    //     }
    //
    //     st_iter++;
    // }

    return;
}

template <class Impl>
Fault
LSQUnit<Impl>::checkViolations(typename LoadQueue::iterator& loadIt,
        const DynInstPtr& inst)
{
    Addr inst_eff_addr1 = inst->effAddr >> depCheckShift;
    Addr inst_eff_addr2 = (inst->effAddr + inst->effSize - 1) >> depCheckShift;

    /** @todo in theory you only need to check an instruction that has executed
     * however, there isn't a good way in the pipeline at the moment to check
     * all instructions that will execute before the store writes back. Thus,
     * like the implementation that came before it, we're overly conservative.
     */
    while (loadIt != loadQueue.end()) {
        DynInstPtr ld_inst = loadIt->instruction();
        if (!ld_inst->effAddrValid() || ld_inst->strictlyOrdered()) {
            ++loadIt;
            continue;
        }

        Addr ld_eff_addr1 = ld_inst->effAddr >> depCheckShift;
        Addr ld_eff_addr2 =
            (ld_inst->effAddr + ld_inst->effSize - 1) >> depCheckShift;

        if (inst_eff_addr2 >= ld_eff_addr1 && inst_eff_addr1 <= ld_eff_addr2) {
            if (inst->isLoad()) {
                // If this load is to the same block as an external snoop
                // invalidate that we've observed then the load needs to be
                // squashed as it could have newer data
                if (ld_inst->hitExternalSnoop()) {
                    if (!memDepViolator ||
                            ld_inst->seqNum < memDepViolator->seqNum) {
                        DPRINTF(LSQUnit, "Detected fault with inst [sn:%lli] "
                                "and [sn:%lli] at address %#x\n",
                                inst->seqNum, ld_inst->seqNum, ld_eff_addr1);
                        memDepViolator = ld_inst;

                        ++lsqMemOrderViolation;

                        return std::make_shared<GenericISA::M5PanicFault>(
                            "Detected fault with inst [sn:%lli] and "
                            "[sn:%lli] at address %#x\n",
                            inst->seqNum, ld_inst->seqNum, ld_eff_addr1);
                    }
                }

                // Otherwise, mark the load has a possible load violation
                // and if we see a snoop before it's commited, we need to squash
                ld_inst->possibleLoadViolation(true);
                DPRINTF(LSQUnit, "Found possible load violation at addr: %#x"
                        " between instructions [sn:%lli] and [sn:%lli]\n",
                        inst_eff_addr1, inst->seqNum, ld_inst->seqNum);
            } else {
                // A load/store incorrectly passed this store.
                // Check if we already have a violator, or if it's newer
                // squash and refetch.
                if (memDepViolator && ld_inst->seqNum > memDepViolator->seqNum)
                    break;

                DPRINTF(LSQUnit, "Detected fault with inst [sn:%lli] and "
                        "[sn:%lli] at address %#x\n",
                        inst->seqNum, ld_inst->seqNum, ld_eff_addr1);
                memDepViolator = ld_inst;

                ++lsqMemOrderViolation;

                return std::make_shared<GenericISA::M5PanicFault>(
                    "Detected fault with "
                    "inst [sn:%lli] and [sn:%lli] at address %#x\n",
                    inst->seqNum, ld_inst->seqNum, ld_eff_addr1);
            }
        }

        ++loadIt;
    }
    return NoFault;
}




template <class Impl>
Fault
LSQUnit<Impl>::executeLoad(const DynInstPtr &inst)
{
    using namespace TheISA;
    // Execute a specific load.
    Fault load_fault = NoFault;

    DPRINTF(LSQUnit, "Executing load PC %s, [sn:%lli]\n",
            inst->pcState(), inst->seqNum);

    assert(!inst->isSquashed());

    load_fault = inst->initiateAcc();

    if (load_fault == NoFault && !inst->readMemAccPredicate()) {
        assert(inst->readPredicate());
        inst->setExecuted();
        inst->completeAcc(nullptr);
        iewStage->instToCommit(inst);
        iewStage->activityThisCycle();
        DPRINTF(SilentStores, "[sn:%lli]: executed and completed\n",
                inst->seqNum);
        DPRINTF(SilentStores, "[sn:%lli]: returning no fault\n", inst->seqNum);
        return NoFault;
    }

    if (inst->isTranslationDelayed() && load_fault == NoFault) {
        DPRINTF(SilentStores, "[sn:%lli]: translation delayed\n",
                inst->seqNum);
        return load_fault;
    }

    if (load_fault != NoFault && inst->translationCompleted() &&
        inst->savedReq->isPartialFault() && !inst->savedReq->isComplete()) {
        DPRINTF(SilentStores, "[sn:%lli]: partial fault\n", inst->seqNum);
        assert(inst->savedReq->isSplit());
        // If we have a partial fault where the mem access is not complete yet
        // then the cache must have been blocked. This load will be re-executed
        // when the cache gets unblocked. We will handle the fault when the
        // mem access is complete.
        return NoFault;
    }

    // If the instruction faulted or predicated false, then we need to send it
    // along to commit without the instruction completing.
    if (load_fault != NoFault || !inst->readPredicate()) {
        // Send this instruction to commit, also make sure iew stage
        // realizes there is activity.  Mark it as executed unless it
        // is a strictly ordered load that needs to hit the head of
        // commit.
        if (!inst->readPredicate())
            inst->forwardOldRegs();
        DPRINTF(LSQUnit, "Load [sn:%lli] not executed from %s\n",
                inst->seqNum,
                (load_fault != NoFault ? "fault" : "predication"));
        if (!(inst->hasRequest() && inst->strictlyOrdered()) ||
            inst->isAtCommit()) {
            inst->setExecuted();
        }
        iewStage->instToCommit(inst);
        iewStage->activityThisCycle();
    } else {
        if (inst->effAddrValid()) {
            auto it = inst->lqIt;
            ++it;

            if (checkLoads)
                return checkViolations(it, inst);
        }
    }

    return load_fault;
}

template <class Impl>
Fault
LSQUnit<Impl>::executeStore(const DynInstPtr &store_inst)
{
    using namespace TheISA;
    // Make sure that a store exists.
    assert(stores != 0);

    int store_idx = store_inst->sqIdx;

    DPRINTF(LSQUnit, "Executing store PC %s [sn:%lli]\n",
            store_inst->pcState(), store_inst->seqNum);

    assert(!store_inst->isSquashed());

    // Check the recently completed loads to see if any match this store's
    // address.  If so, then we have a memory ordering violation.
    typename LoadQueue::iterator loadIt = store_inst->lqIt;

    Fault store_fault = store_inst->initiateAcc();

    if (store_inst->isTranslationDelayed() &&
        store_fault == NoFault)
        return store_fault;

    if (!store_inst->readPredicate()) {
        DPRINTF(LSQUnit, "Store [sn:%lli] not executed from predication\n",
                store_inst->seqNum);
        store_inst->forwardOldRegs();
        return store_fault;
    }

    if (storeQueue[store_idx].size() == 0) {
        DPRINTF(LSQUnit,"Fault on Store PC %s, [sn:%lli], Size = 0\n",
                store_inst->pcState(), store_inst->seqNum);

        return store_fault;
    }

    assert(store_fault == NoFault);

    if (store_inst->isStoreConditional() || store_inst->isAtomic()) {
        // Store conditionals and Atomics need to set themselves as able to
        // writeback if we haven't had a fault by here.
        storeQueue[store_idx].canWB() = true;

        ++storesToWB;
    }

    return checkViolations(loadIt, store_inst);

}

template <class Impl>
void
LSQUnit<Impl>::commitLoad()
{
    assert(loadQueue.front().valid());

    DPRINTF(LSQUnit, "Committing head load instruction, PC %s\n",
            loadQueue.front().instruction()->pcState());

    loadQueue.front().clear();
    loadQueue.pop_front();

    --loads;
}

template <class Impl>
void
LSQUnit<Impl>::commitLoads(InstSeqNum &youngest_inst)
{
    assert(loads == 0 || loadQueue.front().valid());

    while (loads != 0 && loadQueue.front().instruction()->seqNum
            <= youngest_inst) {
        commitLoad();
    }
}

template <class Impl>
void
LSQUnit<Impl>::commitStores(InstSeqNum &youngest_inst)
{
    assert(stores == 0 || storeQueue.front().valid());

    /* Forward iterate the store queue (age order). */
    for (auto& x : storeQueue) {
        assert(x.valid());
        // Mark any stores that are now committed and have not yet
        // been marked as able to write back.
        if (!x.canWB()) {
            if (x.instruction()->seqNum > youngest_inst) {
                // DPRINTF sanify check: see this every time we see the not
                // WB in writeback function
                break;
            }
            DPRINTF(LSQUnit, "Marking store as able to write back, PC "
                    "%s [sn:%lli]\n",
                    x.instruction()->pcState(),
                    x.instruction()->seqNum);

            x.canWB() = true;

            ++storesToWB;

        }
    }
}

template <class Impl>
void
LSQUnit<Impl>::writebackBlockedStore()
{
    assert(isStoreBlocked);
    storeWBIt->request()->sendPacketToCache();
    if (storeWBIt->request()->isSent()){
        storePostSend();
    }
}

template <class Impl>
void
LSQUnit<Impl>::silentStores()
{
    auto silentLDIt = storeWBIt;

    while(performSlstLoads
            && silentLDIt.dereferenceable()
            && silentLDIt->valid()
            && lsq->cachePortAvailable(true)) {
        // Use the store's request as a template for the load request
        LSQRequest* req = silentLDIt->request();

        // log in traces
        DynInstPtr inst = silentLDIt->instruction();

        assert(!inst->isLoad());

        if (!req) {
            silentLDIt++;
            continue;
        }

        if (req->isSplit()) {
            silentLDIt++;
            continue;
        }

        if (inst->isAtomic() || inst->isStoreConditional()) {
            silentLDIt++;
            continue;
        }

        if (req->request()->isLocalAccess()) {
            silentLDIt++;
            continue;
        }

        if (inst->isSquashed()) {
            silentLDIt++;
            continue;
        }

        if (silentLDIt->committed()) {
            silentLDIt++;
            continue;
        }

        if (silentLDIt->isWaitingForLoad()) {
            /* already sent load! nothing to do but wait */
            silentLDIt++;
            continue;
        }

        if (silentLDIt->hasValue()) {
            silentLDIt++;
            continue;
        }

        if (silentLDIt->dropLoad()) {
            silentLDIt++;
            continue;
        }

        DPRINTF(SilentStores, "[sn:%lli] is %s\n",
                inst->seqNum,
                inst->staticInst->getName());

        if (inst->staticInst->getName() == "clflushopt") {
            DPRINTF(SilentStores, "Skipping clflushopt [sn:%lli]\n",
                    inst->seqNum);
            silentLDIt++;
            continue;
        }

        // if (inst->seqNum == 34837607) {
        //     silentLDIt++;
        //     continue;
        // }

        // ensure we have a physical address load
        if (!(inst->translationStarted() && req->isTranslationComplete() &&
                req->request()->hasPaddr())) {
            silentLDIt++;
            continue;
        }

        // if (req->request()->getPaddr() > 0x2000000000000000) {
        //     // assumed to be a PIO address -- DO NOT SLST
        //     silentLDIt++;
        //     continue;
        // }
        // if (!isPhysMemAddress(req->request()-getPaddr())) {
        //     // assumed to be a PIO address -- DO NOT SLST
        //     DPRINTF(SilentStores, "[sn:%lli]: skipping PIO address %#x \n",
        //             inst->seqNum, req->request()->getPaddr());
        //     silentLDIt++;
        //     continue;
        // }

        assert(inst->fault == NoFault);
        if (inst->fault != NoFault) {
            silentLDIt++;
            continue;
        }

        // check for younger stores to a matching address
        // don't issue loads for stores with an older store to the same address
        bool found_older = false;

        auto older_it = silentLDIt;

        // go backwards through SQ
        while (older_it != storeWBIt) {
            older_it--;  // iterate over older stores
            if (!older_it->valid()) {
                // DPRINTF(SilentStores, "%s [sn:%lli]: [sn:%lli] not valid\n",
                //         inst->pcState(), inst->seqNum,
                //         older_it->instruction()->seqNum);
                continue;
            }

            // there shouldn't be any stores _younger_ than this one _earlier_
            // in the SQ...
            assert(older_it != silentLDIt);
            DPRINTF(SilentStores, "Comparing [sn:%lli] and [sn:%lli]\n",
                    older_it->instruction()->seqNum, inst->seqNum);
            assert(older_it->instruction()->seqNum < inst->seqNum);

            int store_size = older_it->size();

            // Check if the store data is within the lower and upper bounds of
            // addresses that the request needs.
            auto req_s = req->mainRequest()->getVaddr();
            auto req_e = req_s + req->mainRequest()->getSize();
            auto st_s = older_it->instruction()->effAddr;
            auto st_e = st_s + store_size;

            bool store_has_lower_limit = req_s >= st_s;
            bool store_has_upper_limit = req_e <= st_e;
            bool lower_load_has_store_part = req_s < st_e;
            bool upper_load_has_store_part = req_e > st_s;

            if (st_s == 0 || st_e == 0) {
                // DPRINTF(SilentStores, "[sn:%lli] Stalling on unresolved [sn:%lli]\n",
                //         inst->seqNum, older_it->instruction()->seqNum);
                // found_older = true;
                // break;

                DPRINTF(SilentStores, "Marking [sn:%lli] as spec because "
                        "[sn:%lli] has an unresolved address\n", inst->seqNum,
                        older_it->instruction()->seqNum);
                silentLDIt->speculative() = true;
                older_it->root_of_spec() = true;
                continue;
            }

            auto coverage = AddrRangeCoverage::NoAddrRangeCoverage;

            if (store_size != 0 &&
                    // !older_it->instruction()->strictlyOrdered() &&
                    !(older_it->request()->mainRequest() &&
                        older_it->request()->mainRequest()->isCacheMaintenance()))
            {
                // todo assert or check+break?
                assert(older_it->instruction()->effAddrValid());

                // If the store entry is not atomic (atomic does not have valid
                // data), the store has all of the data needed, and
                // the load is not LLSC, then
                // we can forward data from the store to the load
                // todo atomic??
                if (store_has_lower_limit && store_has_upper_limit
                        // && !older_it->instruction()->isAtomic()
                   ) {
                    const auto& store_req = older_it->request()->mainRequest();
                    coverage = store_req->isMasked() ?
                        AddrRangeCoverage::PartialAddrRangeCoverage :
                        AddrRangeCoverage::FullAddrRangeCoverage;
                    DPRINTF(SilentStores, "%s [sn:%lli]: [sn:%lli] has partial "
                            "or full addr range coverage\n", inst->pcState(),
                            inst->seqNum, older_it->instruction()->seqNum);

                } else if (
                        ((store_has_lower_limit && lower_load_has_store_part) ||
                         (store_has_upper_limit && upper_load_has_store_part) ||
                         (lower_load_has_store_part && upper_load_has_store_part)) ||
                        ((store_has_lower_limit || upper_load_has_store_part) &&
                         (store_has_upper_limit || lower_load_has_store_part))) {
                    coverage = AddrRangeCoverage::PartialAddrRangeCoverage;
                    DPRINTF(SilentStores, "%s [sn:%lli]: [sn:%lli] has partial "
                            "coverage\n", inst->pcState(), inst->seqNum,
                            older_it->instruction()->seqNum);
                }

                if (coverage == AddrRangeCoverage::FullAddrRangeCoverage ||
                        coverage == AddrRangeCoverage::PartialAddrRangeCoverage) {
                    // stall this silent-store load until any conflicting prior
                    // stores leave the SQ
                    found_older = true;
                    break;
                }
            }  // end if

        }  // end iterate over older stores in SQ

        if (found_older) {
          DPRINTF(SilentStores, "%s [sn:%lli]: Found matching older store, "
              "stalling silent-store load\n", inst->pcState(), inst->seqNum);
          silentLDIt++;
          continue;
        }

        // create sender state if none exists
        if (req->senderState() == nullptr) {
            assert(req->sl_senderState() == nullptr);

            SQSenderState *state = new SQSenderState(silentLDIt);
            state->isLoad = false;
            state->needWB = false;
            state->inst = inst;
            req->senderState(state);

            SQSenderState *sl_state = new SQSenderState(silentLDIt);
            sl_state->isLoad = true;
            sl_state->needWB = false;
            sl_state->inst = inst;
            req->sl_senderState(sl_state);
        }

        assert(!(inst->isStoreConditional()
                    || inst->isAtomic()
                    || inst->isLoad()));

        req->buildPackets(true);
        req->sendPacketToCache(true);

        silentLDIt->isWaitingForLoad() = req->isSent();
        slstLoadsInFlight += req->isSent() ? 1 : 0;

        if (req->_packets.at(0)->isPIO()) {
            DPRINTF(SilentStores, "[sn:%lli]: Dropped PIO silent-store load\n",
                    inst->seqNum);
            silentLDIt->dropLoad() = true;

        } else if (!req->isSent()) {
            DPRINTF(LSQUnit, "D-Cache became blocked when writing [sn:%lli], "
                    "will retry later\n",
                    inst->seqNum);
            // don't break: cache port available should fail in loop

        } else {
            DPRINTF(SilentStores, "[sn:%lli] issued silent-store load for "
                    "%#x p-address\n", inst->seqNum,
                    req->request()->getPaddr());
            DPRINTF(SilentStores, "%i silent store loads in flight now\n",
                    slstLoadsInFlight);


        }

        silentLDIt++;

    }  // end while (silentLDIt)
}

template <class Impl>
void
LSQUnit<Impl>::writebackStores()
{
    if (isStoreBlocked) {
        DPRINTF(LSQUnit, "Writing back  blocked store\n");
        writebackBlockedStore();
    }

    while (storesToWB > 0 &&
           storeWBIt.dereferenceable() &&
           storeWBIt->valid() &&
           storeWBIt->canWB() &&
           // BOOM style in order commit, always stall while in flight
           (/*(!needsTSO) ||*/ (!storeInFlight)) &&
           lsq->cachePortAvailable(false)) {

        if (isStoreBlocked) {
            DPRINTF(LSQUnit, "Unable to write back any more stores, cache"
                    " is blocked!\n");
            break;
        }

        // Store didn't write any data so no need to write it back to
        // memory.
        if (storeWBIt->size() == 0) {
            /* It is important that the preincrement happens at (or before)
             * the call, as the the code of completeStore checks
             * storeWBIt. */
            completeStore(storeWBIt++);
            continue;
        }

        if (storeWBIt->instruction()->isDataPrefetch()) {
            storeWBIt++;
            continue;
        }

        assert(storeWBIt->hasRequest());
        assert(!storeWBIt->committed());

        DynInstPtr inst = storeWBIt->instruction();
        LSQRequest* req = storeWBIt->request();

        bool match = false;
        if (storeWBIt->isWaitingForLoad()) {
            assert(!storeWBIt->hasValue());

            DPRINTF(SilentStores, "[sn:%lli]: WB Dropping silent-store load\n",
                    inst->seqNum);

            storeWBIt->dropLoad() = true;
            storeWBIt->request()->clearSent();

        } else if (storeWBIt->hasValue() && !storeWBIt->dropLoad()) {
            match = !memcmp(storeWBIt->data(), inst->slData, req->_size);

            DPRINTF(SilentStores, "[sn:%lli]: Comparing %#x <--> %#x \n",
                    inst->seqNum, (int)*(storeWBIt->data()),
                    (int)*(inst->slData));

        } else if (req->_packets.size() == 0) {
            assert(!inst->memData);
            inst->memData = new uint8_t[req->_size];
        }

        /* if this store was unresolved when a younger one sent a load, we need
         * to check younger stores and either ensure there was no confligt or
         * drop the load of all conflicting younger ones */
        if (storeWBIt->root_of_spec()) {
            auto younger_it = storeWBIt;
            younger_it++;

            while (younger_it.dereferenceable() && younger_it->valid()) {
                if (younger_it->speculative()) {
                    assert(younger_it->instruction()->seqNum > inst->seqNum);

                    // get the bounds
                    auto req_s = req->mainRequest()->getVaddr();
                    auto req_e = req_s + req->mainRequest()->getSize();
                    auto st_s = younger_it->instruction()->effAddr;
                    auto st_e = st_s + younger_it->size();

                    bool store_has_lower_limit = req_s >= st_s;
                    bool store_has_upper_limit = req_e <= st_e;
                    bool lower_load_has_store_part = req_s < st_e;
                    bool upper_load_has_store_part = req_e > st_s;

                    auto coverage = AddrRangeCoverage::NoAddrRangeCoverage;

                    if (store_has_lower_limit && store_has_upper_limit) {
                        const auto& store_req =
                            younger_it->request()->mainRequest();
                        coverage = store_req->isMasked() ?
                            AddrRangeCoverage::PartialAddrRangeCoverage :
                            AddrRangeCoverage::FullAddrRangeCoverage;

                    } else if (
                            ((store_has_lower_limit &&
                              lower_load_has_store_part) ||
                             (store_has_upper_limit &&
                              upper_load_has_store_part) ||
                             (lower_load_has_store_part &&
                              upper_load_has_store_part)) ||
                            ((store_has_lower_limit ||
                              upper_load_has_store_part) &&
                             (store_has_upper_limit ||
                              lower_load_has_store_part))) {
                        coverage = AddrRangeCoverage::PartialAddrRangeCoverage;
                    }

                    if (coverage == AddrRangeCoverage::FullAddrRangeCoverage ||
                            coverage ==
                            AddrRangeCoverage::PartialAddrRangeCoverage) {
                        DPRINTF(SilentStores, "[sn:%lli] is squashing younger "
                                "[sn:%lli] from partial or full addr range "
                                "coverage\n", inst->seqNum,
                                younger_it->instruction()->seqNum);

                        younger_it->dropLoad() = true;
                    }
                }  /* end spec-silent-store handling */

                younger_it++;
            }  /* end iterate over younger stores */
        }  /* end root-of-spec handling */

        DPRINTF(SilentStores, "[sn:%lli]: WB committed\n", inst->seqNum);
        storeWBIt->committed() = true;

        assert(inst->memData);

        if (!match) {  // comparison failed, sending to memory
            if (storeWBIt->isAllZeros())
                memset(inst->memData, 0, req->_size);
            else
                memcpy(inst->memData, storeWBIt->data(), req->_size);

            if (req->senderState() == nullptr) {
                assert(!req->sl_senderState());
                SQSenderState *state = new SQSenderState(storeWBIt);
                state->isLoad = false;
                state->needWB = false;
                state->inst = inst;

                req->senderState(state);
            }
            req->buildPackets();

            DPRINTF(LSQUnit, "D-Cache: WB Writing back store idx:%i PC:%s "
                    "to Addr:%#x, data:%#x [sn:%lli]\n",
                    storeWBIt.idx(), inst->pcState(),
                    req->request()->getPaddr(), (int)*(inst->memData),
                    inst->seqNum);

        }

        // @todo: Remove this SC hack once the memory system handles it.
        if (inst->isStoreConditional()) {
            assert(!match);  // not handled by silent stores
            // Disable recording the result temporarily.  Writing to
            // misc regs normally updates the result, but this is not
            // the desired behavior when handling store conditionals.
            inst->recordResult(false);
            bool success = TheISA::handleLockedWrite(inst.get(),
                    req->request(), cacheBlockMask);
            inst->recordResult(true);
            req->packetSent();

            if (!success) {
                req->complete();
                // Instantly complete this store.
                DPRINTF(LSQUnit, " WB Store conditional [sn:%lli] failed.  "
                        "Instantly completing it.\n",
                        inst->seqNum);
                PacketPtr new_pkt = new Packet(*req->packet());
                WritebackEvent *wb = new WritebackEvent(inst,
                        new_pkt, this);
                cpu->schedule(wb, curTick() + 1);
                completeStore(storeWBIt);
                if (!storeQueue.empty())
                    storeWBIt++;
                else
                    storeWBIt = storeQueue.end();
                continue;
            }
        }

        if (req->request()->isLocalAccess()) {
            assert(!match);  // not handled by silent stores
            assert(inst->memData);
            assert(!inst->isStoreConditional());

            ThreadContext *thread = cpu->tcBase(lsqID);
            PacketPtr main_pkt = new Packet(req->mainRequest(),
                                            MemCmd::WriteReq);
            main_pkt->dataStatic(inst->memData);
            req->request()->localAccessor(thread, main_pkt);
            delete main_pkt;
            completeStore(storeWBIt);
            storeWBIt++;
            continue;
        }

        if (!match) {
            /* Send to cache */
            DPRINTF(SilentStores, "Sending store to cache [sn:%llu]\n",
                    inst->seqNum);
            req->sendPacketToCache();

            if (req->isSent()) {
                storePostSend();
            } else {
                DPRINTF(LSQUnit, " WB D-Cache became blocked when writing "
                        "[sn:%lli], will retry later\n",
                        inst->seqNum);
            }

        } else {
            assert(!storeInFlight);  // will be forced to false

            DPRINTF(SilentStores, "Completing store silently [sn:%llu]\n",
                    inst->seqNum);
            completeStore(storeWBIt);

            if (cpu->checker) {
                cpu->checker->verify(inst);
            }

            if (!storeQueue.empty())
                storeWBIt++;
            else
                storeWBIt = storeQueue.end();
        }
    }
    assert(stores >= 0 && storesToWB >= 0);
}

template <class Impl>
void
LSQUnit<Impl>::squash(const InstSeqNum &squashed_num)
{
    DPRINTF(LSQUnit, "Squashing until [sn:%lli]!"
            "(Loads:%i Stores:%i)\n", squashed_num, loads, stores);

    while (loads != 0 &&
            loadQueue.back().instruction()->seqNum > squashed_num) {
        DPRINTF(LSQUnit,"Load Instruction PC %s squashed, "
                "[sn:%lli]\n",
                loadQueue.back().instruction()->pcState(),
                loadQueue.back().instruction()->seqNum);

        if (isStalled() && loadQueue.tail() == stallingLoadIdx) {
            stalled = false;
            stallingStoreIsn = 0;
            stallingLoadIdx = 0;
        }

        // Clear the smart pointer to make sure it is decremented.
        loadQueue.back().instruction()->setSquashed();
        loadQueue.back().clear();

        --loads;

        loadQueue.pop_back();
        ++lsqSquashedLoads;
    }

    if (memDepViolator && squashed_num < memDepViolator->seqNum) {
        memDepViolator = NULL;
    }

    while (stores != 0 &&
           storeQueue.back().instruction()->seqNum > squashed_num) {
        // Instructions marked as can WB are already committed.
        if (storeQueue.back().canWB()) {
            break;
        }

        DPRINTF(LSQUnit,"Store Instruction PC %s squashed, "
                "idx:%i [sn:%lli]\n",
                storeQueue.back().instruction()->pcState(),
                storeQueue.tail(), storeQueue.back().instruction()->seqNum);

        // I don't think this can happen.  It should have been cleared
        // by the stalling load.
        if (isStalled() &&
            storeQueue.back().instruction()->seqNum == stallingStoreIsn) {
            panic("Is stalled should have been cleared by stalling load!\n");
            stalled = false;
            stallingStoreIsn = 0;
        }

        // Clear the smart pointer to make sure it is decremented.
        storeQueue.back().instruction()->setSquashed();

        // Must delete request now that it wasn't handed off to
        // memory.  This is quite ugly.  @todo: Figure out the proper
        // place to really handle request deletes.
        storeQueue.back().clear();
        --stores;

        storeQueue.pop_back();
        ++lsqSquashedStores;
    }
}

template <class Impl>
void
LSQUnit<Impl>::storePostSend()
{
    if (isStalled() &&
        storeWBIt->instruction()->seqNum == stallingStoreIsn) {
        DPRINTF(LSQUnit, "Unstalling, stalling store [sn:%lli] "
                "load idx:%i\n",
                stallingStoreIsn, stallingLoadIdx);
        stalled = false;
        stallingStoreIsn = 0;
        iewStage->replayMemInst(loadQueue[stallingLoadIdx].instruction());
    }

    if (!storeWBIt->instruction()->isStoreConditional()) {
        // The store is basically completed at this time. This
        // only works so long as the checker doesn't try to
        // verify the value in memory for stores.
        storeWBIt->instruction()->setCompleted();

        if (cpu->checker) {
            cpu->checker->verify(storeWBIt->instruction());
        }
    }

    // if (needsTSO) {
        storeInFlight = true;
    // }

    storeWBIt++;
}

template <class Impl>
void
LSQUnit<Impl>::writeback(const DynInstPtr &inst, PacketPtr pkt)
{
    iewStage->wakeCPU();

    // Squashed instructions do not need to complete their access.
    if (inst->isSquashed()) {
        assert(!inst->isStore());
        ++lsqIgnoredResponses;
        return;
    }

    if (!inst->isExecuted()) {
        inst->setExecuted();

        if (inst->fault == NoFault) {
            // Complete access to copy data to proper place.
            inst->completeAcc(pkt);
        } else {
            // If the instruction has an outstanding fault, we cannot complete
            // the access as this discards the current fault.

            // If we have an outstanding fault, the fault should only be of
            // type ReExec or - in case of a SplitRequest - a partial
            // translation fault
            assert(dynamic_cast<ReExec*>(inst->fault.get()) != nullptr ||
                   inst->savedReq->isPartialFault());

            DPRINTF(LSQUnit, "Not completing instruction [sn:%lli] access "
                    "due to pending fault.\n", inst->seqNum);
        }
    }

    // Need to insert instruction into queue to commit
    iewStage->instToCommit(inst);

    iewStage->activityThisCycle();

    // see if this load changed the PC
    iewStage->checkMisprediction(inst);
}

template <class Impl>
void
LSQUnit<Impl>::completeStore(typename StoreQueue::iterator store_idx)
{
    assert(store_idx->valid());

    // A bit conservative because a store completion may not free up entries,
    // but hopefully avoids two store completions in one cycle from making
    // the CPU tick twice.
    cpu->wakeCPU();
    cpu->activityThisCycle();

    /* We 'need' a copy here because we may clear the entry from the
     * store queue. */
    DynInstPtr store_inst = store_idx->instruction();

    store_idx->completed() = true;
    --storesToWB;

    if (store_idx == storeQueue.begin()) {
        do {
            storeQueue.front().clear();
            storeQueue.pop_front();
            --stores;
        } while (storeQueue.front().completed() &&
                 !storeQueue.empty());

        iewStage->updateLSQNextCycle = true;
    }

    DPRINTF(LSQUnit, "Completing store [sn:%lli], idx:%i, store head "
            "idx:%i\n",
            store_inst->seqNum, store_idx.idx() - 1, storeQueue.head() - 1);

#if TRACING_ON
    if (DTRACE(O3PipeView)) {
        store_inst->storeTick =
            curTick() - store_inst->fetchTick;
    }
#endif

    if (isStalled() &&
        store_inst->seqNum == stallingStoreIsn) {
        DPRINTF(LSQUnit, "Unstalling, stalling store [sn:%lli] "
                "load idx:%i\n",
                stallingStoreIsn, stallingLoadIdx);
        stalled = false;
        stallingStoreIsn = 0;
        iewStage->replayMemInst(loadQueue[stallingLoadIdx].instruction());
    }

    store_inst->setCompleted();

    // if (needsTSO) {
        storeInFlight = false;
    // }

    // Tell the checker we've completed this instruction.  Some stores
    // may get reported twice to the checker, but the checker can
    // handle that case.
    // Store conditionals cannot be sent to the checker yet, they have
    // to update the misc registers first which should take place
    // when they commit
    if (cpu->checker && !store_inst->isStoreConditional()) {
        cpu->checker->verify(store_inst);
    }
}

template <class Impl>
bool
LSQUnit<Impl>::trySendPacket(bool isLoad, PacketPtr data_pkt)
{
    bool ret = true;
    bool cache_got_blocked = false;

    auto state = dynamic_cast<LSQSenderState*>(data_pkt->senderState);

    DPRINTF(SilentStores, "[sn:%lli]: Trying to send packet\n",
            state->inst->seqNum);
    DPRINTF(SilentStores, "[sn:%lli]: Cache %s\n", state->inst->seqNum,
            lsq->cacheBlocked() ? "blocked" : "not blocked");
    DPRINTF(SilentStores, "[sn:%lli]: cache port %s\n", state->inst->seqNum,
            lsq->cachePortAvailable(isLoad) ? "available" : "not available");
    DPRINTF(SilentStores, "[sn:%lli]: %s\n", state->inst->seqNum,
            isLoad ? "is load" : "is not load");

    if (!lsq->cacheBlocked() &&
        lsq->cachePortAvailable(isLoad)) {
        if (!dcachePort->sendTimingReq(data_pkt)) {
            ret = false;

            if (data_pkt->isPIO() && data_pkt->isSLST()) {
                DPRINTF(SilentStores, "[sn:%lli]: Is PIO, drop\n",
                        state->inst->seqNum);
            } else {
                DPRINTF(SilentStores, "[sn:%lli]: cache_got_blocked\n",
                        state->inst->seqNum);
                cache_got_blocked = true;
            }
        }
    } else {
        ret = false;
    }

    if (ret) {
        if (!isLoad) {
            isStoreBlocked = false;
        }
        lsq->cachePortBusy(isLoad);
        state->outstanding++;
        state->request()->packetSent();
    } else {
        if (cache_got_blocked) {
            lsq->cacheBlocked(true);
            ++lsqCacheBlocked;
        }
        if (!isLoad) {
            assert(state->request() == storeWBIt->request());
            isStoreBlocked = true;
        }
        state->request()->packetNotSent();
    }
    return ret;
}

template <class Impl>
void
LSQUnit<Impl>::recvRetry()
{
    if (isStoreBlocked) {
        DPRINTF(LSQUnit, "Receiving retry: blocked store\n");
        writebackBlockedStore();
    }
}

template <class Impl>
void
LSQUnit<Impl>::dumpInsts() const
{
    cprintf("Load store queue: Dumping instructions.\n");
    cprintf("Load queue size: %i\n", loads);
    cprintf("Load queue: ");

    for (const auto& e: loadQueue) {
        const DynInstPtr &inst(e.instruction());
        cprintf("%s.[sn:%llu] ", inst->pcState(), inst->seqNum);
    }
    cprintf("\n");

    cprintf("Store queue size: %i\n", stores);
    cprintf("Store queue: ");

    for (const auto& e: storeQueue) {
        const DynInstPtr &inst(e.instruction());
        cprintf("%s.[sn:%llu] ", inst->pcState(), inst->seqNum);
    }

    cprintf("\n");
}

template <class Impl>
unsigned int
LSQUnit<Impl>::cacheLineSize()
{
    return cpu->cacheLineSize();
}

#endif//__CPU_O3_LSQ_UNIT_IMPL_HH__
