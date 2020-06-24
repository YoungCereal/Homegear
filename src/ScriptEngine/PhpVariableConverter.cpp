/* Copyright 2013-2020 Homegear GmbH
*
* Homegear is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Homegear is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
*
* In addition, as a special exception, the copyright holders give
* permission to link the code of portions of this program with the
* OpenSSL library under certain conditions as described in each
* individual source file, and distribute linked combinations
* including the two.
* You must obey the GNU General Public License in all respects
* for all of the code used other than OpenSSL.  If you modify
* file(s) with this exception, you may extend this exception to your
* version of the file(s), but you are not obligated to do so.  If you
* do not wish to do so, delete this exception statement from your
* version.  If you delete this exception statement from all source
* files in the program, then also delete it here.
*/

#ifndef NO_SCRIPTENGINE

#include "php_config_fixes.h"
#include "../GD/GD.h"
#include "PhpVariableConverter.h"

namespace Homegear
{

PhpVariableConverter::PhpVariableConverter()
{
}

PhpVariableConverter::~PhpVariableConverter()
{
}

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

BaseLib::PVariable PhpVariableConverter::getVariable(zval* value, bool arraysAreStructs, bool subArraysAreStructs)
{
    try
    {
        BaseLib::PVariable variable;
        if(!value) return variable;
        if(Z_TYPE_P(value) == IS_NULL)
        {
            variable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tVoid);
        }
        else if(Z_TYPE_P(value) == IS_LONG)
        {
            variable = std::make_shared<BaseLib::Variable>(Z_LVAL_P(value));
            variable->type = (SIZEOF_ZEND_LONG == 8) ? BaseLib::VariableType::tInteger64 : BaseLib::VariableType::tInteger;
        }
        else if(Z_TYPE_P(value) == IS_DOUBLE)
        {
            variable = std::make_shared<BaseLib::Variable>((double)Z_DVAL_P(value));
        }
        else if(Z_TYPE_P(value) == IS_TRUE)
        {
            variable = std::make_shared<BaseLib::Variable>(true);
        }
        else if(Z_TYPE_P(value) == IS_FALSE)
        {
            variable = std::make_shared<BaseLib::Variable>(false);
        }
        else if(Z_TYPE_P(value) == IS_STRING)
        {
            if(Z_STRLEN_P(value) > 0) variable = std::make_shared<BaseLib::Variable>(std::string(Z_STRVAL_P(value), Z_STRLEN_P(value)));
            else variable = std::make_shared<BaseLib::Variable>(std::string(""));
        }
        else if(Z_TYPE_P(value) == IS_ARRAY)
        {
            zval* element = nullptr;
            HashTable* ht = Z_ARRVAL_P(value);
            zend_string* key = nullptr;
            ulong keyIndex = 0;
            if(zend_hash_num_elements(ht) == 0)
            {
                variable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
                return variable;
            }
            bool arraysAreStructsLocal = arraysAreStructs;
            for(int32_t i = 0; i < 2; i++)
            {
                int64_t indexSum = 0;
                ZEND_HASH_FOREACH_KEY_VAL(ht, keyIndex, key, element)
                        {
                            if(!variable)
                            {
                                if(key || arraysAreStructsLocal) variable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                                else
                                {
                                    variable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
                                    variable->arrayValue->reserve(zend_hash_num_elements(ht));
                                }
                            }
                            BaseLib::PVariable arrayElement = getVariable(element, subArraysAreStructs, subArraysAreStructs);
                            if(!arrayElement) continue;
                            if(key || arraysAreStructsLocal)
                            {
                                std::string keyName;
                                keyName = key ? std::string(key->val, key->len) : std::to_string(SIZEOF_ZEND_LONG == 8 ? (int64_t)keyIndex : (int32_t)keyIndex);
                                if(keyName.size() > 1 && keyName.at(0) == '\\') keyName = keyName.substr(1);
                                variable->structValue->emplace(keyName, arrayElement);
                            }
                            else
                            {
                                if(keyIndex < 0) indexSum = -1;
                                if(indexSum >= 0) indexSum += keyIndex;
                                variable->arrayValue->push_back(arrayElement);
                            }
                        }ZEND_HASH_FOREACH_END();

                if(variable->type == BaseLib::VariableType::tArray && (uint64_t) indexSum * 2 != (variable->arrayValue->size() - 1) * variable->arrayValue->size())
                {
                    arraysAreStructsLocal = true;
                    variable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                }
                else break;
            }
        }
        else
        {
            variable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tVoid);
        }

        return variable;
    }
    catch(const std::exception& ex)
    {
        GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::PVariable();
}

#pragma GCC diagnostic warning "-Wunused-but-set-variable"

void PhpVariableConverter::getPHPVariable(BaseLib::PVariable input, zval* output)
{
    try
    {
        if(!input || !output) return;

        if(input->type == BaseLib::VariableType::tArray)
        {
            array_init(output);
            for(std::vector<BaseLib::PVariable>::iterator i = input->arrayValue->begin(); i != input->arrayValue->end(); ++i)
            {
                zval element;
                getPHPVariable(*i, &element);
                add_next_index_zval(output, &element);
            }
            return;
        }
        else if(input->type == BaseLib::VariableType::tStruct)
        {
            array_init(output);
            for(std::map<std::string, BaseLib::PVariable>::iterator i = input->structValue->begin(); i != input->structValue->end(); ++i)
            {
                zval element;
                getPHPVariable(i->second, &element);
                add_assoc_zval_ex(output, i->first.c_str(), i->first.size(), &element);
            }
            return;
        }

        if(input->type == BaseLib::VariableType::tVoid)
        {
            ZVAL_NULL(output);
        }
        else if(input->type == BaseLib::VariableType::tBoolean)
        {
            ZVAL_BOOL(output, input->booleanValue);
        }
        else if(input->type == BaseLib::VariableType::tInteger)
        {
            ZVAL_LONG(output, input->integerValue);
        }
        else if(input->type == BaseLib::VariableType::tInteger64)
        {
            ZVAL_LONG(output, input->integerValue64);
        }
        else if(input->type == BaseLib::VariableType::tFloat)
        {
            ZVAL_DOUBLE(output, input->floatValue);
        }
        else if(input->type == BaseLib::VariableType::tString || input->type == BaseLib::VariableType::tBase64)
        {
            if(input->stringValue.empty()) ZVAL_STRINGL(output, "", 0); //At least once, input->stringValue.c_str() on an empty string was a nullptr causing a segementation fault, so check for empty string
            else ZVAL_STRINGL(output, input->stringValue.c_str(), input->stringValue.size());
        }
        else if(input->type == BaseLib::VariableType::tBinary)
        {
            if(input->binaryValue.empty()) ZVAL_STRINGL(output, "", 0); //At least once, input->stringValue.c_str() on an empty string was a nullptr causing a segementation fault, so check for empty string
            else ZVAL_STRINGL(output, (char*) input->binaryValue.data(), input->binaryValue.size());
        }
        else
        {
            ZVAL_STRINGL(output, "UNKNOWN", sizeof("UNKNOWN") - 1);
        }
    }
    catch(const std::exception& ex)
    {
        GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

}

#endif
