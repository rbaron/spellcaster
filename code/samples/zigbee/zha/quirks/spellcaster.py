"""Device handler for the spellcaster magic wand."""

from zigpy.quirks import CustomDevice
from zhaquirks.const import (
    CLUSTER_ID,
    COMMAND,
    COMMAND_STEP,
    SKIP_CONFIGURATION,
    DEVICE_TYPE,
    ENDPOINT_ID,
    ENDPOINTS,
    INPUT_CLUSTERS,
    MODELS_INFO,
    OUTPUT_CLUSTERS,
    PARAMS,
    PROFILE_ID,
)

ENDPOINTS_ = {
    10: {
        PROFILE_ID: 0x0104,
        DEVICE_TYPE: 0x0006,
        INPUT_CLUSTERS: [
            0x0000,
            0x0001,
            0x0003,
        ],
        OUTPUT_CLUSTERS: [
            0x0003,
            0x0004,
            0x0005,
            0x0006,
            0x0008,
        ],
    },
}


def slot_trigger(step_size):
    return {
        COMMAND: COMMAND_STEP,
        CLUSTER_ID: 8,
        ENDPOINT_ID: 10,
        PARAMS: {"step_mode": 1, "step_size": step_size},
    }


class Spellcaster(CustomDevice):
    signature = {
        MODELS_INFO: [("rbaron", "spellcaster")],
        ENDPOINTS: ENDPOINTS_,
    }

    replacement = {
        SKIP_CONFIGURATION: False,
        ENDPOINTS: ENDPOINTS_,
    }

    device_automation_triggers = {
        ("spell", f"{slot + 1}"): slot_trigger(slot) for slot in range(10)
    }
