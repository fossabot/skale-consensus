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

    @file Schain.cpp
    @author Stan Kladko
    @date 2018
*/

#include "../SkaleConfig.h"
#include "../Log.h"
#include "../exceptions/FatalError.h"

#include "../thirdparty/json.hpp"

#include "../abstracttcpserver/ConnectionStatus.h"
#include "../node/ConsensusEngine.h"

#include <unordered_set>

#include "leveldb/db.h"

#include "../node/Node.h"

#include "../headers/BlockProposalHeader.h"
#include "../pendingqueue/PendingTransactionsAgent.h"
#include "../blockproposal/pusher/BlockProposalPusherAgent.h"
#include "../catchup/client/CatchupClientAgent.h"
#include "../catchup/server/CatchupServerAgent.h"
#include "../node/NodeInfo.h"
#include "../messages/Message.h"
#include "../messages/MessageEnvelope.h"
#include "../messages/NetworkMessageEnvelope.h"
#include "../messages/InternalMessageEnvelope.h"

#include "../exceptions/EngineInitException.h"
#include "../exceptions/ParsingException.h"


#include "../blockproposal/received/ReceivedBlockProposalsDatabase.h"
#include "../protocols/ProtocolInstance.h"
#include "../protocols/blockconsensus/BlockConsensusAgent.h"
#include "../network/Sockets.h"

#include "../network/ClientSocket.h"
#include "../network/ZMQServerSocket.h"
#include "SchainMessageThreadPool.h"
#include "../pendingqueue/ExternalQueueSyncAgent.h"
#include "../network/IO.h"

#include "../exceptions/ExitRequestedException.h"
#include "../messages/ConsensusProposalMessage.h"
#include "../datastructures/CommittedBlock.h"
#include "../datastructures/CommittedBlockList.h"
#include "../datastructures/BlockProposal.h"
#include "../datastructures/MyBlockProposal.h"
#include "../datastructures/ReceivedBlockProposal.h"
#include "../datastructures/BlockProposalSet.h"
#include "../datastructures/Transaction.h"
#include "../datastructures/PendingTransaction.h"
#include "../datastructures/ImportedTransaction.h"
#include "../datastructures/TransactionList.h"

#include "../exceptions/FatalError.h"
#include "../node/ConsensusEngine.h"
#include "SchainTest.h"
#include "../pendingqueue/TestMessageGeneratorAgent.h"
#include "Schain.h"


void Schain::postMessage(ptr<MessageEnvelope> m) {


    lock_guard<mutex> lock(messageMutex);

    ASSERT(m);
    ASSERT((uint64_t) m->getMessage()->getBlockId() != 0);


    messageQueue.push(m);
    messageCond.notify_all();
}


void Schain::messageThreadProcessingLoop(Schain *s) {

    ASSERT(s);

    setThreadName(__CLASS_NAME__);
    s->waitOnGlobalStartBarrier();




    using namespace std::chrono;

    try {

        s->startTime = getCurrentTimeMilllis();


        logThreadLocal_ = s->getNode()->getLog();

        queue<ptr<MessageEnvelope>> newQueue;

        while (!s->getNode()->isExitRequested()) {

            {
                unique_lock<mutex> mlock(s->messageMutex);
                while (s->messageQueue.empty()) {
                    s->messageCond.wait(mlock);
                    if (s->getNode()->isExitRequested()) {
                        s->getNode()->getSockets()->consensusZMQSocket->closeSend();
                        return;
                    }
                }

                newQueue = s->messageQueue;


                while (!s->messageQueue.empty()) {

                    s->messageQueue.pop();
                }
            }


            while (!newQueue.empty()) {

                ptr<MessageEnvelope> m = newQueue.front();


                ASSERT((uint64_t) m->getMessage()->getBlockId() != 0);

                try {

                    s->getBlockConsensusInstance()->routeAndProcessMessage(m);
                } catch (Exception &e) {
                    if (s->getNode()->isExitRequested()) {
                        s->getNode()->getSockets()->consensusZMQSocket->closeSend();
                        return;
                    }
                    Exception::log_exception(e);
                }

                newQueue.pop();
            }
        }


        s->getNode()->getSockets()->consensusZMQSocket->closeSend();
    } catch (FatalError *e) {
        s->getNode()->exitOnFatalError(e->getMessage());
    }

}


