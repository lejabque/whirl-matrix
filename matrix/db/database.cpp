#include <whirl/engines/matrix/db/database.hpp>

#include <whirl/engines/matrix/world/global/random.hpp>

using whirl::node::db::Key;
using whirl::node::db::Value;
using whirl::node::db::WriteBatch;

namespace whirl::matrix::db {

Database::Database(node::fs::IFileSystem* fs)
: fs_(fs) {
}

void Database::Open(const std::string& directory) {
  wal_path_ = directory + "/wal";
  wal_.emplace(fs_, wal_path_);
  ReplayWAL();
}

void Database::Put(const Key& key, const Value& value) {
  auto guard = write_mutex_.Guard();

  WHIRL_LOG_INFO("Put({}, {})", key, value);

  wal_->Put(key, value);
  mem_table_.Put(key, value);
}

void Database::Delete(const Key& key) {
  auto guard = write_mutex_.Guard();

  WHIRL_LOG_INFO("Delete({})", key);

  wal_->Delete(key);
  mem_table_.Delete(key);
}

std::optional<Value> Database::TryGet(const Key& key) const {
  if (ReadCacheMiss()) {
    // TODO
    // disk_.Read(1);  // Access SSTable-s
  }
  WHIRL_LOG_INFO("TryGet({})", key);
  return mem_table_.TryGet(key);
}

void Database::Write(WriteBatch /*batch*/) {
  WHEELS_PANIC("Not implemented");
}

bool Database::ReadCacheMiss() const {
  //return GlobalRandomNumber(10) == 1;  // Move to time model?
  return false;
}

void Database::ReplayWAL() {
  mem_table_.Clear();

  WHIRL_LOG_INFO("Replaying WAL");

  if (!fs_->Exists(wal_path_)) {
    return;
  }

  WALReader wal_reader(fs_, wal_path_);

  while (auto mut = wal_reader.Next()) {
    switch (mut->type) {
      case node::db::MutationType::Put:
        WHIRL_LOG_INFO("Replay Put({}, {})", mut->key, *(mut->value));
        mem_table_.Put(mut->key, *(mut->value));
        break;
      case node::db::MutationType::Delete:
        WHIRL_LOG_INFO("Replay Delete({})", mut->key);
        mem_table_.Delete(mut->key);
        break;
    }
  }

  WHIRL_LOG_INFO("Mem table populated");
}

}  // namespace whirl::matrix::db