local proto = require("proto.core")

local person = {
    name = "Alice",
    age = 30
}

local bin = proto.serialize("example.Person", person)
print("Serialized size:", #bin)

local tbl = proto.deserialize("example.Person", bin)
print("Deserialized:--------")
for k, v in pairs(tbl) do
    print(k, v)
end
print("Deserialized:--------end")

local dbg = proto.debugstr("example.Person", bin, "debug")
print("DebugString:\n" .. dbg)
