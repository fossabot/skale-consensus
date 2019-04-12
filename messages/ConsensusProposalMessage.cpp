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

    @file ConsensusProposalMessage.cpp
    @author Stan Kladko
    @date 2018
*/

#include "../chains/Schain.h"

#include "../protocols/ProtocolKey.h"
#include "ConsensusProposalMessage.h"

ConsensusProposalMessage::ConsensusProposalMessage(Schain& _schain,  const block_id &_blockID, ptr<vector<bool>> _proposals) : Message(
        _schain.getSchainID(), MSG_CONSENSUS_PROPOSAL,
        msg_id(0), node_id(0), node_id(0), _blockID,
                schain_index(0)) {
    this->proposals = _proposals;

}


const ptr<vector<bool>> &ConsensusProposalMessage::getProposals() const {
    return proposals;
}
