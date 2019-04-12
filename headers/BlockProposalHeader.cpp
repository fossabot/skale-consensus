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

    @file BlockProposalHeader.cpp
    @author Stan Kladko
    @date 2018
*/

#include "../SkaleConfig.h"
#include "../Log.h"
#include "../exceptions/FatalError.h"
#include "../thirdparty/json.hpp"
#include "../abstracttcpserver/ConnectionStatus.h"
#include "BlockProposalHeader.h"
#include "../datastructures/BlockProposal.h"
#include "../node/Node.h"
#include "../node/NodeInfo.h"
#include "../chains/Schain.h"



using namespace std;

BlockProposalHeader::BlockProposalHeader() : Header() {

}

BlockProposalHeader::BlockProposalHeader(Schain &_sChain, schain_index _dstIndex, ptr<BlockProposal> proposal,
                                                       string _type) :
        Header() {


    this->srcNodeID = _sChain.getNode()->getNodeID();
    this->srcSchainIndex = _sChain.getSchainIndex();
    this->schainID = _sChain.getSchainID();
    this->blockID = proposal->getBlockID();

    if( ! (_sChain.getNode()->getNodeInfosByIndex().count(_dstIndex) > 0) ) {
        ASSERT( 0 );
    }

    this->proposedBlockHash = proposal->getHash();

    this->partialHashesCount = (uint64_t) proposal->getTransactionsCount();
    this->type = _type;
    this->timeStamp = proposal->getTimeStamp();


    ASSERT(timeStamp > MODERN_TIME);

    complete = true;

}

void BlockProposalHeader::addFields(nlohmann::basic_json<> &j) {

    auto hash = getProposedBlockHash();

    auto encoding = Header::sha2hex(hash);

    j["type"] = type;

    j["schainID"] = (uint64_t ) schainID;

    j["srcNodeID"] = (uint64_t ) srcNodeID;

    j["srcSchainIndex"] = (uint64_t ) srcSchainIndex;

    j["blockID"] = (uint64_t ) blockID;

    j["hash"] = *encoding;

    j["partialHashesCount"] = partialHashesCount;

    ASSERT(timeStamp > MODERN_TIME);

    j["timeStamp"] = timeStamp;

}


