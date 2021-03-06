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

    @file BlockProposalPusherAgent.cpp
    @author Stan Kladko
    @date 2018
*/

#include "../../SkaleConfig.h"
#include "../../Log.h"
#include "../../Agent.h"
#include "../../exceptions/FatalError.h"
#include "../../thirdparty/json.hpp"

#include "../../crypto/SHAHash.h"
#include "../../abstracttcpserver/ConnectionStatus.h"
#include "../../datastructures/PartialHashesList.h"
#include "../../datastructures/Transaction.h"
#include "../../datastructures/TransactionList.h"


#include "../../node/Node.h"
#include "../../chains/Schain.h"
#include "../../network/IO.h"
#include "../../network/TransportNetwork.h"
#include "../../pendingqueue/PendingTransactionsAgent.h"
#include "../../headers/BlockProposalHeader.h"
#include "../../network/ClientSocket.h"
#include "../../node/Node.h"
#include "../../exceptions/NetworkProtocolException.h"
#include "../../network/Connection.h"
#include "../../headers/MissingTransactionsRequestHeader.h"
#include "../../headers/MissingTransactionsResponseHeader.h"
#include "../../datastructures/BlockProposal.h"
#include "../../datastructures/CommittedBlock.h"

#include "../../exceptions/ExitRequestedException.h"
#include "../../exceptions/PingException.h"
#include "../../abstracttcpclient/AbstractClientAgent.h"
#include "BlockProposalPusherThreadPool.h"
#include "BlockProposalClientAgent.h"



BlockProposalClientAgent::BlockProposalClientAgent(Schain &_sChain) : AbstractClientAgent(_sChain, PROPOSAL) {
    LOG(info, "Constructing blockProposalPushAgent");

    this->blockProposalThreadPool = make_shared<BlockProposalPusherThreadPool>(
            num_threads((uint64_t) _sChain.getNodeCount()), this);
    blockProposalThreadPool->startService();
}


nlohmann::json BlockProposalClientAgent::readProposalResponseHeader(ptr<ClientSocket> _socket) {
    return sChain->getIo()->readJsonHeader(_socket->getDescriptor(), "Read proposal resp");
}


ptr<MissingTransactionsRequestHeader>
BlockProposalClientAgent::readAndProcessMissingTransactionsRequestHeader(
        ptr<ClientSocket> _socket) {
    auto js = sChain->getIo()->readJsonHeader(_socket->getDescriptor(), "Read missing trans request");
    auto mtrh = make_shared<MissingTransactionsRequestHeader>();

    auto status = (ConnectionStatus) Header::getUint64(js, "status");
    auto substatus = (ConnectionSubStatus) Header::getUint64(js, "substatus");
    mtrh->setStatusSubStatus(status, substatus);

    auto count = (uint64_t) Header::getUint64(js, "count");
    mtrh->setMissingTransactionsCount(count);

    mtrh->setComplete();
    LOG(trace, "Push agent processed missing transactions header");
    return mtrh;
}




