/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 *
 */

#include "srsgnb/hdr/stack/ric/e2sm_kpm.h"
#include "srsgnb/hdr/stack/ric/e2sm_kpm_metrics.h"
#include "srsgnb/hdr/stack/ric/e2sm_kpm_report_service.h"
#include <numeric>

const std::string e2sm_kpm::short_name       = "ORAN-E2SM-KPM";
const std::string e2sm_kpm::oid              = "1.3.6.1.4.1.53148.1.2.2.2";
const std::string e2sm_kpm::func_description = "KPM Monitor";
const uint32_t    e2sm_kpm::revision         = 0;

e2sm_kpm::e2sm_kpm(srslog::basic_logger& logger_) : e2sm(short_name, oid, func_description, revision), logger(logger_)
{
  random_gen = srsran_random_init(1234);

  // add supported metrics
  for (auto& metric : get_e2sm_kpm_28_552_metrics()) {
    if (metric.supported) {
      supported_meas_types.push_back(metric);
    }
  }
  for (auto& metric : get_e2sm_kpm_34_425_metrics()) {
    if (metric.supported) {
      supported_meas_types.push_back(metric);
    }
  }
  for (auto& metric : e2sm_kpm_oran_metrics()) {
    if (metric.supported) {
      supported_meas_types.push_back(metric);
    }
  }
  for (auto& metric : e2sm_kpm_custom_metrics()) {
    if (metric.supported) {
      supported_meas_types.push_back(metric);
    }
  }
}

bool e2sm_kpm::generate_ran_function_description(RANfunction_description& desc, ra_nfunction_item_s& ran_func)
{
  desc.function_shortname = short_name;
  desc.function_e2_sm_oid = oid;
  desc.function_desc      = func_description;

  e2_sm_kpm_ra_nfunction_description_s e2sm_kpm_ra_nfunction_description;
  e2sm_kpm_ra_nfunction_description.ran_function_name.ran_function_short_name.from_string(short_name.c_str());
  e2sm_kpm_ra_nfunction_description.ran_function_name.ran_function_e2_sm_oid.from_string(oid.c_str());
  e2sm_kpm_ra_nfunction_description.ran_function_name.ran_function_description.from_string(func_description.c_str());
  if (desc.function_instance) {
    e2sm_kpm_ra_nfunction_description.ran_function_name.ran_function_instance_present = true;
    e2sm_kpm_ra_nfunction_description.ran_function_name.ran_function_instance         = desc.function_instance;
  }

  // O-RAN.WG3.E2SM-KPM-R003-v03.00, 7.3.1 Event Trigger Style Types
  auto& event_trigger_style_list = e2sm_kpm_ra_nfunction_description.ric_event_trigger_style_list;
  event_trigger_style_list.resize(1);
  event_trigger_style_list[0].ric_event_trigger_style_type = 1;
  event_trigger_style_list[0].ric_event_trigger_style_name.from_string("Periodic report");
  event_trigger_style_list[0].ric_event_trigger_format_type = 1; // uses RIC Event Trigger Definition Format 1

  // O-RAN.WG3.E2SM-KPM-R003-v03.00, 7.4.1 REPORT Service Style Type
  /* TODO: seems that flexric does not like this part and crashes
  auto& report_style_list = e2sm_kpm_ra_nfunction_description.ric_report_style_list;
  report_style_list.resize(5);

  report_style_list[0].ric_report_style_type = 1;
  report_style_list[0].ric_report_style_name.from_string("E2 Node Measurement");
  report_style_list[0].ric_action_format_type  = 1;
  report_style_list[0].ric_ind_hdr_format_type = 1;
  report_style_list[0].ric_ind_msg_format_type = 1;
  // A measurement ID can be used for subscription instead of a measurement type if an identifier of a certain
  // measurement type was exposed by an E2 Node via the RAN Function Definition IE.
  // measurement name to ID mapping (local to the E2 node), here only an example:
  report_style_list[0].meas_info_action_list.resize(1);
  report_style_list[0].meas_info_action_list[0].meas_name.from_string("RRU.PrbTotDl");
  report_style_list[0].meas_info_action_list[0].meas_id = 123;

  report_style_list[1].ric_report_style_type = 2;
  report_style_list[1].ric_report_style_name.from_string("E2 Node Measurement for a single UE");
  report_style_list[1].ric_action_format_type  = 2; // includes UE ID
  report_style_list[1].ric_ind_hdr_format_type = 1;
  report_style_list[1].ric_ind_msg_format_type = 1;

  report_style_list[2].ric_report_style_type = 3;
  report_style_list[2].ric_report_style_name.from_string("Condition-based, UE-level E2 Node Measurement");
  report_style_list[2].ric_action_format_type  = 3;
  report_style_list[2].ric_ind_hdr_format_type = 1;
  report_style_list[2].ric_ind_msg_format_type = 2;

  report_style_list[3].ric_report_style_type = 4;
  report_style_list[3].ric_report_style_name.from_string("Common Condition-based, UE-level Measurement");
  report_style_list[3].ric_action_format_type  = 4;
  report_style_list[3].ric_ind_hdr_format_type = 1;
  report_style_list[3].ric_ind_msg_format_type = 3;

  report_style_list[4].ric_report_style_type = 5;
  report_style_list[4].ric_report_style_name.from_string("E2 Node Measurement for multiple UEs");
  report_style_list[4].ric_action_format_type  = 5;
  report_style_list[4].ric_ind_hdr_format_type = 1;
  report_style_list[4].ric_ind_msg_format_type = 3;
  */
  logger.info("Generating RAN function description");
  srsran::unique_byte_buffer_t buf = srsran::make_byte_buffer();
  asn1::bit_ref                bref(buf->msg, buf->get_tailroom());
  if (e2sm_kpm_ra_nfunction_description.pack(bref) != asn1::SRSASN_SUCCESS) {
    printf("Failed to pack TX E2 PDU\n");
    return false;
  }
  buf->N_bytes = bref.distance_bytes();

  ran_func.ran_function_definition.resize(buf->N_bytes);
  buf->msg[1] = 0x30; // TODO: needed to keep wireshak happy, need better fix
  std::copy(buf->msg, buf->msg + buf->N_bytes, ran_func.ran_function_definition.data());

  return true;
}

