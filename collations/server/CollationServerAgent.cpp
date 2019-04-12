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

    @file CollationServerAgent.cpp
    @author Stan Kladko
    @date 2018
*/

#include "../../node/NodeInfo.h"
#include "../../chainstorage/PersistentChainStorage.h"
#include "../../pendingqueue/PendingTransactionsAgent.h"

#include "../../network/UDPNetwork.h"
#include "../../network/IO.h"

#include "../../network/Connection.h"
#include "../../network/Buffer.h"
#include "../CollationRequestHeader.h"
#include "../../chains/Schain.h"
#include "../../chains/TopSubChain.h"
#include "../CollationMessage.h"
#include "CollationServerAgent.h"

using namespace std;


ptr<PartialHashesMessage>
CollationServerAgent::readPartialHashes(ptr<Connection> /*connectionEnvelope*/, nlohmann::json /*jsonRequest*/) {
    return make_shared<PartialHashesMessage>(transaction_count(0));
}


void CollationServerAgent::addAndStoreProposalMessage(SubChain &subChain, ptr<PartialHashesMessage> bdm,
                                                      block_id /*_blockID*/, schain_index /*_proposerIndex*/) {

    assert(subChain.isTop());

    ((TopSubChain &) subChain).getReceivedCollationsAgent()->addCollationbyMessage(
            dynamic_pointer_cast<CollationMessage>(bdm));

}

CollationServerAgent::CollationServerAgent(Node &node_, ptr<IncomingSocket> s_) : AbstractProposalServerAgent(
        "Collation server", node_, s_) {
    collationWorkerThreadPool = new CollationWorkerThreadPool(num_threads(1), this);

    collationWorkerThreadPool->startService();


}


ptr<Header> CollationServerAgent::readAndProcessFullHeader(ptr<Header> /*requestHeader*/,
                                                                   ptr<Connection> /*connectionEnvelope*/) {

    return nullptr;
}

void CollationServerAgent::completeResponseHeader(nlohmann::json &/*j*/,
                                                  ptr<Header> /*tcpResponseHeader*/,
                                                  SubChain &/*subChain*/) {

}

ptr<vector<ptr<partial_sha3_hash>>>
CollationServerAgent::getMissingMessageHashes(SubChain &/*subChain_*/, ptr<Header> /*tcpHeader_*/,
                                              ptr<PartialHashesMessage> /*bdm_*/) {
    assert(false);
    return make_shared<vector<ptr<partial_sha3_hash>>>();
}

ptr<MissingTransactionsMessage>
CollationServerAgent::readMissingTransactions(ptr<Connection> /*connectionEnvelope_*/,
                                              nlohmann::json /*missingTransactionsResponseHeader*/) {
    return ptr<MissingTransactionsMessage>();
}













