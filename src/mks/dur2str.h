//
// Created by Michal Němec on 08/08/2020.
//

#ifndef MKS_DUR2STR_H
#define MKS_DUR2STR_H

#include <chrono>
#include <iomanip>
#include <sstream>

/**
 * String format std::chrono::duration<> to human readable string with automatic scaling and without any compression
 * Resolution supported up to nanoseconds
 * When formated time has trailing zeros, they re automatically discarded
 *
 * Example usage:
 *      auto start = std::chrono::high_resolution_clock::now();
 *      .. some computation ...
 *      auto start = std::chrono::high_resolution_clock::now();
 *      auto printable_string = dur2str(end - start);
 *
 * @tparam Rep
 * @tparam Period
 * @param diff
 * @return duration format: hh::mm::ss.mmm'uuu'nnnn, trailing zeroes are omitted
 */
template <class Rep, class Period>
std::string dur2str(const std::chrono::duration<Rep, Period> &diff) {
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(diff);
    auto wall_time = ns.count();
    if(wall_time == 0) {
        return "0 ns";
    }
    auto nano = wall_time % 1000LL;
    auto micro = (wall_time / 1000LL) % 1000LL;
    auto milli = (wall_time / 1000000LL) % 1000LL;
    auto secs_base = wall_time / 1000000000LL;
    auto hours = secs_base / 3600LL;
    auto minutes = (secs_base / 60LL) % 60LL;
    auto seconds = secs_base % 60LL;

    bool enable_tm[6];
    bool enable_tm_back[6];
    int64_t enable_tm_vals[6] = {nano, micro, milli, seconds, minutes, hours};
    for(int i = 0; i != 6; ++i) {
        enable_tm_back[i] = (enable_tm_vals[i] == 0);
        enable_tm[i] = (enable_tm_vals[i] > 0);
    }
    const char *enable_tm_end[6] = {"ns", "us", "ms", "s", "min", "h"};
    uint8_t enable_tm_sizes[6] = {3, 3, 3, 2, 2, 0};
    uint8_t enable_tm_separator[6] = {'\0', '\'', '\'', '.', ':', ':'};

    // formatting is done in format
    // format: hh::mm::ss.mmm'uuu'nnnn
    //    idx:  5   4   3   2   1    0
    // we want to exclude highest zero values
    {
        // enable_tm contains all positions that will be formatted
        // eg. 00:30:00.333222000 enable_tm would contain
        // enable_tm[0] = false
        // enable_tm[1] = true
        // enable_tm[2] = true
        // enable_tm[3] = false
        // enable_tm[4] = true
        // enable_tm[5] = false
        //
        // after loop we set true value to all before minutes
        // enable_tm[0] = true
        // enable_tm[1] = true
        // enable_tm[2] = true
        // enable_tm[3] = true
        // enable_tm[4] = true
        // enable_tm[5] = false
        //
        // these values will be formatted
        auto prev = enable_tm[5];
        for(uint8_t i = 4; i != 0; i--) {
            if(prev) {
                enable_tm[i] = true;
            }
            prev = enable_tm[i];
        }
    }

    // find first nonzero element in time elements
    // format: hh::mm::ss.mmm'uuu'nnnn
    //    idx:  5   4   3   2   1    0
    // if nanosecods and microseconds are zero then we want to exclude them
    // stop_i would be 2
    // from previous example if we are printing 00:30:00.333222000
    // we would format only values 30:00.333222
    //  ns: enable_tm[0] = true -> nanoseconds contains zero
    //  us: enable_tm[1] = true -> format only microseconds to minutes
    //  ms: enable_tm[2] = true
    //   s: enable_tm[3] = true
    // min: enable_tm[4] = true
    //   h: enable_tm[5] = false
    uint8_t stop_i = 0;
    for(; stop_i != 6; stop_i++) {
        if(!enable_tm_back[stop_i])
            break;
    }

    std::ostringstream oss;
    {
        auto prev = false;
        const char *end = nullptr;
        for(int8_t i = 5; i != -1; i--) {
            if(enable_tm[i]) {
                char sep = enable_tm_separator[i];
                if(!prev) {
                    oss << enable_tm_vals[i];
                    if(sep == '\'') {
                        // separator '\'' is for numbers behand decimal place
                        // if time is sub-seconds we want to use '.' instead
                        // eg. 32'333 us -> 32.333 us
                        sep = '.';
                    }
                    end = enable_tm_end[i];
                } else {
                    // i < 3 is for nano, micro, mili sec (idx 0,1,2)
                    if(i < 3 && i == stop_i) {
                        // when formating last element behind decimal place
                        // cutoff all zeroes at the end
                        // 1.101'100 ms -> 1.101'1 ms
                        auto reduced = enable_tm_vals[i];
                        int places_zero = 3;
                        while(places_zero != 0) {
                            if(reduced % 10 == 0) {
                                reduced /= 10;
                                --places_zero;
                            } else {
                                break;
                            }
                        }
                        if(places_zero > 0) {
                            oss << std::setfill('0') << std::setw(places_zero) << reduced;
                            // oss << fmt::format("{:0>{}}", reduced, places_zero);
                        }
                    } else {
                        oss << std::setfill('0') << std::setw(enable_tm_sizes[i]) << enable_tm_vals[i];
                        // oss << fmt::format("{:0>{}}", enable_tm_vals[i], enable_tm_sizes[i]);
                    }
                }
                if(i > stop_i) {
                    if(sep != '\0') {
                        oss << sep;
                    }
                }
            }
            if(i == stop_i) {
                break;
            }
            prev = enable_tm[i];
        }
        if(end != nullptr) {
            oss << " " << end;
        } else {
            // should not happen
            oss << "0 ns";
        }
    }
    return oss.str();
}

#endif // MKS_DUR2STR_H
