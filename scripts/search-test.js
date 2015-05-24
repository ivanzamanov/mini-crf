var async = require('async'),
	_ = require('lodash'),
	printf = require('sprintf-js').sprintf;

var R = 0.61803;

function goldenSearchStep(a, b, c, func, callback) {
	var x;
	if (c - b > b - a)
		x = b + R * (c - b);
	else
		x = b - R * (b - a);

	async.mapLimit([x, b], 1, func, function(err, values) {
    debugger;
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

function MultiParamFunction(reevaluate) {
	var values = [];

	return function (args, callback) {
		var result;
		_.forEach(values, function(valuesEntry) {
			if(_.isEqual(args, valuesEntry.args))
				result = values.value;
		});

		if(result == undefined) {
			reevaluate(args, function(value) {
				values.push({
					value: value,
					args: _.cloneDeep(args)
				});
				callback(null, value);
			});
		} else {
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
  callback(dist);
}

matrixGoldenSearch();
function matrixGoldenSearch() {
	var dimension = 0;
	var multiParamFunc = MultiParamFunction(matrixFunc);

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
			goldenSearchStep(nextValues[0], nextValues[1], nextValues[2], func, stepCallback);
		}
	}
	var values = ranges[dimension].values;
	stepCallback(values[0], values[1], values[2]);
}
