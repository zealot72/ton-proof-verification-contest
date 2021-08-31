# ZKP applied to portfolio risk reporting

**This repo contains a proof of concept project made for the TON contest:**
[#32 Groth16 zkSNARK Proof Verification Use Cases Part II](https://devex.gov.freeton.org/proposal?proposalAddress=0%3A9da091db1d766a6de5309463b63e5120fe2ad8a36ee2a5aad3b537a7249662c0)

![Web application](webapp.jpg)

## Abstract

Nowadays there is an information asymmetry between investment funds and their clients. Clients don’t have the full information on their portfolio content and are not able to verify if the asset manager is going beyond their preferred risk limit. Funds keep their investment strategies confidential in order not to put their competitive advantage at risk.

We want to propose a Zero Knowledge Proof based solution for funds to perform risk reporting in a way that enables clients to verify the proof of portfolio weighted risk and be sure it's not beyond their limit.


## Usage instructions

Clone contest repo:
```
git clone https://github.com/zealot72/ton-proof-verification-contest --recursive
```


Build cli tool:
```
mkdir build && cd build
cmake ..
make cli
```

Go to contracts directory:
```
cd ../contracts
```

Generate proof using prepared assets.csv file with risks and weights
```
../build/bin/cli/cli --generate --assets "../assets.csv" -l 101
```

Verify generated proof offchain using Blueprint library:
```
../build/bin/cli/cli --verify
```

Compile verification contract with Nil's tvm-solidity compiler:
```
tondev sol compile verification.sol
```

Verify generated proof in blockchain emulator using TestSuite4 library:
```
python verify.py
```
