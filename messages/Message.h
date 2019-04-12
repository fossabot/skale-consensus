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

    @file Message.h
    @author Stan Kladko
    @date 2018
*/

#pragma once



enum MsgType {CHILD_COMPLETED, PARENT_COMPLETED,

    BVB_BROADCAST, AUX_BROADCAST, BIN_CONSENSUS_COMMIT, BIN_CONSENSUS_HISTORY_DECIDE,
    BIN_CONSENSUS_HISTORY_CC, BIN_CONSENSUS_HISTORY_BVSELF, BIN_CONSENSUS_HISTORY_AUXSELF, BIN_CONSENSUS_HISTORY_NEW_ROUND,
    MSG_BLOCK_CONSENSUS_INIT, MSG_CONSENSUS_PROPOSAL };


class ProtocolInstance;
class ProtocolKey;

class Message {


protected:

    schain_id schainID;
    block_id  blockID;
    schain_index blockProposerIndex;
    MsgType msgType;
    msg_id msgID;
    node_id srcNodeID;
    node_id dstNodeID;

    ptr<ProtocolKey> protocolKey;

public:
    Message(const schain_id &schainID, MsgType msgType, const msg_id &msgID, const node_id &srcNodeID,
            const node_id &dstNodeID, const block_id &blockID = block_id(0),
            const schain_index &blockProposerIndex = schain_index(0));

    node_id getSrcNodeID() const;

    node_id getDstNodeID() const;

    msg_id getMessageID() const;

    MsgType getMessageType() const;

    const block_id getBlockId() const;

    const schain_index &getBlockProposerIndex() const ;

    schain_id getSchainID() const;


    ptr<ProtocolKey> createDestinationProtocolKey();

    void setSrcNodeID(const node_id &srcNodeID);

    void setDstNodeID(const node_id &dstNodeID);

    const block_id &getBlockID() const;

    MsgType getMsgType() const;

    const msg_id &getMsgID() const;

    virtual ~Message();


    static uint64_t getTotalObjects() {
        return totalObjects;
    }

private:


    static atomic<uint64_t>  totalObjects;

};