uint64_t Schain::getHighResolutionTime() {

    auto now = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

}


chrono::milliseconds Schain::getCurrentTimeMilllis() {
    return chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch());
}


uint64_t Schain::getCurrentTimeSec() {
    uint64_t result = chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count();

    ASSERT(result < (uint64_t) MODERN_TIME + 1000000000);

    return result;
}


void Schain::startThreads() {

    this->consensusMessageThreadPool->startService();


}


Schain::Schain(Node &_node, schain_index _schainIndex, const schain_id &_schainID, ConsensusExtFace *_extFace)
        : Agent(*this, true, true), totalTransactions(0),
          extFace(_extFace), schainID(_schainID), consensusMessageThreadPool(new SchainMessageThreadPool(this)),
          node(_node),
          schainIndex(_schainIndex) {


    committedBlockID.store(0);
    bootstrapBlockID.store(0);
    committedBlockTimeStamp.store(0);


    this->io = make_shared<IO>(this);


    ASSERT(getNode()->getNodeInfosByIndex().size() > 0);

    for (auto const &iterator : getNode()->getNodeInfosByIndex()) {


        if (*iterator.second->getBaseIP() == *getNode()->getBindIP()) {
            ASSERT(thisNodeInfo == nullptr && iterator.second != nullptr);
            thisNodeInfo = iterator.second;
        }
    }

    if (thisNodeInfo == nullptr) {
        throw EngineInitException("Schain: " + to_string((uint64_t) getSchainID()) +
                                  " does not include " + "current node with IP " +
                                  *getNode()->getBindIP() + "and node id " +
                                  to_string(getNode()->getNodeID()), __CLASS_NAME__);
    }

    ASSERT(getNodeCount() > 0);

    constructChildAgents();

    string x = SchainTest::NONE;

    blockProposerTest = make_shared<string>(x);


    getNode()->getAgents().push_back(this);


}

const ptr<IO> &Schain::getIo() const {
    return io;
}


void Schain::constructChildAgents() {

    std::lock_guard<std::recursive_mutex> aLock(getMainMutex());


    pendingTransactionsAgent = make_shared<PendingTransactionsAgent>(*this);
    blockProposalClient = make_shared<BlockProposalPusherAgent>(*this);
    catchupClientAgent = make_shared<CatchupClientAgent>(*this);

    //blockRetrieverAgent = make_shared<BlockRetrieverAgent>(*this);
    blockConsensusInstance = make_shared<BlockConsensusAgent>(*this);
    blockProposalsDatabase = make_shared<ReceivedBlockProposalsDatabase>(*this);
    testMessageGeneratorAgent = make_shared<TestMessageGeneratorAgent>(*this);


    if (extFace) {
        externalQueueSyncAgent = make_shared<ExternalQueueSyncAgent>(*this, extFace);
    }


}


void Schain::blockCommitsArrivedThroughCatchup(ptr<CommittedBlockList> _blocks) {

    ASSERT(_blocks);

    auto b = _blocks->getBlocks();

    ASSERT(b);

    if (b->size() == 0) {
        return;
    }


    std::lock_guard<std::recursive_mutex> aLock(getMainMutex());


    atomic<uint64_t> committedIDOld(committedBlockID.load());

    uint64_t previosBlockTimeStamp = 0;


    ASSERT((*b)[0]->getBlockID() <= (uint64_t) committedBlockID + 1);

    for (size_t i = 0; i < b->size(); i++) {
        if ((*b)[i]->getBlockID() > committedBlockID.load()) {
            committedBlockID++;
            processCommittedBlock((*b)[i]);
            previosBlockTimeStamp = (*b)[i]->getTimeStamp();
        }
    }

    if (committedIDOld < committedBlockID) {
        LOG(info, "Successful catchup, proposing next block)");
        proposeNextBlock(previosBlockTimeStamp);
    }
}


