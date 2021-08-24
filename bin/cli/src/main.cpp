//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//---------------------------------------------------------------------------//

#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "detail/r1cs_examples.hpp"
#include "detail/sha256_component.hpp"

#include <nil/crypto3/algebra/curves/bls12.hpp>
#include <nil/crypto3/algebra/fields/bls12/base_field.hpp>
#include <nil/crypto3/algebra/fields/bls12/scalar_field.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/bls12.hpp>
#include <nil/crypto3/algebra/curves/params/multiexp/bls12.hpp>
#include <nil/crypto3/algebra/curves/params/wnaf/bls12.hpp>
#include <nil/crypto3/algebra/pairing/bls12.hpp>
#include <nil/crypto3/algebra/pairing/mnt4.hpp>
#include <nil/crypto3/algebra/pairing/mnt6.hpp>

#include <nil/crypto3/zk/components/blueprint.hpp>
#include <nil/crypto3/zk/components/blueprint_variable.hpp>
#include <nil/crypto3/zk/components/disjunction.hpp>

#include <nil/crypto3/zk/snark/schemes/ppzksnark/r1cs_gg_ppzksnark.hpp>
#include <nil/crypto3/zk/snark/schemes/ppzksnark/r1cs_gg_ppzksnark/marshalling.hpp>

#include <nil/crypto3/zk/snark/algorithms/generate.hpp>
#include <nil/crypto3/zk/snark/algorithms/verify.hpp>
#include <nil/crypto3/zk/snark/algorithms/prove.hpp>

#include <nil/marshalling/status_type.hpp>
#include <nil/crypto3/marshalling/types/zk/r1cs_gg_ppzksnark/primary_input.hpp>
#include <nil/crypto3/marshalling/types/zk/r1cs_gg_ppzksnark/proof.hpp>
#include <nil/crypto3/marshalling/types/zk/r1cs_gg_ppzksnark/verification_key.hpp>

#include "detail/risk_component.hpp"
#include "csv.cpp"

using namespace nil::crypto3;
using namespace nil::crypto3::marshalling;
using namespace nil::crypto3::zk;

typedef algebra::curves::bls12<381> curve_type;
typedef typename curve_type::scalar_field_type scalar_field_type;
using Endianness = nil::marshalling::option::big_endian;
typedef zk::snark::r1cs_gg_ppzksnark<curve_type> scheme_type;


std::vector<std::uint8_t> load_file(boost::filesystem::path path) {
    boost::filesystem::ifstream stream(path, std::ios::in | std::ios::binary);
    auto eos = std::istreambuf_iterator<char>();
    auto buffer = std::vector<uint8_t>(std::istreambuf_iterator<char>(stream), eos);
    return buffer;
}

void save_file(
    boost::filesystem::path path,
    std::vector<std::uint8_t> blob
) {
    boost::filesystem::ofstream out(path);
    for (const auto &v : blob) {
        out << v;
    }
    out.close();
}

bool generate_keys(
    boost::filesystem::path pk_path,
    boost::filesystem::path vk_path
) {
    blueprint<scalar_field_type> bp;
    contest::risk_reporting_component<scalar_field_type> contest_component(bp, 100);
    contest_component.generate_r1cs_constraints();

    const r1cs_constraint_system<scalar_field_type> constraint_system = bp.get_constraint_system();

    scheme_type::keypair_type keypair = generate<scheme_type>(constraint_system);

    std::vector<std::uint8_t> proving_key_byteblob =
        nil::marshalling::verifier_input_serializer_tvm<scheme_type>::process(keypair.first);
    std::vector<std::uint8_t> verification_key_byteblob =
        nil::marshalling::verifier_input_serializer_tvm<scheme_type>::process(keypair.second);

    save_file(pk_path, proving_key_byteblob);
    std::cout << "prooving key saved to " << pk_path << std::endl;

    save_file(vk_path, verification_key_byteblob);
    std::cout << "verification key saved to " << vk_path << std::endl;

    return true;
}


bool generate_proof(
    boost::filesystem::path proof_path,
    boost::filesystem::path pk_path,
    std::vector<float> weights,
    std::vector<float> risks,
    float min_risk,
    float max_risk
) {
    std::vector<std::uint8_t> proving_key_byteblob = load_file(pk_path);
    nil::marshalling::status_type provingProcessingStatus = nil::marshalling::status_type::success;
    typename scheme_type::proving_key_type pk = nil::marshalling::verifier_input_deserializer_tvm<scheme_type>::proving_key_process(
        proving_key_byteblob.cbegin(),
        proving_key_byteblob.cend(),
        provingProcessingStatus);

    blueprint<scalar_field_type> bp;
    contest::risk_reporting_component<scalar_field_type> contest_component(bp, 100);
    contest_component.generate_r1cs_constraints();
    contest_component.generate_r1cs_witness(weights, risks, min_risk, max_risk);

    std::cout << "Circuit satisfied: " << bp.is_satisfied() << std::endl;
    if (!bp.is_satisfied()) {
        return false;
    }

    const scheme_type::proof_type proof = prove<scheme_type>(pk, bp.primary_input(), bp.auxiliary_input());

    std::vector<std::uint8_t> proof_byteblob =
        nil::marshalling::verifier_input_serializer_tvm<scheme_type>::process(proof);

    std::cout << "proof is saved to " << proof_path << std::endl;
    save_file(proof_path, proof_byteblob);

    return true;
}


