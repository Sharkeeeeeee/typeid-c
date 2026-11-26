"""Tests for the SQLite extension against the official spec vectors.

Usage: python3 tests/test_sqlite.py ./typeid.so tests/spec
"""
import json
import sqlite3
import sys

ext, spec = sys.argv[1], sys.argv[2]
db = sqlite3.connect(":memory:")
db.enable_load_extension(True)
db.load_extension(ext)

failures = 0


def q(sql, *args):
    return db.execute(sql, args).fetchone()[0]


def check(cond, msg):
    global failures
    if not cond:
        failures += 1
        print("FAIL", msg)


def raises(sql, *args):
    try:
        q(sql, *args)
    except sqlite3.OperationalError:
        return True
    return False


with open(spec + "/valid.json", encoding="utf-8") as f:
    valid = json.load(f)
with open(spec + "/invalid.json", encoding="utf-8") as f:
    invalid = json.load(f)

for v in valid:
    check(q("select typeid_encode(?, ?)", v["prefix"], v["uuid"]) == v["typeid"], v["name"] + " encode")
    check(q("select typeid_uuid(?)", v["typeid"]) == v["uuid"], v["name"] + " uuid")
    check(q("select typeid_prefix(?)", v["typeid"]) == v["prefix"], v["name"] + " prefix")
    check(q("select typeid_valid(?)", v["typeid"]) == 1, v["name"] + " valid")

for v in invalid:
    check(q("select typeid_valid(?)", v["typeid"]) == 0, v["name"] + " valid")
    check(raises("select typeid_uuid(?)", v["typeid"]), v["name"] + " should raise")

tid = "user_01h455vb4pex5vsknk084sn02q"
new = q("select typeid_new('user')")
check(new.startswith("user_") and len(new) == 31, "typeid_new: " + new)
check(len(q("select typeid_new()")) == 26, "typeid_new() without prefix")
check(len(q("select typeid_new(NULL)")) == 26, "typeid_new(NULL)")
check(raises("select typeid_new('User')"), "typeid_new bad prefix")
ids = [r[0] for r in db.execute(
    "with recursive n(i) as (select 1 union all select i + 1 from n where i < 20000) "
    "select typeid_new('evt') from n")]
check(ids == sorted(ids) and len(set(ids)) == len(ids), "typeid_new is monotonic per connection")
check(q("select typeid_suffix(?)", tid) == "01h455vb4pex5vsknk084sn02q", "suffix")
check(q("select typeid_encode('user', typeid_uuid_blob(?))", tid) == tid, "blob roundtrip")
check(raises("select typeid_encode('user', x'0102')"), "short blob")
check(raises("select typeid_encode('User', '01890a5d-ac96-774b-bcce-b302099a8057')"), "encode bad prefix")
check(q("select typeid_timestamp(?)", tid) == 0x01890A5DAC96, "timestamp")
check(q("select typeid_check(?, 'user')", tid) == 1, "check same prefix")
check(q("select typeid_check(?, 'post')", tid) == 0, "check other prefix")
check(q("select typeid_check('garbage', 'user')") == 0, "check garbage")
check(q("select typeid_uuid(NULL)") is None, "NULL passthrough")
check(q("select typeid_valid(NULL)") is None, "NULL valid")
check(q("select typeid_encode(NULL, '01890a5d-ac96-774b-bcce-b302099a8057')") is None, "encode NULL prefix")
check(q("select typeid_encode('', '01890a5d-ac96-774b-bcce-b302099a8057')") == "01h455vb4pex5vsknk084sn02q", "encode empty prefix")
check(q("select typeid_check(?, NULL)", tid) is None, "check NULL prefix")
check(q("select typeid_new(NULL) is not null") == 1, "typeid_new(NULL) means no prefix")
check(raises("select typeid_new('abc' || char(0) || 'def')"), "embedded NUL in prefix")
check(q("select typeid_valid(x'7573657200')") == 0, "blob with NUL")

db.execute(
    "create table users ("
    " id text primary key default (typeid_new('user')) check (typeid_check(id, 'user')),"
    " name text)"
)
db.execute("insert into users(name) values ('a'), ('b')")
check(q("select count(*) from users where typeid_check(id, 'user')") == 2, "column default")
try:
    db.execute("insert into users values ('post_01h455vb4pex5vsknk084sn02q', 'x')")
    check(False, "check constraint should reject other prefix")
except sqlite3.IntegrityError:
    pass

db.close()
print("sqlite extension: %d failures (%d valid + %d invalid spec vectors)" % (failures, len(valid), len(invalid)))
sys.exit(1 if failures else 0)
