#pragma once
#include <string>
#include <vector>
#include <set>
#include <algorithm>

namespace topologos::topology {

    // 0-Simplex: 단일 정보 출처
    struct Vertex {
        std::string source_id; // 예: "TechCrunch"
        std::string content_hash; // 정보 내용의 해시
        
        bool operator<(const Vertex& other) const {
            return source_id < other.source_id;
        }
    };

    // 정보의 위상적 차원(Dimension)을 계산하는 클래스
    class SimplexEngine {
    public:
        // 주어진 정보에 대해 지지하는 출처들의 집합을 입력받음
        // 반환값: 차원 (0=점, 1=선, 2=면(진실))
        static int calculate_dimension(const std::vector<std::string>& sources) {
            // 중복 제거 및 정렬
            std::set<std::string> unique_sources(sources.begin(), sources.end());
            
            size_t n = unique_sources.size();
            
            if (n == 0) return -1; // 존재하지 않음 (Void)
            if (n == 1) return 0;  // 0-Simplex (주장, Claim)
            if (n == 2) return 1;  // 1-Simplex (상관관계, Correlation)
            if (n >= 3) return 2;  // 2-Simplex 이상 (인과관계/사실, Causality/Fact)
            
            return 0;
        }
    };
}