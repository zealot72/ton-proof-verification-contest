#define BOOST_TEST_MODULE circuit_test
#include <boost/test/included/unit_test.hpp>

#include "../bin/cli/src/types.h"
#include "../bin/cli/src/detail/risk_component.hpp"

using namespace std;
using namespace nil::crypto3;
using namespace nil::crypto3::zk;

uint get_agg_risk(
    vector<ushort> weights,
    vector<ushort> risks
) {
    assert(weights.size() == risks.size());
    uint aggRisk = 0;
    for (size_t i = 0; i < weights.size(); ++i) {
        aggRisk += weights[i] * risks[i];
    }
    return aggRisk;
}

template<typename FieldType>
void check_bp(
    size_t n,
    vector<ushort> weights,
    vector<ushort> risks,
    uint min_risk,
    uint max_risk,
    bool is_satisfied
) {
    blueprint<FieldType> bp;
    contest::risk_reporting_component<FieldType> report(bp, n);
    report.generate_r1cs_constraints();
    report.generate_r1cs_witness(weights, risks, min_risk, max_risk);
    BOOST_CHECK(bp.is_satisfied() == is_satisfied);
}

BOOST_AUTO_TEST_SUITE(test_suite)

BOOST_AUTO_TEST_CASE(test) {
    vector<ushort> weights = {10, 20, 30, 25, 15};
    vector<ushort> risks = {0, 20, 0, 30, 50};
    uint agg_risk = get_agg_risk(weights, risks);
    size_t n = weights.size();
    BOOST_CHECK(agg_risk == 1900);

    cout << "Testing satisfied blueprint..." << endl;
    check_bp<scalar_field_type>(n, weights, risks, 0, 2000, true);
    check_bp<scalar_field_type>(n, weights, risks, 0, agg_risk, true);
    check_bp<scalar_field_type>(n, weights, risks, agg_risk, 2000, true);

    cout << "Testing unsatisfied blueprint..." << endl;
    check_bp<scalar_field_type>(n, weights, risks, 0, 1800, false);
    check_bp<scalar_field_type>(n, weights, risks, 0, agg_risk - 1, false);
    check_bp<scalar_field_type>(n, weights, risks, agg_risk + 1, 2000, false);

    // cout << "Testing asserts..." << endl;
    // BOOST_REQUIRE_THROW(
    //     check_bp<scalar_field_type>(n + 1, weights, risks, 0, 2000, false),
    //     boost::exception);
    // BOOST_REQUIRE_THROW(
    //     check_bp<scalar_field_type>(n - 1, weights, risks, 0, 2000, false),
    //     boost::exception);
    // BOOST_REQUIRE_THROW(
    //     check_bp<scalar_field_type>(n, {10, 20, 30, 25}, risks, 0, 2000, false),
    //     boost::exception);
    // BOOST_REQUIRE_THROW(
    //     check_bp<scalar_field_type>(n, risks, {0, 20, 0, 30, 50}, 0, 2000, false),
    //     boost::exception);
}
BOOST_AUTO_TEST_SUITE_END()
