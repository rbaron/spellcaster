// #include <ram_pwrdn.h>
#include <sclib/adc.h>
#include <sclib/button.h>
#include <sclib/caster.h>
#include <sclib/led.h>
#include <sclib/vibration.h>
#include <stdint.h>
#include <zb_nrf_platform.h>
#include <zboss_api.h>
#include <zboss_api_addons.h>
#include <zcl/zb_zcl_power_config.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zigbee/zigbee_app_utils.h>
#include <zigbee/zigbee_error_handler.h>

#include "factory_reset.h"
#include "zb_sc_defs.h"

#define SC_ENDPOINT 10

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

// Power configuration cluster - section 3.3.2.2.3.
typedef struct {
  // Units of 100 mV. 0x00 - 0xff (optional, not reportable :()).
  zb_uint8_t voltage;
  // Units of 0.5%. 0x00 (0%) - 0xc8 (100%) (optional, reportable).
  zb_uint8_t percentage;
  // Whole number of battery cells used to power device
  zb_uint8_t quantity;
  // Enumeration of battery size
  enum zb_zcl_power_config_battery_size_e size;
} sc_batt_attrs_t;

struct zb_device_ctx {
  // zb_zcl_basic_attrs_t basic_attr;
  zb_zcl_basic_attrs_ext_t basic_attr;
  zb_zcl_identify_attrs_t identify_attr;
  sc_batt_attrs_t batt_attrs;
};

static struct zb_device_ctx dev_ctx;

static bool joining_signal_received = false;
static bool stack_initialised = false;

static void led_flashing_cb(struct k_timer *timer) {
  sc_led_toggle();
}

K_TIMER_DEFINE(led_flashing_timer, led_flashing_cb, /*stop_fn=*/NULL);

static zb_zcl_status_t sc_zb_set_attr_value(zb_uint16_t cluster_id,
                                            zb_uint16_t attr_id, void *data) {
  return zb_zcl_set_attr_val(SC_ENDPOINT, cluster_id,
                             ZB_ZCL_CLUSTER_SERVER_ROLE, attr_id,
                             (zb_uint8_t *)data, ZB_FALSE);
}

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(
    basic_server_attr_list,
    /*zcl_version=*/&dev_ctx.basic_attr.zcl_version,
    /*app_version=*/&dev_ctx.basic_attr.app_version,
    /*stack_version=*/&dev_ctx.basic_attr.stack_version,
    /*hw_version=*/&dev_ctx.basic_attr.hw_version,
    /*mf_name=*/&dev_ctx.basic_attr.mf_name,
    /*model_id=*/&dev_ctx.basic_attr.model_id,
    /*date_code=*/&dev_ctx.basic_attr.date_code,
    /*power_source=*/&dev_ctx.basic_attr.power_source,
    /*location_id=*/&dev_ctx.basic_attr.location_id,
    /*ph_env=*/&dev_ctx.basic_attr.ph_env,
    /*sw_ver=*/&dev_ctx.basic_attr.sw_ver);

// https://devzone.nordicsemi.com/f/nordic-q-a/85315/zboss-declare-power-config-attribute-list-for-battery-bat_num
#define bat_num
ZB_ZCL_DECLARE_POWER_CONFIG_BATTERY_ATTRIB_LIST_EXT(
    batt_attr_list, &dev_ctx.batt_attrs.voltage,
    /*battery_size=*/&dev_ctx.batt_attrs.size,
    /*battery_quantity=*/&dev_ctx.batt_attrs.quantity,
    /*battery_rated_voltage=*/NULL,
    /*battery_alarm_mask=*/NULL,
    /*battery_voltage_min_threshold=*/NULL,
    /*battery_percentage_remaining=*/&dev_ctx.batt_attrs.percentage,
    /*battery_voltage_threshold1=*/NULL,
    /*battery_voltage_threshold2=*/NULL,
    /*battery_voltage_threshold3=*/NULL,
    /*battery_percentage_min_threshold=*/NULL,
    /*battery_percentage_threshold1=*/NULL,
    /*battery_percentage_threshold2=*/NULL,
    /*battery_percentage_threshold3=*/NULL,
    /*battery_alarm_state=*/NULL);

ZB_ZCL_DECLARE_IDENTIFY_CLIENT_ATTRIB_LIST(identify_client_attr_list);

ZB_ZCL_DECLARE_IDENTIFY_SERVER_ATTRIB_LIST(
    identify_server_attr_list, &dev_ctx.identify_attr.identify_time);

ZB_ZCL_DECLARE_SCENES_CLIENT_ATTRIB_LIST(scenes_client_attr_list);

ZB_ZCL_DECLARE_GROUPS_CLIENT_ATTRIB_LIST(groups_client_attr_list);

ZB_ZCL_DECLARE_ON_OFF_CLIENT_ATTRIB_LIST(on_off_client_attr_list);

