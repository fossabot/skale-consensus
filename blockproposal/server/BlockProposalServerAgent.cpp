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

    @file BlockProposalServerAgent.cpp
    @author Stan Kladko
    @date 2018
*/

#include "../../Agent.h"
#include "../../SkaleConfig.h"

#include "../../Log.h"
#include "../../exceptions/FatalError.h"

#include "../../thirdparty/json.hpp"

#include "../../abstracttcpserver/ConnectionStatus.h"
#include "../../exceptions/CouldNotReadPartialDataHashesException.h"
#include "../../exceptions/CouldNotSendMessageException.h"
#include "../../exceptions/InvalidNodeIDException.h"
#include "../../exceptions/InvalidSchainException.h"
#include "../../exceptions/InvalidSourceIPException.h"
#include "../../exceptions/InvalidSubchainIndexException.h"
#include "../../exceptions/OldBlockIDException.h"
#include "../../exceptions/ExitRequestedException.h"
#include "../../exceptions/PingException.h"
#include "../../node/NodeInfo.h"


#include "../../pendingqueue/PendingTransactionsAgent.h"
#include "../pusher/BlockProposalPusherAgent.h"
#include "../received/ReceivedBlockProposalsDatabase.h"

#include "../../abstracttcpserver/AbstractServerAgent.h"
#include "../../chains/Schain.h"
#include "../../headers/Header.h"
#include "../../headers/MissingTransactionsRequestHeader.h"
#include "../../network/Connection.h"
#include "../../network/IO.h"
#include "../../network/Sockets.h"
#include "../../network/TransportNetwork.h"

#include "../../node/Node.h"

#include "../../datastructures/BlockProposal.h"
#include "../../datastructures/ReceivedBlockProposal.h"
#include "../../datastructures/PartialHashesList.h"
#include "../../datastructures/Transaction.h"
#include "../../datastructures/TransactionList.h"
#include "../../headers/BlockProposalHeader.h"


#include "BlockProposalServerAgent.h"
#include "BlockProposalWorkerThreadPool.h"


ptr<unordered_map<ptr<partial_sha3_hash>, ptr<Transaction>, PendingTransactionsAgent::Hasher, PendingTransactionsAgent::Equal>>
BlockProposalServerAgent::readMissingTransactions(
        ptr<Connection> connectionEnvelope_, nlohmann::json missingTransactionsResponseHeader) {
    ASSERT(missingTransactionsResponseHeader > 0);

    auto transactionSizes = make_shared<vector<size_t> >();

    nlohmann::json jsonSizes = missingTransactionsResponseHeader["sizes"];

    if (!jsonSizes.is_array()) {
       BOOST_THROW_EXCEPTION(NetworkProtocolException("jsonSizes is not an array", __CLASS_NAME__));
    } ;


    size_t totalSize = 0;

    for (auto &&size : jsonSizes) {
        transactionSizes->push_back(size);
        totalSize += (size_t) size;
    }

    auto serializedTransactions = make_shared<vector<uint8_t> >(totalSize);


    try {
        getSchain()->getIo()->readBytes(connectionEnvelope_,
                                        (in_buffer *) serializedTransactions->data(), msg_len(totalSize));
    } catch (ExitRequestedException &) { throw; }
    catch (...) {
        BOOST_THROW_EXCEPTION(NetworkProtocolException("Could not read serialized exceptions", __CLASS_NAME__));
    }

    auto list = make_shared<TransactionList>(transactionSizes, serializedTransactions);

    auto trs = list->getItems();

    auto missed = make_shared<unordered_map<ptr<partial_sha3_hash>, ptr<Transaction>,
            PendingTransactionsAgent::Hasher, PendingTransactionsAgent::Equal>>();

    for (auto &&t: *trs) {
        (*missed)[t->getPartialHash()] = t;
    }

    return missed;
}


BlockProposalWorkerThreadPool *BlockProposalServerAgent::getBlockProposalWorkerThreadPool() const {
    return blockProposalWorkerThreadPool.get();
}


