#pragma once

#include "simple_coro.h"

class SharedDataBase {
  public:
    virtual simple::Future asyncGet() = 0;
    virtual ~SharedDataBase() = default;
};

class SharedDataStorage : public SharedDataBase {
  public:
    simple::Future asyncGet() final {
        co_return co_await data_;
    }

    void setData(int data) {
        data_.setData(data);
    }

  private:
    simple::SharedData data_;
};

template <typename TransformFunc>
class SharedDataTransform : public SharedDataBase {
  public:
    SharedDataTransform(TransformFunc&& func,
                        std::shared_ptr<SharedDataBase> baseData)
        : baseData_(baseData),
          transformFunc_(std::forward<TransformFunc>(func)) {
    }

    simple::Future asyncGet() final {
        int value = co_await baseData_->asyncGet();
        co_return transformFunc_(value);
    }

  private:
    std::shared_ptr<SharedDataBase> baseData_;
    TransformFunc transformFunc_;
};