ZB_ZCL_DECLARE_LEVEL_CONTROL_CLIENT_ATTRIB_LIST(level_control_client_attr_list);

ZB_DECLARE_SC_CLUSTER_LIST(sc_clusters, basic_server_attr_list,
                           identify_client_attr_list, identify_server_attr_list,
                           scenes_client_attr_list, groups_client_attr_list,
                           on_off_client_attr_list,
                           level_control_client_attr_list, batt_attr_list);

ZB_DECLARE_SC_EP(sc_ep, SC_ENDPOINT, sc_clusters);

ZBOSS_DECLARE_DEVICE_CTX_1_EP(sc_ctx, sc_ep);

static void app_clusters_attr_init(void) {
  dev_ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
  dev_ctx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_BATTERY;

  ZB_ZCL_SET_STRING_VAL(
      dev_ctx.basic_attr.mf_name, CONFIG_SC_ZB_MANUFACTURER_NAME,
      ZB_ZCL_STRING_CONST_SIZE(CONFIG_SC_ZB_MANUFACTURER_NAME));

  ZB_ZCL_SET_STRING_VAL(dev_ctx.basic_attr.model_id, CONFIG_SC_ZB_MODEL_ID,
                        ZB_ZCL_STRING_CONST_SIZE(CONFIG_SC_ZB_MODEL_ID));

  ZB_ZCL_SET_STRING_VAL(dev_ctx.basic_attr.date_code, CONFIG_SC_ZB_BUILD_DATE,
                        ZB_ZCL_STRING_CONST_SIZE(CONFIG_SC_ZB_BUILD_DATE));

  dev_ctx.basic_attr.hw_version = CONFIG_SC_ZB_HARDWARE_VERSION;

  dev_ctx.batt_attrs.quantity = 1;
  dev_ctx.batt_attrs.size = ZB_ZCL_POWER_CONFIG_BATTERY_SIZE_OTHER;

  dev_ctx.identify_attr.identify_time =
      ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
}

void update_battery_cb(zb_uint8_t arg) {
  LOG_DBG("Reading battery & updating power cluster...");

  // Reschedule the same callback.
  ZB_ERROR_CHECK(ZB_SCHEDULE_APP_ALARM(
      update_battery_cb,
      /*param=*/0,
      ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000 *
                                         CONFIG_SC_ZB_BATT_READ_INTERVAL_SEC)));

  // TODO: fix battery adc.
  // Read batt voltage & percentage.
  sc_batt_t batt_read;
  __ASSERT(!sc_adc_batt_read(&batt_read), "Failed to call sc_adc_batt_read()");

  // Battery voltage in units of 100 mV.
  uint8_t batt_voltage = batt_read.adc_read.millivolts / 100;
  sc_zb_set_attr_value(ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
                       ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID,
                       &batt_voltage);

  // Battery percentage in units of 0.5%.
  zb_uint8_t batt_percentage = 2 * 100 * batt_read.percentage;
  sc_zb_set_attr_value(ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
                       ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID,
                       &batt_percentage);
}

void zboss_signal_handler(zb_bufid_t bufid) {
  // See zigbee_default_signal_handler() for all available signals.
  zb_zdo_app_signal_hdr_t *sig_hndler = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(bufid,
                                                   /*sg_p=*/&sig_hndler);
  zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(bufid);
  switch (sig) {
    case ZB_BDB_SIGNAL_STEERING:         // New network.
    case ZB_BDB_SIGNAL_DEVICE_REBOOT: {  // Previously joined network.
      LOG_DBG("Steering complete. Status: %d", status);
      if (status == RET_OK) {
        LOG_DBG("Steering successful. Status: %d", status);
        k_timer_stop(&led_flashing_timer);
        sc_led_off();
        // Update the long polling parent interval - needs to be done after
        // joining.
        zb_zdo_pim_set_long_poll_interval(
            1000 * CONFIG_SC_ZB_PARENT_POLL_INTERVAL_SEC);
      } else {
        LOG_DBG("Steering failed. Status: %d", status);
      }
      joining_signal_received = true;
      break;
    }
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      joining_signal_received = true;
      break;
    case ZB_ZDO_SIGNAL_LEAVE:
      if (status == RET_OK) {
        k_timer_start(&led_flashing_timer, K_NO_WAIT, K_SECONDS(1));
        zb_zdo_signal_leave_params_t *leave_params =
            ZB_ZDO_SIGNAL_GET_PARAMS(sig_hndler, zb_zdo_signal_leave_params_t);
        LOG_DBG("Network left (leave type: %d)", leave_params->leave_type);

        // Set joining_signal_received to false so broken rejoin procedure can
        // be detected correctly.
        if (leave_params->leave_type == ZB_NWK_LEAVE_TYPE_REJOIN) {
          joining_signal_received = false;
        }
      }
      break;
    case ZB_ZDO_SIGNAL_SKIP_STARTUP: {
      stack_initialised = true;
      LOG_DBG("Started zigbee stack and waiting for connection to network.");
      k_timer_start(&led_flashing_timer, K_NO_WAIT, K_SECONDS(1));
      ZB_ERROR_CHECK(ZB_SCHEDULE_APP_ALARM(
          update_battery_cb,
          /*param=*/0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000)));
      break;
    }
    case ZB_NLME_STATUS_INDICATION: {
      zb_zdo_signal_nlme_status_indication_params_t *nlme_status_ind =
          ZB_ZDO_SIGNAL_GET_PARAMS(
              sig_hndler, zb_zdo_signal_nlme_status_indication_params_t);
      if (nlme_status_ind->nlme_status.status ==
          ZB_NWK_COMMAND_STATUS_PARENT_LINK_FAILURE) {
        // Nordic's suggested workaround for errata KRKNWK-12017.
        // https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/known_issues.html?v=v2-3-0
        if (stack_initialised && !joining_signal_received) {
          zb_reset(0);
        }
      }
      break;
    }
  }

  ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));
  if (bufid) {
    zb_buf_free(bufid);
  }
}

