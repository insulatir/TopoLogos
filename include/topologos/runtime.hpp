#pragma once
#include <iostream>
#include <expected>
#include <type_traits>
#include <string>

namespace topologos {

    // 1. 메타 프로그래밍: T가 std::expected인지 검사
    template<typename T>
    struct is_expected : std::false_type {};

    template<typename T, typename E>
    struct is_expected<std::expected<T, E>> : std::true_type {};

    template<typename T>
    constexpr bool is_expected_v = is_expected<T>::value;

    // 2. 검증 헬퍼 함수 (사용자는 이것만 호출하면 됨)
    // T가 Strong Link(값)인지 Weak Link(expected)인지 자동으로 판단해서 출력
    template <typename T>
    bool verify_connection(const T& result, const std::string& step_name) {
        // 입력 타입(T)에서 참조와 const를 제거한 실제 타입
        using RawType = std::decay_t<T>;

        if constexpr (is_expected_v<RawType>) {
            // [Weak Link]
            if (result.has_value()) {
                std::cout << "[Runtime] " << step_name << " (Weak): AI admits connection. (Safe)\n";
                return true;
            } else {
                std::cout << "[Runtime] " << step_name << " (Weak): Connection Broken! Reason: " << result.error() << "\n";
                return false;
            }
        } else {
            // [Strong Link]
            std::cout << "[Runtime] " << step_name << " (Strong): AI is 100% sure! Direct connection.\n";
            return true;
        }
    }
}