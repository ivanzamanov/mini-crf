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

function isEmpty(line) {
	return !line;
}

var tempFileIndex = 0;
function getTempFile() {
	return path.normalize(path.join(tmpDir, 'tempFile' + ++tempFileIndex + '.tmp'));
}

function runCommand(command, input, callback) {
	var output = "";

	console.log('Running: ');
	console.log(command);
	if(input)
		console.log('With input: ' + input);

	callback = _.once(callback);
	var child = require('child_process').spawn(command[0], command.slice(1), { cwd: tmpDir });
	child.stderr.pipe(process.stderr);

	child.stdout.on('data', function (data) {
		output += data.toString('UTF-8');
	});
	child.on('error', callback);
	child.on('exit', function(code) { if(code != 0) callback(code); else callback(null); });
	child.stdout.on('end', function() {
		callback(null, output);
	});

	child.stdin.end(input);
	return child;
}

var PARALLEL_PROCESSES = 1;

var tmpDir = '/tmp/';
var synthDBPath = '/home/ivo/SpeechSynthesis/db-synth.bin';
var testDBPath = '/home/ivo/SpeechSynthesis/db-test-single.bin';
var trainingArguments = ['--mode', 'train', '--synth-database', synthDBPath, '--test-database', testDBPath ];
var trainingCommand = '/home/ivo/SpeechSynthesis/mini-crf/src/main-opt';

var PRAAT = 'praat',
	SYNTH_SCRIPT = '/home/ivo/SpeechSynthesis/mini-crf/scripts/concat.praat',
	COMPARE_SCRIPT = '/home/ivo/SpeechSynthesis/mini-crf/scripts/cepstral-distance.praat';

function getTrainingCommand() {
	return [trainingCommand].concat(trainingArguments);
}

function getSynthCommand(synthInputPath, synthOutputPath) {
	return [PRAAT, SYNTH_SCRIPT, synthInputPath, synthOutputPath];
}

function getComparisonCommand(sound1, sound2) {
	return [PRAAT, COMPARE_SCRIPT, sound1, sound2];
}

function runTraining(runConfig, callback) {
	var coefs = runConfig.coefs,
		input = "",
		output = "";

	_.forOwn(coefs, function(value, key) { input += (key + "=" + value + "\n") });

	runCommand(getTrainingCommand(), input, function(error, result) {
		if(error) {
			callback(error);
			return;
		}
		runConfig.trainingOutput = result;
		console.log('Training finished');
		callback(null, runConfig);
	});
}

function synthesize(synthInputPath, callback) {
	var synthesizedSound = getTempFile();
	var synthCommand = getSynthCommand(synthInputPath, synthesizedSound);
	runCommand(synthCommand, '', function(err, output) {
		console.log(output);
		callback(null, synthesizedSound);
	});
}

function runComparison(originalSound, synthesizedSound, callback) {
	runCommand(getComparisonCommand(originalSound, synthesizedSound), '', callback);
}

function runSynthesis(runConfig, runSynthesisCallback) {
	var comparisonConfigs = [];
	var synthOutputs = runConfig.trainingOutput.split('----------');
	_.remove(synthOutputs, isEmpty);

	async.eachLimit(synthOutputs, 1, function (synthOutput, callback) {
		var lines = synthOutput.split('\n');

		_.remove(lines, isEmpty);
		var originalSound = lines[0];

		lines.splice(0, 1);
		var tempFile = getTempFile();
		fs.writeFileSync(tempFile, lines.join('\n'));

		async.waterfall([
			synthesize.bind(null, tempFile),
			runComparison.bind(null, originalSound),
			function(comparisonResult, callback) {
				debugger;
				runConfig.result += parseFloat(comparisonResult.trim());
				callback();
			}],
			callback);
	}, runSynthesisCallback);
}

var allRunConfigs = [];
function trainWithCoefficients(coefs, callback) {
	var runConfig = {
		result: 0,
		coefs: coefs
	};
	allRunConfigs.push(runConfig);
	console.log('training started');

	async.waterfall([
		runTraining.bind(null, runConfig),
		runSynthesis
	], callback);
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
	mkRange('trans-mfcc', 1, 2),
	mkRange('trans-pitch', 999, 1000),
	mkRange('state-pitch', 999, 1000),
	mkRange('state-duration', 999, 1000)
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
	async.eachLimit([ allCoefficients[0] ], PARALLEL_PROCESSES, trainWithCoefficients, onAllFinished);
}

function onAllFinished() {
	console.log('All done');
	var min;
	_.forEach(allRunConfigs, function (config) {
		if(!min || config.result < min.result)
			min = config;
	});
	console.log('Best:');
	delete min.trainingOutput;
	console.log(min);
}

trainAll();

process.on('uncaughtException', function(err) {
	console.log('Uncaught exception:');
	console.log(err);
});
