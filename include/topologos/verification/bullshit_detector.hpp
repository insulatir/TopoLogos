#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp> // CMake에서 추가한 JSON 라이브러리

namespace topologos::verification {

    // 판결 결과 구조체
    struct Verdict {
        bool is_truth;                       // 진실 여부 (Pass/Fail)
        double integrity_score;              // 무결성 점수 (0~100)
        std::vector<std::string> violations; // 위반 사항 목록
    };

    // 거짓 탐지기 클래스
    class BullshitDetector {
    public:
        // 생성자: 규칙 파일(life.topo) 경로를 받아 초기화
        explicit BullshitDetector(const std::string& rule_file_path);

        // 검증 함수: JSON 증거 데이터를 받아 판결을 내림
        Verdict judge(const nlohmann::json& evidence_json);

    private:
        // [The Laws] 검증 임계값 (life.topo에서 로드하거나 기본값 사용)
        double THERMO_THRESHOLD = 1000.0; // 물리: 투입 대비 보상 비율
        double ENTROPY_CLARITY_MIN = 0.7; // 물리: 텍스트 명확성 최소값
        int MAX_URGENCY_COUNT = 2;        // 심리: 허용 긴급 키워드 수
        int MAX_ABSOLUTISM_COUNT = 1;     // 심리: 허용 절대적 표현 수
        size_t MIN_SOURCES = 3;           // 구조: 최소 인용 출처 수

        // 규칙 로드 헬퍼 함수
        void load_rules(const std::string& filepath);
    };

} // namespace topologos::verification