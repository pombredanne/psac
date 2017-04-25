
#include <iostream>
#include <vector>
#include <string>

#include <stringset.hpp>
#include <shifting.hpp>
#include <kmer.hpp>
#include <mxx/env.hpp>
#include <mxx/comm.hpp>
#include <mxx/distribution.hpp>

#include <cxx-prettyprint/prettyprint.hpp>

void test_kmer() {
    std::vector<std::string> vec = {"abc", "cba", "bbbb", "a"};
    // expected coding: a: 01, b: 10, c: 11
    // "abc" -> 00011011 -> 1b
    //          00101100 -> 2c
    //          00110000 -> 30
    // "cba" -> 00111001 -> 39
    //          00100100 -> 24
    //          00010000 -> 10
    // "bbbb"-> 00101010 -> 2a 2a 28 20
    // "a"   -> 00010000 -> 10
    // => [1b, 2c, 30, 39, 24, 10, 2a, 2a, 28, 20, 10] (hex)
    std::vector<uint16_t> ex_kmers = {0x1b, 0x2c, 0x30, 0x39, 0x24,0x10,
                                      0x2a, 0x2a, 0x28, 0x20, 0x10};
    vstringset ss(vec);


    std::string unique_chars = "abc";
    alphabet<char> alpha = alphabet<char>::from_sequence(unique_chars.begin(), unique_chars.end()); // TODO from vec of chars
    std::cout << "Alphabet: " << alpha << std::endl;
    std::vector<uint16_t> kmers = kmer_gen_stringset<uint16_t>(ss, 3, alpha);

    std::cout << std::hex << kmers << std::endl;
}


void test_shift() {
    // generate input
    mxx::comm c;
    std::vector<int> x;
    int n = 23;
    if (c.rank() == 0) {
        x.resize(n);
        for (size_t i = 0; i < x.size(); ++i) {
            x[i] = i;
        }
    }
    // distribute equally
    mxx::stable_distribute_inplace(x, c);

    for (int i = 0; i < n; ++i) {
        std::vector<int> r = left_shift_dvec(x, c, i);

        std::vector<int> g = mxx::gatherv(r, 0, c);
        // check g
        if (c.rank() == 0) {
            std::cout << "shift " << i << ": " << g << std::endl;
            for (unsigned int j = 0; j < g.size(); ++j) {
                if (g[j] != (int)j + i && !(i+(int)j >= n && g[j] == 0))  {
                    std::cout << "ERROR ERROR" << std::endl;
                }
            }
        }
    }

}

void test_global_copy() {
    // generate input
    mxx::comm c;
    std::vector<int> x;
    int n = 20;
    if (c.rank() == 0) {
        x.resize(n);
        for (size_t i = 0; i < x.size(); ++i) {
            x[i] = i;
        }
    }
    // distribute equally
    mxx::stable_distribute_inplace(x, c);

    // TODO: create ranges + test global copies
    dvector_const_wrapper<int, blk_dist> src(x, c);
    dvector<int, blk_dist> dst(c, x.size());
    copy_global_range(src, 4, 13, dst, 1, 10);
    mxx::sync_cout(c) << dst.vec << std::endl;
}


std::vector<int> rand_buckets(size_t n, size_t max_bs, const mxx::comm& c) {
    std::vector<int> seq;
    if (c.rank() == 0) {
        seq.resize(n);
        size_t i = 0;
        while (i < n) {
            size_t bs = (rand() % std::min(max_bs, n-i)) + 1;
            for (size_t j = i; j < bs; ++j) {
                seq[j] = i;
            }
        }
    }
    mxx::stable_distribute_inplace(seq, c);
    return seq;
}

void test_shift_buckets() {
    mxx::comm c;

    // 1) generate random buckets
    size_t n = 20;
    size_t max_bs = 10;
    std::vector<int> b = rand_buckets(n, max_bs, c);

    // 2) wrap as distributed range (blk_dist)
    // 3) init distributed buckets by value
}

#define SDEBUG(x) mxx::sync_cerr(c) << "[" << c.rank() << "]: " #x " = " << (x) << std::endl

std::string rand_dna(size_t size) {
    std::string result;
    result.resize(size);
    char alpha[4] = {'a', 'c', 't', 'g'};
    for (size_t i = 0; i < size; ++i) {
        result[i] = alpha[rand() % sizeof(alpha)];
    }
    return result;
}

std::string flatten_strings(const std::vector<std::string>& v, const char sep = '$') {
    std::string result;
    size_t outsize = 0;
    for (auto s : v) {
        outsize += s.size() + 1;
    }
    result.resize(outsize);
    auto outit = result.begin();
    for (auto s : v) {
        outit = std::copy(s.begin(), s.end(), outit);
        *outit = sep;
        ++outit;
    }
    return result;
}

void test_dist_ss() {
    mxx::comm c;
    //std::string randseq = random_dstringset(20, c);

    // generate strings of given sizes on master node, join into single string
    // with '$' as seperator and distribute equally among processors
    std::vector<size_t> ssizes = {88, 57, 8, 20, 3, 4, 1, 1, 11};
    std::string randseq;
    if (c.rank() == 0) {
        std::vector<std::string> strs;
        for (size_t s : ssizes) {
            strs.emplace_back(rand_dna(s));
        }

        std::cout << strs << std::endl;
        std::string flatstr = flatten_strings(strs);
        std::cout << "Flat str: \"" << flatstr << "\"" << std::endl;
        // vec of string to strings seperated by $
        randseq = flatstr;
    }
    randseq = mxx::stable_distribute(randseq, c);

    // construct distribute stringset by parsing the string according to
    // '$' separating character
    simple_dstringset ss(randseq.begin(), randseq.end(), c);

    SDEBUG(randseq);
    SDEBUG(ss.sizes);
    SDEBUG(ss.left_size);
    SDEBUG(ss.right_size);

    //mxx::sync_cout(c) << ss;

    // create the distributed sequences prefix_sizes format (with shadow els)
    dist_seqs ds = dist_seqs::from_dss(ss, c);

    // gather sizes and print
    // TODO: gtest for checking of size array is same as the one defined above
    std::vector<size_t> all_sizes = mxx::allgatherv(ds.sizes(), c);
    mxx::sync_cout(c) << ds << std::endl;
    if (c.rank() == 0) {
        std::cout << all_sizes << std::endl;
    }

    // TODO: more test cases, test cases with lots of inbalance, etc

    // TODO: test if the redistribution works properly: use chars as bucket and see if it reproduces original strings
    // TODO: test shifting next
    // TODO: kmer generation for stringset
}

int main(int argc, char *argv[]) {

    mxx::env e(argc, argv);

    //test_shift();
    //test_global_copy();
    test_dist_ss();

    return 0;
}
