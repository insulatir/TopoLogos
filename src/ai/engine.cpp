#include "topologos/ai/engine.hpp"
#include <iostream>
#include <fstream>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <cstring> 

namespace topologos::ai {

    std::vector<int64_t> tokenize(const std::string& text, const std::map<std::string, int64_t>& vocab) {
        std::vector<int64_t> tokens;
        tokens.push_back(101); // [CLS]
        for (char c : text) {
            if (tokens.size() >= 510) break;
            tokens.push_back(static_cast<int64_t>(c) % 30000 + 100); 
        }
        tokens.push_back(102); // [SEP]
        return tokens;
    }

    NLIEngine::NLIEngine(const EngineConfig& config) : env_(ORT_LOGGING_LEVEL_WARNING, "TopoLogosEngine") {
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

        session_ = std::make_shared<Ort::Session>(env_, config.model_path.c_str(), session_options);
        
        Ort::AllocatorWithDefaultOptions allocator;

        // [New] 입력 노드 이름 동적 감지
        size_t num_input_nodes = session_->GetInputCount();
        input_node_names_.clear();
        for(size_t i = 0; i < num_input_nodes; i++) {
            auto input_name = session_->GetInputNameAllocated(i, allocator);
            input_node_names_.push_back(strdup(input_name.get()));
        }

        // 출력 노드 이름 동적 감지
        size_t num_output_nodes = session_->GetOutputCount();
        output_node_names_.clear();
        for(size_t i = 0; i < num_output_nodes; i++) {
            auto output_name = session_->GetOutputNameAllocated(i, allocator);
            output_node_names_.push_back(strdup(output_name.get())); 
        }
        
        std::cout << "[Engine] Model Loaded. Inputs: " << input_node_names_.size() 
                  << ", Outputs: " << output_node_names_.size() << std::endl;
    }

    NLIEngine::~NLIEngine() {
        for(auto name : input_node_names_) free(name);
        for(auto name : output_node_names_) free(name);
    }

    std::vector<float> NLIEngine::get_embedding(const std::string& text) {
        if (!session_) return {};

        // 1. 데이터 준비
        std::vector<int64_t> input_ids = tokenize(text, vocab_);
        std::vector<int64_t> input_mask(input_ids.size(), 1);
        std::vector<int64_t> segment_ids(input_ids.size(), 0); // token_type_ids
        
        size_t batch_size = 1;
        size_t seq_len = input_ids.size();
        std::vector<int64_t> dims = {static_cast<int64_t>(batch_size), static_cast<int64_t>(seq_len)};

        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        
        // 2. [핵심] 모델이 요구하는 입력 순서대로 텐서 배치
        std::vector<Ort::Value> input_tensors;
        
        for(const char* name : input_node_names_) {
            std::string sname = name;
            if (sname == "input_ids") {
                input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(memory_info, input_ids.data(), input_ids.size(), dims.data(), 2));
            } else if (sname == "attention_mask") {
                input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(memory_info, input_mask.data(), input_mask.size(), dims.data(), 2));
            } else if (sname == "token_type_ids") {
                // 모델이 요구할 때만 추가!
                input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(memory_info, segment_ids.data(), segment_ids.size(), dims.data(), 2));
            } else {
                // 혹시 모를 다른 입력은 무시하거나 0으로 채워야 함 (여기선 생략)
                std::cerr << "[Warning] Unknown input required: " << sname << std::endl;
            }
        }

        // 3. 실행
        // input_node_names_의 순서와 input_tensors의 순서가 정확히 일치함
        auto output_tensors = session_->Run(
            Ort::RunOptions{nullptr},
            input_node_names_.data(), 
            input_tensors.data(),
            input_tensors.size(), 
            output_node_names_.data(), 
            1 
        );

        // 4. 결과 처리 (Mean Pooling)
        float* floatarr = output_tensors[0].GetTensorMutableData<float>();
        auto type_info = output_tensors[0].GetTensorTypeAndShapeInfo();
        auto shape = type_info.GetShape();
        
        int hidden_size = (shape.size() > 2) ? shape[2] : shape[1]; 
        std::vector<float> embedding(hidden_size, 0.0f);
        
        if (shape.size() > 2) {
            for (size_t i = 0; i < seq_len; ++i) {
                for (int j = 0; j < hidden_size; ++j) {
                    embedding[j] += floatarr[i * hidden_size + j];
                }
            }
            for (float& val : embedding) val /= seq_len;
        } else {
            for (int j = 0; j < hidden_size; ++j) {
                embedding[j] = floatarr[j];
            }
        }

        // L2 Normalize
        float norm = 0.0f;
        for (float val : embedding) norm += val * val;
        norm = std::sqrt(norm);
        if (norm > 1e-6) {
            for (float& val : embedding) val /= norm;
        }

        return embedding;
    }
}