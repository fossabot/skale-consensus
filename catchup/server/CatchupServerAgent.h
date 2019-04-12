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

    @file CatchupServerAgent.h
    @author Stan Kladko
    @date 2018
*/

#pragma once





#include <mutex>
#include <queue>
#include <condition_variable>
#include "../../abstracttcpserver/ConnectionStatus.h"
#include "../../abstracttcpserver/AbstractServerAgent.h"

#include "../../headers/Header.h"
#include "../../network/Connection.h"
#include "../../datastructures/PartialHashesList.h"
#include "../../headers/Header.h"

#include "CatchupWorkerThreadPool.h"
#include "../../Agent.h"

class CommittedBlock;
class CommittedBlockList;
class CatchupResponseHeader;

class CatchupServerAgent : public AbstractServerAgent {

   ptr<CatchupWorkerThreadPool> catchupWorkerThreadPool;


public:
    CatchupServerAgent(Schain &_schain, ptr<TCPServerSocket> _s);
    ~CatchupServerAgent() override;

    CatchupWorkerThreadPool *getCatchupWorkerThreadPool() const;

    ptr<vector<uint8_t>> createCatchupResponseHeader(ptr<Connection> _connectionEnvelope,
                                nlohmann::json _jsonRequest, ptr<CatchupResponseHeader> _responseHeader);

    void processNextAvailableConnection(ptr<Connection> _connection) override;

    ptr<vector<uint8_t>> getSerializedBlock(uint64_t i) const;
};
