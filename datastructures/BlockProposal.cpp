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

    @file BlockProposal.cpp
    @author Stan Kladko
    @date 2018
*/

#include "../SkaleConfig.h"
#include "../Log.h"
#include "../exceptions/FatalError.h"

#include "../node/ConsensusEngine.h"
#include "../exceptions/InvalidArgumentException.h"
#include "../exceptions/OldBlockIDException.h"
#include "Transaction.h"
#include "TransactionList.h"
#include "PartialHashesList.h"
#include "../chains/Schain.h"
#include "../pendingqueue/PendingTransactionsAgent.h"

#include "BlockProposal.h"



using namespace std;

ptr<sha3_hash> BlockProposal::getHash() {
    ASSERT(hash);
    return hash;

}




void BlockProposal::calculateHash() {


    CryptoPP::SHA3 sha3(SHA3_HASHLEN);

    sha3.Update( reinterpret_cast < uint8_t * > ( &schainID      ), sizeof(schainID) );
    sha3.Update( reinterpret_cast < uint8_t * > ( &blockID       ), sizeof(blockID) );
    sha3.Update( reinterpret_cast < uint8_t * > ( &transactionCount ), sizeof(transactionCount));

    for (uint64_t i = 0; i < transactionCount; i++) {
        auto t = transactionList->getItems();
        ASSERT((*t)[i]);
        sha3.Update((*t)[i]->getHash()->data(), SHA3_HASHLEN);
    }

    uint8_t buf[SHA3_HASHLEN];
    sha3.Final(buf);
    this->hash = make_shared<sha3_hash>(buf);
};



BlockProposal::BlockProposal(uint64_t _timeStamp) : timeStamp(_timeStamp) {
};

BlockProposal::BlockProposal(Schain &_sChain, block_id _blockID, schain_index _proposerIndex,
                             ptr<TransactionList> _transactions, uint64_t _timeStamp) : schainID(_sChain.getSchainID()),
                                                                                                blockID(_blockID),
                                                                                                proposerIndex(_proposerIndex),
                                                                                                timeStamp(_timeStamp),
                                                                                                transactionList(_transactions){
    ASSERT(timeStamp > MODERN_TIME);
    transactionCount = transactionList->getItems()->size();
    calculateHash();

}



ptr<PartialHashesList> BlockProposal::createPartialHashesList() {



    auto s = (uint64_t)this->transactionCount * PARTIAL_SHA3_HASHLEN;
    auto t = transactionList->getItems();


    if (s > MAX_BUFFER_SIZE) {
        throw InvalidArgumentException("Buffer size too large", __CLASS_NAME__);
    }

    auto partialHashes = make_shared<vector<uint8_t>>(s);

    for (uint64_t i = 0; i < transactionCount; i++) {

        for (size_t j = 0; j < PARTIAL_SHA3_HASHLEN; j++) {
            (*partialHashes)[i * PARTIAL_SHA3_HASHLEN + j] = (*(*t)[i]->getHash())[j];
        }
    }

    return make_shared<PartialHashesList>((transaction_count)transactionCount, partialHashes);

}

BlockProposal::~BlockProposal() {

}

block_id BlockProposal::getBlockID() const {
    return blockID;
}

const transaction_count &BlockProposal::getTransactionsCount() const {
    return transactionCount;
}

const schain_index &BlockProposal::getProposerIndex() const {
    return proposerIndex;
}

ptr<TransactionList>  BlockProposal::getTransactionList()  {
    return transactionList;
}

const schain_id &BlockProposal::getSchainID() const {
    return schainID;
}

const transaction_count &BlockProposal::getTransactionCount() const {
    return transactionCount;
}

uint64_t BlockProposal::getTimeStamp() const {
    return timeStamp;
}


