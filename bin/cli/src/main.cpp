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
#include <boost/lexical_cast.hpp>
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

using namespace std;
using namespace nil::marshalling;
using namespace nil::crypto3;
using namespace nil::crypto3::zk;

typedef algebra::curves::bls12<381> curve_type;
typedef typename curve_type::scalar_field_type scalar_field_type;
typedef scalar_field_type::value_type value_type;
typedef zk::snark::r1cs_gg_ppzksnark<curve_type> scheme_type;

// number of assets in S&P100 index
const size_t ASSETS_LENGTH = 101;

struct assets {
    vector<string> symbols;
    vector<ushort> risks;
    vector<ushort> weights;
};

assets load_assets(std::string fname) {
    // 100%
    const size_t DENOMINATOR = 10000;
    const size_t SYMBOLS_COLUMN = 0;
    const size_t RISK_COLUMN = 1;
    const size_t WEIGHT_COLUMN = 2;
    assets data;
    uint weights_sum = 0;

    cout << "Loading assets from csv file " << fname << endl;
    vector<vector<string>> table = readCSV(fname);
    for (size_t row = 0; row < table.size(); ++row) {
        data.symbols.push_back(table[row][SYMBOLS_COLUMN]);
        const ushort risk = boost::lexical_cast<ushort>(table[row][RISK_COLUMN]);
        const ushort weight = boost::lexical_cast<ushort>(table[row][WEIGHT_COLUMN]);

        assert(risk < DENOMINATOR);

        data.risks.push_back(risk);
        data.weights.push_back(weight);
        weights_sum += weight;

        // log assets table
        // cout << "[" << row + 1 << "]: ";
        // for (size_t column = 0; column < table[row].size(); ++column) {
        //     cout << table[row][column] << "\t";
        // }
        // cout << endl;
    }

    assert(data.risks.size() == ASSETS_LENGTH);
    assert(data.risks.size() == data.weights.size());
    // sum of all weights should be equal to denominator (100%)
    assert(weights_sum == DENOMINATOR);

    return data;
}


vector<uint8_t> load_blob(boost::filesystem::path path) {
    boost::filesystem::ifstream stream(path, ios::in | ios::binary);
    auto eos = istreambuf_iterator<char>();
    auto buffer = vector<uint8_t>(istreambuf_iterator<char>(stream), eos);
    return buffer;
}

void save_blob(
    boost::filesystem::path path,
    vector<uint8_t> blob
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
    contest::risk_reporting_component<scalar_field_type> report(bp, ASSETS_LENGTH);
    cout << "Generating r1cs constraints..." << endl;
    report.generate_r1cs_constraints();

    const r1cs_constraint_system<scalar_field_type> constraint_system = bp.get_constraint_system();

    cout << "Generating key pair..." << endl;
    scheme_type::keypair_type keypair = generate<scheme_type>(constraint_system);

    vector<uint8_t> proving_key_byteblob =
        verifier_input_serializer_tvm<scheme_type>::process(keypair.first);
    vector<uint8_t> verification_key_byteblob =
        verifier_input_serializer_tvm<scheme_type>::process(keypair.second);

    cout << "Serializing proving key to " << pk_path << endl;
    save_blob(pk_path, proving_key_byteblob);

    cout << "Serializing verification key to " << vk_path << endl;
    save_blob(vk_path, verification_key_byteblob);

    return true;
}

bool generate_proof(
    boost::filesystem::path proof_path,
    boost::filesystem::path pk_path,
    vector<ushort> weights,
    vector<ushort> risks,
    uint min_risk,
    uint max_risk
) {
    cout << "Deserializing proving key from file " << pk_path << endl;
    vector<uint8_t> proving_key_byteblob = load_blob(pk_path);
    status_type provingProcessingStatus = status_type::success;
    typename scheme_type::proving_key_type pk = verifier_input_deserializer_tvm<scheme_type>::proving_key_process(
        proving_key_byteblob.cbegin(),
        proving_key_byteblob.cend(),
        provingProcessingStatus);

    blueprint<scalar_field_type> bp;
    contest::risk_reporting_component<scalar_field_type> report(bp, ASSETS_LENGTH);
    cout << "Generating r1cs constraints..." << endl;
    report.generate_r1cs_constraints();
    cout << "Generating r1cs witness..." << endl;
    report.generate_r1cs_witness(weights, risks, min_risk, max_risk);

    cout << "aggRisk = " << bp.val(report.aggRisk).data << endl;
    cout << "minRisk = " << bp.val(report.minRisk).data << endl;
    cout << "maxRisk = " << bp.val(report.maxRisk).data << endl;
    cout << "out = " << bp.val(report.out).data << endl;
    cout << "minRiskLess = " << bp.val(report.minRiskLess).data << endl;
    cout << "minRiskLessOrEq = " << bp.val(report.minRiskLessOrEq).data << endl;
    cout << "maxRiskLess = " << bp.val(report.maxRiskLess).data << endl;
    cout << "maxRiskLessOrEq = " << bp.val(report.maxRiskLessOrEq).data << endl;

    // log inputs
    // r1cs_primary_input<scalar_field_type> input = bp.primary_input();
    // for (size_t i = 0; i < input.size(); ++i) {
    //     cout << "primary_input[" << i << "] = " << input[i].data << endl;
    // }
    // r1cs_auxiliary_input<scalar_field_type> input2 = bp.auxiliary_input();
    // for (size_t i = 0; i < input2.size(); ++i) {
    //     cout << "auxiliary_input[" << i << "] = " << input2[i].data << endl;
    // }

    cout << "Blueprint primary_input size: " << bp.primary_input().size() << endl;
    cout << "Blueprint auxiliary_input size: " << bp.auxiliary_input().size() << endl;
    cout << "Blueprint num_constraints: " << bp.num_constraints() << endl;
    cout << "Blueprint num_inputs: " << bp.num_inputs() << endl;
    cout << "Blueprint num_variables: " << bp.num_variables() << endl;
    cout << "Blueprint satisfied: " << bp.is_satisfied() << endl;

    if (!bp.is_satisfied()) {
        return false;
    }

    cout << "Generating proof..." << endl;
    const scheme_type::proof_type proof = prove<scheme_type>(pk, bp.primary_input(), bp.auxiliary_input());

    vector<uint8_t> proof_byteblob =
        verifier_input_serializer_tvm<scheme_type>::process(proof);

    cout << "Serializing proof to file" << proof_path << endl;
    save_blob(proof_path, proof_byteblob);

    return true;
}

