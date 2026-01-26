#pragma once
#pragma once
#include <expected>
#include <string>
#include <vector>
#include <map>

namespace TopoLogosKnowledge {
    struct Coding {};
    struct Bug {};
    struct Tired {};
    struct EyeStrain {};
    struct Overtime {};
    
    inline auto relation_Coding_to_Bug(Coding input) -> Bug { return Bug(); }
    inline auto relation_Bug_to_Tired(Bug input) -> std::expected<Tired, std::string> { return Tired(); }
    inline auto relation_Coding_to_EyeStrain(Coding input) -> EyeStrain { return EyeStrain(); }
    inline auto relation_EyeStrain_to_Tired(EyeStrain input) -> std::expected<Tired, std::string> { return Tired(); }
    inline auto relation_Coding_to_Overtime(Coding input) -> std::expected<Overtime, std::string> { return Overtime(); }
    inline auto relation_Overtime_to_Tired(Overtime input) -> std::expected<Tired, std::string> { return Tired(); }
    
    // Adjacency List for Dynamic Search
    inline std::map<std::string, std::vector<std::string>> get_graph() {
        std::map<std::string, std::vector<std::string>> g;
        g["Coding"].push_back("Bug");
        g["Bug"].push_back("Tired");
        g["Coding"].push_back("EyeStrain");
        g["EyeStrain"].push_back("Tired");
        g["Coding"].push_back("Overtime");
        g["Overtime"].push_back("Tired");
        return g;
    }
} // namespace
