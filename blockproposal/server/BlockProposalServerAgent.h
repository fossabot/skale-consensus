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

    @file BlockProposalServerAgent.h
    @author Stan Kladko
    @date 2018
*/

#pragma once

#include "../../abstracttcpserver/AbstractServerAgent.h"
#include "../../pendingqueue/PendingTransactionsAgent.h"

class BlockProposalWorkerThreadPool;

class Transaction;

class TransactionList;


class Comparator {
public:
    bool operator()(const ptr<partial_sha_hash> &a,
                    const ptr<partial_sha_hash> &b) const {
        for (size_t i = 0; i < PARTIAL_SHA_HASH_LEN; i++) {
            if ((*a)[i] < (*b)[i])
                return false;
            if ((*b)[i] < (*a)[i])
                return true;
        }
        return false;
    }
};


class BlockProposalServerAgent : public AbstractServerAgent {

    ptr<BlockProposalWorkerThreadPool> blockProposalWorkerThreadPool;


    void processProposalRequest(ptr<Connection> _connection, nlohmann::json _proposalRequest);


    void processFinalizeRequest(ptr<Connection> _connection, nlohmann::json _finalizeRequest);


public:
    BlockProposalServerAgent(Schain &_schain, ptr<TCPServerSocket> _s);

    ~BlockProposalServerAgent() override;

    ptr<unordered_map<ptr<partial_sha_hash>, ptr<Transaction>, PendingTransactionsAgent::Hasher, PendingTransactionsAgent::Equal>>
    readMissingTransactions(ptr<Connection> connectionEnvelope_, nlohmann::json missingTransactionsResponseHeader);


    pair<ptr<map<uint64_t, ptr<Transaction>>>,
            ptr<map<uint64_t, ptr<partial_sha_hash>>>> getPresentAndMissingTransactions(Schain &subChain_,
                                                                                         ptr<Header>,
                                                                                         ptr<PartialHashesList> _phm);


    BlockProposalWorkerThreadPool *getBlockProposalWorkerThreadPool() const;

    void checkForOldBlock(const block_id &_blockID);

    ptr<Header>
    createProposalResponseHeader(ptr<Connection> _connectionEnvelope,
                                 nlohmann::json _jsonRequest);

    ptr<Header>
    createFinalizeResponseHeader(ptr<Connection> _connectionEnvelope,
                                 nlohmann::json _jsonRequest);


    nlohmann::json
    readMissingTransactionsResponseHeader(ptr<Connection> _connectionEnvelope);


    void processNextAvailableConnection(ptr<Connection> _connection) override;

};
