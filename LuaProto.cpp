#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <lua.hpp>

#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <span>

namespace LuaModule {

// Use modern type aliases with consistent naming
using GPMessageFactory      = google::protobuf::MessageFactory;
using GPMessage             = google::protobuf::Message;
using GPDescriptorPool      = google::protobuf::DescriptorPool;
using GPFieldDescriptor     = google::protobuf::FieldDescriptor;
using GPReflection          = google::protobuf::Reflection;
using GPEnumValueDescriptor = google::protobuf::EnumValueDescriptor;
using GPDescriptor          = google::protobuf::Descriptor;

// C++20 concepts for type safety
template<typename T>
concept ProtobufPointer = std::is_pointer_v<T> && 
    (std::is_same_v<std::remove_const_t<std::remove_pointer_t<T>>, GPMessage> ||
     std::is_same_v<std::remove_const_t<std::remove_pointer_t<T>>, GPReflection> ||
     std::is_same_v<std::remove_const_t<std::remove_pointer_t<T>>, GPFieldDescriptor> ||
     std::is_same_v<std::remove_const_t<std::remove_pointer_t<T>>, GPDescriptor> ||
     std::is_same_v<std::remove_const_t<std::remove_pointer_t<T>>, GPDescriptorPool> ||
     std::is_same_v<std::remove_const_t<std::remove_pointer_t<T>>, GPMessageFactory> ||
     std::is_same_v<std::remove_const_t<std::remove_pointer_t<T>>, GPEnumValueDescriptor>);

// Helper function using concepts for null-checking with branch prediction
template<ProtobufPointer T>
[[nodiscard]] constexpr bool is_valid(T ptr) noexcept {
	return ptr != nullptr;
}

// Forward declarations
static void _msg2table(lua_State *L, const GPMessage *pMsg) noexcept(false);
static void _table2msg(lua_State *L, GPMessage *pMsg) noexcept(false);
static void _pushfield(lua_State *L, const GPMessage *pMsg,
                       const GPReflection      *pReflection,
                       const GPFieldDescriptor *field) noexcept(false);
static void _setfield(lua_State *L, GPMessage *pMsg,
                      const GPReflection      *pReflection,
                      const GPFieldDescriptor *field) noexcept(false);

[[nodiscard]] static GPMessage *_newmsg(lua_State *L, const char *name) noexcept {
	const auto *pDPool = static_cast<const GPDescriptorPool *>(
	    lua_touserdata(L, lua_upvalueindex(1)));
	auto *pMFactory = static_cast<GPMessageFactory *>(
	    lua_touserdata(L, lua_upvalueindex(2)));

	if (!is_valid(pDPool) || !is_valid(pMFactory)) [[unlikely]]
		return nullptr;

	const auto *pDescriptor = pDPool->FindMessageTypeByName(name);

	if (!is_valid(pDescriptor)) [[unlikely]]
		return nullptr;

	const auto *pMessage = pMFactory->GetPrototype(pDescriptor);

	if (!is_valid(pMessage)) [[unlikely]]
		return nullptr;

	return pMessage->New();
}

static void _pushsingle(lua_State *L, const GPMessage *pMsg,
                        const GPReflection      *pReflection,
                        const GPFieldDescriptor *field) noexcept(false) {
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
		const auto *valuetype = pReflection->GetEnum(*pMsg, field);
		if (valuetype) [[likely]]
			lua_pushstring(L, valuetype->name().c_str());
	} break;
	case GPFieldDescriptor::CPPTYPE_STRING: {
		std::string str;
		str = pReflection->GetStringReference(*pMsg, field, &str);
		lua_pushlstring(L, str.c_str(), str.length());
	} break;
	case GPFieldDescriptor::CPPTYPE_MESSAGE: {
		const auto &msg = pReflection->GetMessage(*pMsg, field);
		_msg2table(L, &msg);
	} break;
	default:
		lua_pushnil(L);
		break;
	}
}

static void _pusharray(lua_State *L, const GPMessage *pMsg,
                       const GPReflection      *pReflection,
                       const GPFieldDescriptor *field) noexcept(false) {
	const auto fieldsize = pReflection->FieldSize(*pMsg, field);
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
			const auto *valuetype = pReflection->GetRepeatedEnum(*pMsg, field, i);
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
			const auto &msg = pReflection->GetRepeatedMessage(*pMsg, field, i);
			_msg2table(L, &msg);
			lua_rawseti(L, -2, i + 1);
		}
	} break;
	default:
		break;
	}
}

