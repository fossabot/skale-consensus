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

    @file TransactionList.h
    @author Stan Kladko
    @date 2018
*/

#pragma once

#include "DataStructure.h"


class Transaction;

class TransactionList : public DataStructure  {

    ptr<vector<uint8_t>> serializedTransactions = nullptr;

    ptr<vector<ptr<Transaction>>> transactions = nullptr;

public:


    TransactionList(ptr<vector<size_t>> transactionSizes_, ptr<vector<uint8_t>> serializedTransactions, uint32_t  offset = 0);

    TransactionList(ptr<vector<ptr<Transaction>>> _transactions);

    ptr<vector<ptr<Transaction>>> getItems() ;

    shared_ptr<vector<uint8_t>> serialize() ;

    size_t size();


    static atomic<uint64_t>  totalObjects;

    static uint64_t getTotalObjects() {
        return totalObjects;
    }

    virtual ~TransactionList();


};



