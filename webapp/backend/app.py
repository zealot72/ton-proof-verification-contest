import binascii
import csv
import os
import tempfile
import time
import tonos_ts4.ts4 as ts4
import subprocess

from flask import Flask, request, abort
from flask_cors import CORS

app = Flask(__name__)
CORS(app)


CLI_PATH = os.path.abspath('../../build/bin/cli/cli')
CONTRACTS_PATH = os.path.abspath('../../contracts')


def _bin2hex(file):
    data = file.read()
    return binascii.hexlify(data).decode("utf-8")


@app.route("/proof/", methods=['POST'])
def proof():
    weights = request.json['weights']
    risks = request.json['risks']
    min_risk = request.json['min_risk']
    max_risk = request.json['max_risk']

    temp_proof = tempfile.NamedTemporaryFile()
    temp_primary_input = tempfile.NamedTemporaryFile()
    temp_verification_key = tempfile.NamedTemporaryFile()

    with tempfile.NamedTemporaryFile(suffix='.csv', mode='w', delete=False) as temp_csv:
        print('Creating csv file with assets...')
        csv_writer = csv.writer(temp_csv, delimiter=',', lineterminator='\n')
        for weight, risk in zip(weights, risks):
            csv_writer.writerow(['-', risk, weight])

    cli_args = [
        CLI_PATH, "--generate",
        "--assets", temp_csv.name,
        "-l", str(len(weights)),
        "--min-risk", str(min_risk),
        "--max-risk", str(max_risk),
        "--proof-output", temp_proof.name,
        "--primary-input-output", temp_primary_input.name,
        "--verifying-key-output", temp_verification_key.name,
    ]
    print('Calling cli:\n' + ' '.join(cli_args))
    result = subprocess.run(cli_args, capture_output=True)
    for line in result.stdout.split(b'\n'):
        print(str(line))
    for line in result.stderr.split(b'\n'):
        print(str(line))
    if result.returncode != 0:
        return abort(400)

    # get hex data from files
    proof_hex = _bin2hex(temp_proof)
    temp_proof.close()

    primary_input_hex = _bin2hex(temp_primary_input)
    temp_primary_input.close()

    verification_key_hex = _bin2hex(temp_verification_key)
    temp_verification_key.close()

    return {
        'proof': proof_hex,
        'primary_input': primary_input_hex,
        'verification_key': verification_key_hex,
    }


@app.route("/verify/", methods=['POST'])
def verify():
    ts4.init(CONTRACTS_PATH, verbose=False)
    contract = ts4.BaseContract('verification', {})

    proof_hex = request.json['proof']
    primary_input_hex = request.json['primary_input']
    verification_key_hex = request.json['verification_key']

    try:
        contract.call_method('setVerificationKey', {'vkey': ts4.Bytes(verification_key_hex)})

        is_verified = contract.call_method('verify', {
            'proof': ts4.Bytes(proof_hex),
            'primary_input': ts4.Bytes(primary_input_hex),
        })
    except Exception as e:
        print(e)
        is_verified = False

    return {
        'status': is_verified,
    }


if __name__ == '__main__':
    app.run(debug=True)
