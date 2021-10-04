#include <matrix/db/database.hpp>

#include <matrix/db/snapshot.hpp>

#include <matrix/world/global/random.hpp>
#include <matrix/world/global/log.hpp>

#include <matrix/log/bytes.hpp>

#include <timber/log.hpp>

using whirl::node::db::Key;
using whirl::node::db::Value;
using whirl::node::db::WriteBatch;

namespace whirl::matrix::db {

Database::Database(node::fs::IFileSystem* fs)
    : fs_(fs), logger_("Database", GetLogBackend()) {
}

void Database::Open(const std::string& directory) {
  wal_path_ = directory + "/wal";
  wal_.emplace(fs_, wal_path_);
  ReplayWAL();
}

void Database::Put(const Key& key, const Value& value) {
  //LOG_INFO("Put('{}', '{}')", key, log::FormatMessage(value));

  node::db::WriteBatch batch;
  batch.Put(key, value);
  DoWrite(batch);
}

void Database::Delete(const Key& key) {
  //LOG_INFO("Delete('{}')", key);

  node::db::WriteBatch batch;
  batch.Delete(key);
  DoWrite(batch);
}

std::optional<Value> Database::TryGet(const Key& key) const {
  if (ReadCacheMiss()) {
    // TODO
    // disk_.Read(1);  // Access SSTable-s
  }
  LOG_INFO("TryGet({})", key);
  return mem_table_.TryGet(key);
}

node::db::ISnapshotPtr Database::MakeSnapshot() {
  LOG_INFO("Make snapshot at version {}", version_);
  return std::make_shared<Snapshot>(mem_table_.GetEntries());
}

void Database::Write(WriteBatch batch) {
  LOG_INFO("Write({} mutations)", batch.muts.size());
  DoWrite(batch);
}

void Database::DoWrite(WriteBatch& batch) {
  auto guard = write_mutex_.Guard();

  wal_->Append(batch);
  ApplyToMemTable(batch);
  ++version_;
}

void Database::ApplyToMemTable(const node::db::WriteBatch& batch) {
  for (const auto& mut : batch.muts) {
    switch (mut.type) {
      case node::db::MutationType::Put:
        LOG_INFO("Put('{}', '{}')", mut.key, log::FormatMessage(*mut.value));
        mem_table_.Put(mut.key, *mut.value);
        break;
      case node::db::MutationType::Delete:
        LOG_INFO("Delete('{}')", mut.key);
        mem_table_.Delete(mut.key);
        break;
    }
  }
}

bool Database::ReadCacheMiss() const {
  // return GlobalRandomNumber(10) == 1;  // Move to time model?
  return false;
}

void Database::ReplayWAL() {
  mem_table_.Clear();

  LOG_INFO("Replaying WAL -> MemTable");

  if (!fs_->Exists(wal_path_)) {
    return;
  }

  version_ = 0;

  WALReader wal_reader(fs_, wal_path_);

  while (auto batch = wal_reader.ReadNext()) {
    ApplyToMemTable(*batch);
    ++version_;
  }

  LOG_INFO("MemTable populated");
}

}  // namespace whirl::matrix::db