pair<ptr<map<uint64_t, ptr<Transaction>>>,
        ptr<map<uint64_t, ptr<partial_sha3_hash >>>> BlockProposalServerAgent::getPresentAndMissingTransactions(
        Schain &subChain_, ptr<Header> /*tcpHeader*/, ptr<PartialHashesList> _phm) {
    LOG(debug, "Calculating missing hashes");

    auto transactionsCount = _phm->getTransactionCount();

    auto presentTransactions = make_shared<map<uint64_t, ptr<Transaction> > >();
    auto missingHashes = make_shared<map<uint64_t, ptr<partial_sha3_hash> > >();

    for (uint64_t i = 0; i < transactionsCount; i++) {
        auto hash = _phm->getPartialHash(i);
        ASSERT(hash);
        auto transaction =
                subChain_.getPendingTransactionsAgent()->getKnownTransactionByPartialHash(hash);
        if (transaction == nullptr) {
            (*missingHashes)[i] = hash;
        } else {
            (*presentTransactions)[i] = transaction;
        }
    }

    return {presentTransactions, missingHashes};
}


BlockProposalServerAgent::BlockProposalServerAgent(Schain &_schain, ptr<TCPServerSocket> _s)
        : AbstractServerAgent("Block proposal server", _schain, _s) {
    blockProposalWorkerThreadPool =
            make_shared<BlockProposalWorkerThreadPool>(num_threads(1), this);
    blockProposalWorkerThreadPool->startService();
    createNetworkReadThread();
}

BlockProposalServerAgent::~BlockProposalServerAgent() {}


