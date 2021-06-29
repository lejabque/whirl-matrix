#pragma once

#include <whirl/services/uid.hpp>

#include <whirl/engines/matrix/world/global/random.hpp>
#include <whirl/engines/matrix/world/global/time.hpp>
#include <whirl/engines/matrix/world/global/uids.hpp>

#include <wheels/support/string_builder.hpp>

namespace whirl::matrix {

struct UidGenerator : public IUidGenerator {
  UidGenerator(size_t server_id) : server_id_(server_id) {
  }

  Uid Generate() override {
    // return GenerateLocal

    // Short and globally unique id
    return GenerateUid();
  }

 private:
  // {random-number}-{server-id}-{global-time}-{uid-request}
  Uid GenerateLocal() {
    return wheels::StringBuilder() << GlobalRandomNumber() << "-" << server_id_
                                   << "-" << GlobalNow() << "-" << ++request_;
  }

 private:
  const size_t server_id_;
  size_t request_{0};
};

}  // namespace whirl::matrix
