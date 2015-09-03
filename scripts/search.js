var fs = require('fs'),
    async = require('async'),
    _ = require('lodash'),
    printf = require('sprintf-js').sprintf;

var R = 0.61803;

function pickNext(a, b, c) {
    if (c - b > b - a)
        return b + R * (c - b);
    else
        return b - R * (b - a);
}

function goldenSearchStep(a, b, c, func, callback) {
    var x = pickNext(a, b, c);
    async.mapLimit([x, b], 1, func, function(err, values) {
        var xVal = values[0], bVal = values[1];
        if (xVal < bVal) {
            if (c - b > b - a) {
                callback(b, x, c, func);
            } else {
                callback(a, x, b, func);
            }
        } else {
            if (c - b > b - a) {
                callback(a, b, x, func);
            } else {
                callback(x, b, c, func);
            }
        }
    });
}

function MultiParamFunction(reevaluate, valuesFilePath) {
    var values;
    if(valuesFilePath && fs.existsSync(valuesFilePath)) {
        values = JSON.parse(fs.readFileSync(valuesFilePath));
        if(!Array.isArray(values))
            values = [];
    } else {
        values = [];
    }
    values = _.filter(values, function(val) {
      //console.log('filter');
      return val.value !== undefined;
    });

    return function (args, callback) {
        var result;
        _.forEach(values, function(valuesEntry) {
            if(_.isEqual(args, valuesEntry.args) && valuesEntry.value !== undefined) {
                result = valuesEntry.value;
                return false;
            }
            return true;
        });

        if(result === undefined) {
            reevaluate(args, function(value) {
                values.push({
                    value: value,
                    args: _.cloneDeep(args)
                });
                if(!value) {
                    debugger;
                    console.log('Bad value', value, args);
                }
                fs.writeFileSync(valuesFilePath, JSON.stringify(values, null, 1));
                callback(null, value);
            });
        } else {
            console.log('Found in cache');
            // Value found, yey
            callback(null, result);
        }
    };
}

var matrixSize = 100;
var ranges = [
    {name: 'dim1', values: [0, R * matrixSize, matrixSize]},
    {name: 'dim2', values: [0, R * matrixSize, matrixSize]}
];

var maxX = 5, maxY = 5;
function matrixFunc(args, callback) {
    var dist = (maxX - args[1]) * (maxX - args[1]) + (maxY - args[0]) * (maxY - args[0]);
    debugger;
    callback(dist);
}

function matrixGoldenSearch(toEval) {
    var dimension = 0;
    var multiParamFunc = MultiParamFunction(toEval);

    var func = function(x, callback) {
        var args = [];
        _.forEach(ranges, function(range) {
            // The mid
            args.push(range.values[1]);
        });
        args[dimension] = x;
        console.log(args, dimension);
        multiParamFunc(args, function(err, value) {
            callback(null, value);
        });
    };

    function stepCallback(a, b, c) {
        // Order is a < b < c
        if(Math.abs(c - a) < 1) {
            console.log(ranges);
            return;
        } else {
            ranges[dimension].values = [a, b, c];
            dimension = (dimension + 1) % ranges.length;

            var nextValues = ranges[dimension].values;
            console.log('Searching in ' + dimensionsToString(ranges) + ' trying ' + dimension);
            goldenSearchStep(nextValues[0], nextValues[1], nextValues[2], func, stepCallback);
        }
    }
    var values = ranges[dimension].values;
    stepCallback(values[0], values[1], values[2]);
}

function dimensionsToString(ranges) {
    var result = '';
    _.forEach(ranges, function(range, index) {
        var values = range.values;
        result += printf('[%s: (%f,%f,%f)] ', range.name, values[0], values[1], values[2]);
    });
    return result;
}

module.exports.dimensionsToString = dimensionsToString;
module.exports.MultiParamFunction = MultiParamFunction;
module.exports.goldenSearchStep = goldenSearchStep;
module.exports.R = R;
