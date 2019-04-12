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

    @file PartialHashesList.cpp
    @author Stan Kladko
    @date 2018
*/

#include "../SkaleConfig.h"
#include "../Log.h"
#include "../exceptions/FatalError.h"
#include "../exceptions/InvalidArgumentException.h"

#include "../exceptions/NetworkProtocolException.h"
#include "PartialHashesList.h"

PartialHashesList::~PartialHashesList() {}

PartialHashesList::PartialHashesList(
        transaction_count _transactionCount, ptr<vector<uint8_t> > _partialHashes)
        : transactionCount(_transactionCount), partialHashes(_partialHashes) {}

PartialHashesList::PartialHashesList(transaction_count _transactionCount)
        : transactionCount(_transactionCount) {
    auto s = size_t(uint64_t(_transactionCount)) * PARTIAL_SHA3_HASHLEN;

    if (s > MAX_BUFFER_SIZE) {
        BOOST_THROW_EXCEPTION(InvalidArgumentException("Buffer size too large", __CLASS_NAME__));
    }
    partialHashes = make_shared<vector<uint8_t> >(s);
}

transaction_count PartialHashesList::getTransactionCount() const {
    return transactionCount;
}

ptr<vector<uint8_t> > PartialHashesList::getPartialHashes() const {
    return partialHashes;
}


ptr<partial_sha3_hash> PartialHashesList::getPartialHash(uint64_t i) {
    if (i >= transactionCount) {
        BOOST_THROW_EXCEPTION(
                NetworkProtocolException("Index i is more than messageCount:" + to_string(i), __CLASS_NAME__));
    }
    auto hash = make_shared<array<uint8_t, PARTIAL_SHA3_HASHLEN> >();

    for (size_t j = 0; j < PARTIAL_SHA3_HASHLEN; j++) {
        (*hash)[j] = (*partialHashes)[PARTIAL_SHA3_HASHLEN * i + j];
    }

    return hash;
}