void BlockProposalServerAgent::processNextAvailableConnection(ptr<Connection> _connection) {


    try {
        sChain->getIo()->readMagic(_connection->getDescriptor());
    }
    catch (ExitRequestedException &) { throw; }
    catch (PingException&) {return;}
    catch (...) {
        throw_with_nested(NetworkProtocolException("Could not read magic number", __CLASS_NAME__));
    }



    nlohmann::json proposalRequest = nullptr;

    try {
        proposalRequest = getSchain()->getIo()->readJsonHeader(_connection->getDescriptor(), "Read proposal req");
    } catch (ExitRequestedException &) { throw; }
    catch (...) {
        throw_with_nested(CouldNotSendMessageException("Could not read proposal request", __CLASS_NAME__));
    }


    ptr<Header> responseHeader = nullptr;

    try {
        responseHeader = this->createProposalResponseHeader(_connection, proposalRequest);
    } catch (ExitRequestedException &) { throw; }
    catch (...) {
        throw_with_nested(NetworkProtocolException("Could not create response header", __CLASS_NAME__));
    }

    try {
        send(_connection, responseHeader);
    }
    catch (ExitRequestedException &) { throw; }
    catch (...) {
        throw_with_nested(CouldNotSendMessageException("Could not send response header", __CLASS_NAME__));
    }

    if (responseHeader->getStatus() != CONNECTION_PROCEED) {
        return;
    }

    ptr<PartialHashesList> partialHashesList = nullptr;

    try {

        partialHashesList = readPartialHashes(_connection, proposalRequest);
    }
    catch (ExitRequestedException &) { throw; }
    catch (...) {
        throw_with_nested(NetworkProtocolException("Could not read partial hashes", __CLASS_NAME__));
    }


    auto schainID = schain_id(Header::getUint64(proposalRequest, "schainID"));

    if (!sChain->getSchainID() == schainID) {
        BOOST_THROW_EXCEPTION(InvalidSchainException("Invalid schain id", __CLASS_NAME__));
    }


    auto result = getPresentAndMissingTransactions(*sChain, responseHeader, partialHashesList);

    auto presentTransactions = result.first;
    auto missingTransactionHashes = result.second;

    //auto proposerNodeID = node_id( Header::getUint64( proposalRequest, "srcNodeID" ) );
    auto proposerIndex = schain_index(Header::getUint64(proposalRequest, "srcSchainIndex"));
    auto blockID = block_id(Header::getUint64(proposalRequest, "blockID"));
    auto timeStamp = Header::getUint64(proposalRequest, "timeStamp");


    auto missingHashesRequestHeader = make_shared<MissingTransactionsRequestHeader>(missingTransactionHashes);

    try {
        send(_connection, missingHashesRequestHeader);
    } catch (ExitRequestedException &) { throw; }
    catch (...) {
        throw_with_nested(CouldNotSendMessageException("Could not send missing hashes request header", __CLASS_NAME__));
    }


    ptr<unordered_map<ptr<partial_sha3_hash>, ptr<Transaction>, PendingTransactionsAgent::Hasher,
            PendingTransactionsAgent::Equal>> missingTransactions = nullptr;

    if (missingTransactionHashes->size() == 0) {
        LOG(debug, "Server: No missing partial hashes");
    } else {
        LOG(debug, "Server: missing partial hashes");
        try {
            getSchain()->getIo()->writePartialHashes(
                    _connection->getDescriptor(), missingTransactionHashes);
        } catch (ExitRequestedException &) { throw; }
        catch (...) {
            BOOST_THROW_EXCEPTION(
                    CouldNotSendMessageException("Could not send missing hashes request header", __CLASS_NAME__));
        }


        auto missingMessagesResponseHeader =
                this->readMissingTransactionsResponseHeader(_connection);

        missingTransactions = readMissingTransactions(_connection, missingMessagesResponseHeader);


        if (missingTransactions == nullptr) {
            BOOST_THROW_EXCEPTION(
                    CouldNotReadPartialDataHashesException("Null missing transactions", __CLASS_NAME__));
        }


        for (auto &&item: *missingTransactions) {
            sChain->getPendingTransactionsAgent()->pushKnownTransaction(item.second);
        }

    }


    if (sChain->getCommittedBlockID() >= blockID)
        return;

    LOG(debug, "Storing block proposal");


    auto transactions = make_shared<vector<ptr<Transaction> > >();

    auto transactionCount = partialHashesList->getTransactionCount();

    for (uint64_t i = 0; i < transactionCount; i++) {
        auto hash = partialHashesList->getPartialHash(i);
        ASSERT(hash);

        if (getSchain()->getPendingTransactionsAgent()->isCommitted(hash)) {
            checkForOldBlock(blockID);
            BOOST_THROW_EXCEPTION(
                    CouldNotReadPartialDataHashesException("Committed transaction", __CLASS_NAME__));
        }

        ptr<Transaction> transaction;

        if (presentTransactions->count(i) > 0) {
            transaction = (*presentTransactions)[i];
        } else {

            transaction = (*missingTransactions)[hash];
        };


        if (transaction == nullptr) {
            checkForOldBlock(blockID);
            ASSERT(missingTransactions);

            if (missingTransactions->count(hash) > 0) {
                LOG(err, "Found in missing");
            }

            ASSERT(false);
        }


        ASSERT(transactions != nullptr);

        transactions->push_back(transaction);
    }

    ASSERT(transactionCount == 0 || (*transactions)[(uint64_t) transactionCount - 1]);

    ASSERT(timeStamp > 0);

    auto transactionList = make_shared<TransactionList>(transactions);

    auto proposal =
            make_shared<ReceivedBlockProposal>(*sChain, blockID, proposerIndex, transactionList, timeStamp);

    sChain->proposedBlockArrived(proposal);
}


void BlockProposalServerAgent::checkForOldBlock(const block_id &_blockID) {
    LOG(debug, "BID:" + to_string(_blockID) +
               ":CBID:" + to_string(getSchain()->getCommittedBlockID()) +
               ":MQ:" + to_string(getSchain()->getMessagesCount()));
    if (_blockID <= getSchain()->getCommittedBlockID())
        throw OldBlockIDException("Old block ID", nullptr, nullptr, __CLASS_NAME__);
}


