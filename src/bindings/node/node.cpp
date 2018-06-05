#include <node_api.h>

#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>
#include <sstream>
#include <string>
#include <iostream>

#include "valhalla/tyr/actor.h"

boost::property_tree::ptree Configuration;

boost::property_tree::ptree json_to_pt(const std::string& json) {
  std::stringstream ss;
  ss << json;
  boost::property_tree::ptree pt;
  boost::property_tree::read_json(ss, pt);
  return pt;
}

boost::property_tree::ptree make_conf(const std::string config) {
  return json_to_pt(config);
}

napi_value Route(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value myStr;

  size_t argc = 1;
  napi_value argv[1];

  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Failed to parse arguments");
  }

  char buffer[1024];
  size_t out;

  status = napi_get_value_string_utf8(env, argv[0], buffer, 1024, &out);

  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Failed to parse first argument to string");
  }
  std::string reqString(buffer, out);
  std::string route_json;
  try {
    valhalla::tyr::actor_t actor(Configuration);
    route_json = actor.route(reqString);
    actor.cleanup();
  } catch (const std::exception& e) {
    napi_throw_error(env, NULL, e.what());
  }

  const char* outBuff = route_json.c_str();
  const auto nchars = route_json.size();

  status = napi_create_string_utf8(env, outBuff, nchars, &myStr);
  return myStr;
}

napi_value Setup(napi_env env, napi_callback_info info) {
  napi_status status;
  size_t argc = 1;
  napi_value argv[1];

  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Failed to parse arguments");
  }

  char buffer[5000];
  size_t buffer_size;

  // parse config string passed in
  status = napi_get_value_string_utf8(env, argv[0], buffer, 5000, &buffer_size);

  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Failed to parse config to string");
  }

  // parse config string into ptree, save in Configuration constant
  const std::string parsed_config_string(buffer, buffer_size);
  Configuration = make_conf(parsed_config_string);

  // create empty object to set functions on
  napi_value return_obj;
  status = napi_create_object(env, &return_obj);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Failed to create result object");
  }
  // create route function and return it
  napi_value fn;
  status = napi_create_function(env, "route", NAPI_AUTO_LENGTH, Route, nullptr, &fn);

  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Failed to create route function");
  }

  status = napi_set_named_property(env, return_obj, "route", fn);

  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Failed to set route function on return object");
  }

  return return_obj;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_value new_exports;
  napi_status status =
      napi_create_function(env, "", NAPI_AUTO_LENGTH, Setup, nullptr, &new_exports);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Failed to create Setup function");
  }
  return new_exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init);
