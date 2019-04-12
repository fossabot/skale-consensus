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

    @file ProtocolInstance.cpp
    @author Stan Kladko
    @date 2018
*/

#include "../SkaleConfig.h"
#include "../Log.h"
#include "../exceptions/FatalError.h"
#include "../thirdparty/json.hpp"
#include "../abstracttcpserver/ConnectionStatus.h"
#include "../messages/ParentMessage.h"
#include "../messages/ChildCompletedMessage.h"
#include "../pendingqueue/PendingTransactionsAgent.h"
#include "../blockproposal/pusher/BlockProposalPusherAgent.h"
#include "../blockproposal/received/ReceivedBlockProposalsDatabase.h"
#include "../chains/Schain.h"
#include "../protocols/ProtocolKey.h"
#include "../protocols/binconsensus/BinConsensusInstance.h"


msg_id ProtocolInstance::createNetworkMessageID() {
    this->messageCounter+=1;
    return messageCounter;
}


ProtocolInstance::ProtocolInstance(
    ProtocolType _protocolType // unused
    , Schain& _sChain
    )
    : sChain(_sChain)
    , protocolType(_protocolType) // unused
    , messageCounter(0)
{
    totalObjects++;
}


Schain *ProtocolInstance::getSchain() const {
    return &sChain;
}


void ProtocolInstance::setStatus(ProtocolStatus _status) {
    ProtocolInstance::status = _status;
}

void ProtocolInstance::setOutcome(ProtocolOutcome outcome) {
    ProtocolInstance::outcome = outcome;
}




ProtocolOutcome ProtocolInstance::getOutcome() const {
    return outcome;
}

ProtocolInstance::~ProtocolInstance() {
    totalObjects--;
}


atomic<uint64_t>  ProtocolInstance::totalObjects(0);






