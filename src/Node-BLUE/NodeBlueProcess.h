/* Copyright 2013-2019 Homegear GmbH
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#ifndef NODEBLUEPROCESS_H_
#define NODEBLUEPROCESS_H_

#include "NodeBlueClientData.h"
#include <homegear-base/BaseLib.h>
#include "FlowInfoServer.h"

namespace Homegear
{

namespace NodeBlue
{

struct FlowFinishedInfo
{
	bool finished = false;
};
typedef std::shared_ptr<FlowFinishedInfo> PFlowFinishedInfo;

class NodeBlueProcess
{
private:
	pid_t _pid = 0;
	std::mutex _flowsMutex;
	std::map<int32_t, PFlowInfoServer> _flows;
	std::map<int32_t, PFlowFinishedInfo> _flowFinishedInfo;
	PNodeBlueClientData _clientData;
	std::atomic_bool _exited{false};
	std::atomic_uint _nodeThreadCount;
public:
	NodeBlueProcess();

	virtual ~NodeBlueProcess();

	std::atomic<int64_t> lastExecution;
	std::condition_variable requestConditionVariable;

	pid_t getPid() { return _pid; }

	void setPid(pid_t value) { _pid = value; }

	PNodeBlueClientData& getClientData() { return _clientData; }

	bool getExited() { return _exited; }

	void setExited(bool value) { _exited = value; }

	void setClientData(PNodeBlueClientData& value) { _clientData = value; }

	void invokeFlowFinished(int32_t exitCode);

	void invokeFlowFinished(int32_t id, int32_t exitCode);

	uint32_t flowCount();

	void reset();

	uint32_t nodeThreadCount();

	PFlowInfoServer getFlow(int32_t id);

	PFlowFinishedInfo getFlowFinishedInfo(int32_t id);

	void registerFlow(int32_t id, PFlowInfoServer& flowInfo);

	void unregisterFlow(int32_t id);
};

typedef std::shared_ptr<NodeBlueProcess> PNodeBlueProcess;

}

}

#endif
