import argparse
import binascii
import csv
import tonos_ts4.ts4 as ts4


def get_risks(fname):
    RISKS_COLUMN = 1
    with open(fname, 'r') as f:
        reader = csv.reader(f)
        return [int(row[RISKS_COLUMN]) for row in reader]


def bin2hex(fname):
    with open(fname, 'rb') as f:
        data = f.read()
        return binascii.hexlify(data).decode("utf-8")


def init_argparse():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-p', '--proof',
        dest='proof',
        help='proof path',
        type=str,
        default='proof',
    )
    parser.add_argument(
        '-i', '--primary-input',
        dest='primary_input',
        help='primary_input path',
        type=str,
        default='primary_input',
    )
    parser.add_argument(
        '-k', '--verification-key',
        dest='verification_key',
        help='verification_key path',
        type=str,
        default='verification_key',
    )
    return parser


def verify_proof(proof_path, primary_input_path, verification_key_path):
    ts4.init('.', verbose=False)
    contract = ts4.BaseContract('verification', {})

    print(f'Loading proof from file "{proof_path}"')
    proof_hex = bin2hex(proof_path)
    print(f'Loading primary input from file "{primary_input_path}"')
    primary_input_hex = bin2hex(primary_input_path)
    print(f'Loading verification key from file "{verification_key_path}"')
    verification_key_hex = bin2hex(verification_key_path)

    print('Verifying proof on chain...')
    contract.call_method('setVerificationKey', {'vkey': ts4.Bytes(verification_key_hex)})

    is_verified = contract.call_method('verify', {
        'proof': ts4.Bytes(proof_hex),
        'primary_input': ts4.Bytes(primary_input_hex),
        # 'risks': get_risks('../assets.csv'),
        # 'min_risk': 0,
        # 'max_risk': 2000000,
    })
    print(f'Proof verification status: {is_verified}')
    assert ts4.eq(is_verified, True)


if __name__ == '__main__':
    parser = init_argparse()
    args = parser.parse_args()
    verify_proof(args.proof, args.primary_input, args.verification_key)
