/*
 * shakuni -- model counter and uniform sampler testing framework
 *            Based on the SHA-1 generator by Vegard Nossum
 *
 *            Thanks Vegard! You didn't get enough praise from the SHA-1 break
 *            people, but what can you expect, they are from Google.
 *
 * Copyright (C) 2011-2012  Vegard Nossum <vegard.nossum@gmail.com>
 *                          Mate Soos <soos.mate@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/program_options.hpp>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "format.hh"

#if defined(_MSC_VER)
#define my_popcnt(x) __popcnt(x)
#else
#define my_popcnt(x) __builtin_popcount(x)
#endif

using std::cout;
using std::cerr;
using std::endl;

/* Instance options */
static std::string config_attack = "preimage";
static unsigned int config_nr_rounds = 30;
static unsigned int config_nr_message_bits = 495;
static unsigned int config_nr_hash_bits = 6;
static unsigned int config_easy_sol_bits = 11;
static bool all_zero_output = false;

/* CNF options */
static bool config_use_tseitin_adders = true;
static bool nocomment = false;

//Counting
int var_switch[1];
std::stringstream indep_vars;
std::vector<int> message_bit_vars;
std::vector<int> input;
std::vector<int> output;
std::vector<uint32_t> message_bits_fixed;
std::vector<uint32_t> hash_bits_fixed;

static std::ostringstream cnf;

static void comment(std::string str)
{
    if (!nocomment) {
        cnf << format("c $\n", str);
    }
}

static int nr_variables = 0;
static unsigned int nr_clauses = 0;
static unsigned int nr_constraints = 0;

static void new_vars(std::string label, int x[], unsigned int n)
{
    for (unsigned int i = 0; i < n; ++i)
        x[i] = ++nr_variables;

    comment(format("var $/$ $", x[0], n, label));
}

template <typename T>
static void args_to_vector(std::vector<T> & /*v*/)
{
}

template <typename T, typename... Args>
static void args_to_vector(std::vector<T> &v, T x, Args... args)
{
    v.push_back(x);
    return args_to_vector(v, args...);
}

static void clause(const std::vector<int> &v)
{
    for (int x: v) {
        cnf << format("$$ ", x < 0 ? "-" : "", abs(x));
    }

    cnf << format("0\n");

    nr_clauses += 1;
    nr_constraints += 1;
}

template <typename... Args>
static void clause_noswitch(Args... args)
{
    std::vector<int> v;
    args_to_vector(v, args...);
    clause(v);
}

template <typename... Args>
static void clause(Args... args)
{
    std::vector<int> v;
    v.push_back(var_switch[0]);
    args_to_vector(v, args...);
    clause(v);
}

static void constant(int r, bool value)
{
    clause(value ? r : -r);
}

static void constant32(int r[], uint32_t value)
{
    comment(format("constant32 ($)", value));

    for (unsigned int i = 0; i < 32; ++i) {
        clause(((value >> i) & 1) ? r[i] : -r[i]);
    }
}

static void new_constant(std::string label, int r[32], uint32_t value)
{
    new_vars(label, r, 32);
    constant32(r, value);
}


static void xor2(int r[], int a[], int b[], unsigned int n)
{
    comment("xor2");

    for (unsigned int i = 0; i < n; ++i) {
        for (unsigned int j = 0; j < 8; ++j) {
            if (my_popcnt(j ^ 1) % 2 == 1)
                continue;

            clause((j & 1) ? -r[i] : r[i], (j & 2) ? a[i] : -a[i], (j & 4) ? b[i] : -b[i]);
        }
    }
}

static void xor3(int r[], int a[], int b[], int c[], unsigned int n = 32)
{
    comment("xor3");
    for (unsigned int i = 0; i < n; ++i) {
        for (unsigned int j = 0; j < 16; ++j) {
            if (my_popcnt(j ^ 1) % 2 == 0)
                continue;

            clause((j & 1) ? -r[i] : r[i], (j & 2) ? a[i] : -a[i], (j & 4) ? b[i] : -b[i],
                   (j & 8) ? c[i] : -c[i]);
        }
    }
}

static void xor4(int r[32], int a[32], int b[32], int c[32], int d[32])
{
    comment("xor4");
    for (unsigned int i = 0; i < 32; ++i) {
        for (unsigned int j = 0; j < 32; ++j) {
            if (my_popcnt(j ^ 1) % 2 == 1)
                continue;

            clause((j & 1) ? -r[i] : r[i], (j & 2) ? a[i] : -a[i], (j & 4) ? b[i] : -b[i],
                   (j & 8) ? c[i] : -c[i], (j & 16) ? d[i] : -d[i]);
        }
    }
}

