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

    @file Utils.cpp
    @author Stan Kladko
    @date 2018
*/

#include "../SkaleConfig.h"
#include "../Log.h"
#include "../exceptions/FatalError.h"
#include "Utils.h"

void Utils::checkTime() {
    auto ip = gethostbyname( "pool.ntp.org" );
    auto fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );


    if ( fd < 0 )
        throw FatalError( "Can not open NTP socket" );

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if ( setsockopt( fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof( tv ) ) < 0 ) {
        perror( "Error" );
    }


    if ( !ip )
        throw FatalError( "Could not get IP address" );


    struct sockaddr_in serverAddress = {};

    memcpy( ( char* ) &serverAddress.sin_addr.s_addr, ip->h_addr, ( size_t ) ip->h_length );

    serverAddress.sin_family = AF_INET;

    serverAddress.sin_port = htons( 123 );

    if ( connect( fd, ( struct sockaddr* ) &serverAddress, sizeof( serverAddress ) ) < 0 )
        throw FatalError( "Could not connect to NTP server" );


    struct {
        uint8_t vnm = 0x1b;
        uint8_t str = 0;
        uint8_t poll = 0;
        uint8_t prec = 0;
        uint32_t rootDelay = 0;
        uint32_t rootDispersion = 0;
        uint32_t refId = 0;
        uint32_t refTmS = 0;
        uint32_t refTmF = 0;
        uint32_t origTmS = 0;
        uint32_t origTmF = 0;
        uint32_t rxTmS = 0;
        uint32_t rxTmF = 0;
        uint32_t txTmS = 0;
        uint32_t txTmF = 0;

    } ntpMessage;


    if ( write( fd, ( char* ) &ntpMessage, sizeof( ntpMessage ) ) <= 0 )
        throw FatalError( "Could not write to NTP" );


    if ( read( fd, ( char* ) &ntpMessage, sizeof( ntpMessage ) ) != sizeof( ntpMessage ) ) {
        if ( errno != EAGAIN )
            throw FatalError( "Could not read from NTP" );
        else
            return;
    }

    if (ntpMessage.str < 1 || ntpMessage.str > 15) {
        return;
    }

    int64_t timeDiff = ntohl( ntpMessage.txTmS ) - TIME_START - time( NULL );

    if ( timeDiff > 1 || timeDiff < -1 )
        throw FatalError(
            "System time is not synchronized. Enable NTP: sudo timedatectl set-ntp on. Timediff:" +
            to_string( timeDiff ) + ":Local:" + to_string( time( NULL ) ) +
            ":ntp:" + to_string( ntohl( ntpMessage.txTmS ) - TIME_START ) );
}


bool Utils::isValidIpAddress( ptr< string > ipAddress ) {
    struct sockaddr_in sa;
    int result = inet_pton( AF_INET, ipAddress->c_str(), &( sa.sin_addr ) );
    return result != 0;
}
