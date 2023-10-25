#ifndef _ZB_SC_H_
#define _ZB_SC_H_

// https://www.rfwireless-world.com/Terminology/Zigbee-Profile-ID-list.html.
// 0x0006 is the Remote Control device ID.
#define ZB_SC_DEVICE_ID 0x0006
#define ZB_DEVICE_VER_SC 0
#define ZB_SC_IN_CLUSTER_NUM 3
#define ZB_SC_OUT_CLUSTER_NUM 5
#define ZB_SC_CLUSTER_NUM (ZB_SC_IN_CLUSTER_NUM + ZB_SC_OUT_CLUSTER_NUM)
#define ZB_SC_REPORT_ATTR_COUNT 1

#define ZB_DECLARE_SC_CLUSTER_LIST(                                         \
    cluster_list_name, basic_server_attr_list, identify_client_attr_list,   \
    identify_server_attr_list, scenes_client_attr_list,                     \
    groups_client_attr_list, on_off_client_attr_list,                       \
    level_control_client_attr_list, batt_attr_list)                         \
  zb_zcl_cluster_desc_t cluster_list_name[] = {                             \
      ZB_ZCL_CLUSTER_DESC(                                                  \
          ZB_ZCL_CLUSTER_ID_BASIC,                                          \
          ZB_ZCL_ARRAY_SIZE(basic_server_attr_list, zb_zcl_attr_t),         \
          (basic_server_attr_list), ZB_ZCL_CLUSTER_SERVER_ROLE,             \
          ZB_ZCL_MANUF_CODE_INVALID),                                       \
      ZB_ZCL_CLUSTER_DESC(                                                  \
          ZB_ZCL_CLUSTER_ID_IDENTIFY,                                       \
          ZB_ZCL_ARRAY_SIZE(identify_server_attr_list, zb_zcl_attr_t),      \
          (identify_server_attr_list), ZB_ZCL_CLUSTER_SERVER_ROLE,          \
          ZB_ZCL_MANUF_CODE_INVALID),                                       \
      ZB_ZCL_CLUSTER_DESC(ZB_ZCL_CLUSTER_ID_POWER_CONFIG,                   \
                          ZB_ZCL_ARRAY_SIZE(batt_attr_list, zb_zcl_attr_t), \
                          (batt_attr_list), ZB_ZCL_CLUSTER_SERVER_ROLE,     \
                          ZB_ZCL_MANUF_CODE_INVALID),                       \
      ZB_ZCL_CLUSTER_DESC(                                                  \
          ZB_ZCL_CLUSTER_ID_IDENTIFY,                                       \
          ZB_ZCL_ARRAY_SIZE(identify_client_attr_list, zb_zcl_attr_t),      \
          (identify_client_attr_list), ZB_ZCL_CLUSTER_CLIENT_ROLE,          \
          ZB_ZCL_MANUF_CODE_INVALID),                                       \
      ZB_ZCL_CLUSTER_DESC(                                                  \
          ZB_ZCL_CLUSTER_ID_SCENES,                                         \
          ZB_ZCL_ARRAY_SIZE(scenes_client_attr_list, zb_zcl_attr_t),        \
          (scenes_client_attr_list), ZB_ZCL_CLUSTER_CLIENT_ROLE,            \
          ZB_ZCL_MANUF_CODE_INVALID),                                       \
      ZB_ZCL_CLUSTER_DESC(                                                  \
          ZB_ZCL_CLUSTER_ID_GROUPS,                                         \
          ZB_ZCL_ARRAY_SIZE(groups_client_attr_list, zb_zcl_attr_t),        \
          (groups_client_attr_list), ZB_ZCL_CLUSTER_CLIENT_ROLE,            \
          ZB_ZCL_MANUF_CODE_INVALID),                                       \
      ZB_ZCL_CLUSTER_DESC(                                                  \
          ZB_ZCL_CLUSTER_ID_ON_OFF,                                         \
          ZB_ZCL_ARRAY_SIZE(on_off_client_attr_list, zb_zcl_attr_t),        \
          (on_off_client_attr_list), ZB_ZCL_CLUSTER_CLIENT_ROLE,            \
          ZB_ZCL_MANUF_CODE_INVALID),                                       \
      ZB_ZCL_CLUSTER_DESC(                                                  \
          ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,                                  \
          ZB_ZCL_ARRAY_SIZE(level_control_client_attr_list, zb_zcl_attr_t), \
          (level_control_client_attr_list), ZB_ZCL_CLUSTER_CLIENT_ROLE,     \
          ZB_ZCL_MANUF_CODE_INVALID),                                       \
  }

#define ZB_ZCL_DECLARE_SC_SIMPLE_DESC(ep_name, ep_id, in_clust_num, \
                                      out_clust_num)                \
  ZB_DECLARE_SIMPLE_DESC(in_clust_num, out_clust_num);              \
  ZB_AF_SIMPLE_DESC_TYPE(in_clust_num, out_clust_num)               \
  simple_desc_##ep_name = {ep_id,                                   \
                           ZB_AF_HA_PROFILE_ID,                     \
                           ZB_SC_DEVICE_ID,                         \
                           ZB_DEVICE_VER_SC,                        \
                           0,                                       \
                           in_clust_num,                            \
                           out_clust_num,                           \
                           {                                        \
                               ZB_ZCL_CLUSTER_ID_BASIC,             \
                               ZB_ZCL_CLUSTER_ID_IDENTIFY,          \
                               ZB_ZCL_CLUSTER_ID_POWER_CONFIG,      \
                               ZB_ZCL_CLUSTER_ID_IDENTIFY,          \
                               ZB_ZCL_CLUSTER_ID_SCENES,            \
                               ZB_ZCL_CLUSTER_ID_GROUPS,            \
                               ZB_ZCL_CLUSTER_ID_ON_OFF,            \
                               ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,     \
                           }}

#define ZB_DECLARE_SC_EP(ep_name, ep_id, cluster_list)                      \
  ZBOSS_DEVICE_DECLARE_REPORTING_CTX(reporting_ctx_##ep_name,               \
                                     ZB_SC_REPORT_ATTR_COUNT);              \
  ZB_ZCL_DECLARE_SC_SIMPLE_DESC(ep_name, ep_id, ZB_SC_IN_CLUSTER_NUM,       \
                                ZB_SC_OUT_CLUSTER_NUM);                     \
                                                                            \
  ZB_AF_DECLARE_ENDPOINT_DESC(                                              \
      ep_name, ep_id, ZB_AF_HA_PROFILE_ID, 0, NULL,                         \
      ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), cluster_list, \
      (zb_af_simple_desc_1_1_t *)&simple_desc_##ep_name,                    \
      ZB_SC_REPORT_ATTR_COUNT, reporting_ctx_##ep_name, 0, NULL)

#endif  // _ZB_SC_H_
