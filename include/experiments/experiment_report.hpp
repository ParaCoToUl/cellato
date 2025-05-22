#ifndef CELLIB_EXPERIMENT_REPORT_HPP
#define CELLIB_EXPERIMENT_REPORT_HPP

#include "./run_params.hpp"

namespace cellib::run {

struct experiment_report {
    run_params params;

    std::vector<double> execution_times_ms;
    std::vector<std::string> checksums;

    void pretty_print() const {
        std::cout << "Execution Times (ms): ";
        for (const auto& time : execution_times_ms) {
            std::cout << time << " ";
        }
        std::cout << "\nChecksums: ";
        for (const auto& checksum : checksums) {
            std::cout << checksum << " ";
        }
        std::cout << std::endl;
    }

    std::string csv_line() const {
        
        return std::to_string(params.x_size) + "," +
               std::to_string(params.y_size) + "," +
               std::to_string(params.steps) + "," +
               std::to_string(params.rounds) + "," +
               std::to_string(params.warmup_rounds) + "," +
               std::to_string(execution_times_ms[0]) + "," +
               checksums[0];
    }
};

}

#endif // CELLIB_EXPERIMENT_REPORT_HPP