void Schain::blockCommitArrived(bool bootstrap, block_id _committedBlockID, schain_index _proposerIndex,
                                uint64_t _committedTimeStamp) {

    std::lock_guard<std::recursive_mutex> aLock(getMainMutex());

    ASSERT(_committedTimeStamp < (uint64_t) 2 * MODERN_TIME);

    if (_committedBlockID <= committedBlockID && !bootstrap)
        return;


    ASSERT(_committedBlockID == (committedBlockID + 1) || committedBlockID == 0);


    committedBlockID.store((uint64_t) _committedBlockID);
    committedBlockTimeStamp = _committedTimeStamp;


    uint64_t previousBlockTimeStamp = 0;

    ptr<BlockProposal> committedProposal = nullptr;


    if (!bootstrap) {

        committedProposal = blockProposalsDatabase->getBlockProposal(_committedBlockID, _proposerIndex);

        ASSERT(committedProposal);

        auto newCommittedBlock = make_shared<CommittedBlock>(*this, committedProposal);

        processCommittedBlock(newCommittedBlock);

        previousBlockTimeStamp = newCommittedBlock->getTimeStamp();

    } else {
        LOG(info, "Jump starting the system with block" + to_string(_committedBlockID));
    }


    proposeNextBlock(previousBlockTimeStamp);

}


void Schain::proposeNextBlock(uint64_t _previousBlockTimeStamp) {


    block_id _proposedBlockID((uint64_t) committedBlockID + 1);

    ASSERT(pushedBlockProposals.count(_proposedBlockID) == 0);

    auto myProposal = pendingTransactionsAgent->buildBlockProposal(_proposedBlockID, _previousBlockTimeStamp);

    ASSERT(myProposal->getProposerIndex() == getSchainIndex());

    if (blockProposalsDatabase->addBlockProposal(myProposal)) {
        startConsensus(_proposedBlockID);
    }

    LOG(debug, "PROPOSING BLOCK NUMBER:" + to_string(_proposedBlockID));

    blockProposalClient->enqueueItem(myProposal);

    pushedBlockProposals.insert(_proposedBlockID);


}

void Schain::processCommittedBlock(ptr<CommittedBlock> _block) {


    std::lock_guard<std::recursive_mutex> aLock(getMainMutex());


    ASSERT(committedBlockID == _block->getBlockID());

    totalTransactions += _block->getTransactionList()->size();

    auto h = Header::sha2hex(_block->getHash())->substr(0, 8);
    LOG(info, "PRPSR:" + to_string(_block->getProposerIndex()) +
              ":BID: " + to_string(_block->getBlockID()) + ":HASH:" +
              h +
              +":BLOCK_TXS:" + to_string(_block->getTransactionCount()) + ":DMSG" + to_string(getMessagesCount()) +
              ":MPRPS:" + to_string(MyBlockProposal::getTotalObjects()) +
              ":RPRPS:" + to_string(ReceivedBlockProposal::getTotalObjects()) +

              ":PTXNS:" + to_string(PendingTransaction::getTotalObjects()) +
              ":RTXNS:" + to_string(ImportedTransaction::getTotalObjects()) +
              ":PNDG:" + to_string(pendingTransactionsAgent->getPendingTransactionsSize()) +
              ":KNWN:" + to_string(pendingTransactionsAgent->getKnownTransactionsSize()) +
              ":CMT:" + to_string(pendingTransactionsAgent->getCommittedTransactionsSize()) +
              ":MGS:" + to_string(Message::getTotalObjects()) +
              ":INSTS:" + to_string(ProtocolInstance::getTotalObjects()) +
              ":BPS:" + to_string(BlockProposalSet::getTotalObjects()) +
              ":TLS:" + to_string(TransactionList::getTotalObjects()) +
              ":HDRS:" + to_string(Header::getTotalObjects()));


    pendingTransactionsAgent->cleanCommittedTransactionsFromQueue(_block);

    saveBlock(_block);

    blockProposalsDatabase->cleanOldBlockProposals(_block->getBlockID());


    if (extFace) {
        pushBlockToExtFace(_block);
    }


}

