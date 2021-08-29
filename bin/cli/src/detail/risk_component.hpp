#include <nil/crypto3/algebra/curves/bls12.hpp>
#include <nil/crypto3/algebra/curves/mnt4.hpp>
#include <nil/crypto3/algebra/curves/mnt6.hpp>
#include <nil/crypto3/multiprecision/number.hpp>

#include <nil/crypto3/zk/components/blueprint.hpp>
#include <nil/crypto3/zk/components/blueprint_variable.hpp>
#include <nil/crypto3/zk/components/comparison.hpp>
#include <nil/crypto3/zk/components/component.hpp>
#include <nil/crypto3/zk/components/hashes/sha256/sha256_component.hpp>
#include <nil/crypto3/zk/components/packing.hpp>
#include <nil/crypto3/zk/components/inner_product.hpp>

#include <nil/crypto3/zk/snark/relations/constraint_satisfaction_problems/r1cs.hpp>

using namespace std;
using namespace nil::crypto3::zk::components;
using namespace nil::crypto3::zk::snark;

namespace contest {

    template<typename FieldType>
    class risk_reporting_component : public component<FieldType> {

        public:
            const size_t n;
            blueprint_variable_vector<FieldType> risks, weights;
            blueprint_variable<FieldType> minRisk, maxRisk, aggRisk;

            blueprint_variable<FieldType> minRiskLess, minRiskLessOrEq;
            blueprint_variable<FieldType> maxRiskLess, maxRiskLessOrEq;

            blueprint_variable<FieldType> out;

            shared_ptr<nil::crypto3::zk::components::inner_product<FieldType>> compute_result;
            shared_ptr<comparison<FieldType>> minRiskCmp, maxRiskCmp;

        risk_reporting_component(
            blueprint<FieldType> &bp,
            size_t n
        ) : component<FieldType>(bp), n(n) {
            risks.allocate(bp, n);
            minRisk.allocate(bp);
            maxRisk.allocate(bp);
            out.allocate(bp);

            weights.allocate(bp, n);
            aggRisk.allocate(bp);

            minRiskLess.allocate(bp);
            minRiskLessOrEq.allocate(bp);
            maxRiskLess.allocate(bp);
            maxRiskLessOrEq.allocate(bp);

            bp.set_input_sizes(n + 3);

            compute_result.reset(
                new nil::crypto3::zk::components::inner_product<FieldType>(
                    bp,
                    risks,
                    weights,
                    aggRisk)
            );
            const size_t comparison_n = 30;
            minRiskCmp.reset(
                new comparison<FieldType>(
                    bp,
                    comparison_n,
                    minRisk,
                    aggRisk,
                    minRiskLess,
                    minRiskLessOrEq));
            maxRiskCmp.reset(
                new comparison<FieldType>(
                    bp,
                    comparison_n,
                    aggRisk,
                    maxRisk,
                    maxRiskLess,
                    maxRiskLessOrEq));
        }

        void generate_r1cs_constraints() {
            compute_result->generate_r1cs_constraints();
            minRiskCmp->generate_r1cs_constraints();
            maxRiskCmp->generate_r1cs_constraints();

            // aggRisk should be in range (minRisk, maxRisk):
            // (minRisk <= aggRisk && aggRisk <= maxRisk) == true
            this->bp.add_r1cs_constraint(r1cs_constraint<FieldType>(minRiskLessOrEq + maxRiskLessOrEq, 1, out));
        }

        void generate_r1cs_witness(
            vector<ushort> _weights,
            vector<ushort> _risks,
            uint _minRisk,
            uint _maxRisk
        ) {
            assert(_weights.size() == n);
            assert(_weights.size() == _risks.size());

            for (size_t i = 0; i < _weights.size(); ++i) {
                this->bp.val(weights[i]) = _weights[i];
                this->bp.val(risks[i]) = _risks[i];
            }

            this->bp.val(minRisk) = _minRisk;
            this->bp.val(maxRisk) = _maxRisk;
            this->bp.val(out) = 2;

            compute_result->generate_r1cs_witness();
            minRiskCmp->generate_r1cs_witness();
            maxRiskCmp->generate_r1cs_witness();
        }
    };

    template<typename FieldType>
    r1cs_primary_input<FieldType> get_public_input(
        vector<ushort> _risks,
        uint _minRisk,
        uint _maxRisk
    ) {
        r1cs_primary_input<FieldType> input;
        for (size_t i = 0; i < _risks.size(); ++i) {
            input.push_back(_risks[i]);
        }
        input.push_back(_minRisk);
        input.push_back(_maxRisk);
        // out variable
        input.push_back(2);
        return input;
    }
}
