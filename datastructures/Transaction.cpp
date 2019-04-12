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

    @file Transaction.cpp
    @author Stan Kladko
    @date 2018
*/

#include "../SkaleConfig.h"
#include "../chains/Schain.h"
#include "../Log.h"
#include "../exceptions/FatalError.h"


#include "Transaction.h"

ptr<sha3_hash> Transaction::getHash() {

    ASSERT(data && data->size() > 0);


    if (hash)
        return hash;

    uint8_t digest[SHA3_HASHLEN];

    CryptoPP::SHA3 hashObject(SHA3_HASHLEN);

    hashObject.Update(data.get()->data(), data->size());
    hashObject.Final(digest);



    hash = make_shared<sha3_hash>(digest);

    return hash;

}


ptr<partial_sha3_hash> Transaction::getPartialHash() {


    auto hash = getHash();

    if (partialHash) {
        return partialHash;
    }

    partialHash = make_shared<partial_sha3_hash>();

    for (size_t i = 0; i < PARTIAL_SHA3_HASHLEN; i++) {
        (*partialHash)[i] = (*hash)[i];
    }

    return partialHash;

}

Transaction::Transaction(const ptr<vector<uint8_t>> data) : data(data) {

};

const ptr<vector<uint8_t>> &Transaction::getData() const {
    return data;
}

Transaction::~Transaction() {

}

