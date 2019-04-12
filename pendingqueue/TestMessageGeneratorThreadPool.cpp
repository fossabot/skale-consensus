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

    @file TestMessageGeneratorThreadPool.cpp
    @author Stan Kladko
    @date 2018
*/

#include "../SkaleConfig.h"
#include "../Log.h"
#include "../exceptions/FatalError.h"


#include "TestMessageGeneratorAgent.h"
#include "TestMessageGeneratorThreadPool.h"

TestMessageGeneratorThreadPool::TestMessageGeneratorThreadPool(num_threads numThreads, void *params_) : WorkerThreadPool(numThreads,
                                                                                                                       params_) {}

void TestMessageGeneratorThreadPool::createThread(uint64_t /*_threadNumber*/) {
    threadpool.push_back(make_shared<thread>(TestMessageGeneratorAgent::workerThreadMessagePushLoop, reinterpret_cast < TestMessageGeneratorAgent * > ( params )  ));
    LOG(debug, __CLASS_NAME__ + " worker bound");
}