bool verify_proof(
    boost::filesystem::path proof_path,
    boost::filesystem::path vk_path,
    std::vector<float> risks,
    float min_risk,
    float max_risk
) {
    std::vector<std::uint8_t> proof_byteblob = load_file(proof_path);
    nil::marshalling::status_type proofProcessingStatus = nil::marshalling::status_type::success;
    typename scheme_type::proof_type proof = nil::marshalling::verifier_input_deserializer_tvm<scheme_type>::proof_process(
        proof_byteblob.cbegin(),
        proof_byteblob.cend(),
        proofProcessingStatus);

    std::vector<std::uint8_t> verification_key_byteblob = load_file(vk_path);
    nil::marshalling::status_type verificationProcessingStatus = nil::marshalling::status_type::success;
    typename scheme_type::verification_key_type vk = nil::marshalling::verifier_input_deserializer_tvm<scheme_type>::verification_key_process(
        verification_key_byteblob.cbegin(),
        verification_key_byteblob.cend(),
        verificationProcessingStatus );

    r1cs_primary_input<scalar_field_type> input = contest::get_public_input<scalar_field_type>(risks, min_risk, max_risk);
    using basic_proof_system = r1cs_gg_ppzksnark<curve_type>;
    const bool verified = verify<basic_proof_system>(vk, input, proof);
    std::cout << "proof verified " << verified << std::endl;

    return verified;
}


int main(int argc, char *argv[]) {
    float min_risk, max_risk;
    boost::filesystem::path weight_path, risks_path;

    boost::filesystem::path pout, pkout, vkout, piout, viout;
    boost::program_options::options_description options(
        "R1CS Generic Group PreProcessing Zero-Knowledge Succinct Non-interactive ARgument of Knowledge "
        "(https://eprint.iacr.org/2016/260.pdf) CLI Proof Generator");
    // clang-format off
    options.add_options()("help,h", "Display help message")
    ("version,v", "Display version")
    ("generate", "Generate private and verification keys")
    ("verify", "verify proofs and/or keys")
    ("proof", "verify proofs and/or keys")
    ("proof-output,po", boost::program_options::value<boost::filesystem::path>(&pout)->default_value("proof"))
    ("primary-input-output,pio", boost::program_options::value<boost::filesystem::path>(&piout)->default_value
("pinput"))
    ("proving-key-output,pko", boost::program_options::value<boost::filesystem::path>(&pkout)->default_value("pkey"))
    ("verifying-key-output,vko", boost::program_options::value<boost::filesystem::path>(&vkout)->default_value("vkey"))
    ("verifier-input-output,vio", boost::program_options::value<boost::filesystem::path>(&viout)->default_value("vio"))
    ("min-risk,x", boost::program_options::value<float>(&min_risk)->default_value(0))
    ("max-risk,y", boost::program_options::value<float>(&max_risk)->default_value(100))
    ("weights,w", boost::program_options::value<boost::filesystem::path>(&weight_path)->default_value("data/weights.csv"))
    ("risks,r", boost::program_options::value<boost::filesystem::path>(&risks_path)->default_value("data/risks.csv"));
    // clang-format on

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(options).run(), vm);
    boost::program_options::notify(vm);

    if (vm.count("help") || argc < 2) {
        std::cout << options << std::endl;
        return 0;
    } else if (vm.count("generate")) {
        generate_keys(pkout, vkout);
    } else if (vm.count("proof") || vm.count("verify")) {
        // TODO read maps from csv files
        // std::vector<std::vector<std::string>> table = readCSV("data.csv");
        std::map<std::string, int> weights_map;
        std::map<std::string, int> risks_map;
        // TODO: trasform maps to arrays with fixed order
        std::vector<float> weights, risks;
        if (vm.count("proof")) {
            generate_proof(pout, pkout, weights, risks, min_risk, max_risk);
        } else if (vm.count("verify")) {
            verify_proof(pout, vkout, risks, min_risk, max_risk);
        }
    }

    return 0;
}
