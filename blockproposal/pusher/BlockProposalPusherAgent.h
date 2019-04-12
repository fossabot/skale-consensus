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

    @file BlockProposalPusherAgent.h
    @author Stan Kladko
    @date 2018
*/

#pragma once

#include "../../Agent.h"
#include "../../pendingqueue/PendingTransactionsAgent.h"


class ClientSocket;
class Schain;
class BlockProposalPusherThreadPool;
class BlockProposal;
class MissingTransactionsRequestHeader;


class BlockProposalPusherAgent : public Agent{

    int connection;







    map<schain_index, ptr<queue<ptr<BlockProposal>>>> itemQueue;




    class Comparator {
    public:
        bool operator()(const ptr<partial_sha3_hash> &a,
                        const ptr<partial_sha3_hash>& b ) const {
            for (size_t i = 0; i < PARTIAL_SHA3_HASHLEN; i++) {
                if ((*a)[i] < (*b)[i])
                    return false;
                if ((*b)[i] < (*a)[i])
                    return true;
            }
            return false;
        }
    };


    atomic<uint64_t> threadCounter;


public:

    ptr<BlockProposalPusherThreadPool> blockProposalThreadPool = nullptr;


    explicit BlockProposalPusherAgent(Schain& subChain_);


    void sendItem(ptr<BlockProposal> _proposal, schain_index _dstIndex);


    static void workerThreadItemSendLoop(BlockProposalPusherAgent *agent);

    uint64_t incrementAndReturnThreadCounter();

    void enqueueItem(ptr<BlockProposal> item);

    nlohmann::json readProposalResponseHeader(ptr<ClientSocket> _socket);


    ptr<MissingTransactionsRequestHeader> readAndProcessMissingTransactionsRequestHeader(ptr<ClientSocket> _socket);



    ptr<unordered_set<ptr<partial_sha3_hash>, PendingTransactionsAgent::Hasher, PendingTransactionsAgent::Equal>>
    readMissingHashes(ptr<ClientSocket> _socket, uint64_t _count);


};

