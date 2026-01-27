#include "../../include/topologos/verification/bullshit_detector.hpp"
#include <fstream>
#include <iostream>
#include <cmath>

using json = nlohmann::json;
using namespace std;

namespace topologos::verification {

    BullshitDetector::BullshitDetector(const string& rule_file_path) {
        load_rules(rule_file_path);
    }

    void BullshitDetector::load_rules(const string& filepath) {
        // 현재는 파일 존재 여부만 확인하고 기본값 사용 (추후 파서 연동 가능)
        ifstream f(filepath);
        if (f.good()) {
            cout << "[Detector] Laws loaded from: " << filepath << endl;
        } else {
            cerr << "[Detector] Warning: Rule file not found (" << filepath << "). Using defaults." << endl;
        }
    }

    Verdict BullshitDetector::judge(const json& data) {
        Verdict v;
        v.violations.clear();
        double score = 100.0;
        
        // JSON 데이터 파싱 (miner.py의 출력 형식에 의존)
        // 데이터가 없으면 예외 처리가 필요할 수 있음
        if (!data.contains("attributes")) {
             v.is_truth = false;
             v.integrity_score = 0.0;
             v.violations.push_back("Invalid Data Format: 'attributes' missing");
             return v;
        }

        auto& attr = data["attributes"];
        
        // 1. Physics (물리)
        double input_energy = attr["physics"].value("input_energy", 0.0);
        double promised_reward = attr["physics"].value("promised_reward", 0.0);
        double clarity = attr["physics"].value("text_clarity_score", 0.0);
        
        // 2. Psychology (심리)
        int urgency = attr["psychology"].value("urgency_keywords_count", 0);
        bool cta = attr["psychology"].value("call_to_action", false);
        int absolutism = attr["psychology"].value("absolute_terms_count", 0);
        
        // 3. Structure (구조)
        auto sources = attr["structure"]["dependency_sources"].get<vector<string>>();
        bool circular = attr["structure"].value("circular_reasoning_detected", false);

        // --- 검증 로직 (The Sieve) ---

        // [물리] 열역학 위반 (공짜 점심)
        if (input_energy > 0 && promised_reward > (input_energy * THERMO_THRESHOLD)) {
            v.violations.push_back("[PHYSICS] Thermodynamics Violation: Impossible ROI");
            score -= 40.0;
        }
        // [물리] 엔트로피 (모호함)
        if (clarity < ENTROPY_CLARITY_MIN) {
            v.violations.push_back("[PHYSICS] High Entropy: Text is too vague");
            score -= 20.0;
        }

        // [심리] 약탈적 의도 (공포 마케팅)
        if (urgency > MAX_URGENCY_COUNT && cta) {
            v.violations.push_back("[PSYCH] Predatory Intent: Urgency trap detected");
            score -= 30.0;
        }
        // [심리] 인지 편향
        if (absolutism > MAX_ABSOLUTISM_COUNT) {
            v.violations.push_back("[PSYCH] Cognitive Bias: Absolutist language");
            score -= 10.0;
        }

        // [구조] 취약성 (SPOF)
        if (sources.size() < MIN_SOURCES) {
            v.violations.push_back("[STRUCTURE] Fragility: Single Point of Failure (SPOF)");
            score -= 25.0;
        }
        // [구조] 순환 논리
        if (circular) {
            v.violations.push_back("[STRUCTURE] Logical Fallacy: Circular reasoning");
            score -= 50.0;
        }

        v.integrity_score = max(0.0, score);
        v.is_truth = (v.integrity_score >= 80.0); // 80점 이상만 진실로 인정
        
        return v;
    }

} // namespace topologos::verification