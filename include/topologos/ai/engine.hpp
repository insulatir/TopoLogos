#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "onnxruntime_cxx_api.h"

namespace topologos::ai {

    struct EngineConfig {
        std::string model_path;
        std::string vocab_path;
    };

    class NLIEngine {
    public:
        explicit NLIEngine(const EngineConfig& config);
        ~NLIEngine();

        std::vector<float> get_embedding(const std::string& text);

    private:
        Ort::Env env_;
        std::shared_ptr<Ort::Session> session_;
        std::map<std::string, int64_t> vocab_;
        
        // [수정] 입력/출력 이름을 모두 동적으로 관리
        std::vector<char*> input_node_names_;
        std::vector<char*> output_node_names_;
    };
}