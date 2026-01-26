#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>
#include "../knowledge.hpp"

namespace TK = TopoLogosKnowledge;

std::string verify_truth(std::string claim_type) {
    // 실제로는 지식 그래프의 속성(Attribute)을 읽어와야 하지만,
    // 여기서는 로직을 시뮬레이션합니다.

    std::string physical = "OK";
    std::string mental = "LOW";
    std::string structural = "SAFE";

    if (claim_type == "GetRichQuick") {
        physical = "BAD";      // 엔트로피 위배 (노력 < 보상)
        mental = "HIGH";       // 조급함 유발
        structural = "CRITICAL"; // 실패 시 파산
    } 
    else if (claim_type == "CodingToTired") {
        physical = "OK";       // 에너지 썼으니 피곤함 (정상)
        mental = "MEDIUM";     // 스트레스 있음
        structural = "SAFE";   // 피곤하다고 죽진 않음 (휴식하면 회복)
    }

    // [핵심 로직] 삼각 측량
    if (physical == "BAD" || mental == "HIGH" || structural == "CRITICAL") {
        return "🚨 SCAM DETECTED";
    } else {
        return "✅ VERIFIED TRUTH";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: ./GraphApp [Start] [Target]\n";
        std::cerr << "Example: ./GraphApp Coding Tired\n";
        return 1;
    }

    std::string start_node = argv[1];
    std::string target_node = argv[2];

    std::cout << "[GraphRAG] Searching path: " << start_node << " -> " << target_node << "...\n\n";

    // 1. 지식 그래프 로드 (동적 데이터)
    auto graph = TK::get_graph();

    // 2. BFS 탐색 (길찾기)
    std::queue<std::pair<std::string, std::vector<std::string>>> q;
    q.push({start_node, {start_node}});

    std::vector<std::string> found_path;
    bool success = false;

    while (!q.empty()) {
        auto [current, path] = q.front();
        q.pop();

        if (current == target_node) {
            found_path = path;
            success = true;
            break;
        }

        // 다음 연결된 노드들 확인
        if (graph.find(current) != graph.end()) {
            for (const auto& neighbor : graph[current]) {
                // (사이클 방지 로직은 간단히 생략하거나 추가 가능)
                std::vector<std::string> new_path = path;
                new_path.push_back(neighbor);
                // 명시적으로 pair를 만들어서 push
                q.push(std::make_pair(neighbor, new_path));
            }
        }
    }

    // 3. 결과 출력
    if (success) {
        std::cout << "✅ Path Found!\n";
        for (size_t i = 0; i < found_path.size(); ++i) {
            std::cout << found_path[i];
            if (i < found_path.size() - 1) std::cout << " -> ";
        }
        std::cout << "\n\n[Analysis] Based on independent community consensus.\n";
    } else {
        std::cout << "❌ No logical path found between '" << start_node << "' and '" << target_node << "'.\n";
        std::cout << "(Try adding more knowledge to life.topo)\n";
    }

    std::cout << "\n=== [Truth Triangulation Engine] ===\n";

    // Case 1: "코딩하면 피곤하다" (우리가 합의한 진실)
    std::cout << "Testing Claim: 'Coding makes you Tired'\n";
    std::cout << "Result: " << verify_truth("CodingToTired") << "\n\n";

    // Case 2: "코딩하면 금방 부자된다" (인터넷의 흔한 사기)
    std::cout << "Testing Claim: 'Coding makes you Rich Quickly'\n";
    std::cout << "Result: " << verify_truth("GetRichQuick") << "\n";
    
    return 0;

    return 0;
}