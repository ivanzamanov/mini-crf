var async = require('async'),
    _ = require('lodash'),
    search = require('./search'),
    printf = require('sprintf-js').sprintf;

var R = 0.61803;

var matrixSize = 100;
var ranges = [
    {name: 'dim1', values: [0, R * matrixSize, matrixSize]},
    {name: 'dim2', values: [0, R * matrixSize, matrixSize]}
];

var maxX = 5, maxY = 5;
function matrixFunc(args, callback) {
    var dist = (maxX - args[1]) * (maxX - args[1]) + (maxY - args[0]) * (maxY - args[0]);
    callback(dist);
}

matrixGoldenSearch();
function matrixGoldenSearch() {
    var dimension = 0;
    var multiParamFunc = search.MultiParamFunction(matrixFunc, 'temp-values.json');

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
            search.goldenSearchStep(nextValues[0], nextValues[1], nextValues[2], func, stepCallback);
        }
    }
    var values = ranges[dimension].values;
    stepCallback(values[0], values[1], values[2]);
}
