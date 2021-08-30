#include <boost/algorithm/hex.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "./types.h"

#include <nil/marshalling/status_type.hpp>

#include <nil/crypto3/zk/snark/schemes/ppzksnark/r1cs_gg_ppzksnark.hpp>
#include <nil/crypto3/zk/snark/schemes/ppzksnark/r1cs_gg_ppzksnark/marshalling.hpp>

#include <nil/crypto3/marshalling/types/zk/r1cs_gg_ppzksnark/primary_input.hpp>
#include <nil/crypto3/marshalling/types/zk/r1cs_gg_ppzksnark/proof.hpp>
#include <nil/crypto3/marshalling/types/zk/r1cs_gg_ppzksnark/verification_key.hpp>

using namespace std;

// using unit_type = unsigned char;
using unit_type = uint8_t;
using Endianness = nil::marshalling::option::big_endian;
using verification_key_marshalling_type =
    nil::crypto3::marshalling::types::r1cs_gg_ppzksnark_verification_key<
        nil::marshalling::field_type<
            Endianness>,
        scheme_type::verification_key_type>;
using proof_marshalling_type =
    nil::crypto3::marshalling::types::r1cs_gg_ppzksnark_proof<
        nil::marshalling::field_type<
            Endianness>,
        scheme_type::proof_type>;
using primary_input_marshalling_type =
    nil::crypto3::marshalling::types::r1cs_gg_ppzksnark_primary_input<
        nil::marshalling::field_type<
            Endianness>,
        scheme_type::primary_input_type>;


vector<unit_type> load_blob(boost::filesystem::path path) {
    boost::filesystem::ifstream stream(path, ios::in | ios::binary);
    auto eos = istreambuf_iterator<char>();
    auto buffer = vector<unit_type>(istreambuf_iterator<char>(stream), eos);
    return buffer;
}

void save_blob(
    boost::filesystem::path path,
    vector<unit_type> blob
) {
    boost::filesystem::ofstream out(path);
    for (const auto &v : blob) {
        out << v;
    }
    out.close();
}


void save_verification_key(scheme_type::verification_key_type vk, boost::filesystem::path path) {
    verification_key_marshalling_type filled =
        nil::crypto3::marshalling::types::fill_r1cs_gg_ppzksnark_verification_key<
            scheme_type::verification_key_type,
            Endianness>(vk);
    vector<unit_type> byteblob;
    byteblob.resize(filled.length(), 0x00);
    auto write_iter = byteblob.begin();
    filled.write(write_iter, byteblob.size());
    save_blob(path, byteblob);
}


scheme_type::verification_key_type load_verification_key(boost::filesystem::path path) {
    vector<unit_type> byteblob = load_blob(path);
    auto read_iter = byteblob.begin();

    verification_key_marshalling_type value;
    value.read(read_iter, byteblob.size());

    return nil::crypto3::marshalling::types::make_r1cs_gg_ppzksnark_verification_key<
        scheme_type::verification_key_type,
        Endianness
    >(value);
}



void save_proof(scheme_type::proof_type proof, boost::filesystem::path path) {
    proof_marshalling_type filled =
        nil::crypto3::marshalling::types::fill_r1cs_gg_ppzksnark_proof<
            typename scheme_type::proof_type,
            Endianness>(proof);

    vector<unit_type> byteblob;
    byteblob.resize(filled.length(), 0x00);
    auto write_iter = byteblob.begin();
    filled.write(write_iter, byteblob.size());
    save_blob(path, byteblob);
}


scheme_type::proof_type load_proof(boost::filesystem::path path) {
    vector<unit_type> byteblob = load_blob(path);
    auto read_iter = byteblob.begin();

    proof_marshalling_type value;
    value.read(read_iter, byteblob.size());

    return nil::crypto3::marshalling::types::make_r1cs_gg_ppzksnark_proof<
        scheme_type::proof_type,
        Endianness
    >(value);
}


void save_primary_input(zk::snark::r1cs_primary_input<scalar_field_type> primary_input, boost::filesystem::path path) {
    primary_input_marshalling_type filled =
        nil::crypto3::marshalling::types::fill_r1cs_gg_ppzksnark_primary_input<
            typename scheme_type::primary_input_type,
            Endianness>(primary_input);

    vector<unit_type> byteblob;
    byteblob.resize(filled.length(), 0x00);
    auto write_iter = byteblob.begin();
    filled.write(write_iter, byteblob.size());
    save_blob(path, byteblob);
}


zk::snark::r1cs_primary_input<scalar_field_type> load_primary_input(boost::filesystem::path path) {
    vector<unit_type> byteblob = load_blob(path);
    auto read_iter = byteblob.begin();

    primary_input_marshalling_type value;
    value.read(read_iter, byteblob.size());

    return nil::crypto3::marshalling::types::make_r1cs_gg_ppzksnark_primary_input<
        zk::snark::r1cs_primary_input<scalar_field_type>,
        Endianness
    >(value);
}
