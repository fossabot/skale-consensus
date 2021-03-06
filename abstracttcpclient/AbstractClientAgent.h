/*
    Copyright (C) 2019 SKALE Labs

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

    @file AbstractClientAgent.h
    @author Stan Kladko
    @date 2019
*/

#ifndef CONSENSUS_ABSTRACTCLIENTAGENT_H
#define CONSENSUS_ABSTRACTCLIENTAGENT_H



#include "../Agent.h"
#include "../../abstracttcpclient/AbstractClientAgent.h"


class BlockProposal;
class ClientSocket;

class AbstractClientAgent : public Agent {



protected:

    port_type portType;

    atomic<uint64_t> threadCounter;

    explicit AbstractClientAgent(Schain &_sChain, port_type _portType);



public:



    static void workerThreadItemSendLoop(AbstractClientAgent *agent);

    void enqueueItem(ptr<BlockProposal> item);


    void enqueueBlock(ptr<CommittedBlock> item);

    void sendItem(ptr<BlockProposal> _proposal, schain_index _dstIndex, node_id _dstNodeId);

    virtual void
    sendItemImpl(ptr<BlockProposal> &_proposal, shared_ptr<ClientSocket> &socket, schain_index _destIndex,
                 node_id _dstNodeId) = 0;

    map<schain_index, ptr<queue<ptr<BlockProposal>>>> itemQueue;

    uint64_t incrementAndReturnThreadCounter();


    class PartialHashComparator {
    public:
        bool operator()(const ptr<partial_sha_hash> &a,
                        const ptr<partial_sha_hash>& b ) const {
            for (size_t i = 0; i < PARTIAL_SHA_HASH_LEN; i++) {
                if ((*a)[i] < (*b)[i])
                    return false;
                if ((*b)[i] < (*a)[i])
                    return true;
            }
            return false;
        }
    };

};


bool operator==(const ptr<partial_sha_hash> &a, const ptr<partial_sha_hash> &b);

#endif //CONSENSUS_ABSTRACTCLIENTAGENT_H
