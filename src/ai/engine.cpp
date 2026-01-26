#include "topologos/ai/engine.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cctype> // for std::isalnum, std::tolower

namespace topologos::ai {

    NLIEngine::NLIEngine(const EngineConfig& config)
        : config_(config), 
          env_(ORT_LOGGING_LEVEL_WARNING, "TopoLogosAI") {
        
        // 1. 모델 로드
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_ = Ort::Session(env_, config_.model_path.c_str(), session_options);

        // 2. 단어장 로드
        load_vocab(config_.vocab_path);
    }

    void NLIEngine::load_vocab(const std::string& path) {
        std::ifstream file(path);
        std::string line;
        int64_t index = 0;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            vocab_[line] = index++;
        }
        if (vocab_.count("[UNK]")) unk_token_ = vocab_["[UNK]"];
        if (vocab_.count("[CLS]")) cls_token_ = vocab_["[CLS]"];
        if (vocab_.count("[SEP]")) sep_token_ = vocab_["[SEP]"];
        
        std::cout << "[AI] Engine Loaded. Threshold: " << config_.threshold << std::endl;
    }

    // [New] 텍스트 정규화 및 토큰화 (리팩토링 핵심)
    std::vector<int64_t> NLIEngine::tokenize_and_encode(const std::string& text) {
        std::vector<int64_t> ids;
        std::string current_word;
        
        // 한 글자씩 읽으면서 전처리
        for (char c : text) {
            // 1. 소문자로 변환
            char lower_c = std::tolower(static_cast<unsigned char>(c));

            // 2. 알파벳이나 숫자면 단어에 추가
            if (std::isalnum(lower_c)) {
                current_word += lower_c;
            } 
            // 3. 공백이나 특수문자를 만나면 단어 끝으로 간주
            else {
                if (!current_word.empty()) {
                    // 단어장에 검색
                    if (vocab_.count(current_word)) {
                        ids.push_back(vocab_[current_word]);
                    } else {
                        ids.push_back(unk_token_);
                    }
                    current_word.clear();
                }
                // (특수문자는 그냥 무시하고 넘어감 -> Cleaning 효과)
            }
        }

        // 마지막 단어 처리
        if (!current_word.empty()) {
            if (vocab_.count(current_word)) ids.push_back(vocab_[current_word]);
            else ids.push_back(unk_token_);
        }

        return ids;
    }

    auto NLIEngine::predict(const std::string& premise, const std::string& hypothesis) -> LogicResult {
        std::vector<int64_t> input_ids;
        input_ids.push_back(cls_token_);
        
        // [변경] 더 강력해진 토크나이저 호출
        auto p_tokens = tokenize_and_encode(premise);
        input_ids.insert(input_ids.end(), p_tokens.begin(), p_tokens.end());
        input_ids.push_back(sep_token_);

        auto h_tokens = tokenize_and_encode(hypothesis);
        input_ids.insert(input_ids.end(), h_tokens.begin(), h_tokens.end());
        input_ids.push_back(sep_token_);

        size_t seq_len = input_ids.size();
        std::vector<int64_t> token_type_ids(seq_len, 0);
        std::vector<int64_t> attention_mask(seq_len, 1);

        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<int64_t> input_shape = {1, (int64_t)seq_len};

        std::vector<Ort::Value> input_tensors;
        input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(memory_info, input_ids.data(), seq_len, input_shape.data(), 2));
        input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(memory_info, token_type_ids.data(), seq_len, input_shape.data(), 2));
        input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(memory_info, attention_mask.data(), seq_len, input_shape.data(), 2));

        const char* input_names[] = {"input_ids", "token_type_ids", "attention_mask"};
        const char* output_names[] = {"logits"};

        auto output_tensors = session_.Run(
            Ort::RunOptions{nullptr}, 
            input_names, input_tensors.data(), 3, 
            output_names, 1
        );

        float* logits = output_tensors.front().GetTensorMutableData<float>();
        
        int max_idx = 0;
        float max_val = logits[0];
        for(int i=1; i<3; ++i) {
            if(logits[i] > max_val) {
                max_val = logits[i];
                max_idx = i;
            }
        }

        // [Refactoring] Config에서 Threshold 가져와서 판단
        // Entailment(0)라도 점수가 낮으면 Neutral 취급
        if (max_idx == 0) {
            if (max_val > config_.threshold) {
                return LogicResult::ENTAILMENT;
            } else {
                return LogicResult::NEUTRAL;
            }
        }
        
        if (max_idx == 1) return LogicResult::NEUTRAL;
        return LogicResult::CONTRADICTION;
    }
}