bool e2sm_kpm::process_ric_event_trigger_definition(ricsubscription_request_s       subscription_request,
                                                    RIC_event_trigger_definition_t& event_def)
{
  e2_sm_kpm_event_trigger_definition_s trigger_def;
  asn1::cbit_ref bref(subscription_request->ricsubscription_details->ric_event_trigger_definition.data(),
                      subscription_request->ricsubscription_details->ric_event_trigger_definition.size());

  if (trigger_def.unpack(bref) != asn1::SRSASN_SUCCESS) {
    return false;
  }

  event_def.type          = RIC_event_trigger_definition_t::e2sm_event_trigger_type_t::E2SM_REPORT;
  event_def.report_period = trigger_def.event_definition_formats.event_definition_format1().report_period;
  return true;
}

bool e2sm_kpm::process_ric_action_definition(ri_caction_to_be_setup_item_s ric_action, E2AP_RIC_action_t& action_entry)
{
  bool                          admit_action = false;
  e2_sm_kpm_action_definition_s e2sm_kpm_action_def;
  asn1::cbit_ref                bref(ric_action.ric_action_definition.data(), ric_action.ric_action_definition.size());

  if (e2sm_kpm_action_def.unpack(bref) != asn1::SRSASN_SUCCESS) {
    return false;
  }

  switch (e2sm_kpm_action_def.ric_style_type) {
    case 1:
      admit_action = e2sm_kpm_report_service_style1::process_ric_action_definition(this, e2sm_kpm_action_def);
      break;
    case 2:
      admit_action = false;
      break;
    case 3:
      admit_action = false;
      break;
    case 4:
      admit_action = false;
      break;
    case 5:
      admit_action = false;
      break;
    default:
      logger.info("Unknown RIC style type %i -> do not admit action %i (type %i)",
                  e2sm_kpm_action_def.ric_style_type,
                  ric_action.ric_action_id,
                  ric_action.ric_action_type);
      return false;
  }

  if (not admit_action) {
    return false;
  }

  action_entry.sm_local_ric_action_id = _generate_local_action_id();
  e2sm_kpm_report_service* report_service;
  switch (e2sm_kpm_action_def.ric_style_type) {
    case 1:
      report_service =
          new e2sm_kpm_report_service_style1(this, action_entry.sm_local_ric_action_id, e2sm_kpm_action_def);
      break;
    case 2:
      admit_action = false;
      break;
    case 3:
      admit_action = false;
      break;
    case 4:
      admit_action = false;
      break;
    case 5:
      admit_action = false;
      break;
    default:
      logger.info("Unknown RIC style type %i -> do not admit action %i (type %i)",
                  e2sm_kpm_action_def.ric_style_type,
                  ric_action.ric_action_id,
                  ric_action.ric_action_type);
      return false;
  }

  registered_actions_data.insert(
      std::pair<uint32_t, e2sm_kpm_report_service*>(action_entry.sm_local_ric_action_id, report_service));

  return admit_action;
}

bool e2sm_kpm::remove_ric_action_definition(E2AP_RIC_action_t& action_entry)
{
  if (registered_actions_data.count(action_entry.sm_local_ric_action_id)) {
    delete registered_actions_data.at(action_entry.sm_local_ric_action_id);
    registered_actions_data.erase(action_entry.sm_local_ric_action_id);
    return true;
  }
  return false;
}

