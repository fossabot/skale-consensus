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

    @file Header.cpp
    @author Stan Kladko
    @date 2018
*/

#include "../SkaleConfig.h"
#include "../Log.h"
#include "../exceptions/FatalError.h"
#include "../abstracttcpserver/ConnectionStatus.h"

#include "../thirdparty/json.hpp"

#include "../abstracttcpserver/AbstractServerAgent.h"
#include "../exceptions/ParsingException.h"
#include "../exceptions/NetworkProtocolException.h"

#include "../network/Buffer.h"
#include "../network/IO.h"

#include "Header.h"



bool Header::isComplete() const {
    return complete;
}

ptr<Buffer> Header::toBuffer() {
    ASSERT(complete);
    nlohmann::json j;

    j["status"] = status;

    j["substatus"] = substatus;

    addFields(j);

    string s = j.dump();

    uint64_t len = s.size();

    ASSERT(len > 16);

    auto buf = make_shared<Buffer>(len + sizeof(uint64_t));
    buf->write(&len, sizeof(len));
    buf->write((void *) s.data(), s.size());

    ASSERT(buf->getCounter() > 16);

    return buf;
}



void Header::nullCheck(nlohmann::json &js, const char *name) {
    if (js[name].is_null()) {
        BOOST_THROW_EXCEPTION(NetworkProtocolException("Null " + string(name) + " in the request", __CLASS_NAME__));
    }
};

uint64_t Header::getUint64(nlohmann::json &_js, const char *_name) {
    nullCheck(_js, _name);
    uint64_t result = _js[_name];
    return result;
};

ptr<string> Header::getString(nlohmann::json &_js, const char *_name) {
    nullCheck(_js, _name);
    string result = _js[_name];
    return make_shared<string>(result);
}

Header::Header() {

    totalObjects++;
    status = CONNECTION_SERVER_ERROR;
    substatus = CONNECTION_ERROR_UNKNOWN_SERVER_ERROR;


}



Header::~Header() {
    totalObjects--;
}


atomic<uint64_t>  Header::totalObjects(0);
