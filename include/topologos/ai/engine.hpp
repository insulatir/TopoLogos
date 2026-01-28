#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
//#include <expected>
#include "onnxruntime_cxx_api.h"

namespace topologos::ai {

    enum class LogicResult {
        ENTAILMENT, // Strong
        NEUTRAL,    // Weak
        CONTRADICTION
    };

    // [New] 설정을 위한 구조체 (하드코딩 제거)
    struct EngineConfig {
        std::string model_path = "bert_nli.onnx";
        std::string vocab_path = "vocab.txt";
        float threshold = 2.5f; // 임계값 외부화
    };

    class NLIEngine {
    public:
        // 생성자가 이제 Config 구조체를 받습니다.
        explicit NLIEngine(const EngineConfig& config);
        
        // 문장을 받아 논리적 관계 판단 (내부에서 전처리 수행)
        auto predict(const std::string& premise, const std::string& hypothesis) -> LogicResult;

        // [New] 임베딩 추출 기능 추가
        std::vector<float> get_embedding(const std::string& text);

    private:
        EngineConfig config_; // 설정 저장

        // --- ONNX Resources ---
        Ort::Env env_;
        Ort::Session session_{nullptr};
        Ort::AllocatorWithDefaultOptions allocator_;

        // --- Tokenizer Resources ---
        std::map<std::string, int64_t> vocab_;
        int64_t unk_token_ = 100;
        int64_t cls_token_ = 101;
        int64_t sep_token_ = 102;

        void load_vocab(const std::string& path);
        
        // [Refactoring] 더 똑똑해진 토크나이저
        // 1. 소문자 변환
        // 2. 특수문자 제거 (.,!?)
        // 3. 토큰 ID 변환
        std::vector<int64_t> tokenize_and_encode(const std::string& text);
    };
}