ptr<Header> BlockProposalServerAgent::createProposalResponseHeader(
        ptr<Connection> _connectionEnvelope, nlohmann::json _jsonRequest) {
    ptr<Header> responseHeader = make_shared<Header>();


    block_id blockID;
    node_id srcNodeID;
    // node_id dstNodeID;
    schain_index srcSchainIndex;
    schain_id schainID;
    uint64_t timeStamp;

    schainID = Header::getUint64(_jsonRequest, "schainID");
    srcNodeID = Header::getUint64(_jsonRequest, "srcNodeID");
    // dstNodeID = Header::getUint64(_jsonRequest,"dstNodeID");
    srcSchainIndex = Header::getUint64(_jsonRequest, "srcSchainIndex");
    blockID = Header::getUint64(_jsonRequest, "blockID");
    timeStamp = Header::getUint64(_jsonRequest, "timeStamp");

// std::cout << "BlockProposalServerAgent::createProposalResponseHeader() got request JSON: " << _jsonRequest.dump(4) << "\n";

    if (sChain->getSchainID() != schainID) {
        responseHeader->setStatusSubStatus(
                CONNECTION_SERVER_ERROR, CONNECTION_ERROR_UNKNOWN_SCHAIN_ID);
        BOOST_THROW_EXCEPTION(
                InvalidSchainException("Incorrect schain " + to_string(schainID), __CLASS_NAME__));
    };


    ptr<NodeInfo> nmi = sChain->getNode()->getNodeInfoByIP(_connectionEnvelope->getIP());

    if (nmi == nullptr) {
        responseHeader->setStatusSubStatus(
                CONNECTION_SERVER_ERROR, CONNECTION_ERROR_DONT_KNOW_THIS_NODE);
        BOOST_THROW_EXCEPTION(InvalidSourceIPException(
                                      "Could not find node info for IP " + *_connectionEnvelope->getIP()));
    }


    if (nmi->getNodeID() != node_id(srcNodeID)) {
        responseHeader->setStatusSubStatus(
                CONNECTION_SERVER_ERROR, CONNECTION_ERROR_INVALID_NODE_ID);

        BOOST_THROW_EXCEPTION(InvalidNodeIDException("Node ID does not match " + srcNodeID, __CLASS_NAME__));
    }

    if (nmi->getSchainIndex() != schain_index(srcSchainIndex)) {
        responseHeader->setStatusSubStatus(
                CONNECTION_SERVER_ERROR, CONNECTION_ERROR_INVALID_NODE_INDEX);
        BOOST_THROW_EXCEPTION(InvalidSubchainIndexException(
                                      "Node subchain index does not match " + srcSchainIndex, __CLASS_NAME__));
    }


    if (sChain->getCommittedBlockID() >= block_id(blockID)) {
        responseHeader->setStatusSubStatus(
                CONNECTION_DISCONNECT, CONNECTION_BLOCK_PROPOSAL_TOO_LATE);
        responseHeader->setComplete();
        return responseHeader;
    }


    ASSERT(timeStamp > MODERN_TIME);

    auto t = Schain::getCurrentTimeSec();

    assert(t < (uint64_t) MODERN_TIME * 2);

    if (Schain::getCurrentTimeSec() + 1 < timeStamp) {
        LOG(info,
            "Incorrect timestamp:" + to_string(timeStamp) + ":vs:" + to_string(Schain::getCurrentTimeSec()));
        responseHeader->setStatusSubStatus(
                CONNECTION_DISCONNECT, CONNECTION_ERROR_TIME_STAMP_IN_THE_FUTURE);
        responseHeader->setComplete();
        ASSERT(false);
        return responseHeader;
    }


    if (sChain->getCommittedBlockTimeStamp() >= timeStamp) {
        LOG(info, "Incorrect timestamp:" + to_string(timeStamp) +
                  ":vs:" + to_string(sChain->getCommittedBlockTimeStamp()));

        responseHeader->setStatusSubStatus(
                CONNECTION_DISCONNECT, CONNECTION_ERROR_TIME_STAMP_EARLIER_THAN_COMMITTED);
        responseHeader->setComplete();
        ASSERT(false);
        return responseHeader;
    }


    responseHeader->setStatus(CONNECTION_PROCEED);


    responseHeader->setComplete();
    return responseHeader;
}


nlohmann::json BlockProposalServerAgent::readMissingTransactionsResponseHeader(
        ptr<Connection> _connectionEnvelope) {
    auto js = sChain->getIo()->readJsonHeader(_connectionEnvelope->getDescriptor(), "Read missing trans response");

    return js;
}
