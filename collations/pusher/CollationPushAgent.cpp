/*
    Copyright (C) 2018-2019 SKALE Labs

    This file is part of skale-consensus.

    skale-consensus is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    skale-consensus is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with skale-consensus.  If not, see <http://www.gnu.org/licenses/>.

    @file CollationPushAgent.cpp
    @author Stan Kladko
    @date 2018
*/

#include "assert.h"
#include "../../chainstorage/PersistentChainStorage.h"
#include "../../pendingqueue/PendingTransactionsAgent.h"
#include "../../network/UDPNetwork.h"
#include "../../network/IO.h"
#include "../../network/ClientSocket.h"

#include "../../chains/Schain.h"
#include "../CollationRequestHeader.h"
#include "../CollationResponseHeader.h"


#include "CollationPusherThreadPool.h"
#include "CollationPushAgent.h"


using namespace std;


CollationPushAgent::CollationPushAgent(SubChain& ref_sChain) : Agent( *ref_sChain.getNode() ), subChain( &ref_sChain ) {

    this->collationPusherThreadPool = new CollationPusherThreadPool(
            num_threads( uint64_t(ref_sChain.getSchain()->getTopSubChain()->getNodeCount()) ),
            this
            );
    threadCounter = 0;
    collationPusherThreadPool->startService();
}

ConnectionStatus CollationPushAgent::sendItem(ptr<CollationMessage> /*collation*/, schain_index /*subChainIndex*/) {
    return STATUS_DUMMY_HACK;
}


std::atomic<int> &CollationPushAgent::getThreadCounter() {
    return threadCounter;
}


void CollationPushAgent::enqueueItem(ptr<CollationMessage> item) {
    for (uint64_t i = 0; i < uint64_t( this->subChain->getNodeCount() ); i++) {

        lock_guard<mutex> lock(this->queueMutex[schain_index(i)]);
        itemQueue[schain_index(i)].push(item);
        queueCond[schain_index(i)].notify_all();
    }
}


void CollationPushAgent::workerThreadItemSendLoop(CollationPushAgent *agent) {

    setThreadName(__CLASS_NAME__);

    Agent::waitOnGlobalBarrier();
    return;

    schain_index subChainIndex = schain_index(agent->getThreadCounter().fetch_add(1));

    while (true) {


        auto item = agent->workerThreadWaitandPopItem(subChainIndex);

        if (subChainIndex != agent->subChain->getSubChainIndex()) {
            agent->sendItem(item, subChainIndex);
        } else {
            // dont send blocks to itself
        }
    };

};


ptr<CollationMessage> CollationPushAgent::workerThreadWaitandPopItem(schain_index subChainIndex) {

    std::unique_lock<std::mutex> mlock(this->queueMutex[subChainIndex]);

    while (itemQueue[subChainIndex].empty()) {
        queueCond[subChainIndex].wait(mlock);
    }


    auto proposal = itemQueue[subChainIndex].front();

    itemQueue[subChainIndex].pop();

    return proposal;

}


