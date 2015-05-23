#!/usr/bin/nodejs
var path = require('path'),
  fs = require('fs'),
  async = require('async'),
  minimist = require('minimist'),
  _ = require('lodash');

var linuxConfig = {
  parallelProcesses: 1,
  parallelComparisons: 4,
  tempDir: '/tmp/',
  synthDB: '/home/ivo/SpeechSynthesis/db-synth.bin',
  testDB: '/home/ivo/SpeechSynthesis/db-test-single.bin',
  trainingCommand: '/home/ivo/SpeechSynthesis/mini-crf/src/main-opt',
  praatCommand: 'praat',
  synthScript: '/home/ivo/SpeechSynthesis/mini-crf/scripts/concat.praat',
  compareScript: '/home/ivo/SpeechSynthesis/mini-crf/scripts/cepstral-distance.praat'
};

var winConfig = {
  parallelProcesses: 1,
  parallelComparisons: 8,
  tempDir: 'D:\\cygwin\\home\\ivo\\SpeechSynthesis\\temp',
  synthDB: '/home/ivo/SpeechSynthesis/db-synth.bin',
  testDB: '/home/ivo/SpeechSynthesis/db-test.bin',
  trainingCommand: '/home/ivo/SpeechSynthesis/mini-crf/src/main-opt',
  praatCommand: '/usr/local/bin/praat-js',
  synthScript: 'D:\\cygwin\\home\\ivo\\SpeechSynthesis\\mini-crf\\scripts\\concat.praat',
  compareScript: 'D:\\cygwin\\home\\ivo\\SpeechSynthesis\\mini-crf\\scripts\\cepstral-distance.praat'
};

var isWindows = require('os').platform() == 'win32';
var config = isWindows ? winConfig : linuxConfig;
config.trainingArguments = ['--mode', 'train', '--synth-database', config.synthDB, '--test-database', config.testDB ];

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
  return path.normalize(path.join(config.tempDir, 'tempFile' + ++tempFileIndex + '.tmp'));
}

function runCommand(command, input, callback) {
  var output = "";

  /*console.log('Running: ');
  console.log(command);
  if(input)
    console.log('With input: ' + input);*/

  callback = _.once(callback);
  var child;
  if(isWindows) {
    command = ['D:\\cygwin\\bin\\bash.exe', 'D:\\cygwin\\home\\ivo\\bin\\stdin-wrap.sh', config.tempDir, command.join(' ')];
    child = require('child_process').spawn(command[0], command.slice(1), { cwd: 'D:\\cygwin\\bin\\' });
  } else {
    child = require('child_process').spawn(command[0], command.slice(1), { cwd: config.tempDir });
  }
  child.stderr.pipe(process.stderr);

  child.stdout.on('data', function (data) {
    output += data.toString('UTF-8');
  });
  child.on('error', function(err) { console.log(error); callback(err) });
  child.on('exit', function(code) { if(code != 0) callback(code); else callback(null); });
  child.stdout.on('end', function() {
    callback(null, output);
  });

  child.stdin.end(input);
  return child;
}

function getTrainingCommand() {
  return [config.trainingCommand].concat(config.trainingArguments);
}

function getSynthCommand(synthInputPath, synthOutputPath) {
  return [config.praatCommand, config.synthScript, synthInputPath, synthOutputPath];
}

function getComparisonCommand(sound1, sound2) {
  return [config.praatCommand, config.compareScript, sound1, sound2];
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
  if(isWindows) {
    originalSound = originalSound.replace('/', '\\');
    originalSound = 'D:\\cygwin' + originalSound;
  }
  runCommand(getComparisonCommand(originalSound, synthesizedSound), '', callback);
}

function runSynthesis(runConfig, runSynthesisCallback) {
  var comparisonConfigs = [];
  var synthOutputs = runConfig.trainingOutput.split('----------');
  _.remove(synthOutputs, isEmpty);

  async.eachLimit(synthOutputs, config.parallelComparisons, function (synthOutput, synthCallback) {
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
        console.log(originalSound + '/' + tempFile + ": " + comparisonResult.trim());
        runConfig.result += parseFloat(comparisonResult.trim());
        callback();
      }],
      synthCallback);
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
  ], function() {
    console.log('Finished comparison for: ');
    delete runConfig.trainingOutput;
    console.log(runConfig);
    callback();
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
  async.eachLimit([ allCoefficients[0] ], config.parallelProcesses, trainWithCoefficients, onAllFinished);
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

/*process.on('uncaughtException', function(err) {
  console.log('Uncaught exception:');
  console.log(err);
});*/
