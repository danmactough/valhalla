const test = require('tape');
var Valhalla = require('../../');

var config = require('./fixtures/basic_config');

test('route: can get a route in Hershey', function(assert) {
  var valhalla = Valhalla(JSON.stringify(config));
  var hersheyRequest = '{"locations":[{"lat":40.546115,"lon":-76.385076,"type":"break"}, {"lat":40.544232,"lon":-76.385752,"type":"break"}],"costing":"auto"}';
  assert.ok(valhalla.route(hersheyRequest));
  // assert some more stuff about the route
  assert.end();
});
