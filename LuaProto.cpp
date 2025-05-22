#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <lua/lua.hpp>

namespace LuaModule {

using GPMessageFactory      = google::protobuf::MessageFactory;
using GPMessage             = google::protobuf::Message;
using GPDescriptorPool      = google::protobuf::DescriptorPool;
using GPFieldDescriptor     = google::protobuf::FieldDescriptor;
using GPReflection          = google::protobuf::Reflection;
using GPEnumValueDescriptor = google::protobuf::EnumValueDescriptor;
using GPDescriptor          = google::protobuf::Descriptor;

static void _msg2table(lua_State *L, const GPMessage *pMsg);
static void _table2msg(lua_State *L, GPMessage *pMsg);
static void _pushfield(lua_State *L, const GPMessage *pMsg,
                       const GPReflection      *pReflection,
                       const GPFieldDescriptor *field);
static void _setfield(lua_State *L, GPMessage *pMsg,
                      const GPReflection      *pReflection,
                      const GPFieldDescriptor *field);

static GPMessage *_newmsg(lua_State *L, const char *name) {
	const GPDescriptorPool *pDPool;
	GPMessageFactory       *pMFactory;
	const GPDescriptor     *pDescriptor;
	const GPMessage        *pMessage;

	pDPool = (const GPDescriptorPool *) lua_touserdata(L, lua_upvalueindex(1));
	pMFactory = (GPMessageFactory *) lua_touserdata(L, lua_upvalueindex(2));

	if (pDPool == nullptr || pMFactory == nullptr)
		return nullptr;

	pDescriptor = pDPool->FindMessageTypeByName(name);

	if (pDescriptor == nullptr)
		return nullptr;

	pMessage = pMFactory->GetPrototype(pDescriptor);

	if (pMessage == nullptr)
		return nullptr;

	return pMessage->New();
}

static void _pushsingle(lua_State *L, const GPMessage *pMsg,
                        const GPReflection      *pReflection,
                        const GPFieldDescriptor *field) {
	switch (field->cpp_type()) {
	case GPFieldDescriptor::CPPTYPE_INT32:
		lua_pushinteger(L, pReflection->GetInt32(*pMsg, field));
		break;
	case GPFieldDescriptor::CPPTYPE_INT64:
		lua_pushinteger(L, pReflection->GetInt64(*pMsg, field));
		break;
	case GPFieldDescriptor::CPPTYPE_UINT32:
		lua_pushinteger(L, pReflection->GetUInt32(*pMsg, field));
		break;
	case GPFieldDescriptor::CPPTYPE_UINT64:
		lua_pushinteger(L, pReflection->GetUInt64(*pMsg, field));
		break;
	case GPFieldDescriptor::CPPTYPE_DOUBLE:
		lua_pushnumber(L, pReflection->GetDouble(*pMsg, field));
		break;
	case GPFieldDescriptor::CPPTYPE_FLOAT:
		lua_pushnumber(L, pReflection->GetFloat(*pMsg, field));
		break;
	case GPFieldDescriptor::CPPTYPE_BOOL:
		lua_pushboolean(L, pReflection->GetBool(*pMsg, field));
		break;
	case GPFieldDescriptor::CPPTYPE_ENUM: {
		auto valuetype = pReflection->GetEnum(*pMsg, field);
		if (valuetype)
			lua_pushstring(L, valuetype->name().c_str());
	} break;
	case GPFieldDescriptor::CPPTYPE_STRING: {
		std::string str;
		str = pReflection->GetStringReference(*pMsg, field, &str);
		lua_pushlstring(L, str.c_str(), str.length());
	} break;
	case GPFieldDescriptor::CPPTYPE_MESSAGE: {
		auto &msg = pReflection->GetMessage(*pMsg, field);
		_msg2table(L, &msg);
	} break;
	default:
		lua_pushnil(L);
		break;
	}
}

static void _pusharray(lua_State *L, const GPMessage *pMsg,
                       const GPReflection      *pReflection,
                       const GPFieldDescriptor *field) {
	auto fieldsize = pReflection->FieldSize(*pMsg, field);
	lua_createtable(L, fieldsize, 0);
	switch (field->cpp_type()) {
	case GPFieldDescriptor::CPPTYPE_INT32: {
		for (int i = 0; i < fieldsize; i++) {
			lua_pushinteger(L, pReflection->GetRepeatedInt32(*pMsg, field, i));
			lua_rawseti(L, -2, i + 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_INT64: {
		for (int i = 0; i < fieldsize; i++) {
			lua_pushinteger(L, pReflection->GetRepeatedInt64(*pMsg, field, i));
			lua_rawseti(L, -2, i + 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_UINT32: {
		for (int i = 0; i < fieldsize; i++) {
			lua_pushinteger(L, pReflection->GetRepeatedUInt32(*pMsg, field, i));
			lua_rawseti(L, -2, i + 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_UINT64: {
		for (int i = 0; i < fieldsize; i++) {
			lua_pushinteger(L, pReflection->GetRepeatedUInt64(*pMsg, field, i));
			lua_rawseti(L, -2, i + 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_DOUBLE: {
		for (int i = 0; i < fieldsize; i++) {
			lua_pushnumber(L, pReflection->GetRepeatedDouble(*pMsg, field, i));
			lua_rawseti(L, -2, i + 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_FLOAT: {
		for (int i = 0; i < fieldsize; i++) {
			lua_pushnumber(L, pReflection->GetRepeatedFloat(*pMsg, field, i));
			lua_rawseti(L, -2, i + 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_BOOL: {
		for (int i = 0; i < fieldsize; i++) {
			lua_pushboolean(L, pReflection->GetRepeatedBool(*pMsg, field, i));
			lua_rawseti(L, -2, i + 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_ENUM: {
		for (int i = 0; i < fieldsize; i++) {
			auto valuetype = pReflection->GetRepeatedEnum(*pMsg, field, i);
			valuetype ? lua_pushstring(L, valuetype->name().c_str())
			          : lua_pushliteral(L, "error enum");
			lua_rawseti(L, -2, i + 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_STRING: {
		for (int i = 0; i < fieldsize; i++) {
			std::string str;
			str =
			    pReflection->GetRepeatedStringReference(*pMsg, field, i, &str);
			lua_pushlstring(L, str.c_str(), str.length());
			lua_rawseti(L, -2, i + 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_MESSAGE: {
		for (int i = 0; i < fieldsize; i++) {
			auto &msg = pReflection->GetRepeatedMessage(*pMsg, field, i);
			_msg2table(L, &msg);
			lua_rawseti(L, -2, i + 1);
		}
	} break;
	default:
		break;
	}
}

static void _msg2kv(lua_State *L, const GPMessage *pMsg) {
	std::vector<const GPFieldDescriptor *> fields;
	const GPReflection                    *pReflection;
	const GPFieldDescriptor               *pField;
	const GPDescriptor                    *pDesc;

	pReflection = pMsg->GetReflection();
	if (pReflection == nullptr)
		luaL_error(L, "GetReflection Failed!");

	pDesc = pMsg->GetDescriptor();
	if (pDesc == nullptr)
		luaL_error(L, "GetDescriptor Failed!");

	pReflection->ListFields(*pMsg, &fields);
	if (fields.size() != 2)
		luaL_error(L, "msg2kv size error!");

	pField = pDesc->FindFieldByName("key");
	if (pField == nullptr)
		luaL_error(L, "no key field!");
	_pushfield(L, pMsg, pReflection, pField);

	pField = pDesc->FindFieldByName("value");
	if (pField == nullptr)
		luaL_error(L, "no value field!");
	_pushfield(L, pMsg, pReflection, pField);
}

static void _pushmap(lua_State *L, const GPMessage *pMsg,
                     const GPReflection      *pReflection,
                     const GPFieldDescriptor *field) {
	if (field->cpp_type() != GPFieldDescriptor::CPPTYPE_MESSAGE)
		luaL_error(L, "map cpptype must be message!");

	auto fieldsize = pReflection->FieldSize(*pMsg, field);
	lua_createtable(L, 0, fieldsize);
	for (int i = 0; i < fieldsize; i++) {
		auto &msg = pReflection->GetRepeatedMessage(*pMsg, field, i);
		_msg2kv(L, &msg);
		lua_rawset(L, -3);
	}
}

static void _pushfield(lua_State *L, const GPMessage *pMsg,
                       const GPReflection      *pReflection,
                       const GPFieldDescriptor *field) {
	switch (field->label()) {
	case GPFieldDescriptor::LABEL_OPTIONAL:
	case GPFieldDescriptor::LABEL_REQUIRED:
		_pushsingle(L, pMsg, pReflection, field);
		break;
	case GPFieldDescriptor::LABEL_REPEATED:
		field->is_map() ? _pushmap(L, pMsg, pReflection, field)
		                : _pusharray(L, pMsg, pReflection, field);
		break;
	default:
		lua_pushnil(L);
		break;
	}
}

static void _msg2table(lua_State *L, const GPMessage *pMsg) {
	std::vector<const GPFieldDescriptor *> fields;
	const GPReflection                    *pReflection;

	pReflection = pMsg->GetReflection();
	if (pReflection == nullptr)
		luaL_error(L, "GetReflection Failed!");

	pReflection->ListFields(*pMsg, &fields);
	lua_createtable(L, 0, fields.size());

	for (auto field : fields) {
		lua_pushstring(L, field->name().c_str());
		_pushfield(L, pMsg, pReflection, field);
		lua_rawset(L, -3);
	}
}

static void _setsingle(lua_State *L, GPMessage *pMsg,
                       const GPReflection      *pReflection,
                       const GPFieldDescriptor *field) {
	switch (field->cpp_type()) {
	case GPFieldDescriptor::CPPTYPE_INT32: {
		auto value = luaL_checkinteger(L, -1);
		pReflection->SetInt32(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_INT64: {
		auto value = luaL_checkinteger(L, -1);
		pReflection->SetInt64(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_UINT32: {
		auto value = luaL_checkinteger(L, -1);
		pReflection->SetUInt32(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_UINT64: {
		auto value = luaL_checkinteger(L, -1);
		pReflection->SetUInt64(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_DOUBLE: {
		auto value = luaL_checknumber(L, -1);
		pReflection->SetDouble(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_FLOAT: {
		auto value = luaL_checknumber(L, -1);
		pReflection->SetFloat(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_BOOL: {
		auto value = lua_toboolean(L, -1);
		pReflection->SetBool(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_ENUM: {
		const char                  *value     = luaL_checkstring(L, -1);
		const GPEnumValueDescriptor *enumvalue = nullptr;
		auto                         enumtype  = field->enum_type();
		if (enumtype)
			enumvalue = enumtype->FindValueByName(value);
		if (enumvalue)
			pReflection->SetEnum(pMsg, field, enumvalue);
	} break;
	case GPFieldDescriptor::CPPTYPE_STRING: {
		const char *value = luaL_checkstring(L, -1);
		pReflection->SetString(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_MESSAGE: {
		auto pSubMsg = pReflection->MutableMessage(pMsg, field);
		_table2msg(L, pSubMsg);
	} break;
	default:
		break;
	}
}

static void _setarray(lua_State *L, GPMessage *pMsg,
                      const GPReflection      *pReflection,
                      const GPFieldDescriptor *field) {
	size_t len;
	if (!lua_istable(L, -1))
		return;
	len = lua_rawlen(L, -1);

	switch (field->cpp_type()) {
	case GPFieldDescriptor::CPPTYPE_INT32: {
		for (int i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			auto value = luaL_checkinteger(L, -1);
			pReflection->AddInt32(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_INT64: {
		for (int i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			auto value = luaL_checkinteger(L, -1);
			pReflection->AddInt64(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_UINT32: {
		for (int i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			auto value = luaL_checkinteger(L, -1);
			pReflection->AddUInt32(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_UINT64: {
		for (int i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			auto value = luaL_checkinteger(L, -1);
			pReflection->AddUInt64(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_DOUBLE: {
		for (int i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			auto value = luaL_checknumber(L, -1);
			pReflection->AddDouble(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_FLOAT: {
		for (int i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			auto value = luaL_checknumber(L, -1);
			pReflection->AddFloat(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_BOOL: {
		for (int i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			auto value = lua_toboolean(L, -1);
			pReflection->AddBool(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_ENUM: {
		for (int i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			const char                  *value     = luaL_checkstring(L, -1);
			const GPEnumValueDescriptor *enumvalue = nullptr;
			auto                         enumtype  = field->enum_type();
			if (enumtype)
				enumvalue = enumtype->FindValueByName(value);
			if (enumvalue)
				pReflection->AddEnum(pMsg, field, enumvalue);
			else
				luaL_error(L, "Invalid Enum In Repeated Field! %s", value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_STRING: {
		for (int i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			const char *value = luaL_checkstring(L, -1);
			pReflection->AddString(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_MESSAGE: {
		for (int i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			auto pSubMsg = pReflection->AddMessage(pMsg, field);
			_table2msg(L, pSubMsg);
			lua_pop(L, 1);
		}
	} break;
	default:
		break;
	}
}

static void _kv2msg(lua_State *L, GPMessage *pMsg) {
	const GPDescriptor      *pDesc;
	const GPReflection      *pReflection;
	const GPFieldDescriptor *pKeyField, *pValueField;

	pReflection = pMsg->GetReflection();
	if (pReflection == nullptr)
		luaL_error(L, "GetReflection Failed!");

	pDesc = pMsg->GetDescriptor();
	if (pDesc == nullptr)
		luaL_error(L, "GetDescriptor Failed!");

	pKeyField = pDesc->FindFieldByName("key");
	if (pKeyField == nullptr)
		luaL_error(L, "no key field!");

	pValueField = pDesc->FindFieldByName("value");
	if (pValueField == nullptr)
		luaL_error(L, "no value field!");

	lua_pushvalue(L, -2); // key, value, key

	_setfield(L, pMsg, pReflection, pKeyField);

	lua_pop(L, 1);

	_setfield(L, pMsg, pReflection, pValueField);
}

static void _setmap(lua_State *L, GPMessage *pMsg,
                    const GPReflection      *pReflection,
                    const GPFieldDescriptor *field) {
	size_t len;
	if (!lua_istable(L, -1))
		return;

	if (field->cpp_type() != GPFieldDescriptor::CPPTYPE_MESSAGE)
		luaL_error(L, "map cpptype must be message!");

	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		auto pSubMsg = pReflection->AddMessage(pMsg, field);
		_kv2msg(L, pSubMsg);
		lua_pop(L, 1);
	}
}

static void _setfield(lua_State *L, GPMessage *pMsg,
                      const GPReflection      *pReflection,
                      const GPFieldDescriptor *field) {
	switch (field->label()) {
	case GPFieldDescriptor::LABEL_OPTIONAL:
	case GPFieldDescriptor::LABEL_REQUIRED:
		_setsingle(L, pMsg, pReflection, field);
		break;
	case GPFieldDescriptor::LABEL_REPEATED:
		field->is_map() ? _setmap(L, pMsg, pReflection, field)
		                : _setarray(L, pMsg, pReflection, field);
		break;
	default:
		break;
	}
}

static void _table2msg(lua_State *L, GPMessage *pMsg) {
	const GPReflection      *pReflection;
	const GPDescriptor      *pDescriptor;
	const GPFieldDescriptor *pFieldDsp;
	pReflection = pMsg->GetReflection();
	pDescriptor = pMsg->GetDescriptor();
	if (pReflection == nullptr || pDescriptor == nullptr)
		return;

	if (!lua_istable(L, -1))
		return;

	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		const char *key   = luaL_checkstring(L, -2);
		auto        field = pDescriptor->FindFieldByName(key);
		if (field == nullptr)
			luaL_error(L, "invalid field %s!", key);
		_setfield(L, pMsg, pReflection, field);
		lua_pop(L, 1);
	}
}

// lua table => binary data / callback(msg)
static int serialize(lua_State *L) {
	GPMessage  *msg;
	const char *name;
	std::string data;

	name = luaL_checkstring(L, 1);
	msg  = _newmsg(L, name);
	if (!msg)
		return 0;

	if (lua_isfunction(L, -1)) {
		lua_replace(L, 1);
	}

	std::unique_ptr<GPMessage> ptr(msg); // below code may throw exception, so
	                                     // use unique_ptr to release memory

	_table2msg(L, msg);

	if (lua_isfunction(L, 1)) {
		lua_settop(L, 1);
		lua_pushlightuserdata(L, (void *) msg);
		lua_call(L, 1, 0);
		return 0;
	} else {
		msg->SerializeToString(&data);
		lua_pushlstring(L, data.c_str(), data.length());
		return 1;
	}
}

// binary data / lightuserdata => lua table
static int deserialize(lua_State *L) {
	GPMessage  *msg;
	const char *name;
	const void *data;
	size_t      sz;

	name = luaL_checkstring(L, 1);
	if (lua_isuserdata(L, 2)) {
		data = (const void *) lua_touserdata(L, 2);
		sz   = luaL_checkinteger(L, 3);
	} else {
		data = luaL_checklstring(L, 2, &sz);
	}

	msg = _newmsg(L, name);
	if (!msg)
		return 0;
	msg->ParseFromArray(data, sz);

	std::unique_ptr<GPMessage> ptr(msg); // below code may throw exception, so
	                                     // use unique_ptr to release memory

	_msg2table(L, msg);

	return 1;
}

// binary data / lightuserdata => lua table
static int debugstr(lua_State *L) {
	GPMessage  *msg;
	const char *name;
	const void *data;
	size_t      sz;
	int         opt;
	std::string str;

	static const char *mode[] = {"debug", "short", "utf8"};

	name = luaL_checkstring(L, 1);
	if (lua_isuserdata(L, 2)) {
		data = (const void *) lua_touserdata(L, 2);
		sz   = luaL_checkinteger(L, 3);
		opt  = luaL_checkoption(L, 4, "short", mode);
	} else {
		data = luaL_checklstring(L, 2, &sz);
		opt  = luaL_checkoption(L, 3, "short", mode);
	}

	msg = _newmsg(L, name);
	if (!msg)
		return 0;
	msg->ParseFromArray(data, sz);

	std::unique_ptr<GPMessage> ptr(msg); // below code may throw exception, so
	                                     // use unique_ptr to release memory

	switch (opt) {
	case 0:
		str = msg->DebugString();
		break;
	case 1:
		str = msg->ShortDebugString();
		break;
	case 2:
		str = msg->Utf8DebugString();
		break;
	default:
		break;
	}
	lua_pushlstring(L, str.c_str(), str.size());
	return 1;
}

int luaopen_proto_core(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {{"serialize", serialize},
	                {"deserialize", deserialize},
	                {"debugstr", debugstr},
	                {NULL, NULL}};
	luaL_newlibtable(L, l);
	lua_pushlightuserdata(L, (void *) GPDescriptorPool::generated_pool());
	lua_pushlightuserdata(L, (void *) GPMessageFactory::generated_factory());
	luaL_setfuncs(L, l, 2);
	return 1;
}

} // namespace LuaModule

extern "C" int luaopen_proto_core(lua_State *L) {
	return LuaModule::luaopen_proto_core(L);
}