static void _msg2kv(lua_State *L, const GPMessage *pMsg) noexcept(false) {
	std::vector<const GPFieldDescriptor *> fields;

	const auto *pReflection = pMsg->GetReflection();
	if (!is_valid(pReflection)) [[unlikely]]
		luaL_error(L, "GetReflection Failed!");

	const auto *pDesc = pMsg->GetDescriptor();
	if (!is_valid(pDesc)) [[unlikely]]
		luaL_error(L, "GetDescriptor Failed!");

	pReflection->ListFields(*pMsg, &fields);
	if (fields.size() != 2) [[unlikely]]
		luaL_error(L, "msg2kv size error!");

	const auto *pField = pDesc->FindFieldByName("key");
	if (!is_valid(pField)) [[unlikely]]
		luaL_error(L, "no key field!");
	_pushfield(L, pMsg, pReflection, pField);

	pField = pDesc->FindFieldByName("value");
	if (!is_valid(pField)) [[unlikely]]
		luaL_error(L, "no value field!");
	_pushfield(L, pMsg, pReflection, pField);
}

static void _pushmap(lua_State *L, const GPMessage *pMsg,
                     const GPReflection      *pReflection,
                     const GPFieldDescriptor *field) noexcept(false) {
	if (field->cpp_type() != GPFieldDescriptor::CPPTYPE_MESSAGE) [[unlikely]]
		luaL_error(L, "map cpptype must be message!");

	const auto fieldsize = pReflection->FieldSize(*pMsg, field);
	lua_createtable(L, 0, fieldsize);
	for (int i = 0; i < fieldsize; i++) {
		const auto &msg = pReflection->GetRepeatedMessage(*pMsg, field, i);
		_msg2kv(L, &msg);
		lua_rawset(L, -3);
	}
}

