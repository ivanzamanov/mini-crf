var async = require('async'),
	_ = require('lodash'),
	printf = require('sprintf-js').sprintf;

var count = 100;
var arr = [];
var R = 0.61803
var MAX = count * 2/3;
for(var i = 0; i < count; i++) {
	arr.push(MAX - Math.abs(MAX - i) );
}

function arrFunc(x) {
	return arr[Math.floor(x)];
}

function goldenSearch(low, high, func) {
	goldenSearchHelper(low, high, low + R * (high - low), func);
}

function goldenSearchHelper(a, b, c, func) {
	// Order is a < b < c
	if(Math.abs(b - a) < 1) {
		console.log('Optimum in (' + a + ', ' + b + ')');
		return;
	} else {
		goldenSearchStep(a, b, c, func, goldenSearchHelper);
	}
}

function goldenSearchStep(a, b, c, func, callback) {
	var x;
	if (c - b > b - a)
		x = b + R * (c - b);
	else
		x = b - R * (b - a);

	async.map([x, b], func, function(err, values) {
		if (values[0] > values[1]) {
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

function wrapper(func) {
	return function(x, callback) {
		process.nextTick(function() {
			callback(null, func(x));
		});
	};
}

function mkRange(featureName, start, end, count) {
	if(!count) count = end - start;

	var result = {
		name: featureName,
		values: []
	}, step = (end - start) / count, i;

	for(i = 0; i < count; i++)
		result.values.push(start + step * i);

	return result;
}

function MultiParamFunction(reevaluate) {
	var values = [];

	return function (args, callback) {
		var result;
		_.forEach(values, function(valuesEntry) {
			if(_.isEqual(args, values.args))
				result = values.value;
		});

		if(result == undefined) {
			reevaluate(args, function(value) {
				values.push({
					value: value,
					args: args.slice(0)
				});
				callback(null, value);
			});
		}
		// Value found, yey
		callback(null, result);
	};
}

var matrixSize = 11;
var matrix = Array(matrixSize);
matrix = _.map(matrix, function() { return Array(matrixSize); });
var maxX = 5, maxY = 5;
_.forEach(matrix, function(row, rowNumber) {
	_.forEach(row, function(cell, colNumber) {
		var dist = (maxX - colNumber) * (maxX - colNumber) + (maxY - rowNumber) * (maxY - rowNumber);
		row[colNumber] = dist;
	});
});

var ranges = [
	{name: 'dim1', values: [0, R * matrixSize, matrixSize]},
	{name: 'dim2', values: [0, R * matrixSize, matrixSize]}
];

function matrixFunc(args) {
	debugger;
	return matrix[Math.round(args[0])][Math.round(args[1])];
}

_.forEach(matrix, function(row, rowNumber) {
	_.forEach(row, function(cell, colNumber) {
		process.stdout.write(printf('%2d ', cell));
	});
	process.stdout.write('\n');
});

function matrixGoldenSearch() {
	var dimension = 0;
	var multiParamFunc = MultiParamFunction(matrixFunc);

	var func = function(x, callback) {
		debugger;
		var args = [];
		_.forEach(ranges, function(range) {
			// The mid
			args.push(range.values[1]);
		});
		multiParamFunc(args, function(err, value) { callback(value) });
	};

	function stepCallback(a, b, c) {
		// Order is a < b < c
		if(Math.abs(b - a) < 1) {
			console.log('Optimum at ' + dimensionsToString(ranges));
			return;
		} else {
			ranges[dimension].values = [a, b, c];
			dimension = (dimension + 1) % ranges.length;

			var values = ranges[dimension].values;
			console.log('Searching in ' + dimensionsToString(ranges) + ' trying ' + dimension);
			goldenSearchStep(values[0], values[1], values[2], func, stepCallback);
		}
	}
	var values = ranges[dimension].values;
	console.log('Searching in ' + dimensionsToString(ranges));
	goldenSearchStep(values[0], values[1], values[2], func, stepCallback);
}

function dimensionsToString(ranges) {
	var result = '';
	_.forEach(ranges, function(range, index) {
		var values = range.values;
		result += printf('[%s: (%2d,%2d,%2d)] ', range.name, values[0], values[1], values[2]);
	});
	return result;
}

matrixGoldenSearch();