void Schain::saveBlock(ptr<CommittedBlock> &_block) {
    saveBlockToBlockCache(_block);
    saveBlockToLevelDB(_block);
}

void Schain::saveBlockToBlockCache(ptr<CommittedBlock> &_block) {


    auto blockID = _block->getBlockID();

    ASSERT(blocks.count(blockID) == 0);

    blocks[blockID] = _block;

    auto storageSize = getNode()->getCommittedBlockStorageSize();

    if (blockID > storageSize && blocks.count(blockID - storageSize) > 0) {
        blocks.erase(committedBlockID - storageSize);
    };


    ASSERT(blocks.size() <= storageSize);


}

void Schain::saveBlockToLevelDB(ptr<CommittedBlock> &_block) {
    auto serializedBlock = _block->serialize();

    using namespace leveldb;

    WriteOptions writeOptions;

    getNode()->getBlocksDB()->Put(writeOptions, Slice(to_string(getNode()->getNodeID()) + ":"
                                                      + to_string(_block->getBlockID())),
                                  Slice((const char *) serializedBlock->data(), serializedBlock->size()));
}

void Schain::pushBlockToExtFace(ptr<CommittedBlock> &_block) {

    auto blockID = _block->getBlockID();

    ConsensusExtFace::transactions_vector tv;

    assert((returnedBlock + 1 == blockID) || returnedBlock == 0);

    for (auto &&t: *_block->getTransactionList()->getItems()) {

        tv.push_back(*(t->getData()));
    }

    returnedBlock = (uint64_t) blockID;

    extFace->createBlock(tv, _block->getTimeStamp(), (__uint64_t) _block->getBlockID());
}


void Schain::startConsensus(const block_id _blockID) {

    {


        std::lock_guard<std::recursive_mutex> aLock(getMainMutex());

        LOG(debug, "Got proposed block set for block:" + to_string(_blockID));

        assert(blockProposalsDatabase->isTwoThird(_blockID));

        LOG(debug, "StartConsensusIfNeeded BLOCK NUMBER:" + to_string((_blockID)));

        if (_blockID <= committedBlockID) {
            LOG(debug, "Too late to start consensus: already committed " + to_string(committedBlockID));
            return;
        }


        if (_blockID > committedBlockID + 1) {
            LOG(debug, "Consensus is in the future" + to_string(committedBlockID));
            return;
        }

        if (startedConsensuses.count(_blockID) > 0) {
            LOG(debug, "already started consensus for this block id");
            return;
        }

        startedConsensuses.insert(_blockID);


    }


    auto proposals = blockProposalsDatabase->getBooleanProposalsVector(_blockID);
    ASSERT(blockConsensusInstance && proposals);


    auto message = make_shared<ConsensusProposalMessage>(*this, _blockID, proposals);

    auto envelope = make_shared<InternalMessageEnvelope>(ORIGIN_EXTERNAL, message, *this);


    LOG(info, "Starting consensus for block id:" + to_string(_blockID));

    postMessage(envelope);

}


void Schain::proposedBlockArrived(ptr<BlockProposal> pbm) {

    std::lock_guard<std::recursive_mutex> aLock(getMainMutex());

    if (blockProposalsDatabase->addBlockProposal(pbm)) {
        startConsensus(pbm->getBlockID());
    }
}


const ptr<PendingTransactionsAgent> &Schain::getPendingTransactionsAgent() const {
    return pendingTransactionsAgent;
}

chrono::milliseconds Schain::getStartTime() const {
    return startTime;
}

const block_id Schain::getCommittedBlockID() const {
    return block_id(committedBlockID.load());
}


ptr<CommittedBlock> Schain::getCachedBlock(block_id _blockID) {

    std::lock_guard<std::recursive_mutex> aLock(getMainMutex());

    if (blocks.count(_blockID > 0)) {
        return blocks[_blockID];
    } else {
        return nullptr;
    }
}

ptr<CommittedBlock> Schain::getBlock(block_id _blockID) {

    std::lock_guard<std::recursive_mutex> aLock(getMainMutex());

    auto block = getCachedBlock(_blockID);

    if (block)
        return block;

    return make_shared<CommittedBlock>(getSerializedBlockFromLevelDB(_blockID));

}

