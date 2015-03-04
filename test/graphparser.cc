#include "test.h"
#include "mjolnir/osmnode.h"
#include "mjolnir/pbfgraphparser.h"
#include "mjolnir/sequence.h"

#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <valhalla/baldr/graphconstants.h>


using namespace std;
using namespace valhalla::mjolnir;
using namespace valhalla::baldr;

namespace {

void write_config(const std::string& filename) {
  std::ofstream file;
  try {
    file.open(filename, std::ios_base::trunc);
    file << "{ \
      \"mjolnir\": { \
        \"input\": { \
          \"type\": \"protocolbuffer\" \
        }, \
        \"hierarchy\": { \
          \"tile_dir\": \"test/tiles\", \
          \"levels\": [ \
            {\"name\": \"local\", \"level\": 2, \"size\": 0.25}, \
            {\"name\": \"arterial\", \"level\": 1, \"size\": 1, \"importance_cutoff\": \"Tertiary\"}, \
            {\"name\": \"highway\", \"level\": 0, \"size\": 4, \"importance_cutoff\": \"Trunk\"} \
          ] \
        }, \
        \"tagtransform\": { \
          \"node_script\": \"test/lua/vertices.lua\", \
          \"node_function\": \"nodes_proc\", \
          \"way_script\": \"test/lua/edges.lua\", \
          \"way_function\": \"ways_proc\" , \
          \"relation_script\": \"test/lua/edges.lua\", \
          \"relation_function\": \"rels_proc\" \
        } \
      } \
    }";
  }
  catch(...) {

  }
  file.close();
}

const auto node_predicate = [](const OSMWayNode& a, const OSMWayNode& b) {
  return a.node.osmid < b.node.osmid;
};

OSMNode GetNode(uint64_t node_id, sequence<OSMWayNode>& way_nodes) {
  OSMWayNode target{{node_id}};
  if(!way_nodes.find(target, node_predicate))
    throw std::runtime_error("Couldn't find node: " + std::to_string(node_id));
  return target.node;
}

auto way_predicate = [](const OSMWay& a, const OSMWay& b){
  return a.osmwayid_ < b.osmwayid_;
};

OSMWay GetWay(uint64_t way_id, sequence<OSMWay>& ways) {
  OSMWay target{way_id};
  if(!ways.find(target, way_predicate))
    throw std::runtime_error("Couldn't find way: " + std::to_string(way_id));
  return target;
}

void BollardsGates(const std::string& config_file) {
  boost::property_tree::ptree conf;
  boost::property_tree::json_parser::read_json(config_file, conf);

  auto osmdata = PBFGraphParser::Parse(conf.get_child("mjolnir"), {"test/data/liechtenstein-latest.osm.pbf"});
  sequence<OSMWayNode> way_nodes(osmdata.way_node_references_file, false);
  way_nodes.sort(node_predicate);

  //We split set the uses at bollards and gates.
  auto node = GetNode(392700757, way_nodes);
  if (!node.intersection())
    throw std::runtime_error("Bollard not marked as intersection.");

  //We split set the uses at bollards and gates.
  node = GetNode(376947468, way_nodes);
  if (!node.intersection())
    throw std::runtime_error("Gate not marked as intersection.");

  //Is a gate with foot and bike flags set; however, access is private.
  node = GetNode(2949666866, way_nodes);
  if (!node.intersection() ||
      node.type() != NodeType::kGate || node.access_mask() != 6)
    throw std::runtime_error("Gate at end of way test failed.");

  //Is a bollard with foot and bike flags set.
  node = GetNode(569645326, way_nodes);
  if (!node.intersection() ||
      node.type() != NodeType::kBollard || node.access_mask() != 6)
    throw std::runtime_error("Bollard(with flags) not marked as intersection.");

  //Is a bollard=block with foot flag set.
  node = GetNode(1819036441, way_nodes);
  if (!node.intersection() ||
      node.type() != NodeType::kBollard || node.access_mask() != 2)
    throw std::runtime_error("Bollard=block not marked as intersection.");
}

void RemovableBollards(const std::string& config_file) {
  boost::property_tree::ptree conf;
  boost::property_tree::json_parser::read_json(config_file, conf);

  auto osmdata = PBFGraphParser::Parse(conf.get_child("mjolnir"), {"test/data/rome.osm.pbf"});
  sequence<OSMWayNode> way_nodes(osmdata.way_node_references_file, false);
  way_nodes.sort(node_predicate);

  //Is a bollard=rising is saved as a gate...with foot flag and bike set.
  auto node = GetNode(2425784125, way_nodes);
  if (!node.intersection() ||
    node.type() != NodeType::kGate || node.access_mask() != 7)
    throw std::runtime_error("Rising Bollard not marked as intersection.");
}

void Exits(const std::string& config_file) {
  boost::property_tree::ptree conf;
  boost::property_tree::json_parser::read_json(config_file, conf);

  auto osmdata = PBFGraphParser::Parse(conf.get_child("mjolnir"), {"test/data/harrisburg.osm.pbf"});
  sequence<OSMWayNode> way_nodes(osmdata.way_node_references_file, false);
  way_nodes.sort(node_predicate);

  auto node = GetNode(33698177, way_nodes);

  if (!node.intersection() ||
      !node.ref() || osmdata.node_ref[33698177] != "51A-B")
    throw std::runtime_error("Ref not set correctly .");


  node = GetNode(1901353894, way_nodes);

  if (!node.intersection() ||
      !node.ref() || osmdata.node_name[1901353894] != "Harrisburg East")
    throw std::runtime_error("Ref not set correctly .");


  node = GetNode(462240654, way_nodes);

  if (!node.intersection() || osmdata.node_exit_to[462240654] != "PA441")
    throw std::runtime_error("Ref not set correctly .");

}

void Baltimore(const std::string& config_file) {
  boost::property_tree::ptree conf;
  boost::property_tree::json_parser::read_json(config_file, conf);

  auto osmdata = PBFGraphParser::Parse(conf.get_child("mjolnir"), {"test/data/baltimore.osm.pbf"});
  sequence<OSMWay> ways(osmdata.ways_file, false);
  ways.sort(way_predicate);

  // bike_forward and reverse is set to false by default.  Meaning defaults for
  // highway = pedestrian.  Bike overrides bicycle=designated and/or cycleway=shared_lane
  // make it bike_forward and reverse = true
  auto way = GetWay(216240466, ways);
  if (way.auto_forward() != false || way.bike_forward() != true || way.pedestrian() != true ||
      way.auto_backward() != false || way.bike_backward() != true) {
    throw std::runtime_error("Access is not set correctly for way 216240466.");
  }
  // access for all
  way = GetWay(138388359, ways);
  if (way.auto_forward() != true || way.bike_forward() != true || way.pedestrian() != true ||
      way.auto_backward() != true || way.bike_backward() != true) {
    throw std::runtime_error("Access is not set correctly for way 138388359.");
  }
  // footway...pedestrian only
  way = GetWay(133689121, ways);
  if (way.auto_forward() != false || way.bike_forward() != false || way.pedestrian() != true ||
      way.auto_backward() != false || way.bike_backward() != false) {
    throw std::runtime_error("Access is not set correctly for way 133689121.");
  }
  // oneway
  way = GetWay(49641455, ways);
  if (way.auto_forward() != true || way.bike_forward() != true || way.pedestrian() != true ||
      way.auto_backward() != false || way.bike_backward() != false) {
    throw std::runtime_error("Access is not set correctly for way 49641455.");
  }

  // Oneway test.  Make sure auto backward is set for ways where oneway=no.
  way = GetWay(192573108, ways);
  if (way.auto_forward() != true || way.bike_forward() != true || way.pedestrian() != true ||
      way.auto_backward() != true || way.bike_backward() != true) {
    throw std::runtime_error("Forward/Backward/Pedestrian access is not set correctly for way 192573108.");
  }

  sequence<OSMWayNode> way_nodes(osmdata.way_node_references_file, false, true);
  way_nodes.sort(node_predicate);
  auto node = GetNode(49473254, way_nodes);

  if (!node.intersection() || node.type() != NodeType::kTollBooth)
    throw std::runtime_error("Toll Booth 49473254 test failed.");

  auto res = osmdata.restrictions.equal_range(98040438);

  if (res.first == osmdata.restrictions.end())
    throw std::runtime_error("Failed to find 98040438 restrictions.");

  for (auto r = res.first; r != res.second; ++r) {
    if (r->second.to() == 6003340) {
      if (r->second.via() != 2123388822 || r->second.type() != RestrictionType::kNoLeftTurn)
        throw std::runtime_error("98040438 restriction test failed for to: 6003340");
    }
    else if (r->second.to() == 98040438) {
      if (r->second.via() != 2123388822 || r->second.type() != RestrictionType::kNoUTurn)
        throw std::runtime_error("98040438 restriction test failed for to: 98040438");
    }
    else throw std::runtime_error("98040438 restriction test failed.");
  }
}

void BicycleTrafficSignals(const std::string& config_file) {
  boost::property_tree::ptree conf;
  boost::property_tree::json_parser::read_json(config_file, conf);

  auto osmdata = PBFGraphParser::Parse(conf.get_child("mjolnir"), {"test/data/nyc.osm.pbf"});
  sequence<OSMWayNode> way_nodes(osmdata.way_node_references_file, false);
  way_nodes.sort(node_predicate);

  auto node = GetNode(42439096, way_nodes);
  if (!node.intersection() || !node.traffic_signal())
    throw std::runtime_error("Traffic Signal test failed.");
/*
  //When we support finding bike rentals, this test will need updated.
  node = GetNode(3146484929, way_nodes);
  if (node.intersection())
    throw std::runtime_error("Bike rental not marked as intersection.");

  //When we support finding shops that rent bikes, this test will need updated.
  node = GetNode(2592264881, way_nodes);
  if (node.intersection())
    throw std::runtime_error("Bike rental at a shop not marked as intersection.");
*/
}

void DoConfig() {
  //make a config file
  write_config("test/test_config");
}

void TestBollardsGates() {
  //write the tiles with it
  BollardsGates("test/test_config");
}

void TestRemovableBollards() {
  //write the tiles with it
  RemovableBollards("test/test_config");
}

void TestBicycleTrafficSignals() {
  //write the tiles with it
  BicycleTrafficSignals("test/test_config");
}

void TestExits() {
  //write the tiles with it
  Exits("test/test_config");
}

void TestBaltimoreArea() {
  //write the tiles with it
  Baltimore("test/test_config");
}

}

int main() {

  // Test data BBs are as follows:
  // Rome:        <bounds minlat="41.8957000" minlon="12.4820400" maxlat="41.8973400" maxlon="12.4855600"/>
  // NYC:         <bounds minlat="40.7330200" minlon="-74.0136900" maxlat="40.7396900" maxlon="-73.9996000"/>
  // Baltimore:   <bounds minlat="39.2586000" minlon="-76.6081000" maxlat="39.3065000" maxlon="-76.5288000"/>
  // Harrisburg:  <bounds minlat="40.2075000" minlon="-76.8459000" maxlat="40.3136000" maxlon="-76.7474000"/>
  // Liechtenstein: None

  test::suite suite("parser");

  suite.test(TEST_CASE(DoConfig));
  suite.test(TEST_CASE(TestBollardsGates));
  suite.test(TEST_CASE(TestRemovableBollards));
  suite.test(TEST_CASE(TestBicycleTrafficSignals));
  suite.test(TEST_CASE(TestExits));
  suite.test(TEST_CASE(TestBaltimoreArea));

  return suite.tear_down();
}
