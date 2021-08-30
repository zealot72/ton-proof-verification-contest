pragma ton-solidity >=0.30.0;
pragma AbiHeader pubkey;

contract Verify {
    bytes public m_vkey = hex"";

    function verify(
        bytes proof,
        bytes primary_input
        // uint16[] risks,
        // uint32 min_risk,
        // uint32 max_risk
    ) public view returns (bool) {
        require(m_vkey.length > 0, 200, "Empty verification key");
        tvm.accept();

        // bytes primary_input = serialize_primary_input(risks, min_risk, max_risk);
        string blob_str = proof;
        blob_str.append(primary_input);
        blob_str.append(m_vkey);
        return tvm.vergrth16(blob_str);
    }

    function setVerificationKey(bytes vkey) public {
        require(msg.pubkey() == tvm.pubkey(), 150);
        tvm.accept();
        m_vkey = vkey;
    }

    function serialize_primary_input(
        uint16[] risks,
        uint32 min_risk,
        uint32 max_risk
    ) public pure returns (bytes) {
        tvm.accept();

        string blob_str = uint64_to_bytes(uint64(risks.length) + 2);
        for(uint256 i; i < risks.length; ++i) {
            blob_str.append(uint256_to_bytes(risks[i]));
        }
        blob_str.append(uint256_to_bytes(0));
        blob_str.append(uint64_to_bytes(0));
        blob_str.append(uint64_to_bytes(0));
        blob_str.append(uint64_to_bytes(min_risk));
        blob_str.append(uint64_to_bytes(max_risk));
        return blob_str;
    }

    function uint64_to_bytes(uint64 number) public pure returns (bytes){
        tvm.accept();

        TvmBuilder ref_builder;
        ref_builder.store(bytes8(number));
        TvmBuilder builder;
        builder.storeRef(ref_builder.toCell());
        return builder.toSlice().decode(bytes);
    }

    function uint256_to_bytes(uint256 number) public pure returns (bytes){
        tvm.accept();

        TvmBuilder ref_builder;
        ref_builder.store(bytes32(number));
        TvmBuilder builder;
        builder.storeRef(ref_builder.toCell());
        return builder.toSlice().decode(bytes);
    }
}
