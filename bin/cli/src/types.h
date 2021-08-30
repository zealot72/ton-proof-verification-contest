#include <nil/crypto3/algebra/curves/bls12.hpp>
#include <nil/crypto3/algebra/fields/bls12/base_field.hpp>
#include <nil/crypto3/algebra/fields/bls12/scalar_field.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/bls12.hpp>
#include <nil/crypto3/algebra/curves/params/multiexp/bls12.hpp>
#include <nil/crypto3/algebra/curves/params/wnaf/bls12.hpp>
#include <nil/crypto3/algebra/pairing/bls12.hpp>
#include <nil/crypto3/algebra/pairing/mnt4.hpp>
#include <nil/crypto3/algebra/pairing/mnt6.hpp>

// #include <nil/marshalling/status_type.hpp>

#include <nil/crypto3/zk/snark/schemes/ppzksnark/r1cs_gg_ppzksnark.hpp>
// #include <nil/crypto3/zk/snark/schemes/ppzksnark/r1cs_gg_ppzksnark/marshalling.hpp>

// #include <nil/crypto3/marshalling/types/zk/r1cs_gg_ppzksnark/primary_input.hpp>
// #include <nil/crypto3/marshalling/types/zk/r1cs_gg_ppzksnark/proof.hpp>
// #include <nil/crypto3/marshalling/types/zk/r1cs_gg_ppzksnark/verification_key.hpp>

typedef algebra::curves::bls12<381> curve_type;
typedef typename curve_type::scalar_field_type scalar_field_type;
typedef scalar_field_type::value_type value_type;
typedef zk::snark::r1cs_gg_ppzksnark<curve_type> scheme_type;
