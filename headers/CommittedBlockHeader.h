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

    @file CommittedBlockHeader.h
    @author Stan Kladko
    @date 2018
*/

#pragma  once

#include "Header.h"

class NodeInfo;
class BlockProposal;
class Schain;
class CommittedBlock;

class CommittedBlockHeader : public Header {


    node_id srcNodeID;
    schain_id schainID;
    schain_index proposerIndex;
    block_id blockID;
    ptr<sha3_hash> blockHash;
    list<uint32_t> transactionSizes;
    uint64_t timeStamp = 0;

public:
    const node_id &getSrcNodeID() const;

    const schain_id &getSchainID() const;

    const schain_index &getProposerIndex() const;

    const block_id &getBlockID() const;

    CommittedBlockHeader();

    CommittedBlockHeader(CommittedBlock& _block);

    const ptr<sha3_hash> &getBlockHash() const {
        return blockHash;
    }

    virtual void addFields(nlohmann::basic_json<> &j) override;

    ptr<vector<uint8_t >> serialize();

};



