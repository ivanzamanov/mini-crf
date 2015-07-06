var _ = require('lodash'),
    data = '',
    fs = require('fs');

debugger;
data = fs.readFileSync(process.argv[2]);

var obj = JSON.parse(data);

var min = {value: obj[0].value };
_.forEach(obj, function(entry) {
    if(entry.value <= min.value)
        min = entry;
});

console.error("Min value: " + min.value);
_.forEach(min.args, function(arg) {
    console.log(arg.name + "=" + arg.value);
});
