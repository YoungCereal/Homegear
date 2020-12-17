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

#ifndef NODEMANAGER_H_
#define NODEMANAGER_H_

#include <homegear-base/BaseLib.h>
#include <homegear-node/INode.h>
#include <homegear-node/NodeFactory.h>

#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>

#include <dlfcn.h>

namespace Homegear {

class NodeLoader {
 public:
  NodeLoader(const std::string &soPath);

  virtual ~NodeLoader();

  Flows::PINode createNode(const std::atomic_bool *nodeEventsEnabled, const std::string &type, const std::string &nodePath);

 private:
  std::string _soPath;
  void *_handle = nullptr;
  std::unique_ptr<Flows::NodeFactory> _factory;

  NodeLoader(const NodeLoader &) = delete;

  NodeLoader &operator=(const NodeLoader &) = delete;
};

class NodeManager {
 public:
  typedef std::string NodeId; //Node ID from Homegear
  typedef std::string NodeType; //Node type from Homegear
  typedef std::string ModuleName;

  enum class NodeCodeType {
    undefined,
    none, //No code file
    cpp, //.so
    statefulPhp, //.s.php
    statefulPhpEncrypted, //.s.hgn
    simplePhp, //.php
    simplePhpEncrypted, //.hgn
    python, //.py
    pythonEncrypted, //.hgnpy
    javascript, //.js
    javascriptEncrypted //.hgnjs
  };

  struct NodeInfo {
    std::atomic_bool locked{false};
    std::atomic_int referenceCounter{0};
    NodeCodeType codeType = NodeCodeType::undefined;
    uint32_t maxThreadCount = 0;
    std::string filename;
    std::string fullCodefilePath;
  };
  typedef std::shared_ptr<NodeInfo> PNodeInfo;

  struct ManagerModuleInfo {
    std::string name;
    std::string module;
    std::string version;
    std::unordered_map<NodeType, PNodeInfo> nodes;
  };
  typedef std::shared_ptr<ManagerModuleInfo> PManagerNodeInfo;

  explicit NodeManager(const std::atomic_bool *nodeEventsEnabled);

  virtual ~NodeManager();

  /**
   * Returns a map with the maximum thread count of all node types.
   */
  std::unordered_map<NodeType, uint32_t> getMaxThreadCounts();

  /**
   * Returns a struct with information about all modules as required by the frontend.
   */
  BaseLib::PVariable getModuleInfo();

  /**
   * Returns the icons of all modules.
   */
  BaseLib::PVariable getIcons();

  /**
   * Returns the content of all *.hni files.
   */
  std::string getFrontendCode();

  std::string getNodeLocales(std::string &language);

  /**
   * Loads a node. The node needs to be in Homegear's node path.
   * @param type The type (= name) of the node (e. g. variable-in).
   * @param id The id of the node (e. g. 142947a.387ef34ad)
   * @param[out] node If loading was successful, this variable contains the loaded node.
   * @return Returns positive values or 0 on success and negative values on error. 0: Node successfully loaded, 1: Node already loaded, -1: System error, -2: Node does not exists, -4: Node initialization failed
   */
  int32_t loadNode(std::string type, const std::string &id, Flows::PINode &node);

  /**
   * Unloads a previously loaded node.
   * @param id The id of the node (e. g. 142947a.387ef34ad).
   * @return Returns positive values or 0 on success and negative values on error. 0: Node successfully loaded, 1: Node not loaded, -1: System error, -2: Node does not exists
   */
  int32_t unloadNode(const std::string &id);

  /*
   * Returns the node specified by id.
   */
  Flows::PINode getNode(const std::string &id);

  /*
   * Clears all node information. This causes the information to be loaded again from file system on first call to loadNode()
   */
  void clearManagerModuleInfo();
 private:
  std::mutex _nodeLoadersMutex;
  std::unique_ptr<NodeLoader> _pythonNodeLoader;
  std::map<std::string, std::unique_ptr<NodeLoader>> _nodeLoaders;

  std::mutex _nodesMutex;
  std::unordered_map<NodeId, Flows::PINode> _nodes;
  std::unordered_map<ModuleName, PManagerNodeInfo> _managerModuleInfo;
  std::unordered_map<NodeType, PManagerNodeInfo> _managerModuleInfoByNodeType;
  /**
   * This map is just a shortcut for faster access as node info is accessed a lot.
   */
  std::unordered_map<NodeType, PNodeInfo> _nodeInfoByNodeType;

  const std::atomic_bool *_nodeEventsEnabled;

  NodeManager(const NodeManager &) = delete;

  NodeManager &operator=(const NodeManager &) = delete;

  void fillManagerModuleInfo();
};

}

#endif
