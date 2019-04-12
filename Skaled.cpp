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

    @file Skaled.cpp
    @author Stan Kladko
    @date 2018
*/

#include "SkaleConfig.h"

#include "node/ConsensusEngine.h"

#ifdef GOOGLE_PROFILE
#include <gperftools/heap-profiler.h>
#endif


int main(int argc, char **argv) {

#ifdef GOOGLE_PROFILE
    HeapProfilerStart("/tmp/consensusd.profile");
#endif

    signal(SIGPIPE, SIG_IGN);



    if (argc < 2) {
        printf("Usage: skaled nodes_dir node_id1 node_id2 \n");
        exit(1);
    }

    for (int i = 2; i < argc; i++) {

        uint64_t ui64;
        ui64 = static_cast<uint64_t >(stoll(argv[i]));

        Node::nodeIDs.insert(node_id(ui64));

        cerr << node_id(ui64) << endl;
    }

    fs_path dirPath(boost::filesystem::system_complete(fs_path(argv[1])));

    ConsensusEngine engine;

    engine.parseConfigsAndCreateAllNodes(dirPath);


    engine.slowStartBootStrapTest();

    sleep(100000);

    engine.exitGracefully();

    cerr << "Exited" << endl;


#ifdef GOOGLE_PROFILE
    HeapProfilerStop();
#endif




}
