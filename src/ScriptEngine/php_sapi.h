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

#ifndef HOMEGEAR_PHP_SAPI_H_
#define HOMEGEAR_PHP_SAPI_H_

#ifndef NO_SCRIPTENGINE

#include <homegear-base/BaseLib.h>
#include "PhpEvents.h"
#include "CacheInfo.h"
#include "php_homegear_globals.h"
#include "php_node.h"
#include "php_device.h"
#include "php_config_fixes.h"

#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include <map>

#include <php.h>
#include <SAPI.h>
#include <php_main.h>
#include <php_variables.h>
#include <php_ini.h>
#include <zend_API.h>
#include <zend_ini.h>
#include <zend_exceptions.h>
#include <ext/standard/info.h>

#if PHP_VERSION_ID >= 70400
struct hg_stream_handle
{
    std::string buffer;
    size_t position = 0;
};
#endif

void php_homegear_build_argv(std::vector<std::string>& arguments);
int php_homegear_init();
void php_homegear_deinit();

#if PHP_VERSION_ID >= 70400
ssize_t hg_zend_stream_reader(void* handle, char* buf, size_t len);
size_t hg_zend_stream_fsizer(void* handle);
void hg_zend_stream_closer(void* handle);
#endif

#endif
#endif
