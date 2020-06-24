/* Copyright 2013-2020 Homegear GmbH
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

#ifndef DATABASECONTROLLER_H_
#define DATABASECONTROLLER_H_

#include <homegear-base/BaseLib.h>
#include "SQLite3.h"

#include <thread>
#include <condition_variable>
#include <atomic>

namespace Homegear
{

class DatabaseController : public BaseLib::Database::IDatabaseController, public BaseLib::IQueue
{
public:
	class QueueEntry : public BaseLib::IQueueEntry
	{
	public:
		QueueEntry(std::string command, BaseLib::Database::DataRow& data) { _entry = std::make_shared<std::pair<std::string, BaseLib::Database::DataRow>>(command, data); };

		virtual ~QueueEntry() {};

		std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>>& getEntry() { return _entry; }

	private:
		std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>> _entry;
	};

	DatabaseController();

	~DatabaseController() override;

	void dispose() override;

	void init() override;

	// {{{ General
	void open(std::string databasePath, std::string databaseFilename, bool databaseSynchronous, bool databaseMemoryJournal, bool databaseWALJournal, std::string backupPath, std::string backupFilename) override;

	void hotBackup() override;

	bool isOpen() override { return _db.isOpen(); }

	void initializeDatabase() override;

	bool convertDatabase() override;

	void createSavepointSynchronous(std::string& name) override;

	void releaseSavepointSynchronous(std::string& name) override;

	void createSavepointAsynchronous(std::string& name) override;

	void releaseSavepointAsynchronous(std::string& name) override;
	// }}}

	// {{{ Homegear variables
	bool getHomegearVariableString(HomegearVariables::Enum id, std::string& value) override;

	void setHomegearVariableString(HomegearVariables::Enum id, std::string& value) override;
	// }}}

	// {{{ Data
	BaseLib::PVariable setData(std::string& component, std::string& key, BaseLib::PVariable& value) override;

	BaseLib::PVariable getData(std::string& component, std::string& key) override;

	BaseLib::PVariable deleteData(std::string& component, std::string& key) override;
	// }}}

	// {{{ UI
	uint64_t addUiElement(const std::string& elementStringId, const BaseLib::PVariable& data, const BaseLib::PVariable& metadata) override;

	std::shared_ptr<BaseLib::Database::DataTable> getUiElements() override;

    BaseLib::PVariable getUiElementMetadata(uint64_t databaseId) override;

	void removeUiElement(uint64_t databaseId) override;

    BaseLib::PVariable setUiElementMetadata(uint64_t databaseId, const BaseLib::PVariable& metadata) override;
	// }}}

    // {{{ Buildings
    BaseLib::PVariable addStoryToBuilding(uint64_t buildingId, uint64_t storyId) override;

    BaseLib::PVariable createBuilding(BaseLib::PVariable translations, BaseLib::PVariable metadata) override;

    BaseLib::PVariable deleteBuilding(uint64_t storyId) override;

    BaseLib::PVariable getStoriesInBuilding(BaseLib::PRpcClientInfo clientInfo, uint64_t buildingId, bool checkAcls) override;

    BaseLib::PVariable getBuildingMetadata(uint64_t buildingId) override;

    BaseLib::PVariable getBuildings(std::string languageCode) override;

    BaseLib::PVariable removeStoryFromBuildings(uint64_t storyId) override;

    BaseLib::PVariable removeStoryFromBuilding(uint64_t buildingId, uint64_t storyId) override;

    bool buildingExists(uint64_t buildingId) override;

    BaseLib::PVariable setBuildingMetadata(uint64_t buildingId, BaseLib::PVariable metadata) override;

    BaseLib::PVariable updateBuilding(uint64_t buildingId, BaseLib::PVariable translations, BaseLib::PVariable metadata) override;
    // }}}

	// {{{ Stories
	BaseLib::PVariable addRoomToStory(uint64_t storyId, uint64_t roomId) override;

	BaseLib::PVariable createStory(BaseLib::PVariable translations, BaseLib::PVariable metadata) override;

	BaseLib::PVariable deleteStory(uint64_t storyId) override;

	BaseLib::PVariable getRoomsInStory(BaseLib::PRpcClientInfo clientInfo, uint64_t storyId, bool checkAcls) override;

	BaseLib::PVariable getStoryMetadata(uint64_t storyId) override;

	BaseLib::PVariable getStories(std::string languageCode) override;

	BaseLib::PVariable removeRoomFromStories(uint64_t roomId) override;

	BaseLib::PVariable removeRoomFromStory(uint64_t storyId, uint64_t roomId) override;

	bool storyExists(uint64_t storyId) override;

	BaseLib::PVariable setStoryMetadata(uint64_t storyId, BaseLib::PVariable metadata) override;

	BaseLib::PVariable updateStory(uint64_t storyId, BaseLib::PVariable translations, BaseLib::PVariable metadata) override;
	// }}}

	// {{{ Rooms
	BaseLib::PVariable createRoom(BaseLib::PVariable translations, BaseLib::PVariable metadata) override;

	BaseLib::PVariable deleteRoom(uint64_t roomId) override;

    std::string getRoomName(BaseLib::PRpcClientInfo clientInfo, uint64_t roomId);

	BaseLib::PVariable getRoomMetadata(uint64_t roomId) override;

	BaseLib::PVariable getRooms(BaseLib::PRpcClientInfo clientInfo, std::string languageCode, bool checkAcls) override;

	bool roomExists(uint64_t roomId) override;

	BaseLib::PVariable setRoomMetadata(uint64_t roomId, BaseLib::PVariable metadata) override;

	BaseLib::PVariable updateRoom(uint64_t roomId, BaseLib::PVariable translations, BaseLib::PVariable metadata) override;
	// }}}

	// {{{ Categories
	BaseLib::PVariable createCategory(BaseLib::PVariable translations, BaseLib::PVariable metadata) override;

	BaseLib::PVariable deleteCategory(uint64_t categoryId) override;

	BaseLib::PVariable getCategories(BaseLib::PRpcClientInfo clientInfo, std::string languageCode, bool checkAcls) override;

	BaseLib::PVariable getCategoryMetadata(uint64_t categoryId) override;

	bool categoryExists(uint64_t categoryId) override;

	BaseLib::PVariable setCategoryMetadata(uint64_t categoryId, BaseLib::PVariable metadata) override;

	BaseLib::PVariable updateCategory(uint64_t categoryId, BaseLib::PVariable translations, BaseLib::PVariable metadata) override;
	// }}}

    // {{{ Roles
    void createDefaultRoles();

	void createRoleInternal(uint64_t roleId, const BaseLib::PVariable& translations, const BaseLib::PVariable& metadata);

    BaseLib::PVariable createRole(BaseLib::PVariable translations, BaseLib::PVariable metadata) override;

    BaseLib::PVariable deleteRole(uint64_t roleId) override;

    void deleteAllRoles() override;

    BaseLib::PVariable getRoles(BaseLib::PRpcClientInfo clientInfo, std::string languageCode, bool checkAcls) override;

    BaseLib::PVariable getRoleMetadata(uint64_t roleId) override;

    bool roleExists(uint64_t roleId) override;

    BaseLib::PVariable setRoleMetadata(uint64_t roleId, BaseLib::PVariable metadata) override;

    BaseLib::PVariable updateRole(uint64_t roleId, BaseLib::PVariable translations, BaseLib::PVariable metadata) override;
    // }}}

	// {{{ Node data
	BaseLib::PVariable setNodeData(std::string& node, std::string& key, BaseLib::PVariable& value) override;

	BaseLib::PVariable getNodeData(std::string& node, std::string& key, bool requestFromTrustedServer = false) override;

	std::set<std::string> getAllNodeDataNodes() override;

	BaseLib::PVariable deleteNodeData(std::string& node, std::string& key) override;
	// }}}

	// {{{ Metadata
	BaseLib::PVariable setMetadata(BaseLib::PRpcClientInfo clientInfo, uint64_t peerId, std::string& serialNumber, std::string& dataId, BaseLib::PVariable& metadata) override;

	BaseLib::PVariable getMetadata(uint64_t peerId, std::string& dataId) override;

	BaseLib::PVariable getAllMetadata(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<BaseLib::Systems::Peer> peer, bool checkAcls) override;

	BaseLib::PVariable deleteMetadata(uint64_t peerId, std::string& serialNumber, std::string& dataId) override;
	// }}}

	// {{{ System variables
	virtual void deleteSystemVariable(std::string& variableId);

    virtual std::shared_ptr<BaseLib::Database::DataTable> getAllSystemVariables();

	virtual std::shared_ptr<BaseLib::Database::DataTable> getSystemVariable(const std::string& variableId);

	virtual std::shared_ptr<BaseLib::Database::DataTable> getSystemVariablesInRoom(uint64_t roomId);

	virtual void removeCategoryFromSystemVariables(uint64_t categoryId);

    virtual void removeRoleFromSystemVariables(uint64_t categoryId);

	virtual void removeRoomFromSystemVariables(uint64_t roomId);

	virtual BaseLib::PVariable setSystemVariable(std::string& variableId, BaseLib::PVariable& value, uint64_t roomId, const std::string& categories, const std::string& roles, int32_t flags);

	virtual BaseLib::PVariable setSystemVariableCategories(std::string& variableId, const std::string& categories);

    virtual BaseLib::PVariable setSystemVariableRoles(std::string& variableId, const std::string& roles);

	virtual BaseLib::PVariable setSystemVariableRoom(std::string& variableId, uint64_t room);
	// }}}

	// {{{ Users
	virtual bool createUser(const std::string& name, const std::vector<uint8_t>& passwordHash, const std::vector<uint8_t>& salt, const std::vector<uint64_t>& groups);

	virtual bool deleteUser(uint64_t userId);

	virtual std::shared_ptr<BaseLib::Database::DataTable> getPassword(const std::string& name);

	virtual uint64_t getUserId(const std::string& name);

	virtual int64_t getUserKeyIndex1(uint64_t userId);

	virtual int64_t getUserKeyIndex2(uint64_t userId);

	virtual BaseLib::PVariable getUserMetadata(uint64_t userId);

	virtual std::shared_ptr<BaseLib::Database::DataTable> getUsers();

	virtual std::vector<uint64_t> getUsersGroups(uint64_t userId);

	virtual bool updateUser(uint64_t userId, const std::vector<uint8_t>& passwordHash, const std::vector<uint8_t>& salt, const std::vector<uint64_t>& groups);

	virtual void setUserKeyIndex1(uint64_t userId, int64_t keyIndex);

	virtual void setUserKeyIndex2(uint64_t userId, int64_t keyIndex);

	virtual BaseLib::PVariable setUserMetadata(uint64_t userId, BaseLib::PVariable metadata);

	virtual bool userNameExists(const std::string& name);
	// }}}

	//{{{ User data
    BaseLib::PVariable setUserData(uint64_t userId, const std::string& component, const std::string& key, const BaseLib::PVariable& value) override;
    BaseLib::PVariable getUserData(uint64_t userId, const std::string& component, const std::string& key) override;
    BaseLib::PVariable deleteUserData(uint64_t userId, const std::string& component, const std::string& key) override;
	//}}}

	// {{{ Groups
	virtual BaseLib::PVariable createGroup(BaseLib::PVariable translations, BaseLib::PVariable acl);

	virtual BaseLib::PVariable deleteGroup(uint64_t groupId);

	virtual BaseLib::PVariable getAcl(uint64_t groupId);

	virtual BaseLib::PVariable getGroup(uint64_t groupId, std::string languageCode);

	virtual BaseLib::PVariable getGroups(std::string languageCode);

	virtual bool groupExists(uint64_t groupId);

	virtual BaseLib::PVariable updateGroup(uint64_t groupId, BaseLib::PVariable translations, BaseLib::PVariable acl);
	// }}}

	// {{{ Events
	virtual std::shared_ptr<BaseLib::Database::DataTable> getEvents();

	virtual void saveEventAsynchronous(BaseLib::Database::DataRow& event);

	virtual void deleteEvent(std::string& name);
	// }}}

	// {{{ Family
	virtual void deleteFamily(int32_t familyId);

	virtual void saveFamilyVariableAsynchronous(int32_t familyId, BaseLib::Database::DataRow& data);

	virtual std::shared_ptr<BaseLib::Database::DataTable> getFamilyVariables(int32_t familyId);

	virtual void deleteFamilyVariable(BaseLib::Database::DataRow& data);
	// }}}

	// {{{ Device
	virtual std::shared_ptr<BaseLib::Database::DataTable> getDevices(uint32_t family);

	virtual void deleteDevice(uint64_t id);

	virtual uint64_t saveDevice(uint64_t id, int32_t address, std::string& serialNumber, uint32_t type, uint32_t family);

	virtual void saveDeviceVariableAsynchronous(BaseLib::Database::DataRow& data);

	virtual void deletePeers(int32_t deviceID);

	virtual std::shared_ptr<BaseLib::Database::DataTable> getPeers(uint64_t deviceID);

	virtual std::shared_ptr<BaseLib::Database::DataTable> getDeviceVariables(uint64_t deviceID);
	// }}}

	// {{{ Peer
	void deletePeer(uint64_t id) override;

	uint64_t savePeer(uint64_t id, uint32_t parentID, int32_t address, std::string& serialNumber, uint32_t type) override;

    uint64_t savePeerParameterSynchronous(BaseLib::Database::DataRow& data) override;

	void savePeerParameterAsynchronous(BaseLib::Database::DataRow& data) override;

	void saveSpecialPeerParameterAsynchronous(BaseLib::Database::DataRow& data) override;

	void savePeerParameterRoomAsynchronous(BaseLib::Database::DataRow& data) override;

	void savePeerParameterCategoriesAsynchronous(BaseLib::Database::DataRow& data) override;

	void savePeerParameterRolesAsynchronous(BaseLib::Database::DataRow& data) override;

	void savePeerVariableAsynchronous(BaseLib::Database::DataRow& data) override;

	std::shared_ptr<BaseLib::Database::DataTable> getPeerParameters(uint64_t peerID) override;

	std::shared_ptr<BaseLib::Database::DataTable> getPeerVariables(uint64_t peerID) override;

	void deletePeerParameter(uint64_t peerID, BaseLib::Database::DataRow& data) override;

	bool peerExists(uint64_t peerId) override;

	/**
     * {@inheritDoc}
     */
	bool setPeerID(uint64_t oldPeerID, uint64_t newPeerID) override;
	// }}}

	// {{{ Service messages
	std::shared_ptr<BaseLib::Database::DataTable> getServiceMessages(uint64_t peerId) override;

	void saveServiceMessageAsynchronous(uint64_t peerId, BaseLib::Database::DataRow& data) override;

	void saveGlobalServiceMessageAsynchronous(BaseLib::Database::DataRow& data) override;

	void deleteServiceMessage(uint64_t databaseId) override;

	void deleteGlobalServiceMessage(int32_t familyId, int32_t messageId, std::string& messageSubId, std::string& message) override;
	// }}}

	// {{{ License modules
	std::shared_ptr<BaseLib::Database::DataTable> getLicenseVariables(int32_t moduleId) override;

	void saveLicenseVariable(int32_t moduleId, BaseLib::Database::DataRow& data) override;

	void deleteLicenseVariable(int32_t moduleId, uint64_t mapKey) override;
	// }}}

    // {{{ Variable profiles
    uint64_t addVariableProfile(const BaseLib::PVariable& translations, const BaseLib::PVariable& profile) override;

    void deleteVariableProfile(uint64_t profileId) override;

    std::shared_ptr<BaseLib::Database::DataTable> getVariableProfiles() override;

    bool updateVariableProfile(uint64_t profileId, const BaseLib::PVariable& translations, const BaseLib::PVariable& profile) override;
    // }}}
protected:
	std::atomic_bool _disposing;

	Homegear::SQLite3 _db;

	std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;
	std::unique_ptr<BaseLib::Rpc::RpcEncoder> _rpcEncoder;

	std::mutex _dataMutex;
	std::unordered_map<std::string, std::map<std::string, BaseLib::PVariable>> _data;

	std::mutex _nodeDataMutex;
	std::unordered_map<std::string, std::map<std::string, BaseLib::PVariable>> _nodeData;

	std::mutex _metadataMutex;
	std::unordered_map<uint64_t, std::map<std::string, BaseLib::PVariable>> _metadata;

	virtual void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry);
};

}

#endif
