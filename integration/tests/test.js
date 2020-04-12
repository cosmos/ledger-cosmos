import Zemu from "@zondax/zemu";
import CosmosApp from "ledger-cosmos-js";

const Resolve = require("path").resolve;

const APP_PATH = Resolve("../app/bin/app.elf");

console.log(APP_PATH);

describe('Basic checks', function () {
    it('can start and stop container', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start({logging: true});
        } finally {
            await sim.close();
        }
    });


    it('get app version', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start();
            const app = new CosmosApp(sim.getTransport());
            const version = await app.getVersion();

            console.log(version)
        } finally {
            await sim.close();
        }
    });

    it('get app info', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start();
            const app = new CosmosApp(sim.getTransport());
            const info = await app.appInfo();

            console.log(info)
        } finally {
            await sim.close();
        }
    });

    const tx_str = {
        "account_number": "108",
        "chain_id": "cosmoshub-2",
        "fee": {
            "amount": [
                {
                    "amount": "600",
                    "denom": "uatom"
                }
            ],
            "gas": "200000"
        },
        "memo": "",
        "msgs": [
            {
                "type": "cosmos-sdk/MsgWithdrawDelegationReward",
                "value": {
                    "delegator_address": "cosmos1kky4yzth6gdrm8ga5zlfwhav33yr7hl87jycah",
                    "validator_address": "cosmosvaloper1kn3wugetjuy4zetlq6wadchfhvu3x740ae6z6x"
                }
            },
            {
                "type": "cosmos-sdk/MsgWithdrawDelegationReward",
                "value": {
                    "delegator_address": "cosmos1kky4yzth6gdrm8ga5zlfwhav33yr7hl87jycah",
                    "validator_address": "cosmosvaloper1sjllsnramtg3ewxqwwrwjxfgc4n4ef9u2lcnj0"
                }
            },
            {
                "type": "cosmos-sdk/MsgWithdrawDelegationReward",
                "value": {
                    "delegator_address": "cosmos1kky4yzth6gdrm8ga5zlfwhav33yr7hl87jycah",
                    "validator_address": "cosmosvaloper1ey69r37gfxvxg62sh4r0ktpuc46pzjrm873ae8"
                }
            },
            {
                "type": "cosmos-sdk/MsgWithdrawDelegationReward",
                "value": {
                    "delegator_address": "cosmos1kky4yzth6gdrm8ga5zlfwhav33yr7hl87jycah",
                    "validator_address": "cosmosvaloper1648ynlpdw7fqa2axt0w2yp3fk542junl7rsvq6"
                }
            }
        ],
        "sequence": "106"
    };

    it('sign tx', async function () {
        jest.setTimeout(60000);
        const sim = new Zemu(APP_PATH);

        try {
            await sim.start({ logging: true, X11: true });
            const app = new CosmosApp(sim.getTransport());

            const path = [44, 118, 0, 0, 0];
            let tx = JSON.stringify(tx_str);

            // do not wait here..
            const signatureRequest = app.sign(path, tx);

            await Zemu.sleep(2000);

            // Reference window
            await sim.snapshot(Resolve("snapshots/0.png"));

            for (let i = 1; i < 20; i++) {
                await sim.clickRight(Resolve(`snapshots/${i}.png`));
            }

            await sim.close();

            let signature = await signatureRequest;
            console.log(signature);

        } finally {
            await sim.close();
        }
    });
});