static void sc_send_on_off(zb_bufid_t bufid, zb_uint16_t cmd_id) {
  LOG_DBG("Send ON/OFF command: %d", cmd_id);
  // Coordinator.
  zb_uint16_t addr = 0x0;
  ZB_ZCL_ON_OFF_SEND_REQ(bufid, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, 1,
                         SC_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                         ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cmd_id, NULL);
}

static void sc_send_step_up(zb_bufid_t bufid, zb_uint16_t steps) {
  LOG_DBG("Send step up command: steps %d", steps);
  // Coordinator.
  zb_uint16_t addr = 0x0;
  ZB_ZCL_LEVEL_CONTROL_SEND_STEP_REQ(
      bufid, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
      /*endpoint=*/1, SC_ENDPOINT, ZB_AF_HA_PROFILE_ID,
      ZB_ZCL_DISABLE_DEFAULT_RESPONSE, NULL, ZB_ZCL_LEVEL_CONTROL_STEP_MODE_UP,
      steps,
      /*transaction_time=*/2);
}

static void sc_send_step_down(zb_bufid_t bufid, zb_uint16_t steps) {
  LOG_DBG("Send step command: steps %d", steps);
  // Coordinator.
  zb_uint16_t addr = 0x0;
  ZB_ZCL_LEVEL_CONTROL_SEND_STEP_REQ(
      bufid, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
      /*endpoint=*/1, SC_ENDPOINT, ZB_AF_HA_PROFILE_ID,
      ZB_ZCL_DISABLE_DEFAULT_RESPONSE, NULL,
      ZB_ZCL_LEVEL_CONTROL_STEP_MODE_DOWN, steps,
      /*transaction_time=*/2);
}

static void sc_send_move_to_level(zb_bufid_t bufid, zb_uint16_t level) {
  LOG_DBG("Send move to level command: level %d", level);
  // Coordinator.
  zb_uint16_t addr = 0x0;
  ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_TO_LEVEL_WITH_ON_OFF_REQ_ZCL8(
      bufid, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
      /*endpoint=*/1, SC_ENDPOINT, ZB_AF_HA_PROFILE_ID,
      ZB_ZCL_DISABLE_DEFAULT_RESPONSE, NULL, level,
      /*transition_time=*/2, 0, 0);
}

void led_flash_cb(zb_uint8_t times) {
  if (!times) {
    return;
  }

  sc_vib_flash(1);
  // Reschedule the same callback.
  ZB_ERROR_CHECK(ZB_SCHEDULE_APP_ALARM(
      led_flash_cb,
      /*param=*/times - 1, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500)));
}

void identify_cb(zb_bufid_t bufid) {
  LOG_DBG("Remote identify command called");
  ZB_ERROR_CHECK(ZB_SCHEDULE_APP_ALARM(
      led_flash_cb,
      /*param=*/10, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500)));
}

void caster_cb(uint8_t slot) {
  LOG_DBG("Caster callback for slot %d", slot);
  if (zb_buf_get_out_delayed_ext(sc_send_step_down, slot, 0) != RET_OK) {
    LOG_ERR("Failed to get buffer for step down command");
    return;
  }
}

int main(void) {
  __ASSERT_NO_MSG(!sc_caster_init(caster_cb));
  __ASSERT_NO_MSG(!sc_adc_init());
  __ASSERT_NO_MSG(!sc_zb_factory_reset_check());

  zigbee_configure_sleepy_behavior(true);
  // power_down_unused_ram();

  ZB_AF_REGISTER_DEVICE_CTX(&sc_ctx);
  ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(SC_ENDPOINT, identify_cb);

  app_clusters_attr_init();

  zigbee_enable();

  while (true) {
    k_sleep(K_FOREVER);
  }
}
