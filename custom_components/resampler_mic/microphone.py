import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import microphone
from esphome.const import CONF_ID

# Wymagane przez walidator audio w ESPHome 2025.12
CONF_MIN_CHANNELS = "min_channels"
CONF_MAX_CHANNELS = "max_channels"

DEPENDENCIES = ["microphone"]

resampler_mic_ns = cg.esphome_ns.namespace("resampler_mic")
ResamplerMicrophone = resampler_mic_ns.class_(
    "ResamplerMicrophone", microphone.Microphone, cg.Component
)

CONF_SOURCE_MIC_ID = "source_mic_id"
CONF_GAIN = "gain"

CONFIG_SCHEMA = microphone.MICROPHONE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ResamplerMicrophone),
        cv.Required(CONF_SOURCE_MIC_ID): cv.use_id(microphone.Microphone),
        cv.Optional(CONF_GAIN, default=1.0): cv.float_,
        # Dodajemy domyślne wartości kanałów, aby zadowolić walidator audio
        cv.Optional(CONF_MIN_CHANNELS, default=1): cv.int_,
        cv.Optional(CONF_MAX_CHANNELS, default=1): cv.int_,
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await microphone.register_microphone(var, config)

    source_mic = await cg.get_variable(config[CONF_SOURCE_MIC_ID])
    cg.add(var.set_source_mic(source_mic))
    cg.add(var.set_gain(config[CONF_GAIN]))