bool verify_proof(
    boost::filesystem::path proof_path,
    boost::filesystem::path vk_path,
    vector<ushort> risks,
    uint min_risk,
    uint max_risk
) {
    cout << "Deserializing proof from file " << proof_path << endl;
    vector<uint8_t> proof_byteblob = load_blob(proof_path);
    status_type proofProcessingStatus = status_type::success;
    typename scheme_type::proof_type proof = verifier_input_deserializer_tvm<scheme_type>::proof_process(
        proof_byteblob.cbegin(),
        proof_byteblob.cend(),
        proofProcessingStatus);

    cout << "Deserializing verification key from file " << vk_path << endl;
    vector<uint8_t> verification_key_byteblob = load_blob(vk_path);
    status_type verificationProcessingStatus = status_type::success;
    typename scheme_type::verification_key_type vk = verifier_input_deserializer_tvm<scheme_type>::verification_key_process(
        verification_key_byteblob.cbegin(),
        verification_key_byteblob.cend(),
        verificationProcessingStatus );

    r1cs_primary_input<scalar_field_type> input = contest::get_public_input<scalar_field_type>(
        risks, min_risk, max_risk);

    // log public input
    // for (size_t i = 0; i < input.size(); ++i) {
    //     cout << "public input [" << i << "] = " << input[i].data << endl;
    // }

    using basic_proof_system = r1cs_gg_ppzksnark<curve_type>;
    cout << "Verifying proof..." << endl;
    const bool verified = verify<basic_proof_system>(vk, input, proof);
    cout << "Proof verification status: " << verified << endl;

    return verified;
}

int main(int argc, char *argv[]) {
    uint min_risk, max_risk;
    boost::filesystem::path assets_path;
    boost::filesystem::path pout, pkout, vkout, piout, viout;
    boost::program_options::options_description options(
        "R1CS Generic Group PreProcessing Zero-Knowledge Succinct Non-interactive ARgument of Knowledge "
        "(https://eprint.iacr.org/2016/260.pdf) CLI Proof Generator");
    // clang-format off
    options.add_options()("help,h", "Display help message")
    ("version,v", "Display version")
    ("generate", "Generate private and verification keys")
    ("proof", "Generate proofs")
    ("verify", "Verify proofs")
    ("proof-output,po", boost::program_options::value<boost::filesystem::path>(&pout)->default_value("proof"))
    ("primary-input-output,pio", boost::program_options::value<boost::filesystem::path>(&piout)->default_value
("pinput"))
    ("proving-key-output,pko", boost::program_options::value<boost::filesystem::path>(&pkout)->default_value("pkey"))
    ("verifying-key-output,vko", boost::program_options::value<boost::filesystem::path>(&vkout)->default_value("vkey"))
    ("verifier-input-output,vio", boost::program_options::value<boost::filesystem::path>(&viout)->default_value("vio"))
    ("min-risk,x", boost::program_options::value<uint>(&min_risk)->default_value(0))
    ("max-risk,y", boost::program_options::value<uint>(&max_risk)->default_value(2000000))
    ("assets,a", boost::program_options::value<boost::filesystem::path>(&assets_path)->default_value("assets.csv"))
    ;
    // clang-format on

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(options).run(), vm);
    boost::program_options::notify(vm);

    if (vm.count("help") || argc < 2) {
        cout << options << endl;
        return 0;
    } else if (vm.count("generate")) {
        generate_keys(pkout, vkout);
    } else if (vm.count("proof") || vm.count("verify")) {
        assets data = load_assets(assets_path.string());
        if (vm.count("proof")) {
            generate_proof(pout, pkout, data.weights, data.risks, min_risk, max_risk);
        } else if (vm.count("verify")) {
            verify_proof(pout, vkout, data.risks, min_risk, max_risk);
        }
    }

    return 0;
}