void
BlockProposalClientAgent::sendItemImpl(ptr<BlockProposal> &_proposal, shared_ptr<ClientSocket> &socket, schain_index,
                                       node_id ) {

    LOG(trace, "Proposal step 0: Starting block proposal");


    ptr<Header> header = make_shared<BlockProposalHeader>(*sChain, _proposal);


    try {
        getSchain()->getIo()->writeHeader(socket, header);
    } catch (ExitRequestedException &) {
        throw;
    } catch (...) {
        throw_with_nested(NetworkProtocolException("Could not write header", __CLASS_NAME__));
    }


    LOG(trace, "Proposal step 1: wrote proposal header");

    auto response = sChain->getIo()->readJsonHeader(socket->getDescriptor(), "Read proposal resp");


    LOG(trace, "Proposal step 2: read proposal response");


    auto status = (ConnectionStatus) Header::getUint64(response, "status");
    // auto substatus = (ConnectionSubStatus) Header::getUint64(response, "substatus");


    if (status != CONNECTION_PROCEED) {
        LOG(trace, "Proposal Server terminated proposal push");
        return;
    }

    auto partialHashesList = _proposal->createPartialHashesList();


    if (partialHashesList->getTransactionCount() > 0) {
        try {
            getSchain()->getIo()->writeBytesVector(socket->getDescriptor(),
                                                   partialHashesList->getPartialHashes());
        }
        catch (ExitRequestedException &) { throw; }
        catch (...) {
            auto errStr = "Unexpected disconnect writing block data";
            throw_with_nested(NetworkProtocolException(errStr, __CLASS_NAME__));
        }
    }


    LOG(trace, "Proposal step 3: sent partial hashes");

    ptr<MissingTransactionsRequestHeader> missingTransactionHeader;

    try {

        missingTransactionHeader = readAndProcessMissingTransactionsRequestHeader(socket);
    }
    catch (ExitRequestedException &) { throw; }
    catch (...) {
        auto errStr = "Could not read missing transactions request header";
        throw_with_nested(NetworkProtocolException(errStr, __CLASS_NAME__));
    }

    auto count = missingTransactionHeader->getMissingTransactionsCount();

    if (count == 0) {
        LOG(trace, "Proposal complete::no missing transactions");
        return;
    }

    ptr<unordered_set<ptr<partial_sha_hash>, PendingTransactionsAgent::Hasher, PendingTransactionsAgent::Equal >>
            missingHashes;

    try {

        missingHashes = readMissingHashes(socket, count);
    }
    catch (ExitRequestedException &) { throw; }
    catch (...) {
        auto errStr = "Could not read missing hashes";
        throw_with_nested(NetworkProtocolException(errStr, __CLASS_NAME__));
    }


    LOG(trace, "Proposal step 4: read missing transaction hashes");


    auto missingTransactions = make_shared<vector<ptr<Transaction> > >();
    auto missingTransactionsSizes = make_shared<vector<uint64_t> >();

    for (auto &&transaction : *_proposal->getTransactionList()->getItems()) {
        if (missingHashes->count(transaction->getPartialHash())) {
            missingTransactions->push_back(transaction);
            missingTransactionsSizes->push_back(transaction->getData()->size());
        }
    }

    ASSERT2(missingTransactions->size() == count, "Transactions:" + to_string(missingTransactions->size()) +
                                                  ":" + to_string(count));


    auto mtrh = make_shared<MissingTransactionsResponseHeader>(missingTransactionsSizes);

    try {
        getSchain()->getIo()->writeHeader(socket, mtrh);
    } catch (ExitRequestedException &) {
        throw;
    } catch (...) {
        auto errString = "Proposal: unexpected server disconnect writing missing txs response header";
        throw_with_nested(new NetworkProtocolException(errString, __CLASS_NAME__));
    }


    LOG(trace, "Proposal step 5: sent missing transactions header");


    auto mtrm = make_shared<TransactionList>(missingTransactions);

    try {
        getSchain()->getIo()->writeBytesVector(socket->getDescriptor(), mtrm->serialize());
    } catch (ExitRequestedException &) {
        throw;
    } catch (...) {
        auto errString = "Proposal: unexpected server disconnect  writing missing hashes";
        throw_with_nested(new NetworkProtocolException(errString, __CLASS_NAME__));
    }

    LOG(trace, "Proposal step 6: sent missing transactions");
        }





ptr<unordered_set<ptr<partial_sha_hash>, PendingTransactionsAgent::Hasher, PendingTransactionsAgent::Equal >>
BlockProposalClientAgent::readMissingHashes(ptr<ClientSocket> _socket, uint64_t _count) {
    ASSERT(_count);
    auto bytesToRead = _count * PARTIAL_SHA_HASH_LEN;
    vector<uint8_t> buffer;
    buffer.reserve(bytesToRead);


    try {
        getSchain()->getIo()->readBytes(
                _socket->getDescriptor(), (in_buffer *) buffer.data(), msg_len(bytesToRead));
    } catch (ExitRequestedException&) {throw;}
    catch (...) {
        LOG(info, "Could not read partial hashes");
        throw_with_nested(NetworkProtocolException("Could not read partial data hashes", __CLASS_NAME__));
    }


    auto result = make_shared<unordered_set<ptr<partial_sha_hash>, PendingTransactionsAgent::Hasher,
            PendingTransactionsAgent::Equal> >();


    for (
            uint64_t i = 0;
            i < _count;
            i++) {
        auto hash = make_shared<partial_sha_hash>();
        for (
                size_t j = 0;
                j < PARTIAL_SHA_HASH_LEN;
                j++) {
            (*hash)[j] = buffer[
                    PARTIAL_SHA_HASH_LEN * i
                    + j];
        }

        result->
                insert(hash);
        assert(result->count(hash));
    }


    ASSERT(result->size() == _count);

    return
            result;
}