static void and2(int r[], int a[], int b[], unsigned int n)
{
    for (unsigned int i = 0; i < n; ++i) {
        clause(r[i], -a[i], -b[i]);
        clause(-r[i], a[i]);
        clause(-r[i], b[i]);
    }
}

static void or2(int r[], int a[], int b[], unsigned int n)
{
    for (unsigned int i = 0; i < n; ++i) {
        clause(-r[i], a[i], b[i]);
        clause(r[i], -a[i]);
        clause(r[i], -b[i]);
    }
}

static void add2(std::string /*label*/, int r[32], int a[32], int b[32])
{
    comment("add2");

    if (config_use_tseitin_adders) {
        int c[31];
        new_vars("carry", c, 31);

        int t0[31];
        new_vars("t0", t0, 31);

        int t1[31];
        new_vars("t1", t1, 31);

        int t2[31];
        new_vars("t2", t2, 31);

        and2(c, a, b, 1);
        xor2(r, a, b, 1);

        xor2(t0, &a[1], &b[1], 31);
        and2(t1, &a[1], &b[1], 31);
        and2(t2, t0, c, 31);
        or2(&c[1], t1, t2, 30);
        xor2(&r[1], t0, c, 31);
    }
}

static void add5(std::string label, int r[32], int a[32], int b[32], int c[32], int d[32],
                 int e[32])
{
    comment("add5");

    if (config_use_tseitin_adders) {
        int t0[32];
        new_vars("t0", t0, 32);

        int t1[32];
        new_vars("t1", t1, 32);

        int t2[32];
        new_vars("t2", t2, 32);

        add2(label, t0, a, b);
        add2(label, t1, c, d);
        add2(label, t2, t0, t1);
        add2(label, r, t2, e);
    } else {
        assert(false);
    }
}

static void rotl(int r[32], int x[32], unsigned int n)
{
    for (unsigned int i = 0; i < 32; ++i)
        r[i] = x[(i + 32 - n) % 32];
}

class sha1
{
   public:
    int w[80][32];
    int h_in[5][32];
    int h_out[5][32];

    int a[85][32];

    sha1(unsigned int nr_rounds, std::string name)
    {
        comment("sha1");
        comment(format("parameter nr_rounds = $", nr_rounds));

        for (unsigned int i = 0; i < 16; ++i)
            new_vars(format("w$[$]", name, i), w[i], 32);

        /* XXX: Fix this later by writing directly to w[i] */
        int wt[80][32];
        for (unsigned int i = 16; i < nr_rounds; ++i)
            new_vars(format("w$[$]", name, i), wt[i], 32);

        new_vars(format("h$_in0", name), h_in[0], 32);
        new_vars(format("h$_in1", name), h_in[1], 32);
        new_vars(format("h$_in2", name), h_in[2], 32);
        new_vars(format("h$_in3", name), h_in[3], 32);
        new_vars(format("h$_in4", name), h_in[4], 32);

        new_vars(format("h$_out0", name), h_out[0], 32);
        new_vars(format("h$_out1", name), h_out[1], 32);
        new_vars(format("h$_out2", name), h_out[2], 32);
        new_vars(format("h$_out3", name), h_out[3], 32);
        new_vars(format("h$_out4", name), h_out[4], 32);

        for (unsigned int i = 0; i < nr_rounds; ++i)
            new_vars(format("a[$]", i + 5), a[i + 5], 32);

        for (unsigned int i = 16; i < nr_rounds; ++i) {
            xor4(wt[i], w[i - 3], w[i - 8], w[i - 14], w[i - 16]);
            rotl(w[i], wt[i], 1);
        }

        /* Fix constants */
        int k[4][32];
        new_constant("k[0]", k[0], 0x5a827999);
        new_constant("k[1]", k[1], 0x6ed9eba1);
        new_constant("k[2]", k[2], 0x8f1bbcdc);
        new_constant("k[3]", k[3], 0xca62c1d6);

        constant32(h_in[0], 0x67452301);
        constant32(h_in[1], 0xefcdab89);
        constant32(h_in[2], 0x98badcfe);
        constant32(h_in[3], 0x10325476);
        constant32(h_in[4], 0xc3d2e1f0);

        rotl(a[4], h_in[0], 32 - 0);
        rotl(a[3], h_in[1], 32 - 0);
        rotl(a[2], h_in[2], 32 - 30);
        rotl(a[1], h_in[3], 32 - 30);
        rotl(a[0], h_in[4], 32 - 30);

        for (unsigned int i = 0; i < nr_rounds; ++i) {
            int prev_a[32];
            rotl(prev_a, a[i + 4], 5);

            int b[32];
            rotl(b, a[i + 3], 0);

            int c[32];
            rotl(c, a[i + 2], 30);

            int d[32];
            rotl(d, a[i + 1], 30);

            int e[32];
            rotl(e, a[i + 0], 30);

            int f[32];
            new_vars(format("f[$]", i), f, 32);

            if (i < 20) {
                for (unsigned int j = 0; j < 32; ++j) {
                    clause(-f[j], -b[j], c[j]);
                    clause(-f[j], b[j], d[j]);
                    clause(-f[j], c[j], d[j]);

                    clause(f[j], -b[j], -c[j]);
                    clause(f[j], b[j], -d[j]);
                    clause(f[j], -c[j], -d[j]);
                }
            } else if (i >= 20 && i < 40) {
                xor3(f, b, c, d);
            } else if (i >= 40 && i < 60) {
                for (unsigned int j = 0; j < 32; ++j) {
                    clause(-f[j], b[j], c[j]);
                    clause(-f[j], b[j], d[j]);
                    clause(-f[j], c[j], d[j]);

                    clause(f[j], -b[j], -c[j]);
                    clause(f[j], -b[j], -d[j]);
                    clause(f[j], -c[j], -d[j]);
                    //clause(f[j], -b[j], -c[j], -d[j]);
                }
            } else if (i >= 60 && i < 80) {
                xor3(f, b, c, d);
            }

            add5(format("a[$]", i + 5), a[i + 5], prev_a, f, e, k[i / 20], w[i]);
        }

        /* Rotate back */
        int c[32];
        rotl(c, a[nr_rounds + 2], 30);

        int d[32];
        rotl(d, a[nr_rounds + 1], 30);

        int e[32];
        rotl(e, a[nr_rounds + 0], 30);

        add2("h_out", h_out[0], h_in[0], a[nr_rounds + 4]);
        add2("h_out", h_out[1], h_in[1], a[nr_rounds + 3]);
        add2("h_out", h_out[2], h_in[2], c);
        add2("h_out", h_out[3], h_in[3], d);
        add2("h_out", h_out[4], h_in[4], e);
    }
};

