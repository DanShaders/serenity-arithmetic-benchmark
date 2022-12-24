#define AK_DONT_REPLACE_STD
#include <algorithm>
#include <bit>
#include <cmath>
#include <ctime>
#include <iostream>
#include <random>
#include <vector>

#include <AK/FloatingPoint.h>
#include <AK/StringFloatingPointConversions.h>
#include <LibCore/ArgsParser.h>

#include <dragonbox/dragonbox.h>

template<typename T>
inline void doNotOptimizeAway(T&& datum)
{
    asm volatile(""
                 :
                 : "m"(datum)
                 : "memory");
}

template<typename FloatingPoint, typename SameSizeInteger, typename RandomGenerator>
static void benchmark(u64 count, u64 seed = 37714)
{
    RandomGenerator rnd(seed);

    std::cout << "Generating " << count << " numbers..." << std::endl;

    std::vector<FloatingPoint> numbers_to_convert(count);
    for (FloatingPoint& number : numbers_to_convert) {
        while (number == 0 || std::isinf(number) || std::isnan(number)) {
            number = std::bit_cast<FloatingPoint>(static_cast<SameSizeInteger>(rnd()));
        }
    }

    {
        std::cout << "Converting using AK::convert_floating_point_to_decimal_exponential_form..." << std::endl;

        auto start = clock();

        for (FloatingPoint number : numbers_to_convert) {
            doNotOptimizeAway(
                convert_floating_point_to_decimal_exponential_form(number));
        }
        auto finish = clock();

        double cpu_time = finish - start;
        cpu_time /= CLOCKS_PER_SEC;

        auto cpu_time_per_sample = cpu_time / count * 1e9;
        std::cout << "This took " << cpu_time << "s or " << cpu_time_per_sample << "ns/op" << std::endl;
    }

    {
        std::cout << "Converting using jkj::dragonbox::to_decimal..." << std::endl;

        auto start = clock();

        for (FloatingPoint number : numbers_to_convert) {
            doNotOptimizeAway(
                jkj::dragonbox::to_decimal(number));
        }
        auto finish = clock();

        double cpu_time = finish - start;
        cpu_time /= CLOCKS_PER_SEC;

        auto cpu_time_per_sample = cpu_time / count * 1e9;
        std::cout << "This took " << cpu_time << "s or " << cpu_time_per_sample << "ns/op" << std::endl;
    }
}

template<typename FloatingPoint, typename SameSizeInteger>
inline void check(FloatingPoint number)
{
    auto dragonbox = jkj::dragonbox::to_decimal(number);
    auto ours = convert_floating_point_to_decimal_exponential_form(number);

    if (dragonbox.is_negative != ours.sign || dragonbox.significand != ours.fraction || dragonbox.exponent != ours.exponent) {
        std::cout << "WA for " << number << " (" << std::hex << std::bit_cast<SameSizeInteger>(number) << std::dec << "):\n"
                  << "dragonbox: " << dragonbox.is_negative << " " << dragonbox.significand << " " << dragonbox.exponent << "\n"
                  << "ours:      " << ours.sign << " " << ours.fraction << " " << ours.exponent << "\n";
        exit(EXIT_FAILURE);
    }
}

static void stress_double(u64 seed = 37714)
{
    std::mt19937_64 rnd(seed);

    u64 iterations = 0;
    while (true) {
        double number = std::bit_cast<double>(rnd());
        while (number == 0 || std::isinf(number) || std::isnan(number)) {
            number = std::bit_cast<double>(rnd());
        }
        check<double, uint64_t>(number);

        ++iterations;
        if (!(iterations & ((1 << 25) - 1))) {
            std::cout << "Done " << iterations << " comparisons" << std::endl;
        }
    }
}

static void check_interestring_double()
{
    FloatExtractor<double> extractor;

    for (int exponent = 0; exponent < 2047; ++exponent) {
        std::cout << "Checking exponent=" << exponent << std::endl;
        extractor.exponent = exponent;
        for (u64 mantissa = (exponent == 0 ? 1 : 0); mantissa < (1 << 20); ++mantissa) {
            extractor.mantissa = mantissa;
            check<double, uint64_t>(extractor.d);
        }

        for (u64 mantissa = (1ULL << 52) - (1 << 20); mantissa < (1ULL << 52); ++mantissa) {
            extractor.mantissa = mantissa;
            check<double, uint64_t>(extractor.d);
        }
    }

    std::cout << "Checked successfully" << std::endl;
}

static void brute_force_float()
{
    u32 i = 0;
    do {
        float number = std::bit_cast<float>(i);
        if (std::isnan(number) || std::isinf(number) || number == 0) {
            continue;
        }

        check<float, uint32_t>(number);

        if (!(i & ((1 << 25) - 1))) {
            std::cout << "Done " << i << " comparisons" << std::endl;
        }
    } while (++i > 0);
}

int main(int argc, char** argv)
{
    std::cin.tie(0)->sync_with_stdio(0);

    u64 count = 100'000'000;
    int what = 0;
    u64 seed = 37714;
    Core::ArgsParser args_parser;
    args_parser.add_option(what, R"(What to do?
            0 = benchmark double
            1 = stress_double
            2 = check_interestring_double
            3 = brute_force_float
            4 = benchmark float)",
        "type", 't', "type");
    args_parser.add_option(count, "Number of conversions to benchmark", "number", 'n', "number");
    args_parser.add_option(seed, "Seed for random", "seed", 's', "seed");
    args_parser.parse(argc, argv);

    if (what == 0) {
        benchmark<double, uint64_t, std::mt19937_64>(count, seed);
    } else if (what == 1) {
        stress_double(seed);
    } else if (what == 2) {
        check_interestring_double();
    } else if (what == 3) {
        brute_force_float();
    } else if (what == 4) {
        benchmark<float, uint32_t, std::mt19937>(count);
    } else {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
