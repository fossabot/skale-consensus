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

    @file AbstractBlockRequestHeader.h
    @author Stan Kladko
    @date 2019
*/

#ifndef CONSENSUS_ABSTRACTBLOCKREQUESTHEADER_H
#define CONSENSUS_ABSTRACTBLOCKREQUESTHEADER_H



#include "Header.h"

class NodeInfo;
class BlockProposal;
class Schain;


#define PROPOSAL_REQUEST_TYPE "PROPOSAL"
#define FINALIZE_REQUEST_TYPE "FINALIZE"




class AbstractBlockRequestHeader : public Header {

protected:

    schain_id schainID;
    schain_index proposerIndex;
    block_id blockID;
    const char* type;

    void addFields(nlohmann::basic_json<> &jsonRequest) override;

    AbstractBlockRequestHeader(Schain &_sChain, ptr<BlockProposal> proposal,
                               const char *_type, schain_index _proposerIndex);

    virtual ~AbstractBlockRequestHeader(){};

public:


};



#endif //CONSENSUS_ABSTRACTBLOCKREQUESTHEADER_H
