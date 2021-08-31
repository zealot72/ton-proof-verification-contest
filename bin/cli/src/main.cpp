#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include "./types.h"

#include <nil/crypto3/zk/snark/algorithms/generate.hpp>
#include <nil/crypto3/zk/snark/algorithms/verify.hpp>
#include <nil/crypto3/zk/snark/algorithms/prove.hpp>

#include "./detail/risk_component.hpp"
#include "./csv.cpp"
#include "./utils.hpp"

using namespace std;
using namespace nil::crypto3;
using namespace nil::crypto3::marshalling;
using namespace nil::crypto3::zk;


struct assets {
    vector<string> symbols;
    vector<ushort> risks;
    vector<ushort> weights;
};

assets load_assets(std::string fname, uint assets_length) {
    // 100%
    // const size_t DENOMINATOR = 10000;
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

        // assert(risk < DENOMINATOR);

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

    assert(data.risks.size() == assets_length);
    assert(data.risks.size() == data.weights.size());
    // sum of all weights should be equal to denominator (100%)
    // assert(weights_sum == DENOMINATOR);

    return data;
}

// bool generate_keys(
//     boost::filesystem::path pk_path,
//     boost::filesystem::path vk_path,
//     uint assets_length
// ) {
//     blueprint<scalar_field_type> bp;
//     contest::risk_reporting_component<scalar_field_type> report(bp, assets_length);
//     cout << "Generating r1cs constraints..." << endl;
//     report.generate_r1cs_constraints();

//     const r1cs_constraint_system<scalar_field_type> constraint_system = bp.get_constraint_system();

//     cout << "Generating key pair..." << endl;
//     scheme_type::keypair_type keypair = generate<scheme_type>(constraint_system);

//     vector<uint8_t> proving_key_byteblob =
//         nil::marshalling::verifier_input_serializer_tvm<scheme_type>::process(keypair.first);
//     vector<uint8_t> verification_key_byteblob =
//         nil::marshalling::verifier_input_serializer_tvm<scheme_type>::process(keypair.second);

//     cout << "Serializing proving key to " << pk_path << endl;
//     save_blob(pk_path, proving_key_byteblob);

//     cout << "Serializing verification key to " << vk_path << endl;
//     save_blob(vk_path, verification_key_byteblob);

//     return true;
// }

bool generate_proof(
    boost::filesystem::path proof_path,
    boost::filesystem::path vk_path,
    boost::filesystem::path pi_path,
    uint assets_length,
    vector<ushort> weights,
    vector<ushort> risks,
    uint min_risk,
    uint max_risk
) {
    blueprint<scalar_field_type> bp;
    contest::risk_reporting_component<scalar_field_type> report(bp, assets_length);
    cout << "Generating r1cs constraints..." << endl;
    report.generate_r1cs_constraints();
    cout << "Generating r1cs witness..." << endl;
    report.generate_r1cs_witness(weights, risks, min_risk, max_risk);

    cout << "minRisk = " << bp.val(report.minRisk).data << endl;
    cout << "maxRisk = " << bp.val(report.maxRisk).data << endl;
    cout << "aggRisk = " << bp.val(report.aggRisk).data << endl;
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

    const r1cs_constraint_system<scalar_field_type> constraint_system = bp.get_constraint_system();

    cout << "Generating key pair..." << endl;
    scheme_type::keypair_type keypair = generate<scheme_type>(constraint_system);

    cout << "Generating proof..." << endl;
    const typename scheme_type::proof_type proof = prove<scheme_type>(keypair.first, bp.primary_input(), bp.auxiliary_input());


    cout << "Saving proof to " << proof_path << endl;
    save_proof(proof, proof_path);

    cout << "Saving verification_key to " << vk_path << endl;
    save_verification_key(keypair.second, vk_path);

    cout << "Saving primary input to " << pi_path << endl;
    save_primary_input(bp.primary_input(), pi_path);

    return true;
}

bool verify_proof(
    boost::filesystem::path proof_path,
    boost::filesystem::path vk_path,
    boost::filesystem::path pi_path
) {
    cout << "Deserializing proof from file " << proof_path << endl;
    typename scheme_type::proof_type proof = load_proof(proof_path);

    cout << "Deserializing verification key from file " << vk_path << endl;
    typename scheme_type::verification_key_type vk = load_verification_key(vk_path);

    cout << "Deserializing primary input from file " << pi_path << endl;
    r1cs_primary_input<scalar_field_type> input = load_primary_input(pi_path);

    // log public input
    // for (size_t i = 0; i < input.size(); ++i) {
    //     cout << "public input [" << i << "] = " << input[i].data << endl;
    // }

    using basic_proof_system = r1cs_gg_ppzksnark<curve_type>;
    cout << "Verifying proof in blueprint..." << endl;
    const bool verified = verify<basic_proof_system>(vk, input, proof);
    cout << "Proof verification status: " << verified << endl;

    return verified;
}

int main(int argc, char *argv[]) {
    // by default assets_length = 101 - number of assets in S&P100 index
    uint assets_length, min_risk, max_risk;
    boost::filesystem::path assets_path;
    boost::filesystem::path pout, pkout, vkout, piout;
    boost::program_options::options_description options(
        "R1CS Generic Group PreProcessing Zero-Knowledge Succinct Non-interactive ARgument of Knowledge "
        "(https://eprint.iacr.org/2016/260.pdf) CLI Proof Generator");
    // clang-format off
    options.add_options()("help,h", "Display help message")
    ("version,v", "Display version")
    ("generate", "Generate proof and private/verification keys")
    // ("proof", "Generate proofs")
    ("verify", "Verify proofs")
    ("proof-output,po", boost::program_options::value<boost::filesystem::path>(&pout)->default_value("proof"))
    ("primary-input-output,pio", boost::program_options::value<boost::filesystem::path>(&piout)->default_value("primary_input"))
    ("proving-key-output,pko", boost::program_options::value<boost::filesystem::path>(&pkout)->default_value("proving_key"))
    ("verifying-key-output,vko", boost::program_options::value<boost::filesystem::path>(&vkout)->default_value("verification_key"))
    ("assets-length,l", boost::program_options::value<uint>(&assets_length)->default_value(101))
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
        assets data = load_assets(assets_path.string(), assets_length);
        generate_proof(
            pout,
            vkout,
            piout,
            assets_length,
            data.weights,
            data.risks,
            min_risk,
            max_risk);
    } else if (vm.count("verify")) {
        verify_proof(pout, vkout, piout);
    }
    return 0;
}
