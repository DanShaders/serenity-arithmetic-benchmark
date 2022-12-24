#define HERE
#include <iostream>
#include <iomanip>
#include <random>
#include <chrono>

#include <gmp.h>

#define AK_DONT_REPLACE_STD
#include <AK/BigIntBase.h>

using namespace AK;

void mul_schoolbook(size_t n, auto const& data1, auto const& data2, auto& result) {
    DoubleWord carry = 0;
    for (size_t i = 0; i < 2 * n; ++i) {
        result[i] = static_cast<NativeWord>(carry);
        carry >>= word_size;

        size_t start_index = i >= n ? i - n + 1 : 0;
        size_t end_index = min(i + 1, n);

        for (size_t j = start_index; j < end_index; ++j) {
            auto x = static_cast<DoubleWord>(data1[j]) * data2[i - j];

            bool ncarry = false;
            add_words(result[i], static_cast<NativeWord>(x), result[i], ncarry);
            carry += (x >> word_size) + ncarry;
        }
    }
}

constexpr int n = 1 << 21;

u64 *buffer;
u64 ntt_result[2 * n];
u64 result[2 * n];  

static void print(size_t n, u64* const data) {
    for (size_t i = 0; i < n; ++i)
        std::cout << "(" << data[i] << "<<" << i * 64 << ")" << (i == n - 1 ? "" : "+");
    std::cout << std::endl;
}

static void check(size_t n, u64* const data1, u64* const data2) {
    auto msb_start_time = std::chrono::steady_clock::now();
    mul_schoolbook(n, data1, data2, result);
    auto msb_end_time = std::chrono::steady_clock::now();

    auto ntt_start_time = std::chrono::steady_clock::now();
    storage_mul_smart(data1, n, data2, n, ntt_result, 2 * n, buffer);
    auto ntt_end_time = std::chrono::steady_clock::now();

    std::cout << "n=" << n << std::endl;
    std::cout << std::setprecision(6) << std::fixed << "Schoolbook: " << std::chrono::duration<double>(msb_end_time - msb_start_time).count() << "\n"
              << "NTT:        " << std::chrono::duration<double>(ntt_end_time - ntt_start_time).count() << std::endl;

    bool ok = true;
    for (size_t i = 0; i < 2 * n; ++i) {
        ok &= result[i] == ntt_result[i];
    }

    if (!ok) {
        std::cout << "================== WA! ===================\n";
        // print(n, data1);
        // print(n, data2);
        // print(2 * n, result);
        // print(2 * n, ntt_result);
    }
}

u64 operand1[n]{};
u64 operand2[n]{};
std::mt19937_64 rnd;

static void check_random(size_t n) {
    for (size_t i = 0; i < n; ++i) {
        operand1[i] = rnd();
        operand2[i] = rnd();
    }
    check(n, operand1, operand2);
}

int main()
{
    buffer = new u64[n * 33];

    check_random(1 << 4);
    check_random(1 << 4);
    check_random(1 << 5);
    check_random(1 << 5);
    check_random(1 << 6);
    check_random(1 << 6);
    check_random(1 << 7);
    check_random(1 << 7);
    check_random(1 << 8);
    check_random(1 << 8);
    check_random(1 << 9);
    check_random(1 << 9);
    check_random(1 << 10);
    check_random(1 << 10);
    check_random(1 << 11);
    check_random(1 << 11);
    check_random(1 << 12);
    check_random(1 << 12);
    check_random(1 << 13);
    check_random(1 << 13);
    check_random(1 << 14);
    check_random(1 << 14);

    // for (size_t i = 0; i < n; ++i)
    //     std::cout << operand1[i] << " ";
    // std::cout << std::endl;

    // for (size_t i = 0; i < n; ++i)
    //     std::cout << operand2[i] << " ";
    // std::cout << std::endl;

    std::cout << "checking performace" << std::endl;

    for (size_t i = 0; i < n; ++i) {
        operand1[i] = rnd();
        operand2[i] = rnd();
    }

    auto start = std::chrono::high_resolution_clock::now();
    storage_mul_smart(operand1, n, operand2, n, ntt_result, 2 * n, buffer);
    auto end = std::chrono::high_resolution_clock::now();

    mpz_t a, b, c;
    mpz_init(a);
    mpz_init(b);
    mpz_init(c);
    mpz_import(a, n, -1, 8, 0, 0, operand1);
    mpz_import(b, n, -1, 8, 0, 0, operand2);

    auto gmp_start = std::chrono::steady_clock::now();
    mpz_mul(c, a, b);
    auto gmp_end = std::chrono::steady_clock::now();

    std::cout << n * 64 << " bits multiplication took " << std::endl;
    std::cout << "ours: " << std::chrono::duration<double>(end - start).count() << "s" << std::endl;
    std::cout << "gmp:  " << std::chrono::duration<double>(gmp_end - gmp_start).count() << "s" << std::endl;

    std::fill_n(result, 2 * n, 0);
    mpz_export(result, nullptr, -1, 8, 0, 0, c);

    for (size_t i = 0; i < 2 * n; ++i)
        assert(result[i] == ntt_result[i]);
}