ptr<vector<uint8_t>> Schain::getSerializedBlockFromLevelDB(const block_id &_blockID) {
    using namespace leveldb;

    ReadOptions options;
    string value;

    bool result = getSchain()->getNode()->getBlocksDB()->Get(options, to_string((uint64_t) getNode()->getNodeID()) +
                                                                      ":" + to_string((uint64_t) _blockID),&value).ok();
    if (result) {

        auto serializedBlock = make_shared<vector<uint8_t>>();
        serializedBlock->insert(serializedBlock->begin(), value.data(), value.data() + value.size());
        return serializedBlock;
    } else {
        return nullptr;
    }
}


schain_index Schain::getSchainIndex() const {
    return this->schainIndex;
}


Node *Schain::getNode() const {
    return &node;
}


node_count Schain::getNodeCount() {
    auto count = node_count(getNode()->getNodeInfosByIndex().size());
    ASSERT(count > 0);
    return count;
}


transaction_count Schain::getMessagesCount() {

    std::lock_guard<std::recursive_mutex> aLock(getMainMutex());

    return transaction_count(messageQueue.size());
}


schain_id Schain::getSchainID() {
    return schainID;
}


ptr<BlockConsensusAgent> Schain::getBlockConsensusInstance() {
    return blockConsensusInstance;
}


const ptr<NodeInfo> &Schain::getThisNodeInfo() const {
    return thisNodeInfo;
}


const ptr<TestMessageGeneratorAgent> &Schain::getTestMessageGeneratorAgent() const {
    return testMessageGeneratorAgent;
}

void Schain::setBlockProposerTest(const string &blockProposerTest) {
    Schain::blockProposerTest = make_shared<string>(blockProposerTest);
}

const ptr<ExternalQueueSyncAgent> &Schain::getExternalQueueSyncAgent() const {
    return externalQueueSyncAgent;
};


void Schain::bootstrap(block_id _lastCommittedBlockID, uint64_t _lastCommittedBlockTimeStamp) {
    try {
        ASSERT(bootStrapped == false);
        bootStrapped = true;
        bootstrapBlockID.store((uint64_t) _lastCommittedBlockID);
        blockCommitArrived(true, _lastCommittedBlockID, schain_index(0), _lastCommittedBlockTimeStamp);
    } catch (Exception &e) {
        Exception::log_exception(e);
        return;
    }
}


schain_id Schain::getSchainID() const {
    return schainID;
}


uint64_t Schain::getTotalTransactions() const {
    return totalTransactions;
}

uint64_t Schain::getCommittedBlockTimeStamp() {
    return committedBlockTimeStamp;
}

const ptr<ReceivedBlockProposalsDatabase> &Schain::getBlockProposalsDatabase() const {
    return blockProposalsDatabase;
}

block_id Schain::getBootstrapBlockID() const {
    return bootstrapBlockID.load();
}


void Schain::setHealthCheckFile(uint64_t status) {


    ofstream f(*Log::getDataDir() + "/HEALTH_CHECK", ios::trunc);

    f << to_string(status);

}


void Schain::healthCheck() {

    std::unordered_set<uint64_t> connections;

    setHealthCheckFile(1);

    auto beginTime = getCurrentTimeSec();

    LOG(info, "Waiting to connect to peers");

    while (3 * (connections.size() + 1) < 2 * getNodeCount()) {
        if (getCurrentTimeSec() - beginTime > 6000) {
            setHealthCheckFile(0);
            LOG(err, "Coult not connect to 2/3 of peers");
            exit(110);
        }

        for (int i = 0; i < getNodeCount(); i++) {

            if (i != getSchainIndex() && !connections.count(i)) {
                try {
                    auto x = make_shared<ClientSocket>(*this, schain_index(i), port_type::PROPOSAL);
                    LOG(debug, "Health check: connected to peer");
                    getIo()->writeMagic(x, true);
                    x->closeSocket();
                    connections.insert(i);

                } catch (ExitRequestedException &) { throw; }
                catch (...) {

                    usleep(1000);
                }
            }
        }
    }

    setHealthCheckFile(2);
}





