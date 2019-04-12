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

    @file Header.h
    @author Stan Kladko
    @date 2018
*/

#pragma once


#define BLOCK_PROPOSAl_PUSH "PUSH"
#define BLOCK_PROPOSAL_GET "GET"


class Buffer;
class Connection;


class Header {
protected:

    ConnectionStatus status;
    ConnectionSubStatus substatus;


    bool complete = false;

public:
    bool isComplete() const;

protected:
    ptr< sha3_hash > blockProposalHash;

public:
    Header();

    virtual ~Header();

    void setComplete() { complete = true; }


    static void nullCheck( nlohmann::json& js, const char* name );


    ptr< Buffer > toBuffer();


    virtual void addFields(nlohmann::basic_json<> & /*j*/ ) { };

    static uint64_t getUint64( nlohmann::json& _js, const char* _name );

    static ptr< string > getString( nlohmann::json& _js, const char* _name );


    static int char2int( char _input ) {
        if ( _input >= '0' && _input <= '9' )
            return _input - '0';
        if ( _input >= 'A' && _input <= 'F' )
            return _input - 'A' + 10;
        if ( _input >= 'a' && _input <= 'f' )
            return _input - 'a' + 10;
        throw std::invalid_argument( "Invalid input string" );
    }


    static ptr< sha3_hash > hex2sha( ptr< string > _hex ) {

        uint8_t result[SHA3_HASHLEN];  //

        for ( size_t i = 0; i < SHA3_HASHLEN; i++ ) {
            result[i] = char2int( ( *_hex )[i] ) * 16 + char2int( ( *_hex )[i + 1] );
        }

        return make_shared< sha3_hash >( result );
    }


    static ptr< string > sha2hex( ptr< const sha3_hash > _hash ) {
        char hex[2 * SHA3_HASHLEN];

        char hexval[16] = {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
        for ( size_t j = 0; j < SHA3_HASHLEN; j++ ) {
            hex[j * 2] = hexval[( ( ( *_hash )[j] >> 4 ) & 0xF )];
            hex[( j * 2 ) + 1] = hexval[( ( *_hash )[j] ) & 0x0F];
        }

        return make_shared< string >( hex, 2 * SHA3_HASHLEN );
    }


    ConnectionStatus getStatus() { return status; }

    ConnectionSubStatus getSubstatus() { return substatus; }

    void setStatusSubStatus( ConnectionStatus _status, ConnectionSubStatus _substatus ) {
        this->status = _status;
        this->substatus = _substatus;
    }


    void setStatus( ConnectionStatus _status ) { this->status = _status; }

    static uint64_t getTotalObjects() {
        return totalObjects;
    }

private:


    static atomic<uint64_t>  totalObjects;


};



