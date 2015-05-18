var async = require('async');

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
	console.log('Searching in domain (' + a + ', ' + b + ')');
	if(Math.abs(b - a) < 1) {
		console.log('Optimum in (' + a + ', ' + b + ')');
		return;
	}

	var x;
	if (c - b > b - a)
		x = b + R * (c - b);
	else
		x = b - R * (b - a);

	async.map([x, b], func, function(err, values) {
		if (values[0] > values[1]) {
			if (c - b > b - a)
				goldenSearchHelper(b, x, c, func);
			else
				goldenSearchHelper(a, x, b, func);
		} else {
			if (c - b > b - a)
				goldenSearchHelper(a, b, x, func);
			else
				goldenSearchHelper(x, b, c, func);
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

goldenSearch(0, arr.length - 1, wrapper(arrFunc));
