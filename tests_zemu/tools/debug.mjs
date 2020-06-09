import Zemu from "@zondax/zemu";
import CosmosApp from "ledger-cosmos-js";
import path from "path";

const APP_PATH = path.resolve(`./../../app/bin/app.elf`);

const seed = "equip will roof matter pink blind book anxiety banner elbow sun young"
const SIM_OPTIONS = {
    logging: true,
    start_delay: 4000,
    X11: true,
    custom: `-s "${seed}" --color LAGOON_BLUE`
};

const example_tx_str = {
    "account_number": "108",
    "chain_id": "cosmoshub-3",
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
                "delegator_address": "cosmos19umvgcvk8cxsvzemy239nj9ngc2ltukantgyp3",
                "validator_address": "cosmosvaloper1648ynlpdw7fqa2axt0w2yp3fk542junl7rsvq6"
            }
        },
        {
            "type": "cosmos-sdk/MsgDelegate",
            "value": {
                "amount": {
                    "amount": "20139397",
                    "denom": "uatom"
                },
                "delegator_address": "cosmos19umvgcvk8cxsvzemy239nj9ngc2ltukantgyp3",
                "validator_address": "cosmosvaloper1648ynlpdw7fqa2axt0w2yp3fk542junl7rsvq6",
            }
        }
    ],
    "sequence": "106"
};

async function beforeStart() {
    process.on("SIGINT", () => {
        Zemu.default.stopAllEmuContainers(function () {
            process.exit();
        });
    });
    await Zemu.default.checkAndPullImage();
}

async function beforeEnd() {
    await Zemu.default.stopAllEmuContainers();
}

async function debugScenario(sim, app) {
    const path = [44, 118, 0, 0, 0];
    let tx = JSON.stringify(example_tx_str);

//    await Zemu.default.sleep(120000);

    const addr = await app.getAddressAndPubKey(path, "cosmos");
    console.log(addr)

    console.log(tx);

    // do not wait here..
    const signatureRequest = app.sign(path, tx);
    await Zemu.default.sleep(1000);

    await sim.clickRight();
    await sim.clickRight();
    await sim.clickRight();
    await sim.clickRight();
    await sim.clickRight();
    await sim.clickRight();
    await sim.clickRight();
    await sim.clickRight();
    await sim.clickBoth();

    let resp = await signatureRequest;
    console.log(resp);
}

async function main() {
    await beforeStart();

    if (process.argv.length > 2 && process.argv[2] === "debug") {
        SIM_OPTIONS["custom"] = SIM_OPTIONS["custom"] + " --debug";
    }

    const sim = new Zemu.default(APP_PATH);

    try {
        await sim.start(SIM_OPTIONS);
        const app = new CosmosApp.default(sim.getTransport());

        ////////////
        /// TIP you can use zemu commands here to take the app to the point where you trigger a breakpoint

        await debugScenario(sim, app);

        /// TIP

    } finally {
        await sim.close();
        await beforeEnd();
    }
}

(async () => {
    await main();
})();
