#pragma once
#include <memory>

/// <summary>
/// シングルトンパターンを共通化する CRTP 基底クラステンプレート
/// </summary>
template <typename T>
class Singleton {
public:
    /// <summary>
    /// シングルトンインスタンスを取得（初回のみ生成）
    /// </summary>
    static T* GetInstance() {
        if (instance_ == nullptr) {
            struct Helper : public T {
                Helper() : T() {}
            };
            instance_ = std::make_unique<Helper>();
        }
        return instance_.get();
    }

    /// <summary>
    /// シングルトンインスタンスを破棄する
    /// </summary>
    static void Finalize() {
        instance_.reset();
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

protected:
    Singleton() = default;
    virtual ~Singleton() = default;

private:
    static inline std::unique_ptr<T> instance_ = nullptr;
};