static void _pushfield(lua_State *L, const GPMessage *pMsg,
                       const GPReflection      *pReflection,
                       const GPFieldDescriptor *field) noexcept(false) {
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

static void _msg2table(lua_State *L, const GPMessage *pMsg) noexcept(false) {
	std::vector<const GPFieldDescriptor *> fields;

	const auto *pReflection = pMsg->GetReflection();
	if (!is_valid(pReflection)) [[unlikely]]
		luaL_error(L, "GetReflection Failed!");

	pReflection->ListFields(*pMsg, &fields);
	lua_createtable(L, 0, fields.size());

	for (const auto *field : fields) {
		lua_pushstring(L, field->name().c_str());
		_pushfield(L, pMsg, pReflection, field);
		lua_rawset(L, -3);
	}
}

static void _setsingle(lua_State *L, GPMessage *pMsg,
                       const GPReflection      *pReflection,
                       const GPFieldDescriptor *field) noexcept(false) {
	switch (field->cpp_type()) {
	case GPFieldDescriptor::CPPTYPE_INT32: {
		const auto value = luaL_checkinteger(L, -1);
		pReflection->SetInt32(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_INT64: {
		const auto value = luaL_checkinteger(L, -1);
		pReflection->SetInt64(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_UINT32: {
		const auto value = luaL_checkinteger(L, -1);
		pReflection->SetUInt32(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_UINT64: {
		const auto value = luaL_checkinteger(L, -1);
		pReflection->SetUInt64(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_DOUBLE: {
		const auto value = luaL_checknumber(L, -1);
		pReflection->SetDouble(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_FLOAT: {
		const auto value = luaL_checknumber(L, -1);
		pReflection->SetFloat(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_BOOL: {
		const auto value = lua_toboolean(L, -1);
		pReflection->SetBool(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_ENUM: {
		const char *value = luaL_checkstring(L, -1);
		const auto *enumtype = field->enum_type();
		const GPEnumValueDescriptor *enumvalue = nullptr;
		if (enumtype) [[likely]]
			enumvalue = enumtype->FindValueByName(value);
		if (enumvalue) [[likely]]
			pReflection->SetEnum(pMsg, field, enumvalue);
	} break;
	case GPFieldDescriptor::CPPTYPE_STRING: {
		const char *value = luaL_checkstring(L, -1);
		pReflection->SetString(pMsg, field, value);
	} break;
	case GPFieldDescriptor::CPPTYPE_MESSAGE: {
		auto *pSubMsg = pReflection->MutableMessage(pMsg, field);
		_table2msg(L, pSubMsg);
	} break;
	default:
		break;
	}
}

static void _setarray(lua_State *L, GPMessage *pMsg,
                      const GPReflection      *pReflection,
                      const GPFieldDescriptor *field) noexcept(false) {
	if (!lua_istable(L, -1)) [[unlikely]]
		return;
	
	const auto len = lua_rawlen(L, -1);

	switch (field->cpp_type()) {
	case GPFieldDescriptor::CPPTYPE_INT32: {
		for (size_t i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			const auto value = luaL_checkinteger(L, -1);
			pReflection->AddInt32(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_INT64: {
		for (size_t i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			const auto value = luaL_checkinteger(L, -1);
			pReflection->AddInt64(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_UINT32: {
		for (size_t i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			const auto value = luaL_checkinteger(L, -1);
			pReflection->AddUInt32(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_UINT64: {
		for (size_t i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			const auto value = luaL_checkinteger(L, -1);
			pReflection->AddUInt64(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_DOUBLE: {
		for (size_t i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			const auto value = luaL_checknumber(L, -1);
			pReflection->AddDouble(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_FLOAT: {
		for (size_t i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			const auto value = luaL_checknumber(L, -1);
			pReflection->AddFloat(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_BOOL: {
		for (size_t i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			const auto value = lua_toboolean(L, -1);
			pReflection->AddBool(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_ENUM: {
		for (size_t i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			const char *value = luaL_checkstring(L, -1);
			const auto *enumtype = field->enum_type();
			const GPEnumValueDescriptor *enumvalue = nullptr;
			if (enumtype) [[likely]]
				enumvalue = enumtype->FindValueByName(value);
			if (enumvalue) [[likely]]
				pReflection->AddEnum(pMsg, field, enumvalue);
			else [[unlikely]]
				luaL_error(L, "Invalid Enum In Repeated Field! %s", value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_STRING: {
		for (size_t i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			const char *value = luaL_checkstring(L, -1);
			pReflection->AddString(pMsg, field, value);
			lua_pop(L, 1);
		}
	} break;
	case GPFieldDescriptor::CPPTYPE_MESSAGE: {
		for (size_t i = 1; i <= len; i++) {
			lua_rawgeti(L, -1, i);
			auto *pSubMsg = pReflection->AddMessage(pMsg, field);
			_table2msg(L, pSubMsg);
			lua_pop(L, 1);
		}
	} break;
	default:
		break;
	}
}

static void _kv2msg(lua_State *L, GPMessage *pMsg) noexcept(false) {
	const auto *pReflection = pMsg->GetReflection();
	if (!is_valid(pReflection)) [[unlikely]]
		luaL_error(L, "GetReflection Failed!");

	const auto *pDesc = pMsg->GetDescriptor();
	if (!is_valid(pDesc)) [[unlikely]]
		luaL_error(L, "GetDescriptor Failed!");

	const auto *pKeyField = pDesc->FindFieldByName("key");
	if (!is_valid(pKeyField)) [[unlikely]]
		luaL_error(L, "no key field!");

	const auto *pValueField = pDesc->FindFieldByName("value");
	if (!is_valid(pValueField)) [[unlikely]]
		luaL_error(L, "no value field!");

	lua_pushvalue(L, -2); // key, value, key

	_setfield(L, pMsg, pReflection, pKeyField);

	lua_pop(L, 1);

	_setfield(L, pMsg, pReflection, pValueField);
}

static void _setmap(lua_State *L, GPMessage *pMsg,
                    const GPReflection      *pReflection,
                    const GPFieldDescriptor *field) noexcept(false) {
	if (!lua_istable(L, -1)) [[unlikely]]
		return;

	if (field->cpp_type() != GPFieldDescriptor::CPPTYPE_MESSAGE) [[unlikely]]
		luaL_error(L, "map cpptype must be message!");

	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		auto *pSubMsg = pReflection->AddMessage(pMsg, field);
		_kv2msg(L, pSubMsg);
		lua_pop(L, 1);
	}
}

static void _setfield(lua_State *L, GPMessage *pMsg,
                      const GPReflection      *pReflection,
                      const GPFieldDescriptor *field) noexcept(false) {
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

static void _table2msg(lua_State *L, GPMessage *pMsg) noexcept(false) {
	const auto *pReflection = pMsg->GetReflection();
	const auto *pDescriptor = pMsg->GetDescriptor();
	
	if (!is_valid(pReflection) || !is_valid(pDescriptor)) [[unlikely]]
		return;

	if (!lua_istable(L, -1)) [[unlikely]]
		return;

	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		const char *key = luaL_checkstring(L, -2);
		const auto *field = pDescriptor->FindFieldByName(key);
		if (!is_valid(field)) [[unlikely]]
			luaL_error(L, "invalid field %s!", key);
		_setfield(L, pMsg, pReflection, field);
		lua_pop(L, 1);
	}
}

// lua table => binary data / callback(msg)
[[nodiscard]] static int serialize(lua_State *L) noexcept(false) {
	const char *name = luaL_checkstring(L, 1);
	auto *msg = _newmsg(L, name);
	if (!msg) [[unlikely]]
		return 0;

	if (lua_isfunction(L, -1)) {
		lua_replace(L, 1);
	}

	// Use unique_ptr to ensure memory is released even if exception is thrown
	std::unique_ptr<GPMessage> ptr(msg);

	_table2msg(L, msg);

	if (lua_isfunction(L, 1)) {
		lua_settop(L, 1);
		lua_pushlightuserdata(L, static_cast<void *>(msg));
		lua_call(L, 1, 0);
		return 0;
	} else {
		std::string data;
		msg->SerializeToString(&data);
		lua_pushlstring(L, data.c_str(), data.length());
		return 1;
	}
}

// binary data / lightuserdata => lua table
[[nodiscard]] static int deserialize(lua_State *L) noexcept(false) {
	const char *name = luaL_checkstring(L, 1);
	const void *data = nullptr;
	size_t sz = 0;

	if (lua_isuserdata(L, 2)) {
		data = static_cast<const void *>(lua_touserdata(L, 2));
		sz = luaL_checkinteger(L, 3);
	} else {
		data = luaL_checklstring(L, 2, &sz);
	}

	auto *msg = _newmsg(L, name);
	if (!msg) [[unlikely]]
		return 0;
	
	msg->ParseFromArray(data, sz);

	// Use unique_ptr to ensure memory is released even if exception is thrown
	std::unique_ptr<GPMessage> ptr(msg);

	_msg2table(L, msg);

	return 1;
}

// binary data / lightuserdata => lua table
[[nodiscard]] static int debugstr(lua_State *L) noexcept(false) {
	constexpr const char *mode[] = {"debug", "short", "utf8"};

	const char *name = luaL_checkstring(L, 1);
	const void *data = nullptr;
	size_t sz = 0;
	int opt = 0;

	if (lua_isuserdata(L, 2)) {
		data = static_cast<const void *>(lua_touserdata(L, 2));
		sz = luaL_checkinteger(L, 3);
		opt = luaL_checkoption(L, 4, "short", mode);
	} else {
		data = luaL_checklstring(L, 2, &sz);
		opt = luaL_checkoption(L, 3, "short", mode);
	}

	auto *msg = _newmsg(L, name);
	if (!msg) [[unlikely]]
		return 0;
	
	msg->ParseFromArray(data, sz);

	// Use unique_ptr to ensure memory is released even if exception is thrown
	std::unique_ptr<GPMessage> ptr(msg);

	std::string str;
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

int luaopen_proto_core(lua_State *L) noexcept {
	luaL_checkversion(L);
	constexpr luaL_Reg l[] = {
	    {"serialize", serialize},
	    {"deserialize", deserialize},
	    {"debugstr", debugstr},
	    {nullptr, nullptr}
	};
	luaL_newlibtable(L, l);
	lua_pushlightuserdata(L, 
	    static_cast<void *>(const_cast<GPDescriptorPool *>(
	        GPDescriptorPool::generated_pool())));
	lua_pushlightuserdata(L, 
	    static_cast<void *>(GPMessageFactory::generated_factory()));
	luaL_setfuncs(L, l, 2);
	return 1;
}

} // namespace LuaModule

extern "C" int luaopen_proto_core(lua_State *L) noexcept {
	return LuaModule::luaopen_proto_core(L);
}