static uint32_t rotl(uint32_t x, unsigned int n)
{
    return (x << n) | (x >> (32 - n));
}

static void sha1_forward(unsigned int nr_rounds, uint32_t w[80], uint32_t h_out[5])
{
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    for (unsigned int i = 16; i < nr_rounds; ++i)
        w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

    uint32_t a = h0;
    uint32_t b = h1;
    uint32_t c = h2;
    uint32_t d = h3;
    uint32_t e = h4;

    for (unsigned int i = 0; i < nr_rounds; ++i) {
        uint32_t f, k;

        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999;
        } else if (i >= 20 && i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i >= 40 && i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else if (i >= 60 && i < 80) {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        } else {
            assert(false);
        }

        uint32_t t = rotl(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rotl(b, 30);
        b = a;
        a = t;
    }

    h_out[0] = h0 + a;
    h_out[1] = h1 + b;
    h_out[2] = h2 + c;
    h_out[3] = h3 + d;
    h_out[4] = h4 + e;
}

void set_input(uint32_t w[80])
{
    for (unsigned int i = 0; i < 16; ++i) {
        w[i] = 0;
        for(uint32_t i2 = 0; i2 < 32; i2++) {
            w[i] <<= 1;
            w[i] |= (uint32_t)input[i*32+31-i2];
        }
    }
}

uint64_t count_num_solutions()
{
    uint64_t num_solutions = 0;

    std::vector<uint32_t> message_bits_unfixed;
    std::vector<int> message_bits_fixed_lookup(512, 0);
    for(auto& x: message_bits_fixed) {
        message_bits_fixed_lookup[x] = 1;
    }
    for(uint32_t i = 0; i < message_bits_fixed_lookup.size(); i++) {
        if (message_bits_fixed_lookup[i] == 0) {
            message_bits_unfixed.push_back(i);
        }
    }


    uint32_t w[80];
    uint32_t num_unset_bits = 512-config_nr_message_bits;
    assert(num_unset_bits < 32 && "We cannot count over more than 31 unset bits");

    uint64_t num_to_try = 1ULL<<num_unset_bits;
    for(uint64_t counter = 0; counter < num_to_try; counter++) {
        /*cerr << "hash_bits_fixed size: " << hash_bits_fixed.size()
        << " num_to_try: " << num_to_try << " counter: " << counter << endl;*/

        for(uint32_t i2 = 0; i2 < num_unset_bits; i2++) {
            int val = (counter >> i2)&1;
            //cerr << "setting: " << message_bits_unfixed[i2] << " to: " << val << endl;
            input[message_bits_unfixed[i2]] = val;
        }
        set_input(w);

        uint32_t h[5];
        sha1_forward(config_nr_rounds, w, h);

        //sanity
        if (num_unset_bits == 0) {
            for(uint32_t i = 0; i < 5; i++) {
                for(uint32_t i2 = 0; i2 < 32; i2++) {
                    int val = ((h[i] >> i2) & 1);
                    //cerr << "i: " << i << " i2:" << i2 << endl;
                    //cerr << "output[i*32+i2]: " << output[i*32+i2] << endl;
                    //cerr << "val: " << val << endl;
                    assert(output[i*32+i2] == val);
                }
            }
        }
        //sanity over

        bool ok = true;
        for(auto& x: hash_bits_fixed) {
            int val = (h[x/32] >> ((x%32))) & 1;
            /*cerr << "x: " << x << " x/32: " << x/32 << " x%32: " << x%32 << endl;
            cerr << "val: " << val << endl;
            cerr << "output[x]: " << output[x] << endl;*/
            if (val != output[x]) {
                ok = false;
                break;
            }
        }

        if (ok) {
            num_solutions++;
        }
    }
    return num_solutions;
}

static void preimage()
{
    sha1 f(config_nr_rounds, "");

    /* Generate a known-valid (messsage, hash)-pair
     * Only first 16*4 bytes of data is used for message
     *      rest is used during SHA1 computation
     */
    uint32_t w[80];
    for (unsigned int i = 0; i < 16; ++i) {
        w[i] = ((uint32_t)(rand() & 0xffff));
        w[i] |= ((uint32_t)(rand() & 0xffff)) << 16;

        for(uint32_t i2 = 0; i2 < 32; i2++) {
            input.push_back((w[i] >> i2) & 1);
        }
    }
    assert(input.size() == 512);

    uint32_t h[5];
    sha1_forward(config_nr_rounds, w, h);
    for(uint32_t i = 0; i < 5; i++) {
        for(uint32_t i2 = 0; i2 < 32; i2++) {
            output.push_back((h[i] >> i2) & 1);
        }
    }
    assert(output.size() == 160);

    /* Fix message bits */
    comment(format("Fix $ message bits", config_nr_message_bits));

    std::vector<unsigned int> message_bits(512);
    for (unsigned int i = 0; i < 512; ++i)
        message_bits[i] = i;

    std::random_shuffle(message_bits.begin(), message_bits.end());
    for (unsigned int i = 0; i < config_nr_message_bits; ++i) {
        unsigned int r = message_bits[i] / 32;
        unsigned int s = message_bits[i] % 32;

        constant(f.w[r][s], input[message_bits[i]]);
        message_bits_fixed.push_back(message_bits[i]);
    }

    //Fill message_bit_vars
    for (unsigned int i = 0; i < 512; ++i) {
        unsigned int r = message_bits[i] / 32;
        unsigned int s = message_bits[i] % 32;
        message_bit_vars.push_back(f.w[r][s]);
    }

    /* Fix hash bits */
    comment(format("Fix $ hash bits", config_nr_hash_bits));

    //TODO remove all_zero_output
    if (all_zero_output) {
        for (unsigned int i = 0; i < config_nr_hash_bits; ++i) {
            constant(f.h_out[i / 32][i % 32], 0);
        }
    } else {
        std::vector<unsigned int> hash_bits(160);
        for (unsigned int i = 0; i < 160; ++i)
            hash_bits[i] = i;

        std::random_shuffle(hash_bits.begin(), hash_bits.end());
        for (unsigned int i = 0; i < config_nr_hash_bits; ++i) {
            unsigned int r = hash_bits[i] / 32;
            unsigned int s = hash_bits[i] % 32;

            constant(f.h_out[r][s], output[hash_bits[i]]);
            hash_bits_fixed.push_back(hash_bits[i]);
        }
    }
}

int main(int argc, char *argv[])
{
    unsigned long seed = time(0);
    indep_vars << "ind ";

    /* Process command line */
    {
        using namespace boost::program_options;

        options_description options("Options");
        options.add_options()("help,h", "Display this information");

        options_description instance_options("Instance options");
        instance_options.add_options()
        ("seed", value<unsigned long>(&seed), "Random number seed")
        ("rounds", value<unsigned int>(&config_nr_rounds), "Number of rounds (16-80)")
        ("message-bits", value<unsigned int>(&config_nr_message_bits),
            "Number of fixed message bits (0-512)")
        ("hash-bits", value<unsigned int>(&config_nr_hash_bits),
             "Number of fixed hash bits (0-160)")
        ("easy", value<unsigned int>(&config_easy_sol_bits),
             "2**easy will be trivial solutions")
        ("zero", bool_switch(&all_zero_output),
             "When doing preimage attack, hash output should be zero")
        ;

        options_description format_options("Format options");
        format_options.add_options()
            ("nocomment", "Don't add comments");
        ;

        options_description all_options;
        all_options.add(options);
        all_options.add(instance_options);
        all_options.add(format_options);

        positional_options_description p;
        p.add("input", -1);

        variables_map map;
        store(command_line_parser(argc, argv).options(all_options).positional(p).run(), map);
        notify(map);

        if (map.count("help")) {
            cout << all_options;
            return 0;
        }

        if (map.count("nocomment")) {
            nocomment = true;
        }

        if (config_nr_hash_bits > 160) {
            cerr << "ERROR: you cannot have more than 160 hash bits." << endl;
            cerr << "       Did you give a negative number?" << endl;
            exit(-1);
        }

        if (config_nr_message_bits > 512) {
            cerr << "ERROR: you cannot have more than 512 message bits" << endl;
            cerr << "       Did you give a negative number?" << endl;
            exit(-1);
        }

        if (512U-config_nr_message_bits > 25) {
            cerr << "ERROR: number of message bits have no more than 30 unknown bits" << endl;
            cerr << "       our internal counter will die (and so will the SAT solvers)" << endl;
            cerr << "       give a value that's more than 482" << endl;
            exit(-1);
        }

        if (all_zero_output) {
            cerr << "ERROR: Shakuni currently does NOT work with all-zero output" << endl;
            exit(-1);
        }
    }

    comment("");
    comment("Instance generated by sha1-sat");
    comment("Written by Vegard Nossum <vegard.nossum@gmail.com>");
    comment("<https://github.com/vegard/sha1-sat>");
    comment("");

    /* Include command line in instance */
    {
        std::ostringstream ss;

        ss << argv[0];

        for (int i = 1; i < argc; ++i) {
            ss << " ";
            ss << argv[i];
        }

        comment(format("command line: $", ss.str()));
    }

    comment(format("parameter seed = $", seed));
    srand(seed);

    new_vars("switch", var_switch, 1);
    preimage();

    comment(format("Forcing all other vars to 1 when switch is 0"));

    //Indep vars
    indep_vars << var_switch[0] << " ";
    for (int x: message_bit_vars) {
        indep_vars << x << " ";
    }
    indep_vars << "0";
    comment(indep_vars.str());

    //allow expansion on message bits
    std::vector<int> my_shuf(message_bit_vars.size());
    for(uint32_t i = 0; i < message_bit_vars.size(); i++) {
        my_shuf[i] = i;
    }
    std::random_shuffle(my_shuf.begin(), my_shuf.end());
    for(uint32_t i = 0; i < my_shuf.size(); i++) {
        if (i >= config_easy_sol_bits) {
            clause_noswitch(-1, message_bit_vars[my_shuf[i]]);
        }
    }

    //Restrict everything else, but not message bits
    std::vector<char> message_vars_lookup(nr_variables+1, 0);
    for(int x: message_bit_vars) {
        message_vars_lookup[x] = 1;
    }
    for (int i = 2; i <= nr_variables; i++) {
        if (!message_vars_lookup[i]) {
            clause_noswitch(-1, i);
        }
    }
    //clause_noswitch(-1);

    //cout << format("p cnf $ $\n", nr_variables, nr_clauses) << cnf.str();

    uint64_t num_hard_solutions = count_num_solutions();
    uint64_t num_easy_solutions = (1ULL<<config_easy_sol_bits);
    uint64_t num_total_solutions = num_easy_solutions+num_hard_solutions;
    cout << format("p cnf $ $\nc num_solutions $ \n", nr_variables, nr_clauses, num_hard_solutions) << cnf.str();
    comment(format("num hard solutions:  $", num_hard_solutions));
    comment(format("num easy solutions:  $", num_easy_solutions));
    comment(format("total num solutions: $", num_total_solutions));

    cerr << "num hard solutions : " << num_hard_solutions << endl;
    cerr << "num easy solutions : " << num_easy_solutions << endl;
    cerr << "num total solutions: " << num_total_solutions  << endl;
    cerr << "easy vs hard ratio : "
    << std::fixed << std::setprecision(4) << (double)num_easy_solutions/(double)num_total_solutions
    << " vs "
    << std::fixed << std::setprecision(4) << (double)num_hard_solutions/(double)num_total_solutions
    << endl;

    return 0;
}
