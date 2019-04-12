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

    @file CollationServerAgent.h
    @author Stan Kladko
    @date 2018
*/

#pragma once

#include <mutex>
#include <queue>
#include <condition_variable>
#include "../../abstracttcpserver/AbstractServerAgent.h"

#include "../../headers/Header.h"
#include "../../headers/Header.h"
#include "../../datastructures/PartialHashesList.h"
#include "../CollationRequestHeader.h"
#include "CollationWorkerThreadPool.h"

class Connection;
class SubChain;

class CollationServerAgent : public AbstractProposalServerAgent {

    CollationWorkerThreadPool* collationWorkerThreadPool;

public:
    CollationServerAgent(Node& node, ptr<IncomingSocket> _s);

public:


    ptr<PartialHashesMessage> readPartialHashes(ptr<Connection> connectionEnvelope,
                                           nlohmann::json jsonRequest);

    ptr<MissingTransactionsMessage>
    readMissingTransactions(ptr<Connection> connectionEnvelope_, nlohmann::json missingTransactionsResponseHeader) override;

    void addAndStoreProposalMessage(SubChain &subChain, ptr<PartialHashesMessage> bdm,
                                        block_id _blockID, schain_index _proposerIndex);

    void
    completeResponseHeader(nlohmann::json &j, ptr<Header> tcpResponseHeader,
                           SubChain &subChain) override;

    ptr<vector<ptr<partial_sha3_hash>>>
    getMissingMessageHashes(SubChain &subChain_, ptr<Header> tcpHeader_, ptr<PartialHashesMessage> bdm_) override;


    virtual void completeResponseHeader(ptr<Buffer> /*buf*/, ptr<Header> /*tcpResponseHeader*/,
                                        SubChain &/*subChain*/)  {}

    virtual ptr<Header> allocateRequestHeader() {
        return make_shared<CollationRequestHeader>();
    }

    ptr<Header>
    readAndProcessFullHeader(ptr<Header> requestHeader, ptr<Connection> connectionEnvelope);
};

