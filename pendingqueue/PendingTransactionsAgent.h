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

    @file PendingTransactionsAgent.h
    @author Stan Kladko
    @date 2018
*/

#pragma once

#include <boost/functional/hash.hpp>
#include "../Agent.h"


class Schain;
class BlockProposal;
class PartialHashesList;
class Transaction;



class PendingTransactionsAgent : Agent {


public:


    class Hasher {
    public:
        std::size_t operator()(const ptr<partial_sha3_hash>& a) const {
            size_t hash = 0;
            for (size_t i = 0; i < PARTIAL_SHA3_HASHLEN; i++) {
                hash = hash * 31 + (*a)[i];
            }
            return hash;
        };


    };



    class Equal {
    public:
        std::size_t operator() (const ptr<partial_sha3_hash>& a, const ptr<partial_sha3_hash>& b)  const {
            for (size_t i = 0; i < PARTIAL_SHA3_HASHLEN; i++) {
                if ((*a)[i] != (*b)[i]) {
                    return false;
                }
            }
            return true;
        };
    };


private:

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


    unordered_map<ptr<partial_sha3_hash>, ptr<Transaction>, Hasher, Equal> pendingTransactions;
    unordered_set<ptr<partial_sha3_hash>, Hasher, Equal> committedTransactions;
    unordered_map<ptr<partial_sha3_hash>, ptr<Transaction> , Hasher, Equal> knownTransactions;

    list<ptr<partial_sha3_hash>> committedTransactionsList;

    transaction_count transactionCounter = 0;

    uint64_t committedTransactionCounter = 0;


    recursive_mutex transactionsMutex;


public:

    PendingTransactionsAgent(Schain& _sChain);

    uint64_t getCommittedTransactionCounter() const;

    transaction_count getTransactionCounter() const;

    void pushTransaction(ptr<Transaction> _transaction);

    void pushKnownTransaction(ptr<Transaction> _transaction);

    void pushKnownTransactions(ptr<vector<ptr<Transaction>>> _transactions);

    void pushTransactions(ptr<vector<ptr<Transaction>>> _transactions);

    void waitUntilPendingTransaction();

    uint64_t getKnownTransactionsSize();

    uint64_t getPendingTransactionsSize();

    uint64_t getCommittedTransactionsSize();


    bool isCommitted(ptr< partial_sha3_hash > _hash);

    void pushCommittedTransaction(shared_ptr<Transaction> t);


    shared_ptr<vector<ptr<Transaction>>> createTransactionsListForProposal();

    ptr<Transaction> getKnownTransactionByPartialHash(ptr<partial_sha3_hash> hash);


    void cleanCommittedTransactionsFromQueue(ptr<BlockProposal> _committedBlockProposal);

    ptr<BlockProposal> buildBlockProposal(block_id _blockID, uint64_t  _previousBlockTimeStamp);


    void addToCommitted(shared_ptr<partial_sha3_hash> &s);
};