bool e2sm_kpm::execute_action_fill_ric_indication(E2AP_RIC_action_t& action_entry, ric_indication_t& ric_indication)
{
  uint32_t action_id = action_entry.sm_local_ric_action_id;
  if (!registered_actions_data.count(action_id)) {
    logger.info("Unknown RIC action ID: %i (type %i)  (SM local RIC action ID: %i)",
                action_entry.ric_action_id,
                action_entry.ric_action_type,
                action_entry.sm_local_ric_action_id);
    return false;
  }
  e2sm_kpm_report_service* report_service = registered_actions_data.at(action_id);
  ric_indication.indication_type          = ri_cind_type_opts::report;

  // header is the same for all RIC service styles, i.e., type 1
  ric_indication.ri_cind_hdr = srsran::make_byte_buffer();
  this->_generate_indication_header(report_service->get_ind_hdr(), ric_indication.ri_cind_hdr);

  logger.info("Generating E2-SM-KPM Indication Message");
  ric_indication.ri_cind_msg = srsran::make_byte_buffer();
  this->_generate_indication_message(report_service->get_ind_msg(), ric_indication.ri_cind_msg);

  // clear data collected for this action
  report_service->clear_collected_data();
  return true;
}

bool e2sm_kpm::_generate_indication_header(e2_sm_kpm_ind_hdr_s& hdr, srsran::unique_byte_buffer_t& buf)
{
  asn1::bit_ref bref(buf->msg, buf->get_tailroom());
  if (hdr.pack(bref) != asn1::SRSASN_SUCCESS) {
    printf("IND HEADER: Failed to pack TX E2 PDU\n");
    return false;
  }
  buf->N_bytes = bref.distance_bytes();

  return true;
}

bool e2sm_kpm::_generate_indication_message(e2_sm_kpm_ind_msg_s& msg, srsran::unique_byte_buffer_t& buf)
{
  logger.info("Generating E2-SM-KPM Indication Message");
  asn1::bit_ref bref(buf->msg, buf->get_tailroom());
  if (msg.pack(bref) != asn1::SRSASN_SUCCESS) {
    printf("IND MSG: Failed to pack TX E2 PDU\n");
    return false;
  }
  buf->N_bytes = bref.distance_bytes();

  return true;
}

bool e2sm_kpm::_get_meas_definition(std::string meas_name, E2SM_KPM_metric_t& def)
{
  auto name_matches = [&meas_name](const E2SM_KPM_metric_t& x) {
    return (x.name == meas_name.c_str() or x.name == meas_name);
  };
  auto it = std::find_if(supported_meas_types.begin(), supported_meas_types.end(), name_matches);
  if (it == supported_meas_types.end()) {
    return false;
  }
  def = *it;
  return true;
}

void e2sm_kpm::receive_e2_metrics_callback(const enb_metrics_t& m)
{
  last_enb_metrics = m;
  logger.debug("e2sm_kpm received new enb metrics, CPU0 Load: %.1f", last_enb_metrics.sys.cpu_load[0]);

  for (auto& it : registered_actions_data) {
    e2sm_kpm_report_service* report_service = it.second;
    report_service->collect_data(m);
  }
}

bool e2sm_kpm::_extract_last_integer_type_meas_value(E2SM_KPM_meas_value_t& meas_value,
                                                     const enb_metrics_t&   enb_metrics)
{
  // TODO: maybe add ID to metric types in e2sm_kpm_metrics definitions, so we do not have to compare strings?
  // TODO: make string comparison case insensitive
  // all integer type measurements
  // test: no_label
  if (meas_value.name.c_str() == std::string("test")) {
    switch (meas_value.label) {
      case NO_LABEL:
        meas_value.integer_value = (int32_t)enb_metrics.sys.cpu_load[0];
        printf("extract last \"test\" value as int, (filled with CPU0_load) value %i \n", meas_value.integer_value);
        return true;
      default:
        return false;
    }
  }

  // random_int: no_label
  if (meas_value.name.c_str() == std::string("random_int")) {
    switch (meas_value.label) {
      case NO_LABEL:
        meas_value.integer_value = srsran_random_uniform_int_dist(random_gen, 0, 100);
        printf("extract last \"random_int\" value as int, random value %i \n", meas_value.integer_value);
        return true;
      default:
        return false;
    }
  }

  return false;
}

bool e2sm_kpm::_extract_last_real_type_meas_value(E2SM_KPM_meas_value_t& meas_value, const enb_metrics_t& enb_metrics)
{
  // all real type measurements
  // cpu0_load: no_label
  if (meas_value.name.c_str() == std::string("cpu0_load")) {
    switch (meas_value.label) {
      case NO_LABEL:
        meas_value.real_value = enb_metrics.sys.cpu_load[0];
        return true;
      default:
        return false;
    }
  }

  // cpu_load: min,max,avg
  if (meas_value.name.c_str() == std::string("cpu_load")) {
    uint32_t size;
    switch (meas_value.label) {
      case MIN_LABEL:
        meas_value.real_value = *std::min_element(enb_metrics.sys.cpu_load.begin(), enb_metrics.sys.cpu_load.end());
        return true;
      case MAX_LABEL:
        meas_value.real_value = *std::max_element(enb_metrics.sys.cpu_load.begin(), enb_metrics.sys.cpu_load.end());
        return true;
      case AVG_LABEL:
        size = enb_metrics.sys.cpu_load.size();
        meas_value.real_value =
            std::accumulate(enb_metrics.sys.cpu_load.begin(), enb_metrics.sys.cpu_load.end(), 0.0 / size);
        return true;
      default:
        return false;
    }
  }

  return false;
}