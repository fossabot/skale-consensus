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

    @file ConsensusEngine.h
    @author Stan Kladko
    @date 2018
*/

#pragma once





#include "boost/filesystem.hpp"
#include "../Agent.h"
#include "../thirdparty/json.hpp"
#include "../threads/WorkerThreadPool.h"
#include "ConsensusInterface.h"
#include "Node.h"


/**
 * Through this interface Consensus interacts with the rest of the system
 */
class ConsensusExtFace {
public:
    typedef std::map< array< uint8_t, 32 >, std::vector< uint8_t > > transactions_map;
    typedef std::vector< std::vector< uint8_t > > transactions_vector;

    // Returns hashes and bytes of new transactions; blocks if there are no txns
    virtual transactions_map pendingTransactions( size_t _limit ) = 0;
    // Creates new block with specified transactions AND removes them from the queue
    virtual void createBlock(const transactions_vector &_approvedTransactions, uint64_t _timeStamp, uint64_t _blockID) = 0;
    virtual ~ConsensusExtFace() = default;

    virtual void terminateApplication() {};
};


class ConsensusEngine : public ConsensusInterface {

    map< node_id, Node* > nodes;


    static void checkExistsAndDirectory( const fs_path& dirname );

    static void checkExistsAndFile( const fs_path& filename );

    Node* readNodeConfigFileAndCreateNode( const fs_path& path, set< node_id >& nodeIDs );

    node_count nodesCount();


    void readSchainConfigFiles( Node& node, const fs_path& dirname );

    ConsensusExtFace* extFace = nullptr;

    block_id lastCommittedBlockID = 0;

    uint64_t lastCommittedBlockTimeStamp = 0;

public:
    ConsensusEngine();

    ~ConsensusEngine() override;

    ConsensusEngine(ConsensusExtFace &_extFace, uint64_t _lastCommittedBlockID,
                        uint64_t _lastCommittedBlockTimeStamp);

    ConsensusExtFace* getExtFace() const;


    void startAll() override;

    void parseFullConfigAndCreateNode( const string& fullPathToConfigFile ) override;

    // used for standalone debugging

    void parseConfigsAndCreateAllNodes(const fs_path &dirname);

    void exitGracefully() override;

    void bootStrapAll() override;

    // tests

    void slowStartBootStrapTest();

    void init() const;

    void joinAllThreads() const;
};
