var _ = require('lodash'),
    data = '';

process.stdin.on('data', function(str) {
  data += str.toString();
});

process.stdin.on('end', function() {
  var obj = JSON.parse(data);

  _.forEach(obj, function(val) {
    console.log(val.name + '=' + val.value);
  });
});
