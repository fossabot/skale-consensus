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

    @file CatchupRequestHeader.h
    @author Stan Kladko
    @date 2018
*/

#pragma  once



#include "Header.h"

class SHAHash;
class NodeInfo;
class Schain;

class CatchupRequestHeader : public Header{



    node_id srcNodeID;
    node_id dstNodeID;
    schain_id schainID;
    schain_index srcSchainIndex;
    schain_index dstSchainIndex;
    block_id blockID;

public:


    CatchupRequestHeader();

    CatchupRequestHeader(Schain &_sChain, schain_index _dstIndex);


    void addFields(nlohmann::basic_json<> &j) override;

};