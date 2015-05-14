#!/usr/bin/nodejs
var path = require('path'),
	fs = require('fs'),
	async = require('async'),
	minimist = require('minimist'),
	_ = require('lodash');

function parseArguments() {
	var opts = {
		string: ""
	};
	var args = minimist(process.argv.slice(2), opts);
}

var tmpDir = '.'
var synthDBPath = '/home/ivo/SpeechSynthesis/db-synth.bin';
var testDBPath = '/home/ivo/SpeechSynthesis/db-test-single.bin';
var childArguments = ['--mode', 'train', '--synth-database', synthDBPath, '--test-database', testDBPath ];
var commandPath = '/home/ivo/SpeechSynthesis/mini-crf/src/main-opt';

function getTrainingProcess() {
	return require('child_process').spawn(commandPath, childArguments);
}

function getComparisonProcess(originalSound, concatInputFile) {
	
}

function isEmpty(line) {
	return !line;
}

var tempFileIndex = 0;
function getTempFile() {
	return 'tempFile' + ++tempFileIndex + '.txt';
}

function onTrainingFinished(runConfig, output) {
	runConfig.comparisonResults = [];
	var comparisonConfigs = []
	var synthOutputs = output.split('----------');
	_.remove(synthOutputs, isEmpty);
	_.forEach(synthOutputs, function (synthOutput) {
		var lines = synthOutput.split('\n');
		
		_.remove(lines, isEmpty);
		var originalSound = lines[0];
		lines = lines.splice(0, 1);
		
		var tempFile = getTempFile();
		fs.writeFileSync(tempFile, lines.join('\n'));
		
		comparisonConfigs.push({ original: originalSound, synthInput: tempFile });
	});
	
	async.eachSeries(comparisonConfigs, 1, runComparison.bind(null, runConfig), function() {
		var sum = 0;
		_.forEach(runConfig.comparisonResults, function(val) { sum += val; });
		runConfig.result = sum;
		runConfig.finished();
	});
}

function runComparison(runConfig, originalSound, concatInputFile, callback) {
	var originalSound = comparisonConfig.originalSound;
	var concatInputFile = comparisonConfig.synthInput;
	
	var child1 = getConcatenationProcess(originalSound, concatInputFile);
	var comparisonOutput = '';
	
	child.stdout.on('data', function(chunk) {
		comparisonOutput += chunk.toString();
	});
	child.stdout.on('end', function() {
		runConfig.comparisonResults.push(parseDouble(comparisonOutput));
		callback();
	});
}

var allRunConfigs = [];
function trainWithCoefficients(coefs, callback) {
	var runConfig = {
		coefs: coefs,
		finished: callback
	};
	allRunConfigs.push(runConfig);
	console.log('training started');
	
	var child = getTrainingProcess(),
		output = "";
	
	child.stderr.pipe(process.stderr);
	
	_.forOwn(coefs, function(value, key) { child.stdin.write(key + "=" + value + "\n") });
	child.stdin.end();
	
	child.stdout.on('data', function (data) {
		output += data.toString();
	});
	child.stdout.on('end', function() {
		onTrainingFinished(runConfig, output);
		console.log('training finished');
	});
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

var ranges = [
	mkRange('trans-mfcc', 1, 10),
	mkRange('trans-pitch', 990, 1010),
	mkRange('state-pitch', 990, 1010),
	mkRange('state-duration', 990, 1010)
];

function generateCoefficients(ranges, index, coefs, callback) {
	if (index == ranges.length) {
		callback(coefs);
		return;
	}
	
	var range = ranges[index];
	_.forEach(range.values, function(value) {
		coefs[range.name] = value;
		generateCoefficients(ranges, index + 1, coefs, callback);
	});
}

function trainAll() {
	var allCoefficients = [];
	generateCoefficients(ranges, 0, {}, function(coef) { allCoefficients.push(coef) });
	console.log('Combinations: ' + allCoefficients.length);
	async.eachLimit([ allCoefficients[0] ], 1, trainWithCoefficients, onAllFinished);
}

function onAllFinished() {
	console.log('All done');
	var min;
	_.forEach(allRunConfigs, function (config) {
		if(!min || config.result < min.result)
			min = config;
	});
	console.log('Best:');
	console.log(min);
}

trainAll();