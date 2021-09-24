#pragma once

#include <whirl/node/fs/fs.hpp>
#include <whirl/node/time/time_service.hpp>

#include <matrix/server/runtime/detail/disk.hpp>
#include <matrix/fs/fs.hpp>

namespace whirl::matrix {

class FS : public node::fs::IFileSystem {
 public:
  FS(matrix::fs::FileSystem* impl, node::time::ITimeService* time_service)
      : disk_(time_service), impl_(impl) {
  }

  wheels::Result<bool> Create(const node::fs::Path& file_path) override {
    return impl_->Create(file_path);
  }

  wheels::Status Delete(const node::fs::Path& file_path) override {
    return impl_->Delete(file_path);
  }

  bool Exists(const node::fs::Path& file_path) const override {
    return impl_->Exists(file_path);
  }

  node::fs::FileList ListFiles(std::string_view prefix) override {
    // All allocations are made in "userspace"
    node::fs::FileList listed;

    auto iter = impl_->ListAllFiles();
    while (iter.IsValid()) {
      if ((*iter).starts_with(prefix)) {
        listed.push_back({this, *iter});
      }
      ++iter;
    }

    return listed;
  }

  // FileMode::Append creates file if it does not exist
  wheels::Result<node::fs::Fd> Open(const node::fs::Path& file_path,
                                    node::fs::FileMode mode) override {
    return impl_->Open(file_path, mode);
  }

  // Only for FileMode::Append
  wheels::Status Append(node::fs::Fd fd, wheels::ConstMemView data) override {
    disk_.Write(data.Size());
    return impl_->Append(fd, data);
  }

  // Only for FileMode::Read
  wheels::Result<size_t> Read(node::fs::Fd fd,
                              wheels::MutableMemView buffer) override {
    disk_.Read(buffer.Size());  // Blocks
    return impl_->Read(fd, buffer);
  }

  wheels::Status Close(node::fs::Fd fd) override {
    return impl_->Close(fd);
  }

  // Paths

  node::fs::Path RootPath() const override {
    return {this, std::string(impl_->RootPath())};
  }

  node::fs::Path TmpPath() const override {
    return {this, std::string(impl_->TmpPath())};
  }

  std::string PathAppend(const std::string& base,
                         const std::string& name) const override {
    return impl_->PathAppend(base, name);
  }

 private:
  // Emulate latency
  matrix::detail::Disk disk_;

  matrix::fs::FileSystem* impl_;
};

}  // namespace whirl::matrix
